# 🚀 Zeek Dock

**Zeek Dock** is a modern, ultra-smooth desktop dock and taskbar designed for Linux desktops on Wayland (optimized for KDE Plasma 6, Hyprland, Sway, and other modern Wayland compositors).

It combines the aesthetic elegance of macOS and iOS docks with the productivity of Windows 11 taskbars, featuring live window hover previews, multi-instance window management, click-to-minimize, pure icon floating panels, and 60 FPS fluid auto-hide animations.

---

## ✨ Features

### 🪟 Advanced Window Previews
* **Live Thumbnails**: High-performance, real-time window frame capture using KDE Plasma 6's native `org.kde.KWin.ScreenShot2` D-Bus interface.
* **Side-by-Side Multi-Window Layout**: Applications with multiple open windows display cards arranged horizontally side-by-side rather than stacked vertically.
* **Modern Window Cards**:
  * **Title Bar**: Window/application title with elision support.
  * **Close Button**: Red **✕** button to close any individual window directly from the preview.
  * **Live Screenshot Body**: Scaled, high-quality picture of the running window with graceful high-resolution application icon fallbacks.
* **Auto-Hide Lock**: The dock remains securely visible and elevated while hovering over window previews, preventing frustrating disappearances on auto-hide panels.
* **Elevated Positioning**: Previews float cleanly above the dock icons with generous margin spacing, never overlapping the panel.

### 📌 Full Taskbar & Window Management
* **Dual Dock / Taskbar Mode**: Supports both pinned launcher applications and dynamic running window tracking.
* **Click-to-Minimize & Focus**: Clicking an active application's dock icon instantly minimizes or restores the window.
* **Running Indicators**: Sleek status dots indicating active state and count of open windows per application.
* **Intelligent App Matching**: Resolves desktop entries across reverse-DNS naming, binary commands, and custom `.desktop` definitions.

### 🎨 Visual Styles & Customization
* **Icons-Only Mode**: Floating or docked pure-icon style without background container bars.
* **iOS Glass Defaults**: Translucent frosted glass panel with customizable corner rounding, blur, and alpha transparency.
* **Diverse Panel Styles**:
  * Icons-Only (Floating & Non-Floating)
  * Glass 3D
  * Glass 2D
  * Flat 2D
  * Metal 2D
* **Fluid 60 FPS Auto-Hide**: Ultra-smooth opacity fade transitions with an adjustable animation speed control slider.
* **Interactive Parabolic Zoom**: Configurable magnification curve on mouse hover.

### 🧩 Modular Dock Applets
* **Application Menu / Launcher**
* **Window Task Manager**
* **Volume Control & Audio Output**
* **Network / Wi-Fi Manager**
* **Battery & Power Indicator**
* **Digital Clock & Calendar**
* **Trash Can**
* **Virtual Desktop Pager**
* **Keyboard Layout Switcher**

---

## 🛠️ Build and Installation

### Dependencies

Zeek Dock requires **Qt 6**, **LayerShellQt 6**, **Wayland Client**, and **CMake**:

#### Arch Linux / Manjaro
```bash
sudo pacman -S base-devel cmake qt6-base qt6-wayland layer-shell-qt wayland
```

#### Fedora
```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qtbase-private-devel layer-shell-qt-devel wayland-devel
```

#### Ubuntu / Debian (24.04+)
```bash
sudo apt install cmake g++ qt6-base-dev qt6-base-private-dev liblayershellqtinterface-dev libwayland-dev
```

---

### Building from Source

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/zeek-dock.git
cd zeek-dock

# Configure and compile
cmake -S src -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)

# Install system-wide
sudo cmake --install build
```

Alternatively, to install for your local user only:
```bash
cmake -S src -B build -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build -j$(nproc)
cmake --install build
```

---

## 🚀 Running Zeek Dock

Launch Zeek Dock from your application launcher or via terminal:

```bash
zeek-dock &
```

> **Note for KDE Plasma / KWin Users:**
> Zeek Dock utilizes privileged Wayland protocols (`org_kde_plasma_window_management`) and restricted D-Bus interfaces (`org.kde.KWin.ScreenShot2`). Ensure the installed `.desktop` file (`~/.local/share/applications/zeek-dock.desktop` or `/usr/share/applications/zeek-dock.desktop`) is present so KWin grants proper permissions.

---

## ⚙️ Configuration

* Right-click any empty area of the dock (or an applet) and select **Dock Settings** to configure:
  * Dock position (Bottom, Top, Left, Right)
  * Panel style (Icons-Only, Glass 3D, Flat 2D, Metal 2D)
  * Auto-hide behavior (Always Visible, Auto-Hide, Intelligent Auto-Hide)
  * Auto-hide fade animation speed
  * Icon zoom scale and size
* Config files are stored in `~/.config/zeek-dock/`.

---

## 📄 License

Zeek Dock is licensed under the **GNU General Public License v3.0** (GPLv3).
Based on Crystal Dock with extensive custom enhancements.
