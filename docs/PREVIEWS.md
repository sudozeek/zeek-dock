# Window Previews

The task manager can show a popup when the pointer rests over a running application icon. Previews are disabled by default and can be enabled in Task Manager Settings.

When a preview card is constructed, it requests a thumbnail from KWin's `org.kde.KWin.ScreenShot2` D-Bus interface. Requests run asynchronously. The card initially displays the application's icon; if capture does not return an image, the title and icon remain visible without a live thumbnail.

KWin requires the installed desktop entry to declare restricted D-Bus access with:

```ini
X-KDE-DBUS-Restricted-Interfaces=org.kde.KWin.ScreenShot2
```

The project desktop-entry template includes this declaration. Other compositors and their capture support have not been verified. The preview cards are rebuilt when the hovered application's window list changes; asynchronous callback lifetime during that replacement remains a known risk.
