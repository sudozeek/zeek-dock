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
#include <thread>
#include <unistd.h>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMetaObject>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace crystaldock {

namespace {

constexpr int kThumbWidth = 210;
constexpr int kThumbHeight = 130;
constexpr int kMaxTitleWidth = 175;

}  // namespace

// ============================================================================
// KwinScreenshotProvider Implementation
// ============================================================================

KwinScreenshotProvider* KwinScreenshotProvider::instance() {
  static KwinScreenshotProvider provider;
  return &provider;
}

KwinScreenshotProvider::KwinScreenshotProvider() = default;

bool KwinScreenshotProvider::isAvailable() const {
  return true;
}

void KwinScreenshotProvider::requestThumbnail(
    const QString& uuid, QSize size,
    std::function<void(const QPixmap&)> callback) {
  const std::string key = uuid.toStdString();
  auto it = cache_.find(key);
  if (it != cache_.end() && !it->second.isNull()) {
    callback(it->second);
    return;
  }

  // Perform screenshot capture asynchronously in background thread.
  std::thread([this, uuid, key, size, callback]() {
    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
      return;
    }

    // Read end drains pipe into buffer concurrently.
    std::vector<char> buffer;
    std::thread reader([pipeReadFd = pipeFds[0], &buffer]() {
      char buf[65536];
      ssize_t n;
      while ((n = read(pipeReadFd, buf, sizeof(buf))) > 0) {
        buffer.insert(buffer.end(), buf, buf + n);
      }
      close(pipeReadFd);
    });

    QVariantMap options;
    QDBusMessage reply;
    {
      QDBusInterface iface("org.kde.KWin", "/org/kde/KWin/ScreenShot2",
                           "org.kde.KWin.ScreenShot2",
                           QDBusConnection::sessionBus());
      QDBusUnixFileDescriptor writeFd(pipeFds[1]);
      close(pipeFds[1]);  // writeFd holds duplicate.

      if (!uuid.isEmpty()) {
        reply = iface.call("CaptureWindow", uuid, options,
                           QVariant::fromValue(writeFd));
      }
      if (reply.type() == QDBusMessage::ErrorMessage || uuid.isEmpty()) {
        // Fallback to active window capture if window UUID not matched.
        reply = iface.call("CaptureActiveWindow", options,
                           QVariant::fromValue(writeFd));
      }
    }  // writeFd destroyed here, closing write end in this process.

    reader.join();

    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
      return;
    }

    // Parse image metadata from D-Bus reply.
    const auto arg = reply.arguments()[0].value<QDBusArgument>();
    const QVariantMap res = qdbus_cast<QVariantMap>(arg);
    const uint width = res.value("width").toUInt();
    const uint height = res.value("height").toUInt();
    const uint stride = res.value("stride").toUInt();
    const uint fmt = res.value("format").toUInt();

    if (width == 0 || height == 0 || buffer.empty()) {
      return;
    }

    const QImage rawImg(reinterpret_cast<const uchar*>(buffer.data()),
                        width, height, stride, static_cast<QImage::Format>(fmt));
    if (rawImg.isNull()) {
      return;
    }

    const QImage scaled = rawImg.scaled(size, Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation);
    const QPixmap pixmap = QPixmap::fromImage(scaled);

    // Deliver thumbnail on the main GUI thread.
    QMetaObject::invokeMethod(
        QGuiApplication::instance(),
        [this, key, pixmap, callback]() {
          if (!pixmap.isNull()) {
            cache_[key] = pixmap;
            callback(pixmap);
          }
        },
        Qt::QueuedConnection);
  }).detach();
}

// ============================================================================
// WindowPreviewCard Implementation
// ============================================================================

