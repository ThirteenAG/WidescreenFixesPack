![scpt](https://thirteenag.github.io/screens/scpt/main2.jpg)

## Fixes

* **Resolution** - native support for any display resolution, including ultrawide
* **Aspect Ratio** - corrected for all rendered elements
* **HUD** - repositioned and scaled to match the active aspect ratio; at the screen edges or in a centred box (4:3, 16:9, 21:9 or custom)
* **Field of View** - corrected FOV to match the 4:3 values; cinematics keep their original framing at any aspect ratio
* **FMVs** - Fill enlarges the movies past the black bars of the game's 4:3 movie area until the picture fits the screen, without cutting it; Original keeps the 4:3 display; 480p playback limit removed
* **Framerate Issues** - animated textures, camera shake, cloth physics and AI pathing no longer have issues at higher framerates
* **Resolution Scale** - vision mode grain, thermal vision, rain particles now correctly scale at higher resolutions

## Visual Improvements

* **Character LOD** - forces the highest character LOD models
* **Shadow Resolution** - shadow resolution can now be increased past the default values
* **Buffered Shadows** - shadow buffers render correctly on current drivers without a D3D8 wrapper
* **Light Draw Distance** - increased dynamic lighting draw distance
* **Foliage Draw Distance** - increased foliage draw distance

## Miscellaneous

* **FPS Limit** - frame rate limit (60 by default; 0 = unlimited, -1 = the monitor's refresh rate), changes apply while playing
* **Skip Intro** - added an option to skip intro
* **Skip Prompt** - added an option to skip "Press Fire to Continue" prompt
* **Cutscene Borders** - cinematics keep their 16:9 picture: Letterbox bars on screens narrower than 16:9, Pillarbox bars on wider ones, Both, or Off to fill the screen
* **Mouse Acceleration** - disabled mouse acceleration
* **Raw Mouse Input** - added raw input support for mouse
* **Confine Cursor** - keeps the mouse cursor inside the game window while the game is active
* **Pause on Focus Loss** - the game pauses while another window is active, as the original does; turn it off to keep the game running in the background in windowed and borderless modes
* **Save Location** - optional: keeps profiles and saves in Documents\My Games\Splinter Cell Pandora Tomorrow instead of the game's Save folder (saves already made are not moved; copy them over to keep playing them)
* **Update Check** - optional: once per start, asks fusionfix.io whether a newer version is out and tells you in the main menu
* **Gamepad Support** - improved gamepad support via [Xidi](https://github.com/samuelgr/Xidi), with adjustable right-stick camera speed, vertical look inversion and a vibration toggle
* **Controller Prompts** - training hints follow the last input device and use the pad’s button names in English, French, German, Italian and Spanish. Auto uses Xbox names; select PlayStation or Nintendo manually
* **EAX Support** - restored EAX support via [DSOAL](https://github.com/kcat/dsoal)

## Settings

Open Options → Plugins to change the fix’s settings in the game. Window Mode is on the Video page, and the added mouse options are on Controls. Select an option and press F1 or choose Details to read its full explanation. In Details, Left/Right changes pages; Escape or F1 returns.

Choose the resolution, HUD width, gameplay field of view and video framing in FusionFixHub or on fusionfix.io. Video framing offers the original 4:3 display or Fill, which never cuts the picture.

HUD Aspect Ratio and Vision Grain Scaling apply while playing. Shadow, reflection and post-process buffer sizes require a restart because the renderer creates those textures at startup.

Gamepad Icons, Gamepad Camera Speed, Invert Gamepad Y and Vibration are under Options → Plugins → this fix. Choose More pages until the controller rows appear. Camera speed, inversion and vibration changes apply while playing. Keyboard and mouse input returns the training hints to keyboard keys. The prompts assume the supplied Xidi mappings and the default keyboard bindings.

Raw Mouse Input has a separate sensitivity multiplier. Raw Mouse Data bypasses Windows mouse speed and acceleration. Sensitivity changes apply while playing and are kept when changing presets. The default frame rate limit is 60 FPS; higher rates can affect AI and physics.

The gameplay FOV multiplier changes the normal following camera and widens it up to 150 degrees across, so very wide screens keep a usable picture. Aiming, gadgets, mirrors and cinematic views keep their own framing. Cutscene Borders sets how cinematics are framed and changes while playing.

## Configuration

The added settings text is available in English, French, German, Italian and Spanish. Choose Text Language in FusionFixHub to override the automatic choice. If the game language cannot be identified, English is used.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

Retail and current digital executables are supported. Buffered-shadow corrections apply to retail; use projector shadows (`ForceShadowMode=0`, or automatic selection) in the current digital release. Forcing buffered shadows there can prevent startup. The native Direct3D 8 renderer is retained; the Direct3D 9/11 developer overlay does not apply.

1. Download `SplinterCellPandoraTomorrow.FusionFix.zip` from this release.
2. Extract the contents directly into the game folder.
3. Launch once to create the `system/scripts/SplinterCellPandoraTomorrow.FusionFix.cfg`, then configure the fix on fusionfix.io or in FusionFixHub. Restart when an option asks you to.
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

[Website](https://thirteenag.github.io/wfp#scpt) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellPandoraTomorrow.FusionFix/dllmain.cpp)

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>

Goggles Light Color changes Sam's suit and goggle lights and their glow. Original keeps the game's own lights; the other choices are the Ops Suit Light colors from Splinter Cell Blacklist: Classic Green, Topaz, Voron Red, Navy, Pink, Violet, White, Gold, Amber and Ghost Blue. Changing the color requires a restart.
