<div align="center">

<img src="https://thirteenag.github.io/screens/bully/main2.jpg" width="760" alt="Bully: Scholarship Edition Fusion Fix">

**Bully: Scholarship Edition Fusion Fix** corrects ultrawide HUD scaling and field of view, expands the display resolution list, and preserves the selected resolution on startup. Video framing has selectable modes.

[Website](https://thirteenag.github.io/wfp#bully) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Bully.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Support ultrawide resolutions.
- **Aspect ratio** — Correct scaling on ultrawide displays.
- **HUD** — Correct HUD scaling on ultrawide displays.
- **Field of view** — Adjust the field of view for ultrawide displays.
- **Resolution list** — Use display resolutions reported by the OS, within the Video menu’s 128-entry limit.
- **Saved resolution** — Keep the last selected resolution on startup.

## Options

- **Field of view** — Adjust the FOV multiplier on fusionfix.io, in FusionFixHub or in the game's menu. Changes apply to the next camera update. The default is 1.0, with a range of 0.5–2.0. Wider gameplay views stop at 150 degrees across.

FusionFixHub can change the settings while the game runs. The fix adds **Plugins** to title Display Settings and pause Options → Display. It lists this fix first, followed by other loaded plugins with settings. Each plugin has its own settings and presets. FusionFixHub and fusionfix.io remain available.

Skip Intro skips the Rockstar and Mad Doc logos while preserving story and attract-mode movies. The resolution list uses display modes reported by your adapter. The Video menu can show up to 128 entries; if more are available, it keeps 640×480 and the largest 127 modes. The FOV multiplier affects normal on-foot and vehicle views; first-person, cutscene, conversation and shop cameras keep their original framing.

Window Mode selects fullscreen, a framed window or desktop borderless. Alt+Enter switches modes and remembers the last framed position. Switching to another application leaves the game window open. The framed window keeps the selected game resolution.

HUD Aspect Ratio applies immediately. Auto uses the full screen width; choosing a ratio moves the side HUD elements into a centered area. Centered text, character health bars and the full map keep their original placement.

Cutscene Borders offers Off, Letterbox, Pillarbox and Both, with optional animated transitions. Off removes the borders and widens the cutscene view to fill the screen. Normal gameplay framing returns after the cutscene.

The game remembers supported ultrawide resolutions between launches.

Open full option help with F1, the Help button, or Y on an Xbox controller (Triangle on PlayStation, X on Nintendo). Up and Down scroll long descriptions. Back returns to the settings page.

Pause on Focus Loss applies immediately and is enabled by default. Disable it to let the game continue running in the background. The game ignores controls while another application has focus.

Automatic Gamepad detects XInput controllers connected during play and switches bindings and button prompts to match your latest input. Disconnecting returns control to the keyboard and mouse. Gamepad Deadzone adjusts both sticks immediately (default 15%). Gamepad Camera Speed adjusts both axes. The game's invert-Y and vibration options remain available. Gamepad Icons selects Xbox, PlayStation or Nintendo pictures; Auto uses Xbox. Keep `plugins/Bully.FusionFix/buttons.nft` with the fix, since it contains the button artwork.

Raw Mouse Input uses Windows raw relative mouse movement and removes the game's horizontal movement bias. It applies immediately and keeps the game's sensitivity and bindings.

Check for Updates is off by default. When enabled, a notice appears in the game's menus if a newer fix is available. If FusionFixHub is installed beside the game, it opens after the game closes; otherwise the notice points to fusionfix.io.

Ctrl+F10 opens the developer overlay. The overlay requires a valid Supporter token; the normal Plugins menu remains available to everyone.

## Limitations

- **Videos** — Original and Fill both show the whole video at its own aspect, with black bars where needed; nothing is cut off or stretched.
- **Frame rate** — The fix keeps the game’s 30 FPS limit. It does not promise that missions, minigames or physics work correctly at higher frame rates.
- **Display modes** — The native Video screen has 128 entries. If the adapter reports more, it retains 640×480 and the largest 127 sizes.

---

**Confine cursor** is available in the Plugins menu, on fusionfix.io and in FusionFixHub and is enabled by default. It keeps the cursor inside the active game window and releases it when you switch applications, move the window or disable the option.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `plugins/Bully.FusionFix.cfg`.
4. Configure the fix on fusionfix.io, in FusionFixHub or in the game's menu.

## Notes

Open Display Settings → Plugins from the title menu, or Options → Display → Plugins while paused. Each plugin has its own preset and Restore Defaults, applied with confirmation. Keyboard, controller and mouse navigation are available.
