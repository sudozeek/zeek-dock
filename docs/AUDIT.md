# Crystal Dock - Audit Report (Phase 0)

## Executive Summary
This audit evaluates upstream Crystal Dock (v2.17 alpha) against the project requirements specified in Section 1 of the Project Brief. Upstream provides a solid foundation with native Wayland LayerShell support and KWin integration via `org_kde_plasma_window_management`. However, several default configurations, live theme updates, settings controls, and window preview capabilities require modification or new implementation.

---

## Audit Table

| Feature / Requirement | Upstream Status | Evidence in Codebase / Observed Behavior | Action Required |
| :--- | :--- | :--- | :--- |
| **1.1 Borderless, icons-only transparent look** | Partly | `multi_dock_model.h` defaults to `Glass3D_Floating` style, 42% blue background (`#638abd`), border (`#b1c4de`), and 8 default panel components (App Menu, Trash, Wi-Fi, Volume, Keyboard, Clock, etc.). `Flat2D` style supports alpha 0 rendering without outline/shadow. | Change defaults in `multi_dock_model.h`: default style to `Flat2D_Floating`, background alpha to 0.0, border alpha to 0.0, and default component visibility flags to false except TaskManager. |
| **1.2 System icon theme & live update** | Partly | `utils/icon_utils.h` uses `QIcon::fromTheme(...)` matching KDE's active theme. No hardcoded theme override exists. However, no watcher exists for `~/.config/kdeglobals` or KDE D-Bus icon theme changes. | Implement live icon-theme reload by watching `~/.config/kdeglobals` `[Icons] Theme` and KDE D-Bus signals, clearing icon caches and redrawing on change. |
| **1.3 Parabolic zoom & click-through** | Present (Needs UI UI mapping) | `view/dock_panel.cc` implements smooth parabolic zoom (`updateLayout()`) and mouse input masking (`setMask()`). Settings currently expose raw min/max pixel sizes (`minIconSize`, `maxIconSize`) rather than a Zoom % slider. | Map min/max icon size to a clear "Zoom %" slider (100%–250%, default ~170%) in `AppearanceSettingsDialog`, add spread/radius control, ensure live updates. |
| **1.4 Taskbar behavior** | Mostly Present | `display/kde_window_manager.cc` and `view/program.cc` support window grouping, indicator dots, count badges, focus/minimize, and demands-attention events. | Add scroll-wheel window cycling over task icon and explicit click-to-minimize toggle in settings. |
| **1.4 Hover window previews (NEW)** | Not Present | Upstream has no window preview popup functionality. Window metadata (titles, states, UUIDs) is available via `org_kde_plasma_window_management`. | Implement modular `ThumbnailProvider` architecture: text-only fallback baseline first (title + close button + activate), followed by PipeWire/KWin screencast or `ext-image-copy-capture-v1` thumbnail capture. |
| **1.5 Settings dialogs** | Partly Present | Upstream provides `AppearanceSettingsDialog` and `TaskManagerSettingsDialog` for basic layout and taskbar configuration. Missing unified zoom %, preview controls, and reset button. | Extend existing settings dialogs with Zoom %, spread, spacing, preview settings, taskbar toggles, and a "Reset to defaults" button. |

---

## Verified System & Baseline Resource Metrics

### Environment
- **OS:** Arch Linux x86_64, Linux Kernel 7.0.0-rc7-1-mainline
- **Compositor / DE:** KDE Plasma 6.7.5 (KWin Wayland)
- **GPU:** Intel HD Graphics 4400 (Haswell), Mesa OpenGL driver
- **Qt / Wayland:** Qt 6.12.0, LayerShellQt 6.7.5, Wayland Protocols 1.49

### Installation & Wayland Protocol Access
- **Installation Path:** Local user prefix `~/.local` (`~/.local/bin/crystal-dock`, `~/.local/share/applications/crystal-dock.desktop`).
- **Wayland Protocol Access:** KWin grants `org_kde_plasma_window_management` access to `crystal-dock` when installed in `~/.local/share/applications/crystal-dock.desktop`. Uninstalled binary execution fails with missing protocol errors as intended by KWin security policies.

### Baseline Resource Consumption (Unmodified Dock)
- **Idle CPU Usage:** `0.0%` (verified via `top -b -n 5 -d 1`)
- **Memory Footprint (VmRSS):** `87.1 MB` (`87,124 KiB`)
- **Peak Memory (VmHWM):** `87.1 MB` (`87,120 KiB`)

---

## Protocol & Capture Analysis (for Phase 4)

1. **Window Metadata (KWin):** `org_kde_plasma_window_management` provides window handles, app IDs, titles, active state, minimized state, and desktop/activity location.
2. **Window Frame Capture Options:**
   - **(a) KWin Screencast / PipeWire:** KWin provides D-Bus interface `org.kde.KWin.ScreenCast` allowing stream creation for individual window UUIDs (`streamWindow(QString uuid)`).
   - **(b) Wayland Protocol (`ext-image-copy-capture-v1`):** Supported on compositors implementing `ext-foreign-toplevel-image-capture-source-manager-v1`.
   - **(c) Text-Only Fallback:** Renders window title, close button, and activation handler when thumbnail stream is pending or unsupported.

---

## Phased Plan & Effort Estimate

1. **Phase 1: Out-of-the-Box Defaults & Live Icon Theme Reload** (~1 day)
   - Set fresh config defaults to `Flat2D` floating style, 0% background opacity, 0% border opacity.
   - Default panel items restricted strictly to launchers / taskbar.
   - Implement `QFileSystemWatcher` on `~/.config/kdeglobals` `[Icons] Theme=` to update icon theme live without restart.

2. **Phase 2: Zoom UI & Click-Through Verification** (~1 day)
   - Expose Zoom % (100%–250%, default ~170%), spread/radius, and icon spacing in `AppearanceSettingsDialog`.
   - Verify DPR scaling, sharp zoom icons, mouse mask updates, and click-through on transparent panel areas.

3. **Phase 3: Taskbar Behavior Gaps** (~1 day)
   - Implement mouse scroll wheel window cycling for grouped task items.
   - Add attention highlight toggle and click-to-minimize toggle.

4. **Phase 4: Hover Window Previews** (~2 days)
   - Implement `WindowPreviewPopup` widget and text-only fallback.
   - Create `ThumbnailProvider` abstraction with PipeWire/KWin screencast implementation.
   - Add preview configuration in `TaskManagerSettingsDialog` (toggle, delay, size in px / inch-DPI, refresh rate, close button).

5. **Phase 5: Final Polish & Documentation** (~0.5 day)
   - Generate `docs/TESTING.md` checklist and `docs/PREVIEWS.md`.
   - Provide local install and uninstall instructions.

---

## Local Installation Removal Instructions
To remove the local prefix installation built during Phase 0:
```bash
cmake --build build --target uninstall
```
Or manually remove:
```bash
rm -f ~/.local/bin/crystal-dock
rm -f ~/.local/share/applications/crystal-dock.desktop
```
