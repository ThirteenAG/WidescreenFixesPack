<div align="center">

<img src="https://user-images.githubusercontent.com/4904157/196225002-83a52973-0588-4c65-8019-581bffc7c1b8.gif" width="760" alt="Scarface: The World Is Yours Fusion Fix">

**Scarface: The World Is Yours Fusion Fix** adds in-game settings, widescreen framing and controller prompts, and corrects mouse input and framerate-dependent behavior.

[Website](https://thirteenag.github.io/wfp#scarface) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/Scarface.FusionFix)

</div>

---

## Settings

Use the bundled `vorbisfile.dll` ASI loader together with Xidi's `dinput8.dll`, `Xidi.32.dll` and `Xidi.ini`.

Open **Options → Plugins → Scarface Fusion Fix**. Other plugins with settings appear beside it. Left and Right change a value; Enter selects a page or confirms a preset. Move from an option to **Details** to read its help, with **More pages** for longer descriptions. Back returns through the pages. A separate restart line appears when a startup setting changes.

The fix creates `scripts/Scarface.FusionFix.cfg` on first launch. FusionFixHub uses the same settings. FusionFixHub or the developer overlay can override the fix's text language; Auto follows the game's language when it is supported. Vanilla, Recommended and Enhanced presets apply the settings that list preset values; controller artwork and window mode remain personal choices.

## Display and camera

- **Window mode:** native fullscreen, a framed window at the selected resolution, or monitor-sized borderless. The native display path handles resolution changes and Alt+Enter. Window framing is DPI aware.
- **Field of view:** multiplies the ordinary gameplay camera from 0.5 to 2, with an effective horizontal limit of 150°. 1 keeps the normal view unless it already exceeds the limit. Aiming, scoped views, cinematic cameras and blends keep their own camera behavior.
- **Cutscene borders:** Both keeps the original cinematic frame on wide and narrow displays. Off fills the screen and extends the sides on wider displays. Letterbox and Pillarbox keep only the named borders.
- **Movie framing:** Original fits the whole movie in a centered 4:3 area; Fill fits it to the screen.
- **HUD aspect ratio:** Auto uses the screen width; a ratio places HUD elements within a centered area. Menus retain their authored layout.
- **Frame-rate limit:** 30–1000 fps, monitor refresh, or Unlimited. Vehicle camera smoothing, animation timing, rage depletion and mortar force stay correct at any frame rate.

## Input

Controllers use the game's DirectInput support through bundled Xidi, with its XInput backend. Modern controls provide separate on-foot and vehicle mappings. The native Controls menu retains sensitivity, inversion, vibration and binding options.

**Gamepad Icons** applies live. Auto uses Xbox artwork; PlayStation and Nintendo are manual choices, with additional Xbox One, PS3/PS4 and Steam families. Prompts follow the last input device and active mappings. **Controller Camera Speed** multiplies the native right-stick values without changing mouse sensitivity. Turning off native Vibration or leaving the game window stops native feedback.

Raw mouse input uses mouse movement directly, with a separate multiplier for the native mouse slider. Vehicle recenter delay defaults to three seconds. Cursor confinement releases on focus loss, window moves, system menus and while the developer overlay owns input. **Pause in Background** pauses and mutes the game; turn it off to keep the game running in the background. The game pauses while minimized and restores its display when reopened.

## Other features

Optional SMAA, console-style gamma and PS2 character blood use the native Direct3D 9 renderer. Skip Intro skips startup legal screens, logos and the single-button load-success notice. Skip Press Start separately bypasses the start screen. Weapons use their real maximum recoil. Ctrl+F10 opens the Supporter developer overlay; Debug → Info shows the version, build time, graphics API and resolution.

Check for Updates defaults to Off and makes no request while disabled. When enabled, a newer fix version produces a notice in Options. FusionFixHub opens on the update after the game exits when it is installed.

## Known limits

- SMAA requires the game's MSAA to be disabled and has no sharpness control.
- Modern controls, startup corrections and several graphics options need a restart; the in-game warning identifies a pending restart.
- Controllers need XInput for the bundled mapping and activity tracking. Selecting an icon family changes the artwork, not the binding.
