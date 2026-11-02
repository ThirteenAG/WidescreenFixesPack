<div align="center">

<img src="https://thirteenag.github.io/screens/sh4/main2.jpg" width="760" alt="Silent Hill 4: The Room Fusion Fix">

**Silent Hill 4: The Room Fusion Fix** adds widescreen support, a modern camera, analog controller movement and settings in the game's Options screen.

[Website](https://thirteenag.github.io/wfp#sh4) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SilentHill4.FusionFix/dllmain.cpp)

</div>

## Installation

Extract the release archive beside `Silent Hill 4.exe`, preserving its folders. Launch the game to create `scripts/SilentHill4.FusionFix.cfg`. Settings are also available in FusionFixHub and on fusionfix.io. Restart when an option requires it.

## Settings

The fourth Options page, **Plugins**, lists installed plugins with settings. Open **Fusion Fix** to change its options. Longer lists scroll as you move through them. **Preset** selects Vanilla, Recommended or Enhanced; **Restore Defaults** resets that plugin's settings. F1 or the map button opens the selected option's complete description. A separate notice identifies changes that need a restart. Camera mode remains the player's choice when changing presets.

- **Display** chooses fullscreen, a framed window or borderless mode. Alt+Enter switches between fullscreen and the last window mode. Gamma also works in a window.
- **Camera** retains the classic view or enables the modern camera outside the apartment, with speed, inversion, distance and field of view controls.
- **Gameplay FOV** uses 1.0 as neutral and limits widening to 150 degrees across. Aiming and scripted camera shots retain their own projection.
- **Cutscene borders** — Off fills the screen; Letterbox keeps the original top and bottom bars; Pillarbox keeps the 4:3 framing on wider screens; Both combines them.
- **Movies** — Original shows them as the game does, whole inside its centred 4:3 frame with black bars around them; Fill fits the whole movie to the screen. Nothing is cut off or stretched.
- **HUD width** moves the health, charge and interaction indicators to the screen edge or a centered aspect-ratio box. Other interface elements retain their centered layout.
- **Pause in background** pauses gameplay while the game window is inactive.
- **Confine cursor** keeps the pointer inside the active game window and releases it on focus loss, window dragging or system menus.
- **Controller icons** offers Auto/Xbox, PlayStation and Nintendo. Auto uses Xbox; the other families are selected manually. Artwork applies to controller-binding fields.
- **Check for Updates** is off by default. When enabled, available updates appear in the native Options screen.

The archive includes Xidi with its built-in XInput backend. Other controllers can use Steam Input or DS4Windows. Existing custom bindings are retained; the modern default layout replaces the retail layout when it is still unchanged. Analog movement and the modern camera use the game's controller input.

## Known limits

- HUD width does not reposition the complete inventory or every interface element.
- Hardcoded controller instructions retain the game's text; keyboard and mouse binding labels stay native.
- Display resolution changes require a restart.
- The native frame-rate limit remains. There is no developer overlay for Direct3D 8.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
