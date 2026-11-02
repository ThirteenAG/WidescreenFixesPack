<div align="center">

<img src="https://thirteenag.github.io/screens/farcry/main2.jpg" width="760" alt="Far Cry Fusion Fix">

**Far Cry Fusion Fix** adds proper widescreen support, corrects aspect ratio, HUD, field of view and FMV playback, and adds optional post-processing effects.

[Website](https://thirteenag.github.io/wfp#farcry) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/FarCry.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (4:3, 16:9, 21:9, 32:9 or custom)
- **Field of View** - corrected for widescreen
- **FMVs** - fullscreen videos are scaled and centered at the correct aspect ratio
- **Text** - drawn without the widescreen HUD scaling, keeping it at its original proportions

---

## New Options

- **HUD Aspect Ratio Constraint** - Keeps the HUD inside a centered 4:3, 16:9, 21:9, 32:9 or custom area. Auto uses the full window width.
- **Skip intro** - skip the five startup logo/legal movies while retaining story videos and normal startup checks. Enabled by default; changes require restart.
- **Movie framing** - Movies keep their proportions. Original shows them as the game does, whole inside its centred 4:3 frame with black bars around them; Fill fits the whole movie to the screen. Nothing is cut off or stretched. Changes apply while playing.
- **Window mode** - Choose Fullscreen, Windowed or Borderless while playing. Alt+Enter switches between fullscreen and your last window mode. Window placement is remembered.
- **Confine cursor** - Keeps the pointer inside the active game window and releases it when you switch away or turn the setting off. Changes apply while playing.
- **FOV Factor** - Scales the gameplay view from 0.5 to 2.0; 1.0 is neutral. The final horizontal view is limited to 150 degrees.
- **Frame rate limit** - 0 is unlimited, -1 follows the monitor refresh rate, and positive values use 30–1000 FPS. Native Vertical Sync can impose a lower limit.
- **Pause when unfocused** - Pauses single-player simulation and its timers while another window has focus. Existing manual pauses are preserved.
- **Cutscene borders** - Both keeps the original cinematic frame on wide and narrow windows. Letterbox keeps top and bottom borders, Pillarbox keeps side borders, and Off extends the view. Borders can animate over a quarter of a second.
- **Check for updates** - Disabled by default. When enabled, checks once per start and shows an update notice in Plugins.
- **SMAA** - enhanced subpixel morphological antialiasing as a post-processing effect
- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/farcry/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/farcry/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |

</div>

# Ultrawide Screenshot

![scb](https://thirteenag.github.io/screens/farcry/ultrawide.jpg)

---

## Settings and hub

Open **Options → Plugins → Far Cry Fusion Fix** from the title or pause menu. More pages and Previous pages move through the settings; F1 opens full help for the selected option. Vanilla, Recommended and Enhanced presets are available. Changes that need a restart show a notice until their original value is restored or the game is restarted.

The native Plugins menu supports English, French, German, Italian and Spanish. Select the text language in FusionFixHub or the CFG and reopen Plugins. Ctrl+F10 opens the optional Supporter overlay; Debug → Info shows the build and graphics information.

Choose the settings language on fusionfix.io or in FusionFixHub. The first launch writes the settings file next to the fix. FOV factor, HUD aspect constraint, movie framing, cutscene borders, frame rate limit, focus pause, update checks, window mode and cursor confinement apply while the game runs. Intro skipping, SMAA and console gamma require a restart.

Texts are available in English, French, German, Italian and Spanish.

## Known limits

- Vanilla turns SMAA off; Recommended and Enhanced turn it on.
- Pause when unfocused applies to single-player; it does not pause multiplayer.
- The PC game has no native gameplay controller bindings. This fix keeps the native keyboard and mouse controls; it does not emulate keys from a controller.
- Use the supported patched executable.

Movie framing also respects smaller videos embedded in menus: they are shown whole with black bars. Subtitles and other interface elements keep their normal placement.

Skip intro removes only the startup logos. Story videos, menu music and normal startup checks remain.

Window mode supports framed, borderless and fullscreen output. Framed placement is remembered, and the game restores the previous mode if a change fails.

Cursor confinement follows focus and releases the pointer when disabled. Compatibility can depend on other input wrappers installed alongside the fix.

The FOV multiplier follows the normal player view. Aiming, death views, camera overrides and scripted views keep their intended framing.

## Installation

1. Download the `.zip` from this release.
2. Extract the archive into the installation folder that contains `Bin32` and `Bin64`. Keep those folders: each contains the loader, its matching fix and FusionFixHub beside that version of `FarCry.exe`.
3. Launch the game once to create the settings file next to the fix: `FarCry.FusionFix.cfg` for 32-bit or `FarCry64.FusionFix.cfg` for 64-bit.
4. Configure the fix in Options → Plugins, on fusionfix.io or in FusionFixHub. Restart only when the settings menu requests it.

> [!NOTE]
> Installing [SilentPatch for Far Cry](https://github.com/CookiePLMonster/SilentPatchFarCry/releases/latest) is strongly recommended - it's the perfect companion to this fix.

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
