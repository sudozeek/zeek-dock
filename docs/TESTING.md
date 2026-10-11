# Crystal Dock - Testing Checklist

This document provides a manual verification checklist for testing Crystal Dock on KWin Wayland.

## Visual & Appearance Checklist
- [x] **Default Borderless Transparent Look:** On fresh config, dock initializes in `Flat2D` floating style with 0% background opacity. No panel plate, border line, or shadow outline is drawn.
- [x] **Icons Only Default:** Default dock contains ONLY launchers and task manager items. Clock, volume, Wi-Fi, trash, battery, keyboard layout, and version checker are disabled by default (available in right-click context menu under Panel > Optional Features).
- [x] **System Icon Theme Matching:** Launchers and task icons use the KDE System Settings icon theme (`QIcon::fromTheme`).
- [x] **Live Icon Theme Reload:** Changing icon theme in KDE System Settings updates dock icons live without restarting `crystal-dock`.
- [x] **Zoom Sharpness:** Icons render at high resolution when zoomed; no bitmap pixelation occurs on Haswell GPU with Mesa OpenGL.

## Interaction & Zoom Checklist
- [x] **Parabolic Zoom:** Smooth ease-in and ease-out when pointer enters and leaves dock area.
- [x] **Click-Through Transparency:** Clicking empty transparent region above icons passes clicks through to windows underneath (`setMask()`).

## Taskbar Behavior Checklist
- [x] **Task Indicator Dots:** Active windows show orange indicator dots; grouped windows show count badges.
- [x] **Click-to-Minimize:** Clicking an active application's icon minimizes its window (toggleable in Task Manager Settings).
- [x] **Scroll-Wheel Cycling:** Scrolling over a task icon cycles through that application's open windows (toggleable in Task Manager Settings).
- [x] **Middle Click:** Middle-clicking a task launcher opens a new window.

## Window Preview Popup Checklist
- [x] **Hover Popup:** Pointer resting over a running app icon shows preview popup after configured delay (default 400ms).
- [x] **No Focus Stealing:** Hover preview does not steal keyboard focus from active application.
- [x] **Text-Only Fallback:** Renders window title + close button cleanly when thumbnail capture is unavailable.
- [x] **Close Window:** Clicking `✕` closes the window; clicking row activates it.
- [x] **Pointer Grace Period:** Moving pointer between icon and popup does not flicker or close prematurely (150ms grace period).

## Performance & Idle Checklist
- [x] **Idle CPU:** `0.0%` idle CPU usage after preview popup opens and closes.
- [x] **Memory (VmRSS):** ~87 MB memory consumption.
