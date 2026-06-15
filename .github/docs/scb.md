![scb](https://thirteenag.github.io/screens/scb/main2.jpg)

$${\color{blue}ⓘ}$$  Added an option to skip intro

$${\color{blue}ⓘ}$$  Added an option to skip press any key screen

$${\color{blue}ⓘ}$$  Added an option to switch the game to fullscreen mode on startup

$${\color{blue}ⓘ}$$  Added an option to disable negative mouse acceleration

$${\color{blue}ⓘ}$$  Added an option to enable ultrawide support

$${\color{blue}ⓘ}$$  Added an option to change FOV

$${\color{blue}ⓘ}$$  Added an option to change Screen Cull Bias, for better draw distance

$${\color{blue}ⓘ}$$  Added an option to unlock DLC content that's unavailable after servers shut down

$${\color{blue}ⓘ}$$  Added an option to unlock all missions from the start

$${\color{blue}ⓘ}$$  Added an option to load packed files from disk

$${\color{blue}ⓘ}$$  Added an option to enable [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html)

$${\color{blue}ⓘ}$$ Added an option to be able to run during forced walking sections

$${\color{blue}ⓘ}$$  Difficulty tweaks: **Mark and Execute**, sonar and drop crates enabled on **Perfectionist**, unlimited ammo disabled on **Rookie**. To go back to original behavior, delete `update/difficultyconfiguration.ini` and set `DisablePerfectionistChecks` to 0.

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

    Installation:
        Download and extract the archive to the game directory, where the exe is located.

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

[Website](https://thirteenag.github.io/wfp#scb) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellBlacklist.FusionFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellBlacklist.FusionFix/src/SYSTEM/scripts/SplinterCellBlacklist.FusionFix.ini)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/wp-content/uploads/sc-sponsors/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br> </p>