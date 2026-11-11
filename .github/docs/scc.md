<div align="center">

<img src="https://thirteenag.github.io/screens/scc/main2.jpg" width="760" alt="Splinter Cell: Conviction Fusion Fix">

**Splinter Cell: Conviction Fusion Fix** fixes DLC achievement crashes and adds startup skips, widescreen presentation, and configurable visual and gameplay options.

[Website](https://thirteenag.github.io/wfp#scc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellConviction.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Gamepad** — Keep the game’s native XInput support, sensitivity, inversion and vibration controls. Auto icons use Xbox artwork; PlayStation and Nintendo artwork can be selected manually.
- **DLC crash** — Prevent crashes when Xbox 360 achievement conditions are met in DLC levels, including “King of the Crypt” in New Orleans and “Watch the Soap” in Portland.
- **Co-op frame rate** — Changed coop FPS limit to 60 (custom values can be set via `update/convictionsettings.ini`, `SyncMaxStepFrequency` parameter)
- **Ammunition** — Disabled unlimited ammo for all weapons (restore original behavior by deleting `update/weapontuning.ini`)
- **Window focus** — In windowed mode, the game window starts focused and shows the cursor without a click.
- **Mouse acceleration** — Negative mouse acceleration is removed.
- **Hardware detection** — Skip hardware detection messages on startup.
- **CPU cores** — The game runs on all CPU cores.
- **LAN** — Restore LAN support.

## Options

- **Startup skips** — Separately skip startup intros and the "Press any key" screen.
- **Window mode** — Choose fullscreen, a framed window or monitor-sized borderless. The mode applies after restarting.
- **Frame rate limit** — Defaults to 60 FPS and changes live. Use 0 for unlimited or -1 for the monitor refresh rate. Turn off Vertical Sync in Advanced Video Options for rates above the monitor refresh rate.
- **Mouse input** — Optionally disable mouse smoothing, and use raw mouse input so high polling rate mice don't lose movement.
- **Gamepad camera speed** — Adjust how fast the stick turns the camera.

### Split screen

- **Split screen** — Restore the original split screen co-op in one game window (Co-op → Split Screen). Player 1 uses the keyboard and mouse or a gamepad, player 2 a gamepad. Player 2 has their own profile (`Guest` by default) for uniform and loadout, changed in the lobby with Teammate Settings. The game's invert look option is player 1's, player 2's is `InvertYPlayer2`.
- **Two instance split screen** — Launch a second game instance, with each window taking half the screen (top and bottom or side by side). The two instances connect over LAN. Each window uses its own gamepad and the second one uses its own profile.

### Display and effects

- **Widescreen** — Projection and UI placement are corrected on wide and narrow screens. HUD Aspect Ratio Constraint uses Auto for the full player view, or a selected safe-area ratio.
- **Cinematics** — Cutscene Borders preserves the authored 16:9 frame. Both fits it with side bars on wide screens and top/bottom bars on narrow screens; Off fills the view.
- **Videos** — The 16:9 movies are shown whole at their own aspect, centered, with black bars where the screen is wider or narrower. Window resize requests made during a fullscreen video apply when playback finishes, keeping its picture and captions correctly proportioned.
- **Depth of field** — Optionally disable the depth of field effect.
- **Black and white filter** — Optionally disable the desaturation effect.
- **Sonar vision** — Enable the enhanced sonar vision option.
- **Character lighting** — Optionally disable the additional character lighting, including in cutscenes.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).

### Content and controls

- **Loose files** — Load unpacked files from disk.
- **DLC** — Restores DLC content made unavailable by the server shutdown.
- **Controls** — Enable the Blacklist control scheme on keyboard and gamepad: cover is toggled with a press and released by any action that leaves it, drops from ledges are on the cover button, as in Blacklist, and Death from Above is on E / X.

![controls](https://github.com/user-attachments/assets/2498a891-2991-4604-b0dc-b5d309a0c02d)

https://user-images.githubusercontent.com/4904157/192846910-6ddfb3fb-3089-4553-ba94-b8d031855fc7.mp4

## Settings

Open Settings → Plugins → Splinter Cell Conviction Fusion Fix, from the title or pause menu. More Pages reaches the remaining options; F1 opens complete, scrollable help. Back returns to the previous page. Restart options show a notice when changed. FusionFixHub and fusionfix.io offer the same settings.

Local split screen is enabled by default; launching a second instance is optional. Gamepad camera speed and the gameplay FOV multiplier change live. FOV 1.00 is neutral; extra widening is bounded to 150°. The multiplier affects locomotion views; aiming, gadgets and cinematic cameras keep their own framing. Controller artwork applies after restarting because the game caches its Flash image library.

Pause on Focus Loss and Confine Cursor apply immediately. Check for Updates is off by default; enabling it allows an available-update notice in the game’s own menu. Ctrl+F10 opens the developer overlay when available; Debug → Info shows the build, graphics API and backbuffer resolution.

## Configuration

The added settings text is available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

Open Settings → Plugins to change the fix options in game. Short plugin lists can leave unused background bars below their entries.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `src/system`.
3. Launch the game once to create the `src/system/scripts/SplinterCellConviction.FusionFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

When upgrading from an older fix package that bundled Xidi, remove that package's `src/system/dinput8.dll`, `Xidi.32.dll` and `SDL.XidiPlugin.32.dll`. This release uses Conviction's native XInput support and the included `version.dll` loader.

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
