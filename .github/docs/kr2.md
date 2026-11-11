<div align="center">

<img src="https://thirteenag.github.io/screens/kr2/main2.jpg" width="760" alt="Knight Rider 2: The Game Fusion Fix">

**Knight Rider 2: The Game Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#kr2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KnightRider2.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution and aspect ratio** — Supports custom resolutions, including render targets larger than the original 2048-pixel limit. The gameplay view widens at the sides while retaining its original height.
- **HUD** — Auto places the HUD near the screen edges; a choice of centered HUD boxes is available. The cockpit retains its original centered 4:3 view and instruments, with black side areas on wider screens. Menu artwork keeps its original proportions.
- **Videos** — Original fits the whole video in the original 4:3 presentation. Fill preserves the picture and skips the black borders encoded in the intro and new game videos.
- **Frame rate** — An adjustable limiter replaces the native limits, and game time follows real time.
- **Window modes** — Choose fullscreen, framed windowed or borderless. Resolution and mode changes require a restart.
- **Intro skip** — Skips the startup logos and legal screens; mission scenes remain available.
- **Start-up abort** — Fixes the start-up abort on systems where the game's mouse setup fails.

## Options

- **Field of view factor** — 0.5–2.0, live; 1.0 keeps the intended gameplay view. Story shots keep their own framing.
- **Resolution width / height** — 0 uses the desktop resolution; restart required.
- **Window mode** — Fullscreen, Windowed or Borderless; restart required. Borderless uses the desktop resolution.
- **HUD aspect ratio constraint** — Auto uses the screen edges; 4:3, 16:9, 21:9 or 32:9 places the HUD inside a centered box. Applies live.
- **Frame rate limit** — 0 is unlimited, −1 follows the display refresh rate, and a positive value selects the cap. Default 60; applies live.
- **Cutscene borders** — Off or Pillarbox; applies live. Pillarbox keeps the original story framing with side borders.
- **Video framing** — Original or Fill, as described above.
- **Confine cursor** — Keeps the cursor inside the focused game window; applies live.
- **Skip intro / Skip press start** — Enabled by default; restart required.
- **Controller icons** — Auto, Xbox, PlayStation or Nintendo button labels; applies live. Auto uses Xbox labels because XInput does not identify the controller family.
- **Text language** — Auto, English, French, German, Italian or Spanish for the fix's settings and help.
- **Check for updates** — Optional update notice at the main menu; disabled by default.

Settings can be changed on fusionfix.io or in FusionFixHub. Recommended is the default preset; Vanilla and Enhanced are also available. No preset changes the resolution. Supporters can open the developer overlay with Ctrl+F10, or Start and View together on a controller. The game's fixed options scenes do not support an additional Plugins page.

## Controller

The archive includes Xidi for the game's native DirectInput controls. Connect an XInput controller, or expose a PlayStation or Nintendo controller through Steam Input or DS4Windows.

Use the left stick to steer, RT to accelerate and LT to brake. A/B/X/Y select laser, plasma, rockets or no weapon; RB fires the selected offensive gadget, LB jumps, left-stick click resets the car, right-stick click scans and View changes camera. The right stick operates shield, micro jammer and target/camera cycling. The d-pad selects Super Pursuit Mode, targets and ski modes in gameplay; menus use the d-pad, A to confirm and B to go back. Menu pauses or continues. In the pause menu's quit question, A answers yes and B no. The Controls page and the front-end menus follow the last input device; the in-game pause, quit, retry and loading boxes keep their keyboard key pictures.

## Installation

1. Extract the fix archive beside the game executable, preserving its folders and the included Xidi files.
2. Start the game once to create `scripts/KnightRider2.FusionFix.cfg`.
3. Change settings in FusionFixHub or on fusionfix.io.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
