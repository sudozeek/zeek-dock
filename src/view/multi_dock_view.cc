/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2022 Viet Dang (dangvd@gmail.com)
 *
 * Crystal Dock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Crystal Dock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Crystal Dock.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "multi_dock_view.h"

#include <iostream>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPixmapCache>
#include <QSettings>

#include "display/window_system.h"

#include "add_panel_dialog.h"

namespace crystaldock {

MultiDockView::MultiDockView(MultiDockModel* model)
    : model_(model),
      desktopEnv_(DesktopEnv::getDesktopEnv()) {
  connect(model_, SIGNAL(dockAdded(int)), this, SLOT(onDockAdded(int)));
  connect(model_, SIGNAL(wallpaperChanged(int)), this,
          SLOT(setWallpaper(int)));
  connect(WindowSystem::self(), SIGNAL(currentDesktopChanged(std::string_view)),
          this, SLOT(setWallpaper()));
  loadData();

  // Phase 1: Start watching for icon theme changes in KDE's global config file.
  // KDE writes the new icon theme name to [Icons] Theme= in kdeglobals
  // when the user changes the icon theme in System Settings.
  watchIconTheme();
}

/* static */ bool MultiDockView::checkPlatformSupported(const QApplication& app) {
  if (QGuiApplication::platformName().toLower() != "wayland") {
    QMessageBox::critical(nullptr, "Unsupported Platform",
                          "Crystal Dock 2.x only supports Wayland.\n"
                          "For X11, please use Crystal Dock 1.x");
    return false;
  }

  QNativeInterface::QWaylandApplication* waylandApp =
      app.nativeInterface<QNativeInterface::QWaylandApplication>();
  if (!waylandApp) {
    return false;
  }

  return WindowSystem::init(waylandApp->display());
}

void MultiDockView::show() {
  for (const auto& dock : docks_) {
    dock.second->show();
  }
  setWallpaper();
}

void MultiDockView::exit() {
  for (const auto& dock : docks_) {
    dock.second->close();
  }
}

void MultiDockView::onDockAdded(int dockId) {
  docks_[dockId] = std::make_unique<DockPanel>(this, model_, dockId);
  docks_[dockId]->show();
}

bool MultiDockView::setWallpaper() {
  if (!model_->hasPager()) {
    return false;
  }

  for (unsigned int screen = 0; screen < WindowSystem::screens().size(); ++screen) {
    if (!setWallpaper(screen)) {
      return false;
    }
  }

  return true;
}

bool MultiDockView::setWallpaper(int screen) {
  if (!model_->hasPager()) {
    return false;
  }

  QString wallpaper = model_->wallpaper(WindowSystem::currentDesktop(), screen);
  if (wallpaper.isEmpty()) {
    return false;  // nothing to do here.
  }

  if (!QFile::exists(wallpaper)) {
    QMessageBox warning(QMessageBox::Warning, "Error",
                        QString("Failed to load wallpaper from: ") + wallpaper,
                        QMessageBox::Ok, nullptr, Qt::Tool);
    warning.exec();
    return false;
  }

  return desktopEnv_->setWallpaper(screen, wallpaper);
}

// Phase 1: Read the current icon theme name from KDE global settings.
// QSettings parses the INI-format kdeglobals file; the icon theme is stored
// under [Icons] group with key "Theme".
/* static */ QString MultiDockView::readKdeIconTheme() {
  const QString kdeglobals =
      QDir::homePath() + "/.config/kdeglobals";
  QSettings settings(kdeglobals, QSettings::IniFormat);
  settings.beginGroup("Icons");
  const QString theme = settings.value("Theme", QString()).toString();
  settings.endGroup();
  return theme;
}

// Phase 1: Set up file system watcher for ~/.config/kdeglobals.
// The watcher detects any write to kdeglobals (KDE updates the whole file
// when settings change). On each change we compare the stored theme name;
// if it differs we apply the new theme to Qt and reload all dock icons.
void MultiDockView::watchIconTheme() {
  const QString kdeglobals =
      QDir::homePath() + "/.config/kdeglobals";

  if (!QFile::exists(kdeglobals)) {
    std::cerr << "[crystal-dock] kdeglobals not found at " << kdeglobals.toStdString()
              << " — live icon theme updates will not work." << std::endl;
    return;
  }

  currentIconTheme_ = readKdeIconTheme();
  std::cout << "[crystal-dock] Watching icon theme: " << currentIconTheme_.toStdString()
            << " (file: " << kdeglobals.toStdString() << ")" << std::endl;

  kdeGlobalsWatcher_.addPath(kdeglobals);

  // QFileSystemWatcher on Linux emits fileChanged when the file is modified.
  // Note: some editors replace the file atomically (delete + recreate), so
  // we also re-add the path in the slot to keep watching after such replacements.
  connect(&kdeGlobalsWatcher_, &QFileSystemWatcher::fileChanged,
          this, &MultiDockView::onKdeGlobalsChanged);
}

// Phase 1: Slot called when kdeglobals is written.
// Re-adds the watch path (in case the file was atomically replaced),
// then checks if the icon theme has changed and reloads all dock items.
void MultiDockView::onKdeGlobalsChanged(const QString& path) {
  // Re-add path in case it was removed due to atomic replace (inotify IN_DELETE).
  if (!kdeGlobalsWatcher_.files().contains(path)) {
    kdeGlobalsWatcher_.addPath(path);
  }

  const QString newTheme = readKdeIconTheme();
  if (newTheme.isEmpty() || newTheme == currentIconTheme_) {
    return;  // No icon theme change — ignore.
  }

  std::cout << "[crystal-dock] Icon theme changed: "
            << currentIconTheme_.toStdString() << " -> " << newTheme.toStdString()
            << " — reloading dock icons." << std::endl;

  currentIconTheme_ = newTheme;

  // Tell Qt about the new theme so QIcon::fromTheme picks it up immediately.
  QIcon::setThemeName(newTheme);

  // Clear Qt's pixmap cache so stale icon pixmaps are evicted and fresh ones
  // are fetched from the new theme on the next paint.
  QPixmapCache::clear();

  // Reload every dock panel — this clears items and re-creates them,
  // causing all icons to be re-resolved from the new theme.
  for (const auto& dock : docks_) {
    dock.second->reload();
  }
}

void MultiDockView::loadData() {
  docks_.clear();
  for (int dockId = 1; dockId <= model_->dockCount(); ++dockId) {
    docks_[dockId] = std::make_unique<DockPanel>(this, model_, dockId);
  }

  if (docks_.empty()) {
    AddPanelDialog dialog(nullptr, model_, 0 /* dockId not needed */);
    dialog.setMode(AddPanelDialog::Mode::Welcome);
    dialog.exec();
  }
}

}  // namespace crystaldock
