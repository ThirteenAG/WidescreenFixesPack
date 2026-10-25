<div align="center">

<img src="https://thirteenag.github.io/screens/condemned/main2.jpg" width="760" alt="Condemned: Criminal Origins Widescreen Fix">

**Condemned: Criminal Origins Widescreen Fix** corrects ultrawide scaling, adds a field-of-view setting and skips the startup logos, with options for menu backgrounds, saved settings, frame rate drops and window modes.

[Website](https://thirteenag.github.io/wfp#condemned) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Condemned.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — More display resolutions in `Config.exe`.
- **Aspect ratio** — Correct scaling on ultrawide displays.
- **HUD** — Correct HUD scaling on ultrawide displays.
- **Gamepad** — Support controllers through [Xidi](https://github.com/samuelgr/Xidi).

## Options

- **Menu backgrounds** — Correct stretched backgrounds by scaling them vertically.
- **Frame rate drops** — Address the drop to roughly 20–30 FPS that can occur after several minutes.
- **Saved settings** — Move the save location from Public Documents to My Documents so settings can be saved without administrator rights.
- **Window mode** — Choose Fullscreen, Windowed or Borderless in the fix settings, then restart. No `autoexec.cfg` edit is needed. Borderless fills the current monitor; desktop-size framed windows keep their full client area.
- **Field of view** — A multiplier (0.5–2.0, default 1.0) on the gameplay camera's field of view; it applies while the game runs. Scripted cameras and the scope zoom keep their own.
- **Skip intro** — On by default; skips the startup logo videos, as pressing the skip button would.

---

## Settings and hub

Open **Options → Plugins → Condemned Widescreen Fix** for the fix’s settings and presets. Field of view changes while the game runs; most other settings need a restart. Each plugin has its own presets and defaults.

The fix creates `scripts/Condemned.WidescreenFix.cfg` on first start; FusionFixHub can change it, also while the game runs. An old `scripts/Condemned.WidescreenFix.ini` isn't used anymore and can be deleted. Presets keep aspect and menu corrections enabled. Vanilla shows the intro; Recommended and Enhanced skip it. Window and compatibility settings aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The game's own cutscene letterbox is kept; there is no cutscene-border or video setting.
- There is no HUD aspect-ratio setting.
- Window mode changes require restarting; Alt+Enter switching is not supported.
- Auto uses English for the in-game settings menu. Select French, German, Italian or Spanish with the text-language setting, then reopen Plugins.
- The bottom hint can retain the game's stock text; help for the selected fix setting appears above it.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/Condemned.WidescreenFix.cfg` with every setting.
4. Configure the fix in Options → Plugins, on fusionfix.io or in FusionFixHub. Restart when an option requires it.

## About steam files

From PCGW:
> The Steam version's CondemnedA.Arch00 is noticeably smaller when compared to the retail game. Because of this, and reversed audio files within the .Arch00 file, many sounds are simply missing from the Steam version. In particular, the train sounds at the end of Chapter 3 are missing. The footage from "The Dark" game prototype is also corrupted.

I compared the files and made a separate archive with missing/different files. That way there's no need to download and replace large files.
Installation:
- Download [Condemned.MissingSteamFilesFix.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/condemned/Condemned.MissingSteamFilesFix.zip) for a full package **or** [Condemned.MissingSteamFilesFix.Lite.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/condemned/Condemned.MissingSteamFilesFix.Lite.zip) if you don't care about having retail TheDark_4-5-04.bik and FinalCreds.bik.
- Unpack everything to root directory, replacing **default.archcfg** in the process.

Widescreen fix also contains **default.archcfg**, but they are identical between archives. It is used to make the game load arch00 archives without needing to replace them.

Additionally, visit my [steam guide](https://steamcommunity.com/sharedfiles/filedetails/?id=1593741074) for more info about gamepad support in this game.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