WindowPreviewCard::WindowPreviewCard(const WindowPreviewEntry& entry,
                                     ThumbnailProvider* provider,
                                     QWidget* parent)
    : QFrame(parent), window_(entry.window) {
  setCursor(Qt::PointingHandCursor);
  setFixedWidth(kThumbWidth + 16);
  setObjectName("WindowPreviewCard");
  setStyleSheet(
      "#WindowPreviewCard {"
      "  background-color: #242429;"
      "  border: 1px solid #3c3c44;"
      "  border-radius: 8px;"
      "}"
      "#WindowPreviewCard:hover {"
      "  background-color: #2c2c33;"
      "  border: 1px solid #0078d4;"
      "}");

  auto* cardLayout = new QVBoxLayout(this);
  cardLayout->setContentsMargins(6, 6, 6, 6);
  cardLayout->setSpacing(4);

  // --- Title Bar (Header) ---
  auto* headerWidget = new QWidget(this);
  auto* headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(2, 0, 2, 0);
  headerLayout->setSpacing(4);

  titleLabel_ = new QLabel(headerWidget);
  titleLabel_->setStyleSheet("color: #ffffff; font-size: 11px; font-weight: bold;");
  QFontMetrics fm(titleLabel_->font());
  titleLabel_->setText(fm.elidedText(entry.title, Qt::ElideRight, kMaxTitleWidth));
  titleLabel_->setToolTip(entry.title);
  headerLayout->addWidget(titleLabel_, 1);

  // Red Close Button (Mockup ✕ button)
  closeBtn_ = new QPushButton("✕", headerWidget);
  closeBtn_->setObjectName("closeBtn");
  closeBtn_->setFixedSize(18, 18);
  closeBtn_->setCursor(Qt::ArrowCursor);
  closeBtn_->setStyleSheet(
      "QPushButton#closeBtn {"
      "  color: #ff5555;"
      "  background: transparent;"
      "  border: none;"
      "  font-size: 12px;"
      "  font-weight: bold;"
      "  padding: 0px;"
      "  border-radius: 3px;"
      "}"
      "QPushButton#closeBtn:hover {"
      "  color: #ffffff;"
      "  background-color: #e81123;"
      "}");
  void* winHandle = entry.window;
  connect(closeBtn_, &QPushButton::clicked, this, [this, winHandle]() {
    emit closed(winHandle);
  });
  headerLayout->addWidget(closeBtn_);
  cardLayout->addWidget(headerWidget);

  // --- Divider Line ---
  dividerLine_ = new QFrame(this);
  dividerLine_->setFrameShape(QFrame::HLine);
  dividerLine_->setFixedHeight(1);
  dividerLine_->setStyleSheet("background-color: #383840; border: none;");
  cardLayout->addWidget(dividerLine_);

  // --- Thumbnail Body ---
  thumbLabel_ = new QLabel(this);
  thumbLabel_->setFixedSize(kThumbWidth, kThumbHeight);
  thumbLabel_->setAlignment(Qt::AlignCenter);
  thumbLabel_->setStyleSheet(
      "background-color: #19191d; border-radius: 4px; border: 1px solid #2e2e34;");

  // Show app icon as fallback placeholder while live thumbnail loads.
  if (!entry.appIcon.isNull()) {
    thumbLabel_->setPixmap(entry.appIcon.scaled(
        48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
  cardLayout->addWidget(thumbLabel_);

  // Request live window screenshot.
  if (provider && provider->isAvailable()) {
    const QSize targetSize(kThumbWidth, kThumbHeight);
    provider->requestThumbnail(
        entry.uuid, targetSize, [this, targetSize](const QPixmap& pixmap) {
          if (!pixmap.isNull() && thumbLabel_) {
            thumbLabel_->setPixmap(pixmap.scaled(
                targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
          }
        });
  }
}

void WindowPreviewCard::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    emit activated(window_);
  }
}

void WindowPreviewCard::enterEvent(QEnterEvent* /*event*/) {
  setStyleSheet(
      "#WindowPreviewCard {"
      "  background-color: #2c2c33;"
      "  border: 1px solid #0078d4;"
      "  border-radius: 8px;"
      "}");
}

void WindowPreviewCard::leaveEvent(QEvent* /*event*/) {
  setStyleSheet(
      "#WindowPreviewCard {"
      "  background-color: #242429;"
      "  border: 1px solid #3c3c44;"
      "  border-radius: 8px;"
      "}");
}

// ============================================================================
// WindowPreviewPopup Implementation
// ============================================================================

WindowPreviewPopup::WindowPreviewPopup(MultiDockModel* model,
                                       ThumbnailProvider* provider,
                                       Qt::Orientation dockOrientation,
                                       QWidget* parent)
    : QWidget(parent,
              Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      model_(model),
      provider_(provider ? provider : KwinScreenshotProvider::instance()),
      dockOrientation_(dockOrientation) {
  setAttribute(Qt::WA_ShowWithoutActivating, true);
  setAttribute(Qt::WA_TranslucentBackground, true);

  setStyleSheet(
      "WindowPreviewPopup {"
      "  background-color: rgba(22, 22, 26, 0.96);"
      "  border: 1px solid rgba(255, 255, 255, 0.14);"
      "  border-radius: 10px;"
      "}");

  cardsLayout_ = new QHBoxLayout(this);
  cardsLayout_->setContentsMargins(8, 8, 8, 8);
  cardsLayout_->setSpacing(8);
}

void WindowPreviewPopup::setWindows(
    const std::vector<WindowPreviewEntry>& entries) {
  entries_ = entries;

  // Clear existing preview cards.
  QLayoutItem* item;
  while ((item = cardsLayout_->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  // Create cards arranged SIDE BY SIDE horizontally.
  for (const auto& entry : entries_) {
    auto* card = new WindowPreviewCard(entry, provider_, this);
    connect(card, &WindowPreviewCard::activated,
            this, &WindowPreviewPopup::onCardActivated);
    connect(card, &WindowPreviewCard::closed,
            this, &WindowPreviewPopup::onCardClosed);
    cardsLayout_->addWidget(card);
  }

  adjustSize();
}

void WindowPreviewPopup::positionNear(const QRect& iconGlobalRect, const QWidget* dockPanel) {
  adjustSize();
  const QSize sz = sizeHint();

  int x = 0;
  int y = 0;

  if (dockOrientation_ == Qt::Horizontal) {
    // Center horizontally over the hovered icon.
    x = iconGlobalRect.center().x() - sz.width() / 2;

    int dockTopGlobal = iconGlobalRect.top();
    int dockBottomGlobal = iconGlobalRect.bottom();
    if (dockPanel) {
      const QPoint dockOrigin = dockPanel->mapToGlobal(QPoint(0, 0));
      dockTopGlobal = dockOrigin.y();
      dockBottomGlobal = dockOrigin.y() + dockPanel->height();
    }

    // Position above the dock with a clean 16px uplift gap so it never touches or overlaps the dock.
    y = dockTopGlobal - sz.height() - 16;
    if (y < 0) {
      // Top dock: position below the dock with a 16px gap.
      y = dockBottomGlobal + 16;
    }
  } else {
    // Vertical dock.
    y = iconGlobalRect.center().y() - sz.height() / 2;
    int dockRightGlobal = iconGlobalRect.right();
    if (dockPanel) {
      dockRightGlobal = dockPanel->mapToGlobal(QPoint(dockPanel->width(), 0)).x();
    }
    x = dockRightGlobal + 16;
  }

  // Clamp to screen boundaries.
  const auto* screen = QGuiApplication::primaryScreen();
  if (screen) {
    const auto sg = screen->geometry();
    x = qBound(sg.left() + 8, x, sg.right() - sz.width() - 8);
    y = qBound(sg.top() + 8, y, sg.bottom() - sz.height() - 8);
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

void WindowPreviewPopup::onCardActivated(void* window) {
  hide();
  WindowSystem::activateWindow(window);
}

void WindowPreviewPopup::onCardClosed(void* window) {
  WindowSystem::closeWindow(window);
}

}  // namespace crystaldock
