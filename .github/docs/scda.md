<div align="center">

<img src="https://thirteenag.github.io/screens/scda/main2.jpg" width="760" alt="Splinter Cell: Double Agent Fusion Fix (PC)">

**Splinter Cell: Double Agent Fusion Fix (PC)** corrects widescreen scaling and field of view, adds raw mouse input and controller support, and offers shadow, glow, and lighting options.

[Website](https://thirteenag.github.io/wfp#scda) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellDoubleAgent.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — Set the output resolution on fusionfix.io or in FusionFixHub.
- **Aspect ratio** — Correct widescreen aspect ratio scaling.
- **HUD** — Correct HUD scaling for widescreen.
- **Field of view** — Adjust the normal following camera live; 1.00 is neutral and extra widening is bounded to 150°.
- **Cinematics** — Cutscene Borders keeps the authored framing. Both adds side bars on wider screens and top/bottom bars on narrower screens; Off fills the screen with a Hor+ view.
- **Videos** — Keep the movie’s own aspect ratio with borders where needed.

### Input and compatibility

- **Mouse input** — Add raw mouse input support.
- **Gamepad** — Support controllers through [Xidi](https://github.com/samuelgr/Xidi).
- **Audio** — Restore EAX support through [DSOAL](https://github.com/kcat/dsoal).
- **AI hearing** — Fix the [AI hearing issue](https://www.youtube.com/watch?v=uX_FcPvM64c).
- **Loading videos** — Press Enter to skip loading screens that are playing a video.
- **Startup crash** — Fix the crash on startup that otherwise needs the `-ll` command line switch.

## Options

- **Shadows** — Increase shadow resolution.
- **Glow and bloom** — Increase the resolution of glow and bloom effects.
- **Console gamma** — Apply a gamma curve approximating the Xbox 360 version.
- **Anisotropic filtering** — Keep textures sharp at steep viewing angles, up to 16x.

### Startup and frame rate

- **Frame rate limit** — Limit FPS to reduce game bugs; 60 FPS by default.
- **Skip intro** — Skip startup intros.

### Lighting

- **Goggles** — Keep the original lights, which change with your visibility, or pick one of the Splinter Cell Blacklist suit light colors: Classic Green, Topaz, Voron Red, Navy, Pink, Violet, White, Gold, Amber or Ghost Blue.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).
- **Suit indicators** — Use Blacklist-style indicators that respond to the player's visibility.

---

## Settings

Open Options > Plugins > Splinter Cell Double Agent Fusion Fix in the single-player menu, including from Pause > Options. On/off options show On or Off, while indicator and color choices use names. More Pages reaches the remaining settings; F1 opens the selected option’s full help, and Back returns. FusionFixHub and fusionfix.io also configure the fix. HUD width changes apply immediately: Auto puts the HUD at the screen edges, 4:3 keeps the original layout. The gameplay FOV multiplier changes the normal following camera; aiming, cinematic and minimap views keep their own framing.

Raw Mouse Input has a separate sensitivity multiplier. Raw Mouse Data bypasses Windows mouse speed and acceleration. Sensitivity changes apply while playing and are kept when changing presets. The live frame rate limit defaults to 60 FPS; 0 removes the limit and -1 follows the monitor refresh rate.

Native Graphics Overrides lets the fix apply its graphics choices to the game. Disable it to keep the game's own graphics settings. Leave an individual override empty to leave that setting unchanged. A custom loading image can be placed beside the plugin with the same filename and a `.png` extension.

Window Mode selects fullscreen, a framed window or borderless and applies on restart. A framed window uses the chosen client resolution; borderless renders at the target monitor’s resolution. Pause on Focus Loss and Confine Cursor apply immediately. Check for Updates is off by default; enabling it shows an available update in the game’s own menu.

Gamepad Icons uses Xbox labels in Auto; PlayStation and Nintendo labels are selected manually. Gamepad Camera Speed scales the right stick live, with 1.00 preserving the game’s speed and deadzone. Use the game’s own Invert Look Up/Down option for vertical inversion. Vibration can be disabled independently. Controller input stays with the game and the bundled Xidi.

Ctrl+F10 opens the developer overlay when available. Debug > Info reports the build, graphics API and back-buffer resolution.

The online window and frame rate options apply to multiplayer. The added settings menu is for single-player.

## Configuration

The added settings and help text are available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

Vanilla uses the original shadow, glow and reflection resolutions and disables the added RGB lighting. Run during forced walk remains your choice when switching presets. Single Core Affinity is an experimental compatibility option for loading hangs; it is off by default and can reduce performance.

Some custom window sizes are absent from the game’s resolution dropdown. Use the fix’s Horizontal Resolution and Vertical Resolution controls for those sizes. If the display reports fewer than seven modes, some entries can repeat.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game’s installation folder, preserving its directory structure. The included files belong under `SCDA-Offline/System`.
3. Launch the game to create `SCDA-Offline/System/scripts/SplinterCellDoubleAgent.FusionFix.cfg`.
4. Configure in Options > Plugins, on fusionfix.io or in FusionFixHub. Live settings apply while playing; the menu identifies settings that require a restart.

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

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
