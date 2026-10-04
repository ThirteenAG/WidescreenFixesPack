<div align="center">

<img src="https://thirteenag.github.io/screens/mp1/main2.jpg" width="760" alt="Max Payne Widescreen Fix">

**Max Payne Widescreen Fix** corrects widescreen HUD, field of view, and video scaling, with options for graphic novels, cutscene borders, and save loading.

[Website](https://thirteenag.github.io/wfp#mp1) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MaxPayne.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/MaxPayne.WidescreenFix/scripts/MaxPayne.WidescreenFix.ini)

</div>

---

## Fixes

- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.
- **Videos** — Correct FMV scaling for widescreen.
- **Adaptive difficulty** — Fugitive's adaptive difficulty counts deaths, health and play time over reloads, so it eases off again instead of climbing to the hardest settings and staying there.

## Options

- **Field of view** — Adjust the FOV multiplier in the INI file.
- **Xbox 360 Gamma** — A custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360. Requires d3d8to9, which the bundled dxwrapper enables (`D3d8to9 = 1`).
- **SMAA** — Enhanced subpixel morphological antialiasing as a post-processing effect. Requires d3d8to9 like the gamma curve; turns itself off with MSAA.
- **VSync** — Turn off the vertical sync the game always uses in fullscreen mode. The frame rate limit in `dxwrapper.ini` still applies.
- **Graphic novels** — Toggle fullscreen graphic novels with the configured hotkey (F2 by default). In fullscreen, whole pages are shown as large as the screen allows and the playback controls appear while the mouse cursor is at the bottom of the screen; the keyboard navigates pages too. The cursor stays hidden until the mouse moves.
- **Cutscene borders** — In-engine cutscenes keep their original framing at any aspect ratio; choose letterbox borders, pillarbox borders, both, or none. Borders slide in and out.
- **Startup save** — Automatically load a selected, most recent, or last-used save on startup.
- **Save location** — Store saves in a `savegames` folder inside the game directory.
- **Settings file** — Keep the game's settings in `settings.ini` next to the savegames instead of the registry. The first time, the settings already in the registry are copied there.
- **Background play** — Keep the game running when minimized.
- **Mouse smoothing** — Turn off the game's mouse smoothing for a more direct feel.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Optionally edit `scripts/MaxPayne.WidescreenFix.ini` to configure the fix.
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
>    WINEDLLOVERRIDES="dinput,dinputHooked,dsound,dsoal-aldrv,MSVCP60,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="dinput,dinputHooked,dsound,dsoal-aldrv,MSVCP60,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Max Payne in `$HOME/.steam/steam/steamapps/compatdata/12140/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/12140/pfx" winecfg
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
