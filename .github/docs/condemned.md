<div align="center">

<img src="https://thirteenag.github.io/screens/condemned/main2.jpg" width="760" alt="Condemned: Criminal Origins Fusion Fix">

**Condemned: Criminal Origins Fusion Fix** corrects ultrawide scaling, adds a field-of-view setting and skips the startup logos, with options for saved settings and window modes.

[Website](https://thirteenag.github.io/wfp#condemned) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Condemned.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — More display resolutions in `Config.exe`.
- **Aspect ratio** — Correct scaling at any aspect ratio.
- **HUD** — The HUD uses the whole screen, also on screens narrower than 16:9.
- **Menus** — Menu backgrounds keep their proportions.
- **Frame rate drops** — No drop to roughly 20–30 FPS after several minutes.
- **Gamepad** — XInput controllers use [Xidi](https://github.com/samuelgr/Xidi). New profiles are configured with controller bindings alongside keyboard and mouse controls. If a controller does not respond, configure its bindings in the controls menu. Existing profiles keep their bindings. PlayStation and Switch controllers need an XInput mapping through Steam Input or DS4Windows.

## Options

- **Window mode** — Fullscreen, Windowed or Borderless, then restart. No `autoexec.cfg` edit is needed. Windowed gives a centred window whose picture area is exactly the chosen resolution; Borderless fills the current monitor.
- **Field of view** — A multiplier (0.5–2.0, default 1.0) on the gameplay camera's field of view, limited to 150 degrees horizontally; it applies while the game runs. Scripted cameras and the scope zoom keep their own.
- **HUD aspect ratio** — Auto spreads the HUD over the whole screen; a ratio such as 16:9 keeps it in a centred box of that shape. Needs a restart.
- **Cutscene borders** — For cinematic cameras that show the game's black bars: Letterbox keeps them, Pillarbox adds side bars that keep the picture at 16:9 on wider screens, Both does both, Off removes them. Animated cutscene borders keeps the game's fade of the bars.
- **Movies** — Movie screens show the whole video at its own aspect between bars, never cut off or stretched; the extras and credits videos are shown at 16:9.
- **Frame rate limit** — Off, the monitor's refresh rate (default) or a number of frames per second; applies at once.
- **Pause in background** — On by default: the game pauses while another window is active, as it always has. Off keeps it running in a window mode.
- **Confine cursor** — Keeps the mouse cursor inside the game window while the game is active.
- **Check for updates** — Off by default. When on, the fix asks fusionfix.io once per start and shows a line on its settings page when a newer version is out.
- **Skip intro** — On by default; skips the startup logo videos, as pressing the skip button would.
- **Saved settings** — Compatibility option: keeps saves and settings in My Documents so no administrator rights are needed.

---

## Settings and hub

Open **Options → Plugins → Condemned Fusion Fix** (in the main menu or the pause menu) for the fix's settings, presets and help. A line at the bottom tells you when a change needs a restart. Press **Ctrl+F10** for the developer overlay with every setting and a Debug › Info page.

The fix creates `scripts/Condemned.FusionFix.cfg` on first start; FusionFixHub can change it, also while the game runs. Vanilla shows the intro, the game's own bars and its own frame rate; Recommended and Enhanced skip the intro, add pillarbox bars and limit the frame rate to the monitor's. Window, input and compatibility settings aren't changed by presets. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- Only cinematic cameras that the game shows with black bars get borders; the others fill the screen, as in the original.
- The HUD aspect ratio and window mode apply after a restart; Alt+Enter switching is not supported.
- Existing profiles keep their controller bindings; configure them in the controls menu if needed. Button prompts show keyboard keys.
- Auto uses English for the in-game settings menu. Select French, German, Italian or Spanish with the text-language setting, then reopen Plugins.
- The bottom hint can retain the game's stock text; help for the selected fix setting appears above it.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/Condemned.FusionFix.cfg` with every setting.
4. Configure the fix in Options → Plugins, on fusionfix.io or in FusionFixHub. Restart when an option requires it.

## About steam files

From PCGW:
> The Steam version's CondemnedA.Arch00 is noticeably smaller when compared to the retail game. Because of this, and reversed audio files within the .Arch00 file, many sounds are simply missing from the Steam version. In particular, the train sounds at the end of Chapter 3 are missing. The footage from "The Dark" game prototype is also corrupted.

I compared the files and made a separate archive with missing/different files. That way there's no need to download and replace large files.
Installation:
- Download [Condemned.MissingSteamFilesFix.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/condemned/Condemned.MissingSteamFilesFix.zip) for a full package **or** [Condemned.MissingSteamFilesFix.Lite.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/condemned/Condemned.MissingSteamFilesFix.Lite.zip) if you don't care about having retail TheDark_4-5-04.bik and FinalCreds.bik.
- Unpack everything to root directory, replacing **default.archcfg** in the process.

Fusion Fix also contains **default.archcfg**, but they are identical between archives. It is used to make the game load arch00 archives without needing to replace them.

Additionally, visit my [steam guide](https://steamcommunity.com/sharedfiles/filedetails/?id=1593741074) for more info about gamepad support in this game.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
