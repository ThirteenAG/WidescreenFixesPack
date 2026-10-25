<div align="center">

<img src="https://thirteenag.github.io/screens/usm/main2.jpg" width="760" alt="Ultimate Spider-Man Widescreen Fix">

**Ultimate Spider-Man Widescreen Fix** adds proper widescreen and ultrawide support, corrects the HUD, radar, map and field of view, and improves mouse camera movement.

[Website](https://thirteenag.github.io/wfp#usm) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/UltimateSpiderMan.WidescreenFix)

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

Change these settings on fusionfix.io or in FusionFixHub.

- **HUD Placement** - **Auto** places the HUD toward the screen edges. Use 4:3 for the original placement, or a ratio such as 16:9 or 21:9 to bring it closer to the center
- **Camera View** - The field of view factor changes how much of the world you see. 1.0 is the default; higher values give a wider view
- **Window Mode** - Fullscreen, a framed window, or a borderless window covering its monitor. Changes apply while the game runs; Alt+Enter switches between fullscreen and the last window mode, restoring the framed position and size
- **Keep Cursor in the Game** - Keeps the cursor inside the window while the game is active. Alt+Tab releases it; turning it off applies immediately

</td>
<td width="42%" valign="middle" align="center">

<img src="https://thirteenag.github.io/screens/usm/vanilla_fixed_ultra169.jpg" width="360" alt="HUD placement constrained on an ultrawide screen">

</td>
</tr>
<tr>
<td width="58%" valign="middle">

- **Draw Distance** - 1.0 keeps the original distance. Raise it up to 2.0 for more distant detail, or above 2.0 up to 10.0 for the experimental extended-distance mode. Higher values can reduce performance, and some objects may still appear as you approach. The extended-distance mode may cause crashes
- **Frame Rate Limit** - Sets the target frame rate; the default is 60. Set 0 to keep the game's original limiter
- **Smoother Edges** - SMAA reduces jagged edges in the game world without smoothing the HUD or menu text. Off by default; does not work with MSAA
- **Console-Style Colors** - Console gamma gives stronger contrast and deeper colors. Off by default
- **Skip Startup Screens** - Skips the opening logos and copyright screen

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

| Draw distance 1.0 | Draw distance 10.0 |
|:---:|:---:|
| <img src="https://thirteenag.github.io/screens/usm/dd1.jpg" alt="Original draw distance at 1.0"> | <img src="https://thirteenag.github.io/screens/usm/dd10.jpg" alt="Expanded draw distance at 10.0"> |
| Original draw distance | More detail visible in the distance |

<img src="https://thirteenag.github.io/screens/usm/demo.webp" alt="Demo">

</div>

---

**FOV factor** accepts 0.5–2.0 with 1.0 preserving corrected native framing. FOV and HUD aspect-constraint edits apply at the next game frame; Auto HUD uses the full screen width. The multiplier applies only to the player chase view, preserving comic, scripted and zoom framing.

**Window modes** keep the chosen size on high-DPI displays and use square corners.

**Confine cursor** is on by default in all window modes and applies while the game runs. The cursor is released when the game loses focus, while the window is dragged, in system menus or when the option is turned off.

## Installation

1. Download the `.zip` from this release.
2. Extract the contents into the game folder, beside `USM.exe`, preserving the folders in the archive.
3. Launch the game once to create `scripts/UltimateSpiderMan.WidescreenFix.cfg`.
4. Configure the fix on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Settings

SMAA, console gamma, cursor confinement, field of view, HUD aspect ratio and window mode apply while the game runs; FusionFixHub shows which options need a restart. Defaults include a 60 FPS limit and Auto HUD placement. An old `scripts/UltimateSpiderMan.WidescreenFix.ini` isn't used anymore and can be deleted.

## Known limits

- SMAA is off in every preset; turn it on separately.
- No in-game options menu; use fusionfix.io or FusionFixHub.

---

## Support the project

This project is only possible because of Fusion Fix sponsors. If you would like to see further updates and improvements, consider supporting it.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
