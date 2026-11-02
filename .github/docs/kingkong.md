<div align="center">

<img src="https://thirteenag.github.io/screens/kingkong/main2.jpg" width="760" alt="Peter Jackson&#x27;s King Kong Fusion Fix">

**Peter Jackson's King Kong Fusion Fix** corrects widescreen scaling and field of view, fixes timing and camera issues, and improves mouse and gamepad input in Gamer's Edition and Signature Edition.

[Website](https://thirteenag.github.io/wfp#kingkong) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KingKong.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Compatibility** — Works with Gamer's Edition and Signature Edition (DirectX 9 and DirectX 8 executables).
- **Resolution list** — More display resolutions in the settings application. In a window the game uses the size you chose, also sizes your monitor has no display mode for.
- **Aspect ratio** — Correct widescreen aspect ratio scaling with a wider field of view.
- **HUD and menus** — Text, icons and menus keep their proportions. Menus stay in a centred 16:9 area on wider screens and the HUD can be kept in a centred box.
- **Field of view** — Adjust the field of view for widescreen.
- **Videos** — Correct FMV scaling for widescreen.

### Timing and input

- **CPU timing** — Fix excessive game speed when CPU frequency exceeds 4.294 GHz.
- **Frame rate limit** — Limit FPS, or follow the monitor refresh rate.
- **Physics** — Physics runs in fixed steps at any frame rate; Kong To The Rescue and Chased by V-Rex no longer need a 30 FPS limit.
- **Camera shake** — Fix persistent shaking at 60 FPS after the first King Kong encounter and V-Rex encounters in the V-Rex and Call Kong levels.
- **Mouse input** — Use raw input for cursor and camera movement.
- **Swamp performance** — Fix very low frame rates in the Swamps and In the Mud levels on many graphics cards.
- **Fast loading** — Gamer's Edition textures load quickly without a texture cache.
- **Gamepad** — XInput controllers work through the included Xidi, in the game and in the settings application, including vibration.

## Options

- **HUD aspect ratio** — Auto spreads the HUD and on-screen text over the whole screen; a ratio such as 16:9 keeps them in a centred box. Menus keep their 16:9 layout on wider screens. Applies while the game runs.
- **Blur** — The stuck blur in Necropolis, Brontosaurus, In The Mud and New York is removed, and the damage overlay is no longer obscured.
- **Field of view** — Widen or narrow the view in play; menus keep their framing. Applies while the game runs.
- **Mouse sensitivity** — Adjust the raw mouse multiplier while playing; enabling or disabling raw input requires restarting.
- **Broken geometry** — Objects with broken geometry or textures in Gamer's Edition are hidden.
- **Window modes** — Choose fullscreen, windowed or borderless. The window keeps the size chosen in the settings application.
- **Skip intro** — Skips the startup logo videos.
- **Pause when unfocused** — The game pauses while its window is in the background; turn it off to keep it running.

## Settings and rendering

Open **Options → Plugins** for settings and per-plugin presets (Gamer's Edition). In every edition the fix can be configured in FusionFixHub or in `scripts/KingKong.FusionFix.cfg`. Supporters can also press **Ctrl+F10** in the DirectX 9 executables for the settings overlay. Field of view, HUD aspect ratio and the frame rate limit change while the game runs. Restart the game after changing other options.

The frame limiter method can only be changed in the settings file; it applies while playing. A limit of 0 is unlimited; -1 follows the monitor refresh rate. The game no longer needs the fix to restrict it to one CPU core. The fix doesn't write the game's video settings to the registry.

Widescreen, HUD, videos, input, timing and existing Direct3D 9 shader replacements remain supported. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The Plugins page is only in Gamer's Edition; in Signature Edition use FusionFixHub or the settings file.
- Signature Edition: on screens wider than about 2.15:1 the dark background of the pause menu doesn't reach the sides of the screen.
- No cutscene-border setting; cutscenes fill the screen with a wider view. The field-of-view multiplier also applies to scripted cameras.
- The window can't be resized while the game runs (the game draws at the size it started with); choose the size in the settings application.
- The "Press SPACE to continue" screen after the logos stays; press a key to continue.
- The Ctrl+F10 overlay is not available in the DirectX 8 executable (kingkong8.exe of the Signature Edition).
- Long menu entries can continue on another line or page. Changing a value can move nearby entries to a different page.
- Controller bindings are configured with SettingsApplication.exe. Automatic full-game bindings and selectable controller prompt sets are not provided.
- Some small effects, such as swinging chains, still move faster above 60 FPS.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/KingKong.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

<details>
  <summary>Xbox 360 Control Scheme</summary>

<img width="1814" height="1417" alt="Xbox360ControlScheme" src="https://github.com/user-attachments/assets/e383ace8-434b-4ac4-addb-4be06102c294" />

</details>



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
