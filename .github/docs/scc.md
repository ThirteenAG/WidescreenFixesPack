<div align="center">

<img src="https://thirteenag.github.io/screens/scc/main2.jpg" width="760" alt="Splinter Cell: Conviction Fusion Fix">

**Splinter Cell: Conviction Fusion Fix** fixes DLC achievement crashes and adds controller support, startup skips, partial ultrawide support, and configurable visual and gameplay options.

[Website](https://thirteenag.github.io/wfp#scc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellConviction.FusionFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellConviction.FusionFix/src/system/scripts/SplinterCellConviction.FusionFix.ini)

</div>

---

## Fixes

- **Gamepad** — Support controllers through [Xidi](https://github.com/samuelgr/Xidi).
- **DLC crash** — Prevent crashes when Xbox 360 achievement conditions are met in DLC levels, including “King of the Crypt” in New Orleans and “Watch the Soap” in Portland.
- **Co-op frame rate** — Changed coop FPS limit to 60 (custom values can be set via `update/convictionsettings.ini`, `SyncMaxStepFrequency` parameter)
- **Ammunition** — Disabled unlimited ammo for all weapons (restore original behavior by deleting `update/weapontuning.ini`)

## Options

- **Skip intro** — Skip startup intros.
- **Windowed mode** — Run the game in a window.
- **Mouse input** — Optionally disable negative mouse acceleration.
- **Hardware detection** — Skip hardware detection messages on startup.
- **LAN** — Restore LAN support.

### Display and effects

- **Ultrawide** — Enable partial ultrawide support.
- **Depth of field** — Optionally disable the depth of field effect.
- **Black and white filter** — Optionally disable the desaturation effect.
- **Sonar vision** — Enable the enhanced sonar vision option.
- **Character lighting** — Optionally disable the additional character lighting, including in cutscenes.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).

### Content and controls

- **Loose files** — Load unpacked files from disk.
- **DLC** — Unlock DLC content made unavailable by the server shutdown.
- **Controls** — Enable the Blacklist control scheme. Not fully tested; report issues via GitHub.

![controls](https://github.com/user-attachments/assets/2498a891-2991-4604-b0dc-b5d309a0c02d)

https://user-images.githubusercontent.com/4904157/192846910-6ddfb3fb-3089-4553-ba94-b8d031855fc7.mp4

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `src/system`.
3. Optionally edit `src/system/scripts/SplinterCellConviction.FusionFix.ini` to configure the fix.
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



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
