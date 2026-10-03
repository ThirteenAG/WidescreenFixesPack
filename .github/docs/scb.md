<div align="center">

<img src="https://thirteenag.github.io/screens/scb/main2.jpg" width="760" alt="Splinter Cell: Blacklist Fusion Fix">

**Splinter Cell: Blacklist Fusion Fix** adds ultrawide and FOV options, startup skips, input adjustments, and configurable mission and game-mode changes.

[Website](https://thirteenag.github.io/wfp#scb) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellBlacklist.FusionFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellBlacklist.FusionFix/src/SYSTEM/scripts/SplinterCellBlacklist.FusionFix.ini)

</div>

---

## Options

### Startup and input

- **Skip intro** — Skip startup intros.
- **Startup prompt** — Skip the “Press Any Key” screen and checkpoint selection.
- **Mouse input** — Raw mouse input for camera turning, so high polling rate mice don't lose movement; optionally disable negative mouse acceleration.
- **Turn speed** — `TurnSpeed` makes Sam turn faster toward the movement direction while moving, for snappier direction changes.

### Display and camera

- **Window modes** — Windowed, borderless and fullscreen list the same resolutions; windowed and borderless windows are centered and open in front, the window can be resized and the game remembers it.
- **Ultrawide** — The HUD and menus stay 16:9 on screens wider or narrower than 16:9 (menus, dialogs and full screen movies get black bars), the 3D view fills the screen.
- **Letterbox** — On screens narrower than 16:9 (4:3, 16:10), optionally letterbox the 3D view to 16:9.
- **Frame rate limit** — Set a frame rate limit with `FPSLimit`.
- **Field of view** — Adjust `FOVFactor` from 0.5 to 2.5.
- **Draw distance** — Adjust `ScreenCullBias`; lower values keep distant objects visible for longer.

### Gameplay and files

- **DLC** — Unlock DLC content made unavailable by the server shutdown.
- **Mission unlocks** — Separate options unlock campaign and 4th Echelon missions from the start.
- **Loose files** — Load unpacked files from the `update` folder. `DumpPackedFiles` unpacks all .umd archives into an `unpacked` folder next to the exe, its contents can be moved to `update` to run the game from loose files.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).
- **Forced walking** — Allow running during forced walking sections.
- **Split screen co-op** — Restores the console split screen co-op (`[SPLITSCREEN] Enable`), DX11 and DX9: pick a co-op mission in the SMI and choose SPLIT-SCREEN, player 2 joins with START on their gamepad in the lobby and customizes their gear with X (it's saved in the profile). `GamepadPlayer1`/`GamepadPlayer2` assign the gamepads, by default player 1 uses keyboard and mouse only and player 2 the first gamepad while player 2 is in; single player isn't affected. Each player gets their own HUD, button prompts and markers in their half, also on ultrawide screens; player 2 can open and close the pause menu with START, the menu itself is controlled by player 1. Ambient occlusion is turned off while in split screen.

### Fixes

- The grid transition no longer stays over the ending cutscene at high frame rates.
- The game no longer crashes after about 30 minutes of play because of the shut down online servers (the failed logins used up its pool of network requests).

Difficulty tweaks: **Mark and Execute**, sonar and drop crates enabled on **Perfectionist**, unlimited ammo disabled on **Rookie**. To go back to original behavior, delete `update/difficultyconfiguration.ini` and set `DisablePerfectionistChecks` to 0.

# Extraction Mode

Plugin adds an ability to modify the number of enemies for the Extraction game mode with `ExtractionWaveEnemyMultiplier` option.

New config for the Extraction game mode: Random, which randomizes the number and type of enemies in each wave using minimum and maximum values defined in the INI file.

To load custom configs for the Extraction game mode, create a folder inside `Splinter Cell Blacklist\src\SYSTEM\update\Data\ExtractionWaveConfigs\`, copy all XMLs into it, then set the name of the new folder in `ExtractionWaveConfigs` option in the INI file.

E.g.:

```
Splinter Cell Blacklist\src\SYSTEM\update\Data\ExtractionWaveConfigs\
NewConfig\
          DefaultWaveConfig.xml
          D_Amman.xml
          D_Bratislava.xml
          D_Kigali.xml
          D_Sanaa.xml
```

```
[EXTRACTION]
ExtractionWaveConfigs = NewConfig
```

Demo: https://youtu.be/su47XbCcVyw

# Hunter Mode and Coop

Plugin adds an ability to modify the number of reinforcements for the Hunter game mode and coop campaign with `ReinforcementsEnemyMultiplier` option.

New config for the Hunter game mode and coop campaign: Random, which randomizes the total number of reinforcements between minimum and maximum values defined in the INI file.

# Ghost Mode and Campaign

Plugin adds an ability to disable mission failure on detection in Ghost mode and Campaign (SP and COOP).

# Extra tweaks

Disable Active Sprint: [NoActiveSprint.zip](https://github.com/user-attachments/files/16575499/NoActiveSprint.zip)

Hold DPAD-UP to switch between vision modes: [HoldDPADUPToSwitchVisionMode.zip](https://github.com/user-attachments/files/16575500/HoldDPADUPToSwitchVisionMode.zip)

Put edited ini configs in `update` folder.

# Ultrawide Screenshot

![scb](https://github.com/user-attachments/assets/96d6a1e7-457e-4ae4-a2f6-88cc9a632c80)

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `src/SYSTEM`.
3. Optionally edit `src/SYSTEM/scripts/SplinterCellBlacklist.FusionFix.ini` to configure the fix.
4. Launch the game.

> [!WARNING]
> Non-Windows users (Proton/Wine) need to perform a **DLL override**.
>
> <details>
> <summary>Click here for details</summary>
> <br>
>
> You need to tell Wine explicitly to use the correct DLL overrides required for this plugin. There's more than one way to achieve it.
>
> ###
>
> **Method 1**: `WINEDLLOVERRIDES` variable lets you temporarily specify DLL overrides. It can be used from a command line as well as in the Steam launcher. In the case of the command line, simply prepend the usual start command with:
>    ```
>    WINEDLLOVERRIDES="version=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="version=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Splinter Cell: Blacklist in `$HOME/.steam/steam/steamapps/compatdata/235600/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/235600/pfx" winecfg
> ```
> Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.
>
> ![winecfg-dll-override](https://silentsblog.com/assets/img/setup/winecfg-dll-override.png)
>
> Related Wine documentation:
> * [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
> * [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)
> </details>



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
