<div align="center">

<img src="https://thirteenag.github.io/screens/mp1/main2.jpg" width="760" alt="Max Payne Fusion Fix">

**Max Payne Fusion Fix** corrects widescreen HUD and field of view, with options for graphic novels, cutscene borders, and save loading.

[Website](https://thirteenag.github.io/wfp#mp1) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MaxPayne.FusionFix/dllmain.cpp)

</div>

---

Install the official Max Payne 1.05 update before installing the fix. The included ASI loader provides Direct3D 8 to 9 conversion. Keep the included `dinput.dll`, `dinput.ini`, `d3d8.dll` and `d3d8.ini` beside the game executable. The input loader initializes portable settings before the game reads its startup options.

## Fixes

- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.
- **Startup and audio compatibility** - The bundled runtime avoids image-decoding errors on affected AMD processors and keeps the bundled audio replacement working.
- **16-bit display modes** — Picking a 16-bit mode in the options no longer stops the game from starting ("creating the device failed: D3DERR_NOTAVAILABLE"), the 32-bit mode of that resolution is used. The startup dialog only lists the 32-bit modes, largest first.
- **Adaptive difficulty** — Fugitive's adaptive difficulty counts deaths, health and play time over reloads, so it eases off again instead of climbing to the hardest settings and staying there.
- **High-resolution fonts** — Font textures replaced with higher-resolution versions (for example a sharper `Hud_Universal_Medium.tga`) keep the size of the original text on screen instead of growing with the texture.

## Options

- **Cursor confinement** — Keeps the pointer inside the active game window. Applies immediately and releases it when focus is lost or the option is turned off.
- **Pause on focus loss** — Enabled by default; turn it off to keep playing in the background.
- **Frame-rate ownership** — The fix’s Frame Rate Limit controls pacing; there is no second bundled frame cap.

- **In-game options** — The fix's settings sit next to the game's own options, in the main menu and the pause menu: **Video** has Window Mode, Field of View, HUD Aspect Ratio, Anti-Aliasing, Console Gamma, Vertical Sync and Frame Rate Limit; **Game** has Cutscene Borders, Animated Cutscene Borders, Graphic Novel Pages; **Controls** has Controller Icons and **Mouse Settings** has Raw Mouse Input and Mouse Smoothing. The game's Restore Defaults in Video and Controls resets these rows too. One new **Plugins** row at the end of Options lists the fix first, then every other plugin with settings (such as Fusion Rain Droplets): each page has its Preset (Vanilla, Recommended, Enhanced; Apply Preset asks the game's "Are you sure?"), its remaining settings and Restore Defaults for that page. Most settings apply at once; a changed setting that needs a restart shows a separate warning above the native help line until its value is reverted or the game restarts. All settings are stored in `scripts/MaxPayne.FusionFix.cfg`, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub, which changes them in the running game too.
- **Field of view** — A multiplier on top of the widescreen (Hor+) field of view. To avoid a near-180° rectilinear projection, multiplier expansion is capped at 150°; high values can reach the same limit on ultrawide screens.
- **Xbox 360 Gamma** — A custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360. Uses the Direct3D 8 to 9 conversion included in the ASI loader.
- **Anti-aliasing** — SMAA smooths image edges. Requires the included Direct3D 8 to 9 conversion and the game’s own anti-aliasing disabled.
- **VSync** — Turn off the vertical sync the game always uses in fullscreen mode.
- **Frame rate limit** — Unlimited (the game's own behaviour: it has no cap), the monitor's refresh rate (default), or a number of frames per second, paced precisely. Applies at once. Jump takeoff timing and small rising floor-contact steps are corrected so normal jumps and shootdodges keep their height at high frame rates, including slow motion. Scripted sequences and moving platforms still need broader play testing; use a lower limit if a particular sequence misbehaves.
- **Window mode** — Full screen, a window, or a borderless window, without the `-window` switch. Windowed mode uses the selected resolution and is centered on its monitor; borderless covers the monitor at its desktop resolution. A window whose client area is as large as the monitor covers it exactly, with its frame outside the screen. Full screen with `-window` keeps the earlier borderless behavior. Alt+Enter switches between full screen and the window mode. Windows have square corners on Windows 11. Needs a restart.
- **Videos** — Video Framing is on the Plugins page. Original fits the whole movie within a centered 4:3 area; Fill fits it to the screen. Both preserve the whole picture.
- **Skip intro** — Skips the logo movie at startup, as the game's `-skipstartup` switch does (on by default; needs a restart).
- **Graphic novels** — Fill the screen with graphic novel pages or keep the original framing, in the menu or with the configured hotkey (F2 by default). In fullscreen, whole pages are shown as large as the screen allows and the playback controls appear while the mouse cursor is at the bottom of the screen; the keyboard navigates pages too. The cursor stays hidden until the mouse moves.
- **Cutscene borders** — In-engine cutscenes keep their original framing at any aspect ratio; choose letterbox borders, pillarbox borders, both, or none. Borders slide in and out, or appear at once with **Animated Cutscene Borders** off.
- **Startup save** — Automatically load a selected, most recent, or last-used save on startup.
- **Save location** — Store saves in a `savegames` folder inside the game directory.
- **Portable settings** — Enabled by default. Stores the game's original options in `settings.ini` next to the saves instead of the registry. With **Saves in Game Folder** enabled, this is `<game>\savegames\settings.ini`; otherwise it is in Documents\Max Payne Savegames. Both options are available under Options > Plugins > Fusion Fix and need a restart. Existing registry settings are copied only when the file is first created.
- **Background play** — Keep the game running when it loses focus.
- **Raw mouse input** — Reads relative movement through Windows raw input, preserving native sensitivity, inversion, buttons and wheel. Applies immediately. Absolute devices and unavailable raw input use the game's DirectInput path.
- **Mouse smoothing** — The game's mouse smoothing, off by default for a more direct feel. With Raw Mouse Input off, the game reads movement through DirectInput without Windows' pointer acceleration; the fix also keeps movement from being lost or exaggerated when frame times vary.
- **Controller icons** — Native tips, help keys and Controls bindings show Xbox, PlayStation or Nintendo button icons while a controller is used. Keyboard input restores the original key labels. Choose a family or use Auto.


## Known limits


- Moving platforms and scripted sequences have not been exhaustively checked at high frame rates. Lower the frame rate limit if a particular sequence misbehaves.
- Rows show their value in their name; sliders whose name and value would be too long show only the name.
- Long plugin descriptions advance through measured pages in the game's native help line.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/MaxPayne.FusionFix.cfg`.
4. Optionally change the settings in the game's Options (Video, Game, Controls, Plugins), in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.

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
>    WINEDLLOVERRIDES="d3d8,dinput,dsound,dsoal-aldrv,MSVCP60,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="d3d8,dinput,dsound,dsoal-aldrv,MSVCP60,Xidi.32=n,b" %command%`.
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
