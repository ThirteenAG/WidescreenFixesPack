<div align="center">

<img src="https://thirteenag.github.io/screens/scct/main2.jpg" width="760" alt="Splinter Cell: Chaos Theory Widescreen Fix">

**Splinter Cell: Chaos Theory Widescreen Fix** corrects widescreen scaling and field of view, adds raw mouse input and controller support, and restores EAX audio through DSOAL.

[Website](https://thirteenag.github.io/wfp#scct) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellChaosTheory.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Set the output resolution on fusionfix.io, in FusionFixHub or in the game's menu.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the field of view for widescreen.
- **Videos** - Choose Original aspect with black bars, cropped Fill or Stretch.
- **Window modes** — Fullscreen, windowed or borderless, using the game's own window mode (the game always forced fullscreen at startup). Alt+Enter switches between fullscreen and the window mode. A window is centered; at the desktop resolution its frame lies outside the screen. Sharp on high-DPI displays, no rounded corners on Windows 11.

### Input and compatibility

- **Mouse input** - Use raw mouse input with a separate sensitivity multiplier, and choose whether Windows mouse speed and acceleration apply.
- **Gamepad** — Support controllers through [Xidi](https://github.com/samuelgr/Xidi).
- **Audio** — Restore EAX support through [DSOAL](https://github.com/kcat/dsoal).
- **Saved settings** — Prevent settings from resetting on each launch.
- **Steam overlay** — Restore the Steam overlay when available.

## Options

- **Frame rate limit** — Limit FPS to reduce game bugs; the default is 60 FPS.
- **Skip intro** — Skip startup intros.
- **Startup prompt** — Skip the “Press Any Key to Continue” screen (on by default).

### Graphics and lighting

- **Shadows** — Increase shadow resolution.
- **Goggles** — Choose the goggles' light color on fusionfix.io, in FusionFixHub or in the game's menu.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).
- **Suit indicators** — Use Blacklist-style indicators that respond to the player's visibility.

---

## Settings

Open **Settings → Plugins** to configure this fix and other installed plugins. Each plugin has its own presets and Restore Defaults. Select **Details** or press **F1** for the complete setting description; use Left and Right to read longer descriptions, and Escape to return.

**Window Mode** sits below Resolution on the native Display page. Choose fullscreen, windowed or borderless, then restart when prompted. The Display page's Default button also resets Window Mode.

Choose the resolution, HUD width and gameplay field of view in FusionFixHub or on fusionfix.io. The gameplay FOV multiplier changes the normal following camera; aiming, gadgets and cinematic views keep their own framing.

Video Widescreen Mode offers Original aspect with black bars, cropped Fill (the default), and Stretch. Story videos use the same setting. Video framing and gameplay FOV changes apply while playing.

Some videos contain black bars in the video itself. Original and Stretch keep those bars; Fill may leave narrow ones.

Raw Mouse Input has a separate sensitivity multiplier. Raw Mouse Data bypasses Windows mouse speed and acceleration. Sensitivity changes apply while playing and are kept when changing presets. The default frame rate limit is 60 FPS.

The added settings menu is for single-player. Co-op and multiplayer use their own menus.

## Configuration

The added settings text is available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `System`.
3. Launch once to create the `System/scripts/SplinterCellChaosTheory.WidescreenFix.cfg`, then configure the fix in FusionFixHub or on fusionfix.io. Most settings apply after restarting the game; video framing and the gameplay FOV multiplier apply live.
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
>    WINEDLLOVERRIDES="msacm32,msvfw32,dinput8,dsound,dsoal-aldrv,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="msacm32,msvfw32,dinput8,dsound,dsoal-aldrv,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Splinter Cell: Chaos Theory in `$HOME/.steam/steam/steamapps/compatdata/13570/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/13570/pfx" winecfg
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
