![scc](https://thirteenag.github.io/screens/scc/main2.jpg)

$${\color{green}✔}$$  Fixed gamepad support (via [Xidi](https://github.com/samuelgr/Xidi))

$${\color{green}✔}$$  Fixed crash in DLC levels when Xbox 360 achievement conditions are met (e.g., "King of the Crypt" in New Orleans, "Watch the Soap" in Portland)

$${\color{blue}ⓘ}$$  Added an option to skip intro

$${\color{blue}ⓘ}$$  Added an option to enable windowed mode

$${\color{blue}ⓘ}$$  Added an option to disable negative mouse acceleration

$${\color{blue}ⓘ}$$  Added an option to skip system detection errors on startup

$${\color{blue}ⓘ}$$  Added an option to restore LAN support

$${\color{blue}ⓘ}$$  Added an option to enable partial ultrawide support 

$${\color{blue}ⓘ}$$  Added an option to disable depth of field effect

$${\color{blue}ⓘ}$$  Added an option to disable black and white filter

$${\color{blue}ⓘ}$$  Added an option to enable enhanced sonar vision

$${\color{blue}ⓘ}$$  Added an option to disable character lighting 

$${\color{blue}ⓘ}$$  Added an option to enable [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html)

$${\color{blue}ⓘ}$$  Added an option to load packed files from disk

$${\color{blue}ⓘ}$$  Added an option to unlock DLC content that's unavailable after servers shut down

$${\color{blue}ⓘ}$$  Added an option to use Splinter Cell Blacklist control scheme (not fully tested, report issues via GitHub)

$${\color{blue}ⓘ}$$  Changed coop FPS limit to 60 (custom values can be set via `update/convictionsettings.ini`, `SyncMaxStepFrequency` parameter)

$${\color{blue}ⓘ}$$  Disabled unlimited ammo for all weapons (restore original behavior by deleting `update/weapontuning.ini`)

![controls](https://github.com/user-attachments/assets/2498a891-2991-4604-b0dc-b5d309a0c02d)

https://user-images.githubusercontent.com/4904157/192846910-6ddfb3fb-3089-4553-ba94-b8d031855fc7.mp4

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
>    WINEDLLOVERRIDES="dinput8,version,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="dinput8,version,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Splinter Cell: Conviction in `$HOME/.steam/steam/steamapps/compatdata/33220/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/33220/pfx" winecfg
> ```
> Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.
>
> ![winecfg-dll-override](https://silentsblog.com/assets/img/setup/winecfg-dll-override.png)
>
> Related Wine documentation:
> * [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
> * [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)
> </details>

[Website](https://thirteenag.github.io/wfp#scc) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellConviction.FusionFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellConviction.FusionFix/src/system/scripts/SplinterCellConviction.FusionFix.ini)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/wp-content/uploads/sc-sponsors/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br> </p>