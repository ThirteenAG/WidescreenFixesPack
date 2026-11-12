<div align="center">

<img src="https://thirteenag.github.io/screens/gtavc/main2.jpg" width="760" alt="Grand Theft Auto: Vice City - Fusion Fix">

**Grand Theft Auto: Vice City Fusion Fix** adds proper widescreen support, corrects HUD, FOV and cutscene letterboxing, and includes a range of quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#gtavc) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTAVC.FusionFix/dllmain.cpp)

</div>

---

## Fixes

<table>
<tr>
<td width="58%" valign="middle">

- **Resolution** - native support for any display resolution, including ultrawide
- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (Auto, 4:3, 16:9, 21:9, 32:9, or custom); configurable scale for HUD, radar and subtitles
- **Field of View** - corrected for widescreen
- **Resolution List** - every video mode of 640x480 and more is listed, whatever video memory the game detects
- **Cutscenes** - FOV and letterboxing adjust properly with the aspect ratio

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gtavc/hud.jpg" width="360" alt="Widescreen HUD">

</td>
</tr>
</table>

---

## New Options

<table>
<tr>
<td width="58%" valign="middle">

- **In-game menu** - the fix's settings are in the game's own menu, in the main menu and in the pause menu, under **Options > Plugins**, with the selected setting's description at the bottom of the screen: the fix first, then every other plugin with settings (such as Fusion Rain Droplets and III.VC.SA.WindowedMode, which sets the window mode). Each plugin's page has its own Preset row (Vanilla, Recommended, Enhanced; press twice to apply), its settings and Restore defaults. Most settings apply at once, the others at the next start (the page then says so above its rows). Display Setup already holds the twelve rows a screen of the game can have, so none of them is placed there
- **Settings** - stored in `GTAVC.FusionFix.cfg` next to the `.asi`, which the fix writes on its first start with every setting and a comment above each. Change settings in the game, on fusionfix.io or in FusionFixHub (also while the game runs)
- **Field of View** - a multiplier (0.5 to 2.0) of the gameplay cameras' field of view, on top of the widescreen correction; cutscenes keep theirs.
- **Skip Intro** - starts with the loading screen and the main menu, without the logo videos (on by default, after restart)
- **Videos** - the intro videos are never cut off or stretched: **Original Aspect** shows them as the game does, inside its 4:3 picture; **Fill** fits them to the whole screen
- **Animated Cutscene Borders** - the borders slide in and out
- **Light Squares Up Close** - car headlights and taillights keep their square glow when the car is near the camera, as in the original game (Vanilla preset); off by default, which shows it only on cars further away
- **In-Game Options** - a Cutscene Borders mode (off, letterbox, pillarbox, or both) and a Frame Limiter selector (off, 30 up to 240 FPS) are available right in the Display menu. Above 30 FPS, timers, mission countdowns, HUD flashing, particles, smoke and script-moved gates and doors keep the game's original speed

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gtavc/menu.jpg" width="360" alt="In-game options">

</td>
</tr>
<tr>
<td colspan="2" valign="middle">

- **Controllers** - Xbox and other XInput controllers work without extra mods, in the menus too (Start opens and closes the pause menu, B goes back), with vibration. **Modern Controls** (Enhanced preset) use the Definitive Edition's default layout: hold LT to aim over the shoulder and walk or run while shooting (sniper rifles, the rocket launcher and the camera use their scope, the left stick zooms), RT fires, B attacks, A sprints, X jumps, Y enters, LB takes the next weapon, RB is the action button and answers the phone, L3 crouches, R3 looks behind; in vehicles and boats RT accelerates, LT brakes, A fires the vehicle's weapon, RB is the handbrake, X/B look left/right, L3 is the horn, d-pad up starts a side mission, d-pad right changes the radio station, d-pad left sets off a car bomb; in helicopters and planes LB/RB are the rudder and B fires the helicopter's gun. The PC game has no weapon or radio wheel, so those are single presses, and d-pad left on foot takes the previous weapon. Off keeps the original PS2 layout. With GInput installed, the fix leaves controllers to GInput
- **Mouse** - raw relative movement keeps the game's sensitivity and inverted-axis settings. Buttons, wheel and menu cursor keep their native behavior; GInput handles its own input when installed.
- **Button Icons** - prompts and help texts show the buttons of the device you used last: controller icons (Auto uses Xbox; PlayStation or Nintendo can be selected manually) and keyboard and mouse keys as icons; they switch as soon as you change device
- **Free Camera** (on in the Enhanced preset) - look around the vehicle and the player freely with the right stick, like in San Andreas; the camera returns behind after a moment. Camera speed and inverted look are adjustable
- **Update Notice** - optional, off by default: tells you once in game when a newer version of the fix is out

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **No Island Loading** - loads all island data at once, removing the loading screen between islands
- **Seamless Interiors** - no loading screen when entering interiors, making transitions instant

