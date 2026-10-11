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

#ifndef CRYSTALDOCK_MULTI_DOCK_VIEW_H_
#define CRYSTALDOCK_MULTI_DOCK_VIEW_H_

#include <memory>
#include <unordered_map>

#include <QApplication>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

#include "dock_panel.h"
#include <desktop/desktop_env.h>
#include <model/multi_dock_model.h>

namespace crystaldock {

// The view.
class MultiDockView : public QObject {
  Q_OBJECT

 public:
  // No pointer ownership.
  explicit MultiDockView(MultiDockModel* model);
  ~MultiDockView() = default;

  static bool checkPlatformSupported(const QApplication& app);

  void show();

 public slots:
  void exit();

  void onDockAdded(int dockId);

  bool setWallpaper();
  bool setWallpaper(int screen);

  // Phase 1: Called when the KDE icon theme config file changes.
  // Checks whether the icon theme name actually changed and reloads all docks
  // if so, so the dock updates live when System Settings > Icons is changed.
  void onKdeGlobalsChanged(const QString& path);

 private:
  void loadData();

  // Reads the current icon theme name from ~/.config/kdeglobals.
  static QString readKdeIconTheme();

  // Starts watching ~/.config/kdeglobals for icon theme changes.
  void watchIconTheme();

  // Creates a default dock if none exists.
  void createDefaultDock();

  MultiDockModel* model_;  // No ownership.
  std::unordered_map<int, std::unique_ptr<DockPanel>> docks_;
  DesktopEnv* desktopEnv_;

  // Phase 1: file watcher for ~/.config/kdeglobals to detect live icon theme changes.
  QFileSystemWatcher kdeGlobalsWatcher_;
  QString currentIconTheme_;
};

}  // namespace crystaldock

#endif // CRYSTALDOCK_MULTI_DOCK_VIEW_H_
