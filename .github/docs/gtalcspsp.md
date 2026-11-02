<div align="center">

<img src="https://thirteenag.github.io/screens/gtalcspsp/main2.jpg" width="760" alt="Grand Theft Auto: Liberty City Stories Fusion Fix (PPSSPP)">

**Grand Theft Auto: Liberty City Stories Fusion Fix (PPSSPP)** adds HUD scaling, right-stick controls, a modern control scheme, and optional 60 FPS and game loading speed adjustments.

[Website](https://thirteenag.github.io/wfp#gtalcspsp) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTALCS.PPSSPP.FusionFix/main.cpp)

</div>

---

## Fixes

- **Widescreen** - Matches the display shape selected in PPSSPP. You can also choose an aspect ratio in FusionFixHub.
- **Field of view** - Widens the gameplay view for your screen. Gameplay FOV ranges from 0.5 to 2.0. The gameplay FOV multiplier does not change cutscenes.
- **HUD** - Adjust the HUD, radar and multiplayer radar sizes separately. HUD Aspect Ratio Constraint keeps the HUD inside a centered area, or uses the whole screen with Auto. Radar positions can be changed in FusionFixHub.
- **Menu artwork** - Keeps its proportions with black bars at the sides when needed.
- **Cutscene borders** - Choose Off, Letterbox, Pillarbox or Both. Letterbox adds top and bottom bars on narrower screens; Pillarbox adds side bars on wider screens.
- **Videos** - Original and Fill both show the whole video with borders where needed; nothing is cut off. Loading acceleration stops while videos play.

## Options

- **Dual analog controls** - right-stick camera and aim.
- **Vehicle controls** - Modern Control Scheme swaps L/Square and R/Cross in vehicles; swap buttons and triggers in PPSSPP for trigger-based driving.
- **Frame rate** - 30 or 60 FPS (`MAIN.FPSLimit` in the cfg); 30 FPS by default.
- **Loading** - temporarily unthrottles the emulator during loading screens.
- **Skip intro** - skips the startup intros.
- **PC cheats** - Optional keyboard cheat words during gameplay: THUGSTOOLS, PROFESSIONALTOOLS, NUTTERTOOLS, ALWAYSCARRYCASH, PRECIOUSPROTECTION, ASPIRINE, YOUWONTTAKEMEALIVE and LEAVEMEALONE. Keep this off when you do not want typed words to activate cheats.
- **Vehicle lights** - lit headlights, tail, brake and reverse lights on car and bike bodies. Vehicle Lights is on by default; the Vanilla preset turns it off. A change applies the next time the emulated game starts.
- **Controller icons** - Auto uses Xbox 360 artwork; PSP, Xbox 360, Xbox One, PlayStation 3/4/5, Nintendo Switch and Steam Deck sets are also available. Choose the set that matches your PPSSPP bindings; the guest cannot identify the host controller. Missing or incompatible files keep the affected original artwork. Select PSP to restore it.

The supplied controller diagrams include replacement connectors for On Foot Setup 1, In Vehicle Setup 2 and the right-hand layout. Other diagram configurations retain their original connectors.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the directory containing PPSSPP’s `memstick` folder, preserving the folders in the archive.
3. After the first launch, change settings on fusionfix.io, in FusionFixHub or in the game's menu.
4. Launch the game in PPSSPP.
5. If an older `memstick/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix` or `memstick/PSP/PLUGINS/GTALCS.PPSSPP.ImVehLM` folder is still there, delete it. Both are part of this Fusion Fix now; while one of them is installed, the Fusion Fix stays off and its log says why.

# Project2DFX

<details>
  <summary><a href="https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtalcs">Project2DFX</a> is available as a separate plugin</summary>
  <img src="https://user-images.githubusercontent.com/4904157/156891784-3143e193-2e90-470d-bdc1-fdc94bf7ea87.png" width="652px" />
</details>

# Vehicle lights

https://user-images.githubusercontent.com/4904157/218305955-3a8a244f-aae8-456b-a7ef-dac9d5ca7118.mp4

## Settings

The settings are stored in `memstick/PSP/PLUGINS/GTALCS.PPSSPP.FusionFix/GTALCS.PPSSPP.FusionFix.cfg`, created on the first start. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that opens this fix's page. It has its preset (Vanilla, Recommended, Enhanced), Display and Camera (HUD, radar and multiplayer radar size, field of view, cutscene borders and video aspect ratio), Layout (aspect override, HUD constraint and radar positions), Game and Controls (with vehicle lights), and Restore Defaults. Menu changes are saved when the menu closes. Skip intro, modern controls and vehicle lights need a restart. Other changed cfg values are read during gameplay, including after loading a PPSSPP save state. Restart after installing an updated plugin before making new save states: old states contain the older plugin code.

- **Presets** - Vanilla keeps the original look (HUD, radar and multiplayer radar at full size, cutscene letterbox, original controller icons, intros, normal loading and no vehicle lights); Recommended is the default, and Enhanced is the same. Applying a preset changes only settings that differ between presets, preserving your other preferences. Restore Defaults resets all settings.

After updating from an older release, reapply your preferences in FusionFixHub.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by PPSSPP.
- **Mouse and keyboard prompts** - PPSSPP maps host input to PSP buttons; the plugin cannot read those host bindings. Configure the two sticks and buttons in PPSSPP, then select matching controller artwork.
- **Failure notifications** - PPSSPP has no guest-to-OSD notification API. Rejected code is reported in the plugin log; when the Plugins menu is available, unsupported movie correction, PC cheats and controller artwork show UNAVAILABLE. If the menu itself cannot be validated, use the log and sidecar configuration.

If a feature stays original, check `memstick/PSP/PLUGINS/GTALCS.PPSSPP.FusionFix/GTALCS.PPSSPP.FusionFix.log`. The plugin rejects code or texture layouts it does not recognize. Keep the PRX, its JSON files, the `lights` files and the `icons` folder together when updating.

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
