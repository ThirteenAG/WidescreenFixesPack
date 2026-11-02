<div align="center">

<img src="https://thirteenag.github.io/screens/hd2/main2.jpg" width="760" alt="Hidden &amp; Dangerous 2 Fusion Fix">

**Hidden & Dangerous 2 Fusion Fix** corrects aspect ratio and field of view for widescreen displays, keeps the menus and briefings in their 4:3 frame, puts the HUD at the screen edges, frames cutscenes and movies, and adds window modes and a field of view multiplier.

[Website](https://thirteenag.github.io/wfp#hd2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/HiddenandDangerous2.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Aspect ratio and field of view** — The 3D view widens with the screen and keeps the original vertical view (Hor+), also for scripted cameras.
- **Menus, briefings and loading screens** — Shown in their 4:3 frame in the middle of the screen with black sides; the mouse cursor and buttons line up, the cursor keeps its shape and the film preview in the Films menu stays in its panel. On screens narrower than 4:3 they fit the width with bars above and below.
- **HUD** — The soldier panels and the inventory sit at the left edge of the screen and the weapon panel at the right edge, with their names and numbers; the crosshair stays in the middle. The sniper scope keeps its round view with black sides.
- **Cutscenes** — The game's scripted cutscenes are framed for 16:9; they keep that framing on every screen (see Cutscene borders).
- **Movies** — Shown at their own aspect ratio, never cut off.
- **Display scaling** — The game gets the full resolution on scaled desktops.
- **CD key** — Supplies a CD key when the game finds none.

## Options

- **HUD aspect ratio** — Auto (default) keeps the HUD at the screen edges; a ratio such as 16:9 keeps it in a centred box of that shape. Applies at once.
- **Cutscene borders** — Letterbox keeps the game's bars above and below cutscenes on screens narrower than 16:9; Pillarbox keeps the 16:9 picture between side bars on wider screens; Both (default) does both; Off fills the screen. Applies at once, also during a cutscene.
- **Movie display** — Original shows movies as the game does, inside the centred 4:3 frame; Fill (default) shows the whole movie as large as the screen allows. Applies at once.
- **Field of view** — A multiplier (0.5–2.0, default 1.0) for the gameplay camera; applies at once. Cutscenes keep their framing.
- **Skip intro** — On by default: the game starts at the main menu, past the logo videos and the intro movie (it stays in the Films menu). Needs a restart.
- **Window mode** — Fullscreen, Windowed or Borderless, then restart. Windowed gives a centred window whose picture area is exactly the resolution chosen in the game's setup program; Borderless covers the monitor at its resolution.
- **Pause on focus loss** — On by default: the game pauses while another window is active, as it always has. Off keeps it running in the background.
- **Confine cursor** — On by default: keeps the cursor inside the game window while it is active. Applies at once.
- **Check for updates** — Off by default. When on, the fix asks fusionfix.io once per start whether a newer version is out and shows it in the version line of the menus; FusionFixHub opens when you close the game.
- **Text language** — Only in the cfg and FusionFixHub: the language of the fix's texts. Automatic follows the game.

---

## Settings and hub

The fix creates `scripts/HiddenandDangerous2.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it, also while the game runs. Vanilla shows the intro, keeps the game's own cutscene bars and shows movies in the 4:3 frame; Recommended and Enhanced skip the intro, add side bars to cutscenes on wider screens and fit movies to the screen. Window and language settings aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

## Controller

The game reads joysticks through DirectInput. The fix package includes Xidi, so an Xbox or other XInput controller works as the game's joystick; set up its buttons in the game's control setup.

## Known limits

- The game's option screens are prebuilt scenes with a fixed set of buttons, so the fix has no page in them: change its settings in FusionFixHub or on fusionfix.io.
- The message lines at the top left (orders, contacts, "Load done") stay where the game puts them, at the same share of the screen width, and don't follow the HUD aspect ratio box.
- The damage overlay is turned off, as in earlier versions of the fix.
- The resolution is chosen in the game's setup program; a window size change needs a restart.
- Ctrl+F10 developer overlay: not available, the game draws with Direct3D 8.
- Button prompts are the game's own; there are no controller icons.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/HiddenandDangerous2.FusionFix.cfg` with every setting.
4. Pick the resolution in the game's setup program and configure the fix in FusionFixHub or on fusionfix.io. Restart when an option requires it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