</td>
<td width="42%" valign="middle" align="center">

<img src="https://github.com/user-attachments/assets/071f67fb-e0f0-4079-9fe1-0723be063030" data-src="https://thirteenag.github.io/screens/gtavc/island.webp" width="360" alt="Seamless world">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Forced MSAA** - enables multisampling at any supported level (in the settings file and FusionFixHub)
- **AA Artifact Fix** - hides the 1px lines at the top and left of the screen caused by MSAA
- **Anti-Aliasing** - Off or SMAA

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gtavc/msaa.jpg" width="360" alt="Antialiasing">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360
- **Widescreen Menu Background** - fills the screen with the menu background; turn it off to show the whole background with black borders. Transparent Menu takes precedence
- **Transparent Menu** - makes the menu background transparent with a blur

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gtavc/gamma.jpg" width="360" alt="Xbox 360 gamma">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Speed Sensitive FOV** - FOV increases with car speed
- **VCS Camera Shake** - enables GTA: Vice City Stories style camera shake in vehicles at high speed; **Shake Start Speed** and **Shake Intensity** set the speed it starts at and how strong it is
- **Text Outline** - replaces text shadows with an outline

</td>
<td width="42%" valign="middle" align="center">

<img src="https://github.com/user-attachments/assets/f45adeb4-27a7-4ced-83be-b95d966a8f9e" data-src="https://thirteenag.github.io/screens/gtavc/speed.webp" width="360" alt="Gameplay options">

</td>
</tr>
</table>

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/gtavc/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/gtavc/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/gtavc/vanilla_stretched_ultra.jpg" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/gtavc/vanilla_fixed_ultra.jpg" alt="After fixed ultrawide"> |
| The game would also be stretched in ultrawide | Fusion Fix makes it work with any aspect ratio |

| <img src="https://thirteenag.github.io/screens/gtavc/vanilla_fixed_ultra169.jpg" alt="Hud constraint"><br>An option to constrain the HUD is also available |
|:---:|

| <img src="https://github.com/user-attachments/assets/7fbdd49d-a3e0-410e-924d-229601c28a6a" alt="Animated cutscene borders"><br>Animated cutscene borders |
|:---:|

</div>

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `gta_vc.exe`.
3. Launch the game. Change the settings in **Options > Plugins**, on fusionfix.io, or in FusionFixHub. The fix creates `scripts\GTAVC.FusionFix.cfg` on its first start.
4. Anti-aliasing, console gamma and the transparent menu need d3d8to9, which the included Ultimate ASI Loader (`d3d8.dll`) provides with `UseD3D8to9=1` in `scripts\global.ini`.

> [!NOTE]
> Installing [SilentPatch](https://silentsblog.com/mods/gta-vc/) is strongly recommended - it's the perfect companion to this fix and essentially required, fixing numerous bugs, crashes and progression issues.

<details>
  <summary> Non-Windows users (Proton/Wine) - click to expand </summary>
<br>

You need to tell Wine explicitly to use the correct DLL overrides required for this plugin. There's more than one way to achieve it.

**Method 1**: `WINEDLLOVERRIDES` variable lets you temporarily specify DLL overrides. It can be used from a command line as well as in the Steam launcher. In the case of the command line, simply prepend the usual start command with:

```
WINEDLLOVERRIDES="d3d8=n,b"
```

For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="d3d8=n,b" %command%`.

**Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Grand Theft Auto: Vice City in `$HOME/.steam/steam/steamapps/compatdata/12110/pfx`. Then you need to run `winecfg` with that path:

```
WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/12110/pfx" winecfg
```

Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.

Related Wine documentation:
* [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
* [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)

</details>

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>

---

## Also check out: Project2DFX

<div align="center">

**[VC.Project2DFX](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtavc)** - a visual upgrade that pairs perfectly with this Fusion Fix. It adds the Project2DFX LOD corona effect to Grand Theft Auto: Vice City, lighting up the neon-soaked skyline with thousands of distant lamppost and streetlight glows and greatly extending the draw distance.

[Download](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtavc) · [Website](https://fusionfix.io/p2dfx)

<img src="https://raw.githubusercontent.com/ThirteenAG/III.VC.SA.IV.Project2DFX/refs/heads/master/.github/docs/videos/vc_demo.webp" alt="VC.Project2DFX">

</div>
