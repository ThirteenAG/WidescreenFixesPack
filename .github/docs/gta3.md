<div align="center">

<img src="https://thirteenag.github.io/screens/gta3/main2.jpg" width="760" alt="Grand Theft Auto III Widescreen Fix">

**Grand Theft Auto III Widescreen Fix** adds proper widescreen support, corrects HUD, FOV and cutscene letterboxing, and includes a range of quality-of-life improvements.

[Website](https://thirteenag.github.io/wfp#gta3) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA3.WidescreenFix/dllmain.cpp) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA3.WidescreenFix/scripts/GTA3.WidescreenFix.ini)

</div>

---

## Fixes

<table>
<tr>
<td width="58%" valign="middle">

- **Aspect Ratio** - corrected for all rendered elements
- **HUD** - repositioned and scaled to match the active aspect ratio; optional constraint (16:9, 21:9, or custom); configurable scale for HUD, radar and subtitles
- **Field of View** - corrected for widescreen
- **Cutscenes** - FOV and letterboxing adjust properly with the aspect ratio

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gta3/hud.jpg" width="360" alt="Widescreen HUD">

</td>
</tr>
</table>

---

## New Options

<table>
<tr>
<td width="58%" valign="middle">

- **In-Game Options** - a Cutscene Borders mode (off, letterbox, pillarbox, or both) and a Frame Limiter selector (off, 30 up to 240 FPS) are available right in the Display menu (Warning: Widescreen Fix does not fix any high fps related issues)

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gta3/menu.jpg" width="360" alt="In-game options">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **No Island Loading** - loads all island data at once, removing the loading screen between islands
- **Zone Cull Fix** - disables zone culling so objects stop disappearing

</td>
<td width="42%" valign="middle" align="center">

<img src="https://github.com/user-attachments/assets/b92987de-9b82-4c25-ae9e-1f879154910b" width="360" alt="Seamless world">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Forced MSAA** - enables antialiasing at any supported level
- **AA Artifact Fix** - hides the 1px lines at the top and left of the screen caused by antialiasing

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gta3/msaa.jpg" width="360" alt="Antialiasing">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Xbox 360 Gamma** - a custom gamma curve that produces a higher-contrast image with deeper colors, similar to GTA IV on the Xbox 360
- **Transparent Menu** - makes the menu background transparent with a blur

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/gta3/gamma.jpg" width="360" alt="Xbox 360 gamma">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Speed Sensitive FOV** - FOV increases with car speed
- **VCS Camera Shake** - enables GTA: Vice City Stories style camera shake in vehicles at high speed
- **Text Outline** - replaces text shadows with an outline

</td>
<td width="42%" valign="middle" align="center">

<img src="https://github.com/user-attachments/assets/1f62e1bc-a508-49a3-a7cb-77a5d9b3e1c5" width="360" alt="Gameplay options">

</td>
</tr>
</table>

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/gta3/main1.jpg" alt="Before 4:3"> | <img src="https://thirteenag.github.io/screens/gta3/main2.jpg" alt="After 16:9"> |
| Original 4:3 output | Correct 16:9 aspect ratio with proper HUD and FOV |
| <img src="https://thirteenag.github.io/screens/gta3/vanilla_stretched_ultra.jpg" alt="Before stretched ultrawide"> | <img src="https://thirteenag.github.io/screens/gta3/vanilla_fixed_ultra.jpg" alt="After fixed ultrawide"> |
| The game would also be stretched in ultrawide | Widescreen fix makes it work with any aspect ratio |

| <img src="https://thirteenag.github.io/screens/gta3/vanilla_fixed_ultra169.jpg" alt="Hud constraint"><br>An option to constrain the HUD is also available |
|:---:|

| <img src="https://github.com/user-attachments/assets/305b8d1f-97cb-4f93-92f2-75fa1a9b0b85" alt="Animated cutscene borders"><br>Animated cutscene borders |
|:---:|

</div>

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents directly into the game folder - the same folder as `gta3.exe`.
3. Optionally edit `GTA3.WidescreenFix.ini` to configure the available options.
4. Launch the game.

<details>
  <summary> Legacy mode - click to expand </summary>
<br>

To use the previous version of the fix, place `GTA3.WidescreenFixLegacy.ini` next to the `.asi` file with the following content:

```
[MAIN]
; Use this option to override the game resolution (-1|0|*).
ResX = 0
ResY = 0
; Set game aspect ratio (auto|4:3|5:4|16:9|*:*).
ForceAspectRatio = auto
; Used to properly scale frontend textures, such as menu background or loading screens (auto|4:3|5:4|16:9|*:*).
FrontendAspectRatio = auto
; Disable any FOV related fixes, it's not recommended (0|1).
DontTouchFOV = 0
; Use original FOV value during cutscenes (0|1).
RestoreCutsceneFOV = 1 
; The settings below are used to scale parts of the game UI differently.
; If set to "0.0" original values will be used.
HudWidthScale = 0.8     ; Original value is 1.0.
HudHeightScale = 0.8    ; Original value is 1.0714285.
RadarWidthScale = 0.9   ; Original value is 1.0.
SubtitlesScale = 0.0    ; Original value is 1.0.

[MISC]
; Same as in VCS PCE, cutscene border size will be adjusted to fit your resolution. On resolutions bigger than 16:9 vertical borders will be used. Note that you can enable or disable cutscene borders via 'WIDESCREEN'/'BORDERS' option in game menu (0|1).
SmartCutsceneBorders = 1 
; Drawing 1px line at top and left sides of the screen to hide antialiasing bug(0|1|2). (2) creates 4 lines to make picture symmetrical.
HideAABug = 1
; Fixes vehicle lights coronas size. (0|1).
SmallerVehicleCorona = 0
; Remove vehicle light square. (0|1).
NoLightSquare = 1
; Enables antialiasing. Requires asi loader (max|0|*).
ForceMultisamplingLevel = max
; Makes radar size same as in GTA IV (0|1).
IVRadarScaling = 1
; Replaces text shadow with outline (0|1|2).
ReplaceTextShadowWithOutline = 2
```

</details>

<details>
  <summary> Non-Windows users (Proton/Wine) - click to expand </summary>
<br>

You need to tell Wine explicitly to use the correct DLL overrides required for this plugin. There's more than one way to achieve it.

**Method 1**: `WINEDLLOVERRIDES` variable lets you temporarily specify DLL overrides. It can be used from a command line as well as in the Steam launcher. In the case of the command line, simply prepend the usual start command with:

```
WINEDLLOVERRIDES="d3d8=n,b"
```

For Steam, head to the game's properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="d3d8=n,b" %command%`.

**Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Grand Theft Auto III in `$HOME/.steam/steam/steamapps/compatdata/12100/pfx`. Then you need to run `winecfg` with that path:

```
WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/12100/pfx" winecfg
```

Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it's been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.

Related Wine documentation:
* [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
* [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)

</details>

---

## Support the project

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
