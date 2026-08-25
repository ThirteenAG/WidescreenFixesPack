![scda](https://thirteenag.github.io/screens/scda/main2.jpg)

$${\color{green}✔}$$  Fixed Resolution

$${\color{green}✔}$$  Fixed Aspect Ratio

$${\color{green}✔}$$  Fixed HUD

$${\color{green}✔}$$  Fixed Field of View

$${\color{green}✔}$$  Fixed FMVs

$${\color{green}✔}$$  Added raw input support for mouse

$${\color{green}✔}$$  Improved gamepad support (via [Xidi](https://github.com/samuelgr/Xidi))

$${\color{green}✔}$$  Restored EAX support (via [DSOAL](https://github.com/kcat/dsoal))

$${\color{green}✔}$$  Fixed [AI hearing issue](https://www.youtube.com/watch?v=uX_FcPvM64c)

$${\color{green}✔}$$  Loading screens with a video playing are skippable by pressing the Enter key

$${\color{blue}ⓘ}$$  Added an option to increase shadows resolution

$${\color{blue}ⓘ}$$  Added an option to increase glow/bloom resolution

$${\color{blue}ⓘ}$$  Added an option to approximately restore Xbox 360's gamma

$${\color{blue}ⓘ}$$  Added an option to limit FPS to avoid game breaking bugs

$${\color{blue}ⓘ}$$  Added an option to skip intro

$${\color{blue}ⓘ}$$  Added an option to change goggles light color

$${\color{blue}ⓘ}$$  Added an option to enable [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html)

$${\color{blue}ⓘ}$$  Added an option to enable suit indicators to behave like in Blacklist

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
>    WINEDLLOVERRIDES="msacm32,msvfw32,dinput8,dsound,dsoal-aldrv,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="msacm32,msvfw32,dinput8,dsound,dsoal-aldrv,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Splinter Cell: Double Agent in `$HOME/.steam/steam/steamapps/compatdata/13580/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/13580/pfx" winecfg
> ```
> Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.
>
> ![winecfg-dll-override](https://silentsblog.com/assets/img/setup/winecfg-dll-override.png)
>
> Related Wine documentation:
> * [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
> * [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)
> </details>

[Website](https://thirteenag.github.io/wfp#scda) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellDoubleAgent.WidescreenFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellDoubleAgent.WidescreenFix/SCDA-Offline/System/scripts/SplinterCellDoubleAgent.WidescreenFix.ini)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br> </p>
