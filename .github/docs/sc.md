![sc](https://thirteenag.github.io/screens/sc/main2.jpg)

## Fixes

* **Resolution** - native support for any display resolution, including ultrawide
* **Aspect Ratio** - corrected for all rendered elements
* **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom)
* **Field of View** - corrected FOV to match the 4:3 values
* **FMVs** - expanded to fill the screen at widescreen resolutions, with an option to keep the original 4:3 display; 480p playback limit removed
* **Framerate Issues** - animated textures, camera shake, cloth physics and AI pathing no longer have issues at higher framerates
* **Resolution Scale** - vision mode grain now correctly scales at higher resolutions

## Visual Improvements

* **Character LOD** - forces the highest character LOD models
* **Xbox Shadows and Water Shader** - restores these effects from the original Xbox version
* **Shadow Resolution** - shadow resolution can now be increased past the default values
* **Buffered Shadows** - shadow buffers and spot light textures render correctly on current drivers without a D3D8 wrapper

## Miscellaneous

* **FPS Limit** - added an option to limit FPS to avoid game-breaking bugs
* **Skip Intro** - added an option to skip intro
* **Skip Prompt** - added an option to skip "Press Fire to Continue" prompt
* **Cutscene Borders** - added an option to disable cutscene borders
* **Mouse Acceleration** - disabled mouse acceleration
* **Raw Mouse Input** - added raw input support for mouse
* **Gamepad Support** - improved gamepad support via [Xidi](https://github.com/samuelgr/Xidi)
* **EAX Support** - restored EAX support via [DSOAL](https://github.com/kcat/dsoal)

## Settings

Choose the resolution, HUD width, gameplay field of view and video framing in FusionFixHub or on fusionfix.io. Video framing offers the original aspect ratio, cropped Fill and Stretch. Window Mode offers Fullscreen, Windowed and Borderless.

Raw Mouse Input has a separate sensitivity multiplier. Raw Mouse Data bypasses Windows mouse speed and acceleration. Sensitivity changes apply while playing and are kept when changing presets. The default frame rate limit is 60 FPS.

The gameplay FOV multiplier changes the normal following camera. Aiming, gadgets and cinematic views keep their own framing. Restore Cutscene FOV and Cutscene Borders control cinematic presentation separately.

## Configuration

The added settings text is available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used. The in-game font uses unaccented letter equivalents; FusionFixHub keeps the full translations.

In the Plugins page, select a setting and choose Details or press F1 to read its complete description. Use the arrow keys to change description pages and Back or Escape to return.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download `SplinterCell.WidescreenFix.zip` from this release.
2. Extract the contents directly into the game folder - preserving the `system` folder beside the installed game files.
3. Launch the game to create `system/scripts/SplinterCell.WidescreenFix.cfg`.
4. Configure the fix in FusionFixHub or on fusionfix.io. Mouse sensitivity, video framing, HUD width and field of view apply while the game runs; restart the game after changing the other settings.

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
>    WINEDLLOVERRIDES="msacm32,msvfw32,dinput8,dsound,dsoal-aldrv,Xidi.32=n,b"
>    ```
> For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="msacm32,msvfw32,dinput8,dsound,dsoal-aldrv,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://silentsblog.com/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Splinter Cell in `$HOME/.steam/steam/steamapps/compatdata/13560/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/13560/pfx" winecfg
> ```
> Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.
>
> ![winecfg-dll-override](https://silentsblog.com/assets/img/setup/winecfg-dll-override.png)
>
> Related Wine documentation:
> * [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
> * [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)
> </details>

[Website](https://thirteenag.github.io/wfp#sc) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCell.WidescreenFix/dllmain.cpp)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
