![scpt](https://thirteenag.github.io/screens/scpt/main2.jpg)

## Fixes

* **Resolution** - native support for any display resolution, including ultrawide
* **Aspect Ratio** - corrected for all rendered elements
* **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom)
* **Field of View** - corrected FOV to match the 4:3 values
* **FMVs** - expanded to fill the screen at widescreen resolutions, with an option to keep the original 4:3 display; 480p playback limit unlocked
* **Framerate Issues** - animated textures, camera shake, cloth physics and AI pathing no longer have issues at higher framerates
* **Resolution Scale** - vision mode grain, thermal vision, rain particles now correctly scale at higher resolutions

## Visual Improvements

* **Character LOD** - forces the highest character LOD models
* **Shadow Resolution** - shadow resolution can now be increased past the default values
* **Light Draw Distance** - increased dynamic lighting draw distance
* **Foliage Draw Distance** - increased foliage draw distance

## Miscellaneous

* **FPS Limit** - added an option to limit FPS to avoid game-breaking bugs
* **Skip Intro** - added an option to skip intro
* **Skip Prompt** - added an option to skip "Press Fire to Continue" prompt
* **Cutscene Borders** - added an option to disable cutscene borders
* **Mouse Acceleration** - disabled mouse acceleration
* **Raw Mouse Input** - added raw input support for mouse
* **Gamepad Support** - improved gamepad support via [Xidi](https://github.com/samuelgr/Xidi)
* **EAX Support** - restored EAX support via [DSOAL](https://github.com/kcat/dsoal)

## Installation

1. Download `SplinterCellPandoraTomorrow.WidescreenFix.zip` from this release.
2. Extract the contents directly into the game folder.
3. Optionally edit `SplinterCellPandoraTomorrow.WidescreenFix.ini` to configure the available options.
4. Launch the game.

<details>
  <summary> Contributors (click to expand) </summary>

@Joshhhuaaa

</details>


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
>    WINEDLLOVERRIDES="dinput8,msacm32,msvfw32,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="dinput8,msacm32,msvfw32,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Splinter Cell: Pandora Tomorrow in `$HOME/.steam/steam/steamapps/compatdata/3929740/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/3929740/pfx" winecfg
> ```
> Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.
>
> ![winecfg-dll-override](https://silentsblog.com/assets/img/setup/winecfg-dll-override.png)
>
> Related Wine documentation:
> * [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
> * [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)
> </details>

[Website](https://thirteenag.github.io/wfp#scpt) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellPandoraTomorrow.WidescreenFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellPandoraTomorrow.WidescreenFix/system/scripts/SplinterCellPandoraTomorrow.WidescreenFix.ini)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><img width="100%" src="https://fusionlegacyinitiative.com/wp-content/uploads/sc-sponsors/sponsors-progress-wfp.svg"></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></a> <a href="https://ko-fi.com/thirteenag"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></a> <a href="https://paypal.me/SergeyP13"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></a> <a href="https://boosty.to/thirteenag"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></a><br><br> </p>
