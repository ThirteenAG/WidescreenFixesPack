<div align="center">

<img src="https://thirteenag.github.io/screens/scb/main2.jpg" width="760" alt="Splinter Cell: Blacklist Fusion Fix">

**Splinter Cell: Blacklist Fusion Fix** adds ultrawide and FOV options, startup skips, input adjustments, and configurable mission and game-mode changes.

[Website](https://thirteenag.github.io/wfp#scb) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellBlacklist.FusionFix/dllmain.cpp)

</div>

---

## Settings

Open Settings → Plugins → Splinter Cell Blacklist Fusion Fix in the title or pause menu. Use the page links for more options and F1 for full help. Restart-only changes show a restart notice. Presets apply to this plugin after confirmation. FusionFixHub and fusionfix.io also edit the settings. The default setup enables split screen and keeps detection failure enabled in Ghost mode. The default frame rate limit is 60 FPS.

Menus and dialogs remain in an undistorted, centered 16:9 area. The 3D view fills the screen, and HUD Aspect Ratio controls the width used by side HUD elements. Ambient occlusion is disabled during split screen, and player 1 controls the shared pause menu.

### Startup and input

- **Skip intro** — Skip startup intros.
- **Startup prompt** — Skip the “Press Any Key” screen and checkpoint selection.
- **Mouse input** — Raw mouse input for camera turning, so high polling rate mice don't lose movement; negative mouse acceleration is removed.
- **Turn speed** — Makes Sam turn faster toward the movement direction while moving, for snappier direction changes.

### Display and camera

- **Window modes** — Use the native video options. Windowed mode preserves the requested client size; borderless uses the monitor resolution. The windowed resolution list includes reported display modes and the current custom size.
- **Ultrawide** — Keeps menus and dialogs at 16:9 without stretching while the 3D view fills the screen with the correct aspect.
- **HUD aspect ratio** — Auto places side HUD elements at the screen edges. Choose 4:3, 16:9, 21:9 or a custom ratio to keep them in a centered area. Applies immediately.
- **Cutscene borders** — Both preserves the original 16:9 framing on wide and narrow screens. Letterbox and Pillarbox apply borders on one axis; Off extends the view on widescreen displays.
- **Video framing** — Original fits the whole movie in a centered 4:3 area; Fill fits it to the screen.
- **Frame rate limit** — Change the limit while playing: 0 is unlimited, -1 follows the monitor refresh rate, and positive limits range from 30 to 1000 FPS. Native Vertical Sync can still limit the frame rate.
- **Field of view** — Adjust the locomotion camera from 0.5 to 2.0 while playing, with a maximum horizontal view of 150 degrees and the game's normal camera smoothing.
- **Pause when unfocused** — Enabled by default in single player. Disable it to keep the world running while the game window is unfocused.
- **Draw distance** — Adjust the screen cull bias; lower values keep distant objects visible for longer.
- **Developer overlay** — Press Ctrl+F10 to open the settings and diagnostics overlay, including Debug → Info.

### Gameplay and files

- **DLC** — Restores DLC content made unavailable by the server shutdown.
- **Missions** — Separate options make campaign and 4th Echelon missions available from the start.
- **Loose files** — Load unpacked files from the `update` folder. The **Dump packed files** option (in the settings file) unpacks all .umd archives into an `unpacked` folder next to the exe, its contents can be moved to `update` to run the game from loose files.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).
- **Forced walking** — Allow running during forced walking sections.
- **Gamepad icons** — Auto uses Xbox prompts. PlayStation and Nintendo prompts can be selected separately; restart the game after changing the icon set.
- **Split screen co-op** — Restores the console split screen co-op, DX11 and DX9: pick a co-op mission in the SMI and choose SPLIT-SCREEN, player 2 joins with START on their gamepad in the lobby and customizes their gear with X (it's saved in the profile). **Gamepad player 1** and **Gamepad player 2** assign the gamepads; by default player 1 uses keyboard and mouse only and player 2 the first gamepad while player 2 is in; single player isn't affected. Each player gets their own HUD, button prompts and markers in their half, also on ultrawide screens; player 2 can open and close the pause menu with START, the menu itself is controlled by player 1. Ambient occlusion is turned off while in split screen.

### Fixes

Ultrawide correction, full CPU availability and the mouse speed-cap fix are always applied.

- The grid transition no longer stays over the ending cutscene at high frame rates.
- The game no longer crashes after about 30 minutes of play because of the shut down online servers (the failed logins used up its pool of network requests).

Difficulty tweaks: **Mark and Execute**, sonar and drop crates enabled on **Perfectionist**, unlimited ammo disabled on **Rookie**. To go back to original behavior, delete `update/difficultyconfiguration.ini` and set `DisablePerfectionistChecks` to 0.

# Extraction Mode

Plugin adds an ability to modify the number of enemies for the Extraction game mode with `ExtractionWaveEnemyMultiplier` option.

New config for the Extraction game mode: Random, which randomizes the number and type of enemies in each wave using minimum and maximum values defined on fusionfix.io or in FusionFixHub.

To load custom configs for the Extraction game mode, create a folder inside `Splinter Cell Blacklist\src\SYSTEM\update\Data\ExtractionWaveConfigs\`, copy all XMLs into it, then set the name of the new folder in `ExtractionWaveConfigs` option on fusionfix.io or in FusionFixHub.

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

`EnableKobinAndGrimMaps` (off by default) adds Charlie versions of Kobin's and Grim's 8 missions to the SMI map, next to the originals: Opium Farm, Fish Market, Blood Diamond Mine, Dead Coast, Hawkins Seafort, Border Crossing, Hackers' Den and Billionaire's Yacht. They play the 20 waves of a random Charlie map (or a config named after the map, e.g. `D_OpiumFarm.xml`), with the starting wave selectable from the start. They're cosmetic only: not saved in the profile, not counted in the stat bars, mastery or progression, and the original missions are unaffected.

`EnableCoopCampaignMaps` (off by default) does the same for the co-op campaign's 4 missions (Kargil, Bangalore, Chittagong and Koltsovo). The maps' own enemies are removed, only the waves' enemies are there. The original mission's objective markers are hidden, but its story scripts still run, so its doors may get in the way.

# Kestrel DLC

`[KESTREL] EnableKestrelDLC` (off by default) adds the missions of the cut Kestrel DLC: 4 Kestrel missions on the SMI map, next to their maps' originals: Fish Market, Blood Diamond Mine, Border Crossing and Hackers' Den. They're played in their Kobin or Grim mode, the mission is complete when the map's High Value Target is taken down (killed or knocked out). In co-op, player 2 is Kestrel instead of Briggs. Like the Charlie versions above, they're cosmetic only: not saved in the profile, not counted in the stat bars, mastery or progression.

# Hunter Mode and Coop

Plugin adds an ability to modify the number of reinforcements for the Hunter game mode and coop campaign with `ReinforcementsEnemyMultiplier` option.

New config for the Hunter game mode and coop campaign: Random, which randomizes the total number of reinforcements between minimum and maximum values defined on fusionfix.io or in FusionFixHub.

# Ghost Mode and Campaign

Plugin adds an ability to disable mission failure on detection in Ghost mode and Campaign (SP and COOP).

# Extra tweaks

Disable Active Sprint: [NoActiveSprint.zip](https://github.com/user-attachments/files/16575499/NoActiveSprint.zip)

Hold DPAD-UP to switch between vision modes: [HoldDPADUPToSwitchVisionMode.zip](https://github.com/user-attachments/files/16575500/HoldDPADUPToSwitchVisionMode.zip)

Put edited ini configs in `update` folder.

# Ultrawide Screenshot

![scb](https://github.com/user-attachments/assets/96d6a1e7-457e-4ae4-a2f6-88cc9a632c80)

## Known limits

At frame rates above 100 FPS, guards may appear doubled. Some wall shadows can pop as the camera turns. After changing a setting, move to another row and back if its help line resets to "Plugins".

Render-resolution changes made during startup movies take effect after playback. Movie framing follows the current window size while the change is pending.

## Configuration

The added settings text is available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `src/SYSTEM`.
3. Launch the game once to create the `src/SYSTEM/scripts/SplinterCellBlacklist.FusionFix.cfg`.
4. Configure the fix in Settings → Plugins, on fusionfix.io or in FusionFixHub. Live options apply while playing; restart-only options show a restart notice.

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
