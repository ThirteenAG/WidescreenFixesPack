<div align="center">

<img src="https://thirteenag.github.io/screens/condemned/main2.jpg" width="760" alt="Condemned: Criminal Origins Widescreen Fix">

**Condemned: Criminal Origins Widescreen Fix** corrects ultrawide scaling and adds options for menu backgrounds, saved settings, frame rate drops, and borderless windowed mode.

[Website](https://thirteenag.github.io/wfp#condemned) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Condemned.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Condemned.WidescreenFix/scripts/Condemned.WidescreenFix.ini)

</div>

---

## Fixes

- **Resolution** — Unlock available display resolutions in `Config.exe`.
- **Aspect ratio** — Correct scaling on ultrawide displays.
- **HUD** — Correct HUD scaling on ultrawide displays.
- **Gamepad** — Support controllers through [Xidi](https://github.com/samuelgr/Xidi).

## Options

- **Menu backgrounds** — Correct stretched backgrounds by scaling them vertically.
- **Frame rate drops** — Address the drop to roughly 20–30 FPS that can occur after several minutes.
- **Saved settings** — Move the save location from Public Documents to My Documents so settings can be saved without administrator rights.
- **Windowed mode** — Make native windowed mode borderless; add `"Windowed" "1"` to `autoexec.cfg` to enable it.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Optionally edit `scripts/Condemned.WidescreenFix.ini` to configure the fix.
4. Launch the game.




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
