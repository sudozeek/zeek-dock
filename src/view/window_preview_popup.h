/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2025 Viet Dang (dangvd@gmail.com)
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

#ifndef CRYSTALDOCK_WINDOW_PREVIEW_POPUP_H_
#define CRYSTALDOCK_WINDOW_PREVIEW_POPUP_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <display/window_system.h>
#include <model/multi_dock_model.h>

namespace crystaldock {

// One entry shown in the preview popup for a single window.
struct WindowPreviewEntry {
  void* window = nullptr;
  QString title;
  QString uuid;
  QPixmap appIcon;
};

// Abstract interface for thumbnail providers.
class ThumbnailProvider {
 public:
  virtual ~ThumbnailProvider() = default;
  virtual bool isAvailable() const = 0;
  virtual void requestThumbnail(const QString& uuid, QSize size,
                                std::function<void(const QPixmap&)> callback) = 0;
  virtual const char* name() const = 0;
};

// KwinScreenshotProvider: Captures live window thumbnails via KDE Plasma 6
// org.kde.KWin.ScreenShot2 D-Bus interface. Runs asynchronously in a background
// thread so the dock UI never blocks or stutters.
class KwinScreenshotProvider : public ThumbnailProvider {
 public:
  static KwinScreenshotProvider* instance();

  bool isAvailable() const override;
  void requestThumbnail(const QString& uuid, QSize size,
                        std::function<void(const QPixmap&)> callback) override;
  const char* name() const override { return "KwinScreenShot2"; }

 private:
  KwinScreenshotProvider();
  std::unordered_map<std::string, QPixmap> cache_;
};

// WindowPreviewCard: Represents a single window card inside the preview popup.
// Styled according to the mockup:
//   ┌─────────────────────────┐
//   │ App/Window Name       ✕ │  <- Title bar + red close button
//   ├─────────────────────────┤  <- Divider line
//   │                         │
//   │    [Live Screenshot]    │  <- Window picture
//   │                         │
//   └─────────────────────────┘
class WindowPreviewCard : public QFrame {
  Q_OBJECT

 public:
  WindowPreviewCard(const WindowPreviewEntry& entry,
                    ThumbnailProvider* provider,
                    QWidget* parent = nullptr);

 signals:
  void activated(void* window);
  void closed(void* window);

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;

 private:
  void* window_ = nullptr;
  QLabel* titleLabel_ = nullptr;
  QPushButton* closeBtn_ = nullptr;
  QLabel* thumbLabel_ = nullptr;
  QFrame* dividerLine_ = nullptr;
};

// WindowPreviewPopup: Container for window previews.
// Multiple windows for the same app are arranged SIDE BY SIDE horizontally.
class WindowPreviewPopup : public QWidget {
  Q_OBJECT

 public:
  explicit WindowPreviewPopup(MultiDockModel* model,
                              ThumbnailProvider* provider,
                              Qt::Orientation dockOrientation,
                              QWidget* parent = nullptr);
  ~WindowPreviewPopup() override = default;

  // Populate preview cards for the given windows. Arranges them side-by-side.
  void setWindows(const std::vector<WindowPreviewEntry>& entries);

  // Position popup relative to the dock icon's global rectangle, elevated above dock.
  void positionNear(const QRect& iconGlobalRect, const QWidget* dockPanel = nullptr);

  bool isHovered() const { return isHovered_; }

 signals:
  void popupClosed();

 protected:
  void enterEvent(QEnterEvent* e) override;
  void leaveEvent(QEvent* e) override;

 private slots:
  void onCardActivated(void* window);
  void onCardClosed(void* window);

 private:
  MultiDockModel* model_;
  ThumbnailProvider* provider_;
  Qt::Orientation dockOrientation_;

  QHBoxLayout* cardsLayout_ = nullptr;
  std::vector<WindowPreviewEntry> entries_;
  bool isHovered_ = false;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_WINDOW_PREVIEW_POPUP_H_
