<div align="center">

<img src="https://thirteenag.github.io/screens/srs/main2.jpg" width="760" alt="Street Racing Syndicate Fusion Fix">

**Street Racing Syndicate Fusion Fix** corrects resolution handling, aspect ratio, HUD, menus, movies and field of view for widescreen displays and adds window modes.

[Website](https://thirteenag.github.io/wfp#srs) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/StreetRacingSyndicate.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Renders at the desktop resolution, or at the resolution set in the fix's options (out-of-range values fall back to the desktop).
- **Field of view** — Hor+: the view keeps the vertical field of view of the original 4:3 picture and shows more at the sides.
- **HUD** — The timers, lap and position, the minimap, the rev counter, speed and gear can sit at the screen edges or inside a centred HUD box.
- **Menus** — Menus, loading screens, the pause pages and the title keep their 4:3 layout without stretching; fades and dimming cover the whole screen. On screens narrower than 4:3 (5:4) the layout gets black bars above and below, so nothing is cut off.
- **Movies** — The story movies are 16:9 pictures; Original shows them as the game does, inside the 4:3 frame, and Fill shows them as large as the screen allows. Nothing is cropped.
- **Scripted camera moments** — The start of street events and other scripted camera moments keep their 16:9 framing with borders by your choice.
- **Window modes** — Fullscreen, Windowed or Borderless; a window has a picture area of exactly the chosen resolution, also at high display scaling. In window modes the game is no longer minimized when another window is active.
- **Display scaling** — The game stays sharp on scaled (high-DPI) desktops.

## Options

The settings are in the game under **Options › Plugins**, in FusionFixHub and on fusionfix.io. The page shows a short help line for the selected row.

- **Preset** — Vanilla, Recommended (default) or Enhanced.
- **Display mode** — Fullscreen, Windowed or Borderless (restart). Borderless uses the desktop resolution.
- **HUD aspect ratio** — Auto or a centred box (4:3, 16:9, 21:9, 32:9); applies live.
- **Movie display** — Original (Vanilla) or Fill (Recommended, Enhanced); applies live.
- **Cutscene borders** — Off, Letterbox (Vanilla), Pillarbox or Both (Recommended, Enhanced); applies live. In events whose whole race uses the 16:9 framing, Pillarbox and Both also frame the race, and the HUD stays on the picture.
- **Animated cutscene borders** — The borders slide like in the original; off shows them at once.
- **Skip intro** (restart) — Skips the Namco logo and the studio logo movie. The legal screens shown while the game loads stay, and the opening movie still plays (Enter skips it). Vanilla off, Recommended and Enhanced on.
- **Skip title screen** (restart) — Continues past "Please press ENTER to begin" once at startup. Vanilla off, Recommended and Enhanced on.
- **Text language** — Auto, English, French, German, Italian or Spanish for the fix’s settings; change it in FusionFixHub or the settings file, then reopen Plugins.
- **Pause when inactive** — In Windowed and Borderless modes the game stops while another window is active; turn it off to keep it running. Fullscreen always minimizes and pauses as before.
- **Field of view** — 0.5–2.0 while driving; 1.0 keeps the intended framing. Scripted camera moments keep theirs. Applies live.
- **Confine cursor** — Keeps the pointer inside the active game window; applies live.
- **Check for updates** — Optional notice at the main menu, off by default.
- **Resolution width/height** are in FusionFixHub and the settings file.

Defaults are the Recommended preset. No preset changes the resolution or the display mode. Supporters can open the developer overlay with Ctrl+F10.

## Controller

The bundled Xidi presents an Xbox-style controller to the game. Choose it under **Options › Controls** (Player 1) and assign the driving actions there.

## Known limits

- The game's own Resolution option has no effect; the resolution comes from the fix's options.
- The window size follows the resolution chosen at startup; resizing the window needs a restart.
- Button prompts always show the keyboard keys.
- In two-player split screen the HUD keeps its original layout.

## Installation

1. Download the fix archive from this release.
2. Extract it into the game folder, keeping the folders in the archive (the fix goes to `Bin`).
3. Start the game once to create `Bin/scripts/StreetRacingSyndicate.FusionFix.cfg`.
4. Change settings in the game under Options › Plugins, in FusionFixHub or on fusionfix.io.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
