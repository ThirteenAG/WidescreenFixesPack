<div align="center">

<img src="https://thirteenag.github.io/screens/kr2/main2.jpg" width="760" alt="Knight Rider 2: The Game Widescreen Fix">

**Knight Rider 2: The Game Widescreen Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#kr2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/KnightRider2.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options. Resolutions wider or taller than 2048 pixels work. Out-of-range values fall back to the desktop, then 640×480.
- **Aspect ratio and field of view** — Hor+: the original vertical view, wider at the sides.
- **HUD** — Kept at 4:3 proportions in a centered box; night vision and dialog faces stay in place.
- **Videos** — **Correct videos** keeps movies centered at their original 4:3 proportions, with black bars where needed at any screen resolution or aspect ratio.
- **Display scaling** — Scaled desktops neither crop nor blur the picture.

## Options

- **Correct videos** (restart) — on in every preset.
- **Field of view factor** (0.5–2.0, live) — 1.0 in every preset.
- **Resolution width/height** (restart).

Settings are changed on fusionfix.io or in FusionFixHub; options marked live apply while the game runs, the others after a restart. Defaults are the Recommended preset; Vanilla and Enhanced are the other presets. No preset changes the resolution. The fix has no in-game menu. An old `scripts/KnightRider2.WidescreenFix.ini` isn't used anymore and can be deleted.

## Known limits

- The field of view factor also applies to scripted camera shots that use the same projection.
- HUD is a centered 4:3 box (no HUD aspect setting); no window mode setting, skip intro or controller icons.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, beside the game executable, keeping the folders in the archive.
3. Start the game once to create `scripts/KnightRider2.WidescreenFix.cfg`.
4. Change settings on fusionfix.io or in FusionFixHub. Settings marked live apply while the game runs; the others after a restart.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
