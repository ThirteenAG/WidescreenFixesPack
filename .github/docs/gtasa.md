![gtasa](https://thirteenag.github.io/screens/gtasa/main2.jpg)

$${\color{green}✔}$$  Fixed Aspect Ratio

$${\color{green}✔}$$  Fixed HUD

$${\color{green}✔}$$  Fixed Field of View

$${\color{green}✔}$$ Cutscene FOV and letterboxing adjusts properly with aspect ratio

$${\color{blue}ⓘ}$$ Added options to scale the size of HUD elements

$${\color{blue}ⓘ}$$ Added an option to replace the text shadows with an outline

$${\color{blue}ⓘ}$$ Added an option to enable GTA: Vice City Stories camera shake in vehicles at high speed

$${\color{blue}ⓘ}$$ Added an option to make the menu background transparent with a blur 

$${\color{blue}ⓘ}$$ Added an option to enable Xbox 360 gamma


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
>    WINEDLLOVERRIDES="vorbisFile=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="vorbisFile=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Grand Theft Auto: San Andreas in `$HOME/.steam/steam/steamapps/compatdata/12120/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/12120/pfx" winecfg
> ```
> Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.
>
> ![winecfg-dll-override](https://silentsblog.com/assets/img/setup/winecfg-dll-override.png)
>
> Related Wine documentation:
> * [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
> * [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)
> </details>
[Website](https://thirteenag.github.io/wfp#gtasa) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTASA.WidescreenFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTASA.WidescreenFix/scripts/GTASA.WidescreenFix.ini)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/wp-content/uploads/sc-sponsors/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br> </p>
