# Zeek Dock Manual Test Checklist

Run these checks on KDE Plasma 6 Wayland after installing the desktop entry. Leave items unchecked until tested on the target machine.

## Dock and Taskbar

- [ ] Dock appears after launch and has a transparent icons-only default appearance.
- [ ] Hover zoom responds and returns to normal when the pointer leaves.
- [ ] Running applications appear in the taskbar.
- [ ] Clicking a running application focuses it; clicking the active item minimizes it when click-to-minimize is enabled.
- [ ] Always visible, auto-hide, and intelligent auto-hide work and persist after restart.

## Previews and Startup

- [ ] A fresh configuration starts with hover previews off and causes no ScreenShot2 request during hover.
- [ ] Enabling previews in Task Manager Settings opens a preview card for a running window.
- [ ] KWin supplies a thumbnail when ScreenShot2 capture is available; the card remains usable when capture is unavailable.
- [ ] The preview choice persists after restarting the dock.
- [ ] KDE Autostart can be enabled and disabled using the installed Zeek Dock application entry.

## Build Checks

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```
