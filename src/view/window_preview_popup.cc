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

#include "window_preview_popup.h"

#include <iostream>

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSizePolicy>
#include <QString>
#include <QStyleOption>
#include <QPainter>

namespace crystaldock {

namespace {

// Spacing and sizing constants.
constexpr int kPadding = 8;
constexpr int kRowSpacing = 6;
constexpr int kMaxTitleWidth = 180;
constexpr int kCloseBtnSize = 20;

}  // namespace

WindowPreviewPopup::WindowPreviewPopup(MultiDockModel* model,
                                       ThumbnailProvider* provider,
                                       Qt::Orientation dockOrientation,
                                       QWidget* parent)
    : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      model_(model),
      provider_(provider),
      dockOrientation_(dockOrientation) {
  // ToolTip windows don't steal keyboard focus.
  setAttribute(Qt::WA_ShowWithoutActivating, true);
  setAttribute(Qt::WA_TranslucentBackground, false);

  // Use a neutral dark styled background matching the icon theme's tooltip style.
  setStyleSheet(
    "WindowPreviewPopup {"
    "  background-color: #2b2b2b;"
    "  border: 1px solid #555555;"
    "  border-radius: 6px;"
    "}"
    "QLabel#titleLabel {"
    "  color: #e0e0e0;"
    "  font-size: 11px;"
    "}"
    "QPushButton#closeBtn {"
    "  color: #aaaaaa;"
    "  background: transparent;"
    "  border: none;"
    "  font-size: 14px;"
    "  padding: 0px;"
    "}"
    "QPushButton#closeBtn:hover {"
    "  color: #ff5555;"
    "}"
  );

  mainLayout_ = new QVBoxLayout(this);
  mainLayout_->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
  mainLayout_->setSpacing(kRowSpacing);

  // Log thumbnail provider status once on construction.
  static bool providerLogged = false;
  if (!providerLogged) {
    providerLogged = true;
    if (provider_ && !provider_->isAvailable()) {
      std::cerr << "[crystal-dock] Window preview: thumbnail capture unavailable — "
                << (provider_ ? provider_->name() : "no provider")
                << " — showing text-only popup." << std::endl;
    } else if (!provider_) {
      std::cerr << "[crystal-dock] Window preview: no ThumbnailProvider — text-only mode."
                << std::endl;
    }
  }
}

void WindowPreviewPopup::setWindows(const std::vector<WindowPreviewEntry>& entries) {
  entries_ = entries;
  rebuildLayout();
}

void WindowPreviewPopup::positionNear(const QRect& iconGlobalRect) {
  adjustSize();
  const QSize sz = sizeHint();

  int x = 0;
  int y = 0;

  // Place the popup on the opposite side of the dock edge from the screen centre.
  // For a bottom dock: popup above the icon.
  // For a top dock:    popup below the icon.
  // For left/right:    popup to the right/left of the icon.
  if (dockOrientation_ == Qt::Horizontal) {
    // Centre horizontally over the icon.
    x = iconGlobalRect.center().x() - sz.width() / 2;
    // Above icon for bottom dock; for top dock use iconGlobalRect.bottom().
    y = iconGlobalRect.top() - sz.height() - 4;
    if (y < 0) {
      // Top dock: place below instead.
      y = iconGlobalRect.bottom() + 4;
    }
  } else {
    // Vertical dock.
    y = iconGlobalRect.center().y() - sz.height() / 2;
    x = iconGlobalRect.right() + 4;
    // Screen edge clipping.
    const auto* screen = QGuiApplication::primaryScreen();
    if (screen && x + sz.width() > screen->geometry().right()) {
      x = iconGlobalRect.left() - sz.width() - 4;
    }
  }

  // Clamp to screen bounds.
  const auto* screen = QGuiApplication::primaryScreen();
  if (screen) {
    const auto sg = screen->geometry();
    x = qBound(sg.left(), x, sg.right() - sz.width());
    y = qBound(sg.top(), y, sg.bottom() - sz.height());
  }

  move(x, y);
}

void WindowPreviewPopup::enterEvent(QEnterEvent* /*e*/) {
  isHovered_ = true;
}

void WindowPreviewPopup::leaveEvent(QEvent* /*e*/) {
  isHovered_ = false;
  emit popupClosed();
}

void WindowPreviewPopup::onActivateWindow(void* window) {
  hide();
  WindowSystem::activateWindow(window);
}

void WindowPreviewPopup::onCloseWindow(void* window) {
  WindowSystem::closeWindow(window);
  // The dock will receive a windowRemoved signal and refresh normally.
}

QSize WindowPreviewPopup::effectiveThumbnailSize() const {
  int sizePx = model_->previewSizePx();
  if (model_->previewSizeFromDpi()) {
    // Convert 2 logical inches to device pixels using the primary screen's DPI.
    const auto* screen = QGuiApplication::primaryScreen();
    if (screen) {
      sizePx = qRound(2.0 * screen->physicalDotsPerInch());
    }
  }
  return QSize(sizePx, sizePx);
}

void WindowPreviewPopup::rebuildLayout() {
  // Remove all existing child widgets from the layout.
  QLayoutItem* item;
  while ((item = mainLayout_->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  if (entries_.empty()) {
    return;
  }

  // Phase 4: For each window we show one row:
  //   [thumbnail-or-icon-placeholder]  [title label]  [close button]
  // The row is clickable to activate the window.
  // Thumbnails are provided by the ThumbnailProvider asynchronously;
  // if the provider is text-only the thumbnail area stays hidden.
  for (const auto& entry : entries_) {
    QWidget* row = new QWidget(this);
    row->setCursor(Qt::PointingHandCursor);
    row->setToolTip(entry.title);

    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(4, 2, 4, 2);
    rowLayout->setSpacing(6);

    // Thumbnail placeholder: shown as a small dark box when no capture is available.
    // When a ThumbnailProvider delivers a real frame, we swap the pixmap in.
    const QSize thumbSz = effectiveThumbnailSize();
    QLabel* thumbLabel = new QLabel(row);
    thumbLabel->setFixedSize(thumbSz);
    thumbLabel->setStyleSheet("background-color: #1a1a1a; border-radius: 3px;");
    thumbLabel->setAlignment(Qt::AlignCenter);
    rowLayout->addWidget(thumbLabel);

    // Request a thumbnail from the provider. If the provider is text-only this
    // is a no-op and the placeholder stays dark.
    if (provider_ && provider_->isAvailable()) {
      void* windowHandle = entry.window;
      provider_->requestThumbnail(windowHandle, thumbSz,
          [thumbLabel, thumbSz](const QPixmap& pixmap) {
            if (!pixmap.isNull()) {
              // Scale preserving aspect ratio (letterbox inside the box).
              thumbLabel->setPixmap(pixmap.scaled(
                  thumbSz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
          });
    }

    // Title label.
    QLabel* titleLabel = new QLabel(entry.title, row);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setMaximumWidth(kMaxTitleWidth);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    titleLabel->setWordWrap(false);
    // Elide long titles on the right.
    QFontMetrics fm(titleLabel->font());
    titleLabel->setText(fm.elidedText(entry.title, Qt::ElideRight, kMaxTitleWidth));
    rowLayout->addWidget(titleLabel, 1);

    // Close button.
    if (model_->previewShowCloseButton()) {
      QPushButton* closeBtn = new QPushButton("✕", row);
      closeBtn->setObjectName("closeBtn");
      closeBtn->setFixedSize(kCloseBtnSize, kCloseBtnSize);
      closeBtn->setFlat(true);
      void* window = entry.window;
      connect(closeBtn, &QPushButton::clicked,
              this, [this, window]() { onCloseWindow(window); });
      rowLayout->addWidget(closeBtn);
    }

    // Click on the row (not close btn) activates the window.
    void* window = entry.window;
    row->installEventFilter(this);
    // Store the window handle in the row widget's property for retrieval in event filter.
    row->setProperty("windowHandle", QVariant::fromValue(static_cast<quintptr>(
        reinterpret_cast<quintptr>(window))));

    mainLayout_->addWidget(row);
  }

  adjustSize();
}

// Override eventFilter to catch mouse press on the row widgets.
// We can't use QWidget::mousePressEvent on child widgets without subclassing,
// so we use installEventFilter on each row widget instead.
bool WindowPreviewPopup::eventFilter(QObject* obj, QEvent* event) {
  if (event->type() == QEvent::MouseButtonPress) {
    auto* me = static_cast<QMouseEvent*>(event);
    if (me->button() == Qt::LeftButton) {
      auto* widget = qobject_cast<QWidget*>(obj);
      if (widget) {
        QVariant v = widget->property("windowHandle");
        if (v.isValid()) {
          void* window = reinterpret_cast<void*>(
              static_cast<quintptr>(v.value<quintptr>()));
          onActivateWindow(window);
          return true;
        }
      }
    }
  }
  return QWidget::eventFilter(obj, event);
}

}  // namespace crystaldock
