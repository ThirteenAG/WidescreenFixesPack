<div align="center">

<img src="https://thirteenag.github.io/screens/mp2/main2.jpg" width="760" alt="Max Payne 2: The Fall of Max Payne Fusion Fix">

**Max Payne 2: The Fall of Max Payne Fusion Fix** corrects widescreen HUD, field of view, and video scaling, with options for graphic novels, cutscene borders, and save loading.

[Website](https://thirteenag.github.io/wfp#mp2) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MaxPayne2.FusionFix/dllmain.cpp)

</div>

---

The included ASI loader provides Direct3D 8 to 9 conversion. Keep `d3d8.dll` and `d3d8.ini` together beside the game executable. If an existing `global.ini` in the game, `scripts`, `plugins`, or `update` folder sets `UseD3D8to9=0`, it overrides `d3d8.ini`; change that existing value to `1` to use the D3D9 post-effects and developer overlay.

## Fixes

- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.
- **Videos** — Original fits each movie within a centered 4:3 area; Fill fits it to the screen. Both preserve the whole picture.
- **16-bit display modes** — A 16-bit mode in the settings no longer stops the game from starting ("creating the device failed: D3DERR_NOTAVAILABLE"), the 32-bit mode of that resolution is used.
- **Display modes** — The startup dialog lists the display modes largest first.

## Options

- **Cursor confinement** — Keeps the pointer inside the active game window. Applies immediately and releases it when focus is lost or the option is turned off.
- **Pause on focus loss** — Enabled by default; turn it off to keep playing in the background.
- **Frame-rate ownership** — The fix’s Frame Rate Limit controls pacing; there is no second bundled frame cap.

- **In-game options** — The fix's settings sit next to the game's own options, in the main menu and the pause menu: **Video** has Window Mode, Field of View, HUD Aspect Ratio, Anti-Aliasing, Console Gamma, Vertical Sync and Frame Rate Limit; **Game** has Cutscene Borders, Animated Cutscene Borders, Video Framing, Graphic Novel Pages; **Controls** has Controller Icons, Controller Camera Speed, Invert Controller Y and Vibration; **Mouse Settings** has Raw Mouse Input and Mouse Smoothing. The game's Restore Defaults in Video and Controls resets these rows too. One new **Plugins** row at the end of Options lists the fix first, then every other plugin with settings (such as Xbox Rain Droplets): each page has its Preset (Vanilla, Recommended, Enhanced; Apply Preset asks the game's "Are you sure?"), its remaining settings and Restore Defaults for that page. Most settings apply at once; one that needs a restart shows "(after restart)" while its value differs from the one the game started with. All settings are stored in `scripts/MaxPayne2.FusionFix.cfg`, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub, which changes them in the running game too.
- **Field of view** — A multiplier on top of the widescreen (Hor+) field of view. To avoid a near-180° rectilinear projection, multiplier expansion is capped at 150°; high values can reach the same limit on ultrawide screens.
- **Xbox 360 Gamma** — A custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360. Uses the Direct3D 8 to 9 conversion included in the ASI loader.
- **Anti-aliasing** — SMAA smooths image edges. Requires the included Direct3D 8 to 9 conversion and the game’s own anti-aliasing disabled.
- **VSync** — Turn off the vertical sync the game always uses in fullscreen mode.
- **Frame rate limit** — Unlimited (the game's own behaviour: it has no cap), the monitor's refresh rate (default), or a number of frames per second, paced precisely. Applies at once. The game runs on variable frame times; at very high frame rates (several hundred) its physics and scripted events are reported to misbehave, which the refresh-rate default avoids.
- **Window mode** — Full screen, a window, or a borderless window, without the `-window` switch. The window has the size of the resolution picked at startup and is centered on its monitor; a window as large as the monitor covers it exactly, its title bar and borders outside the screen. Full screen with `-window` gives the borderless window, as before. The game is DPI aware (per monitor), so Windows scaling doesn't blur or crop it, and the window has square corners on Windows 11. Needs a restart.
- **Skip intro** — Skips the logo movie at startup, as the game's `-skipstartup` switch does (on by default; needs a restart).
- **Graphic novels** — Fill the screen with graphic novel pages or keep the original framing, in the menu or with the configured hotkey (F2 by default). In fullscreen, whole pages are shown as large as the screen allows and the playback controls appear while the mouse cursor is at the bottom of the screen; the keyboard navigates pages too. The cursor stays hidden until the mouse moves.
- **Cutscene borders** — In-engine cutscenes keep their original framing at any aspect ratio; choose letterbox borders, pillarbox borders, both, or none. Borders slide in and out, or appear at once with **Animated Cutscene Borders** off.
- **Startup save** — Automatically load a selected, most recent, or last-used save on startup.
- **Save location** — Store saves in a `savegames` folder inside the game directory.
- **Settings file** — Keep the game's settings in `settings.ini` next to the savegames instead of the registry. The first time, the settings already in the registry are copied there.
- **Background play** — Keep the game running when minimized.
- **Raw mouse and smoothing** — Raw Mouse Input is on by default for relative camera movement; if the raw-input registration or compatible game hook is unavailable, the game falls back to DirectInput. Mouse Smoothing is off by default.
- **Controller menus** — Move the pointer with the right stick and press A to select a menu item.
- **Controller icons** — Mapped actions in tips and the Movement and Combat binding pages show Xbox, PlayStation or Nintendo artwork while the controller is in use; they return to key names when keyboard or mouse input takes over. Auto chooses the controller family. Unmapped actions retain their key labels. The built-in How to Play diagram is a fixed keyboard image.


Use the retail 1.01 update or the Steam release. Unsupported executables, an outdated ASI loader, or a missing DirectX June 2010 runtime are reported at startup with instructions.

## Known limits


- Very high frame rates can still cause physics or scripted-event problems.
- Rows show their value in their name; sliders whose name and value would be too long show only the name.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/MaxPayne2.FusionFix.cfg`.
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
>    WINEDLLOVERRIDES="d3d8,dinput,dsound,dsoal-aldrv,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="d3d8,dinput,dsound,dsoal-aldrv,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Max Payne 2: The Fall of Max Payne in `$HOME/.steam/steam/steamapps/compatdata/12150/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/12150/pfx" winecfg
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
