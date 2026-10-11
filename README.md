# Zeek Dock

An icon-only Linux dock that combines pinned launchers with a taskbar for running windows.

## Screenshots

### Icons Only (No Background)

The default borderless, background-free dock style.

![Zeek Dock using Icons Only with no background](images/zeek%20dock%20img%202.png)

### Right-Click Menu

Right-click the dock to open its menu. From here, open **Dock Settings**, **Task Manager Settings**, or use the **Appearance** and **Panel** submenus.

![Zeek Dock right-click menu with settings and appearance options](images/zeek%20dock%20img%203.png)

### Glass 3D

Choose **Appearance > Style: Glass 3D** for the translucent three-dimensional panel.

![Zeek Dock Glass 3D panel style](images/zeek%20dock%20img%204.png)

### Glass 2D

Choose **Appearance > Style: Glass 2D** for the translucent two-dimensional panel.

![Zeek Dock Glass 2D panel style](images/zeek%20dock%20img%205.png)

> GIF placeholder: an interaction recording can be added later.

## Features

- Floating, transparent Flat2D icons-only panel is the fresh-install default.
- Tracks running windows alongside pinned launchers; supports grouping, focus, and configurable click-to-minimize behavior.
- Configurable dock position, zoom, panel appearance, and visibility modes: always visible, auto-hide, and intelligent auto-hide.
- Optional panel components include the application menu, volume, Wi-Fi, battery, clock, calendar, trash, virtual desktop pager, and keyboard layout selector.
- Optional hover window preview cards, disabled by default. On KWin, cards request thumbnails through `org.kde.KWin.ScreenShot2`; if capture is unavailable, the card remains with its title and application icon.

## What Is Different from Crystal Dock

Compared with the Crystal Dock baseline recorded in this repository, this fork changes the fresh panel defaults to a transparent, icons-only taskbar composition; adds click-to-minimize and scroll-to-cycle task settings; and adds configurable hover preview cards backed by KWin's ScreenShot2 D-Bus interface. These are code-level differences visible in this fork; this README makes no claims about features upstream cannot support.

## Supported Environment

Tested on KDE Plasma 6 Wayland on Arch Linux. Other Wayland compositors are not currently verified. Window management and live preview capture depend on compositor support; ScreenShot2 thumbnails are KWin-specific.

## Requirements

- CMake 3.20 or newer and a C++20 compiler.
- Qt 6 with DBus, Gui, Test, Widgets, and GuiPrivate development components. CMake requests Qt 6.6 or newer; Qt 6.9+ is the verified configuration for this build.
- Wayland Client development files, version 1.22 or newer.
- LayerShellQt 6.0 or newer.

On Arch Linux:

```sh
sudo pacman -S --needed base-devel cmake qt6-base qt6-wayland layer-shell-qt wayland
```

## Build and Install

Clone the public repository:

```sh
git clone https://github.com/sudozeek/zeek-dock.git
cd zeek-dock
```

Build and install for the current user:

```sh
cmake -S src -B build -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build -j"$(nproc)"
cmake --install build
```

For a system-wide install, configure with `-DCMAKE_INSTALL_PREFIX=/usr`, then run `sudo cmake --install build`.

On KWin, install the application before launching it. KWin grants the required KDE Wayland protocol and restricted D-Bus access through the installed desktop entry; running the executable directly from the build directory does not provide that installed application metadata. The generated entry declares `org_kde_plasma_window_management` and `org.kde.KWin.ScreenShot2`.

## Run and Configure

Launch **Zeek Dock** from the application menu, or run the installed executable in a terminal to see its logs:

```sh
zeek-dock
```

The CMake target installs the `zeek-dock` command. In Settings, open **Task Manager Settings** and enable **Enable window hover preview popups** to turn previews on. The setting is saved in the appearance configuration. Existing users keep their saved value; only fresh configurations use the default, which is off.

Right-click the dock to open its menu. **Dock Settings** opens the appearance settings dialog for panel appearance, sizing, and zoom. For quick style changes, choose **Appearance > Style: Icons Only (No Background)**, **Style: Glass 3D**, or **Style: Glass 2D**. Use **Panel > Position** to place the dock, **Panel > Visibility** to choose Always Visible, Intelligent Auto Hide, Auto Hide, or Always On Top, and **Panel > Optional Features** to show or hide applets. Open **Task Manager Settings** for running-window behavior and hover-preview options. Configuration is stored under `$XDG_CONFIG_HOME/zeek-dock/` (normally `~/.config/zeek-dock/`), in desktop-specific files such as `KDE/appearance.conf` and `KDE/panel_1.conf`.

There is no start-at-login toggle in the application. On KDE Plasma, use **System Settings > Autostart** to add the installed Zeek Dock application; remove or disable that entry to stop automatic startup.

## Hover Previews

Previews are off by default. Enable them in **Task Manager Settings**. On KWin, thumbnail requests use the `org.kde.KWin.ScreenShot2` D-Bus interface and require the installed Zeek Dock desktop entry to declare that restricted interface. The project-generated desktop entry includes this declaration. If capture is unavailable, the preview card still shows its window title and application icon, without a live thumbnail.

## Known Issues

- The thumbnail callback is asynchronous and currently retains a raw pointer to its preview card. Replacing cards while a capture is in flight can leave a callback targeting a deleted card. This is a code-level lifetime risk consistent with the reported crash when moving between icons with previews enabled; it has not been reproduced in this verification run.
- The visibility-setting save path writes the selected mode to the panel configuration. The earlier report of visibility not saving was not confirmed as a current code defect; runtime persistence still needs manual verification.
- Live thumbnail capture is KWin-specific. Other compositors have not been tested, and may show only the preview card's title and application icon.
- The optional Version Checker currently compares against Crystal Dock's upstream release feed; it is not a Zeek Dock update channel.

## Troubleshooting

- If running applications do not appear, launch the installed application entry on KWin and confirm its desktop file includes `X-KDE-Wayland-Interfaces=org_kde_plasma_window_management`. Then run `zeek-dock` in a terminal to inspect startup and runtime messages.
- To obtain a backtrace, stop the running dock and start the installed executable under GDB: `gdb --args zeek-dock`, then enter `run`. After a crash, enter `bt`.
- If previews show no thumbnail, confirm that the installed desktop entry declares `X-KDE-DBUS-Restricted-Interfaces=org.kde.KWin.ScreenShot2` and that the session is running KWin.

## Uninstall

From the configured build directory, run `cmake --build build --target uninstall`. Use the same privileges used for installation; the target removes the files listed in that build's install manifest.

## Contributing

Bug reports and focused patches are welcome. Please include your desktop environment, compositor, build command, relevant terminal output, and reproduction steps. Do not include personal configuration files, logs with private data, or core dumps.

## Credits

Zeek Dock is a modified version of Crystal Dock by Viet Dang (`dangvd`), available at <https://github.com/dangvd/crystal-dock>. Zeek Dock remains licensed under GPL-3.0. Upstream copyright and license notices are retained; see [CREDITS](CREDITS.md) and [LICENSE](LICENSE).

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).
