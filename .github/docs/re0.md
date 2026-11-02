<div align="center">

<img src="https://thirteenag.github.io/screens/re0/main2.jpg" width="760" alt="Resident Evil 0 Fusion Fix">

**Resident Evil 0 Fusion Fix** adds native Plugins settings, movie framing, frame-rate and focus controls, controller button icons and window options. It also scales the TV noise filter for high resolutions and retains door skipping, visual filter choices and Logitech RGB lighting.

[Website](https://thirteenag.github.io/wfp#re0) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvil0.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Noise filter at high resolutions** — The film grain, scanlines and rolling bar of the noise filter are drawn in screen pixels, so they shrank at 1440p and above. They are scaled by whole multiples of the screen height over 1080; 1080p and below look as before.

## Options

- **Skip intro** — Go straight to the title without the logos. Save and load notices that only need OK close by themselves.
- **Skip title prompt** — Continue automatically past the opening Start Game prompt.
- **Confine cursor** — Keep the pointer inside the focused game window. Turning it off, changing focus or opening the developer overlay releases it.
- **Movie framing** — Original shows movies inside the picture frame chosen by the game’s Display option (Wide or Original). Fill uses the whole screen, so with Display set to Original a widescreen movie is no longer boxed in a 4:3 frame. Both keep the whole picture at its own shape, with black bars where needed. Changes apply during playback.
- **Mouse cursor** — Hide the mouse pointer, including the menus’ own pointer, after an idle interval while the game is focused. Moving the mouse shows it again at the same place. 0 keeps the pointer visible.
- **Door animations** — Skip door transitions. Off by default; the Enhanced preset turns it on.
- **Resolutions** — The native video settings use a sorted list of distinct display sizes, keeping the highest refresh rate for each. The list accepts non-multiple-of-eight widths such as 1366 and modes through 8K.
- **Window mode** — Choose Fullscreen, Windowed or Borderless. Changes require a restart. Windowed mode accepts any size saved in the game’s settings, is sharp at high display scaling, and follows resolution changes made in the game’s Video menu.
- **Color correction** — Optionally disable color correction.
- **Noise filter** — Optionally disable the noise filter's grain.
- **Fog** — Optionally disable fog in supported rendering passes; some fog effects may remain.
- **Pause when unfocused** — Pause when another window has focus, or keep the game running in the background. This applies immediately.
- **Frame rate limit** — 60 FPS by default, with a live limit, monitor refresh rate or unlimited. Higher rates remain experimental.
- **Update notice** — Optionally check for a newer fix and read the notice in the native Options and Plugins menus. Disabled by default.
- **Menu language** — Choose the language of the fix’s menu. Automatic follows the language the game runs in.
- **Controller button icons** — Keep the native Xbox buttons with Auto or Xbox, or select PlayStation or Nintendo button glyphs. Changes apply immediately.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ResidentEvil0.FusionFix.cfg`.
4. Open the game’s **Options → Plugins → Resident Evil 0 Fusion Fix** page, or configure the fix in FusionFixHub. Restart when the menu shows a restart notice.

## Settings

The native **Options → Plugins** menu and FusionFixHub change the same settings in
`scripts/ResidentEvil0.FusionFix.cfg`. Use Left/Right to change a value, **F1** or the native **Default** action to
read its full help, and Back to return. The menu offers Recommended, Vanilla and Enhanced presets and shows one
restart notice while any startup setting differs from the running configuration.

Frame rate, movie framing, cursor confinement, background pause, cursor timeout and update checks apply while playing. Window mode, visual filters,
logo and title-prompt skipping, door skipping, and RGB lighting require a restart.

**Ctrl+F10** opens the developer overlay when available. Its supporter access uses the normal Fusion Fix account check.

## Known limits

- HUD aspect and cutscene-border controls are not provided: the game draws its pre-rendered backgrounds, characters, item and map screens and cutscenes inside its own 16:9 (Wide) or 4:3 (Original) frame and adds black bars on other screens, so nothing is stretched or cut off at any aspect ratio.

- The game uses fixed cameras over pre-rendered backgrounds, so there is no adjustable free-camera field of view.
- Keep the default 60 FPS for normal play; high-rate gameplay and scripted sequences still need validation.
- The controller illustration keeps its original Xbox labels; the button glyphs follow the selected icon family.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
