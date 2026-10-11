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

// Phase 4: Window hover preview popup.
//
// Shows a small popup with window title + close button for each window of the
// hovered task icon. Thumbnails require a compositor-specific capture backend;
// this initial implementation is the text-only fallback which always works.
//
// Architecture:
//   WindowPreviewPopup — Qt widget, shown above the dock icon for each task.
//   It is created on-demand and destroyed (or hidden) when the mouse leaves.
//   Does NOT steal keyboard focus (Qt::ToolTip window flag).
//
// Thumbnail strategy (text-only fallback active):
//   KWin Wayland exposes org_kde_plasma_window_management for window metadata
//   but window frame capture requires the KWin Screencast D-Bus interface
//   (org.kde.KWin.ScreenCast / streamWindow) with PipeWire. That requires
//   additional dependencies (pipewire, libspa) and non-trivial buffer handling.
//   We implement ThumbnailProvider as an abstract interface; the text-only
//   fallback is always present and logs why thumbnails are unavailable.
//   A future commit can add a KwinPipeWireThumbnailProvider without touching
//   this widget.

#include <functional>
#include <vector>

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <display/window_system.h>
#include <model/multi_dock_model.h>

namespace crystaldock {

// One entry shown in the preview popup for a single window.
struct WindowPreviewEntry {
  void* window;       // opaque handle for WindowSystem calls.
  QString title;
};

// Abstract interface for thumbnail providers.
// isAvailable() must be cheap (no I/O). requestThumbnail() is async;
// when a frame is ready it calls the provided callback on the Qt event loop.
// A text-only fallback simply never calls the callback.
class ThumbnailProvider {
 public:
  virtual ~ThumbnailProvider() = default;
  virtual bool isAvailable() const = 0;
  virtual void requestThumbnail(void* windowHandle, QSize size,
                                std::function<void(const QPixmap&)> callback) = 0;
  virtual void cancelThumbnail(void* windowHandle) = 0;
  virtual const char* name() const = 0;
};

// TextOnlyFallbackProvider — always available, never delivers thumbnails.
// Logs a single line explaining why on first use.
class TextOnlyFallbackProvider : public ThumbnailProvider {
 public:
  bool isAvailable() const override { return true; }

  void requestThumbnail(void*, QSize, std::function<void(const QPixmap&)>) override {
    // Intentionally empty: text-only fallback shows titles but no thumbnails.
    // To add real thumbnails, implement a compositor-specific ThumbnailProvider
    // subclass (e.g. KwinPipeWireThumbnailProvider) and pass it to
    // WindowPreviewPopup::setThumbnailProvider().
  }

  void cancelThumbnail(void*) override {}

  const char* name() const override {
    return "TextOnlyFallback (no compositor capture backend available)";
  }
};

// WindowPreviewPopup
//
// Displayed as a Qt::ToolTip-style window (no focus steal, no frame).
// Contains one row per window: [thumbnail or icon] + title + close button.
// Clicking a row activates the window; close button closes it.
//
// Lifecycle:
//   - Created and shown by Program::showPreviewPopup() on hover timer.
//   - Hidden when the mouse leaves both the icon and the popup (grace timer).
//   - Deleted when the parent DockPanel is destroyed (parent ownership).
class WindowPreviewPopup : public QWidget {
  Q_OBJECT

 public:
  // sizePx: bounding box size for each thumbnail (width and height).
  // provider: the thumbnail provider to use. Ownership is NOT transferred.
  explicit WindowPreviewPopup(MultiDockModel* model,
                              ThumbnailProvider* provider,
                              Qt::Orientation dockOrientation,
                              QWidget* parent = nullptr);
  ~WindowPreviewPopup() override = default;

  // Populate the popup with window entries. Call before show().
  void setWindows(const std::vector<WindowPreviewEntry>& entries);

  // Position the popup relative to the icon's global rect.
  // For a bottom dock the popup appears above the icon; adjust for other edges.
  void positionNear(const QRect& iconGlobalRect);

  // Returns true if the mouse is currently over the popup.
  bool isHovered() const { return isHovered_; }

 signals:
  // Emitted when the popup wants to close itself (mouse left both popup and icon).
  void popupClosed();

 protected:
  void enterEvent(QEnterEvent* e) override;
  void leaveEvent(QEvent* e) override;
  bool eventFilter(QObject* obj, QEvent* event) override;

 private slots:
  void onActivateWindow(void* window);
  void onCloseWindow(void* window);

 private:
  // Rebuild the UI layout for the current window list.
  void rebuildLayout();

  // Compute the preview thumbnail size respecting DPI if configured.
  QSize effectiveThumbnailSize() const;

  MultiDockModel* model_;            // Not owned.
  ThumbnailProvider* provider_;      // Not owned.
  Qt::Orientation dockOrientation_;

  std::vector<WindowPreviewEntry> entries_;
  QVBoxLayout* mainLayout_ = nullptr;
  bool isHovered_ = false;
};

}  // namespace crystaldock

#endif  // CRYSTALDOCK_WINDOW_PREVIEW_POPUP_H_
