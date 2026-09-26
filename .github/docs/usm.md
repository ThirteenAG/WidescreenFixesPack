<div align="center">

<img src="https://thirteenag.github.io/screens/usm/main2.jpg" width="760" alt="Ultimate Spider-Man Widescreen Fix">

**Ultimate Spider-Man Widescreen Fix** adds proper widescreen and ultrawide support, corrects the HUD, radar, map and field of view, and improves mouse camera movement.

[Website](https://thirteenag.github.io/wfp#usm) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/UltimateSpiderMan.WidescreenFix) · [Default INI](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/UltimateSpiderMan.WidescreenFix/scripts/UltimateSpiderMan.WidescreenFix.ini)

</div>

---

## Fixes

<table>
<tr>
<td width="58%" valign="middle">

- **Widescreen and Ultrawide** - keeps characters and scenery in proportion and shows more of the world on wider screens
- **HUD and Radar** - keeps health bars, race indicators and the radar in proportion and correctly positioned
- **Map and Enemy Markers** - keeps icons aligned with their locations as you move the camera or pan and zoom the map
- **Disappearing Buildings** - fixes buildings incorrectly disappearing and reappearing when moving through the city in widescreen
- **Resolution** - starts at your desktop resolution if no resolution is saved, then remembers your choice in the game's settings

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/usm/hud.jpg" width="360" alt="Widescreen HUD and working radar">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Menus and Comic Cutscenes** - prevents stretched backgrounds and comic panels, adding black side borders where needed; fixes the stretch when a 3D cutscene turns into a comic panel
- **Mouse Camera** - responds to small mouse movements and removes the speed cap on fast movements; waits three seconds after your last camera input before automatically recentering

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/usm/menu.jpg" width="360" alt="Corrected menu sliders and cursor">

</td>
</tr>
</table>

---

## Settings

<table>
<tr>
<td width="58%" valign="middle">

Edit `scripts/UltimateSpiderMan.WidescreenFix.ini` to change these settings.

- **HUD Placement** - `HudAspectRatioConstraint = Auto` places the HUD toward the screen edges. Use `4:3` for the original placement, or a ratio such as `16:9` or `21:9` to bring it closer to the center
- **Camera View** - `FOVFactor` changes how much of the world you see. `1.0` is the default; higher values give a wider view
- **Windowed Mode** - `WindowedMode = 0` uses fullscreen; `1` uses a borderless window at your selected resolution; `2` fills the primary monitor with a borderless window. Restart the game to apply
- **Keep Cursor in the Game** - `ClipCursor = 1` prevents the cursor from leaving the active game window. Alt+Tab releases it

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/usm/vanilla_fixed_ultra169.jpg" width="360" alt="HUD placement constrained on an ultrawide screen">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Draw Distance** - `DrawDistanceScale = 1.0` keeps the original distance. Raise it up to `2.0` for more distant detail, or above `2.0` up to `10.0` for the experimental extended-distance mode. Higher values can reduce performance, and some objects may still appear as you approach. Not fully tested, may cause crashes
- **Frame Rate Limit** - `FPSLimit` sets the target frame rate; the default is `60`. Set `0` to keep the game's original limiter
- **Smoother Edges** - under `[GRAPHICS]`, set `SMAA = 1` to reduce jagged edges in the game world without smoothing the HUD or menu text. Off by default; does not work with MSAA
- **Console-Style Colors** - under `[GRAPHICS]`, set `ConsoleGamma = 1` for stronger contrast and deeper colors. Off by default
- **Skip Startup Screens** - `SkipIntro = 1` skips the opening logos and copyright screen

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/usm/smaa.jpg" width="360" alt="SMAA">

</td>
</tr>
</table>

---

## Before / After

<div align="center">

| Before | After |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/usm/main1.jpg" alt="Original widescreen output"> | <img src="https://thirteenag.github.io/screens/usm/main2.jpg" alt="Corrected widescreen output"> |
| Original widescreen output | Correct proportions, HUD and field of view |
| <img src="https://thirteenag.github.io/screens/usm/vanilla_stretched_ultra.jpg" alt="Original ultrawide output"> | <img src="https://thirteenag.github.io/screens/usm/vanilla_fixed_ultra.jpg" alt="Corrected ultrawide output"> |
| Original ultrawide output | Correct ultrawide view with an aligned radar and HUD |

| `DrawDistanceScale = 1.0` | `DrawDistanceScale = 10.0` |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/usm/dd1.jpg" alt="Original draw distance at DrawDistanceScale 1.0"> | <img src="https://thirteenag.github.io/screens/usm/dd10.jpg" alt="Expanded draw distance at DrawDistanceScale 10.0"> |
| Original draw distance | More detail visible in the distance |

<img src="https://thirteenag.github.io/screens/usm/demo.webp" alt="Demo">

</div>

---

## Installation

1. Download the `.zip` from this release.
2. Extract the contents into the game folder, beside `USM.exe`, preserving the folders in the archive.
3. Optionally edit `scripts/UltimateSpiderMan.WidescreenFix.ini` to configure the available options.
4. Launch the game.

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
