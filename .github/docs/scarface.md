<div align="center">

<img src="https://user-images.githubusercontent.com/4904157/196225002-83a52973-0588-4c65-8019-581bffc7c1b8.gif" width="760" alt="Scarface: The World Is Yours Fusion Fix">

**Scarface: The World Is Yours Fusion Fix** corrects mouse look and framerate-dependent behavior, adds widescreen HUD placement and controller prompts, and provides native fullscreen, framed and borderless window modes and optional PS2 character blood effects.

[Website](https://thirteenag.github.io/wfp#scarface) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/Scarface.FusionFix)

</div>

---

## Fixes

- **Mouse look** — Correct mouse look behavior.
- **Gamepad** — Support controllers through [Xidi](https://github.com/samuelgr/Xidi).
- **Gamepad controls** — Use triggers to brake/accelerate and aim/shoot.
- **Controller prompts** — Show button icons according to the last keyboard, mouse, or controller input. Icons and controls-menu binding names follow the active Xidi profile, including changes while a dialog is open.
- **Weapon selection** — Scroll through weapons with the mouse wheel.
- **Windowed mode** — Select fullscreen, a framed window at the chosen resolution, or desktop borderless through the game's native display path. Framing is DPI-aware and keeps full render dimensions, including a desktop-sized client.
- **Widescreen HUD** — Reposition HUD elements and notifications for the chosen aspect ratio, with an option to retain the original 4:3 placement.
- **Skip intro** — Skip the legal/license screen, intro movies, and press-to-start screen.
- **Background startup** — Fix the input-acquisition loop that can hang the game when it launches without focus.
- **High framerates** — Correct the animation update's minimum frame delta, rage depletion rate, and mortar up-force decay.
- **Weapon recoil** — Correct the horizontal recoil maximum, which originally reads the minimum value.

## Options

- **Vehicle camera** — Delay automatic recentering after stick or mouse look; the default is 3 seconds. Right-mouse-button aim hold keeps its existing behavior.
- **Character blood** — Restore the PS2-style blood texture pass on characters using the game's joint damage and blood texture.
- **Console gamma** — Apply a custom gamma curve similar to GTA IV on Xbox 360.
- **Antialiasing** — Enable SMAA post-processing antialiasing; it does not work with MSAA.

The fix creates `scripts/Scarface.FusionFix.cfg` on first launch. Configure it on fusionfix.io or in FusionFixHub; the FOV multiplier, HUD aspect constraint, window mode, raw mouse switch, mouse sensitivity and focus-pause policy, movie framing, cutscene borders, vehicle recenter delay and frame-rate limit apply while the game runs. Other settings apply after restarting the game. An old `scripts/Scarface.FusionFix.ini` isn't used anymore and can be deleted. Vanilla disables the added raw mouse input, cursor confinement and timing/recoil corrections; Recommended restores their defaults. Window mode, sensitivity and controller artwork remain the player’s choices.

| Setting | Default | Behavior |
|:---|:---|:---|
| Frame Rate Limit | 60 | Frame cap from 30 to 1000, Unlimited, or the monitor's refresh rate; applies while playing. The game's vertical sync may set a lower cap. |
| Cutscene Borders | Both | Letterboxed cinematic cameras: crop without bars, letterbox, pillarbox or both. |
| Movie Framing | Fill | Original aspect with black bars, crop to fill, or stretch; applies while playing. |
| Pause in Background | On | Pauses and mutes the game when it loses focus; turn off to keep it running. Applies while playing. |
| Raw Mouse Input | On | Uses the mouse's raw movement; turn off to use cursor-based movement. |
| Mouse Sensitivity Multiplier | 1.0 | Multiplier for the game's mouse slider, from 0.1 to 5.0; applies while playing. |
| Field of view | 1.0 | Multiplies the normal gameplay field of view, from 0.5 to 2.0; applies while playing. |
| Skip Intro | On | Skips the legal screen, intro movies and press-to-start screen. |
| Modern Controls | On | Modern on-foot and vehicle controls for controllers (through the bundled Xidi). |
| Controller Prompts | On | Switches between keyboard labels and controller icons with the last input. |
| Gamepad Icons | Xbox 360 | Button artwork and controls-menu names: Xbox 360, Xbox One, PS3, PS4, PS5, Switch, Steam Deck or Steam Controller. Restart the game after changing it. |
| Mouse Wheel Weapons | On | Selects weapons with the mouse wheel. |
| Vehicle Camera Recenter Delay | 3.0 | Seconds before the vehicle camera recenters; 0 removes the delay. Applies while playing. |
| Window Mode | Fullscreen | Fullscreen, framed window or desktop borderless. |
| HUD Aspect Ratio | Auto | Auto uses the screen's aspect ratio; 4:3 keeps the original HUD placement; other ratios such as 16:9 work too. Applies while playing. |
| Character Blood | On | Restores the character blood effects. |
| Console Gamma | Off | Console-style gamma. |
| SMAA | Off | SMAA anti-aliasing; turn the game's MSAA off first. |

The following compatibility fixes are on by default:

| Setting | Default |
|:---|:---|
| Vehicle camera smoothing | On |
| Animation Timing Fix | On |
| Rage Timing Fix | On |
| Mortar Timing Fix | On |
| Recoil Range Fix | On |

The controls configuration screen rebinds the selected row when a controller button is pressed. Assigning a button already used by another row can leave that row **Unbound**. The controller exit prompt uses **Start**, matching the game's binding for that screen.

The FOV multiplier applies immediately to normal follow/free-look and vehicle/boat cameras. Scripted views, aiming, sniper zoom and transitions keep their original framing. HUD width also changes immediately. Alt+Enter and Window Mode remember the framed window’s size and position; a failed mode change returns to the previous mode.

Raw mouse input provides relative movement for the camera and map. Absolute pointing devices use the game’s cursor controls. Controller bindings and prompts follow the selected Xidi profile.

Pause on Focus Loss pauses the game in the background. Disable it to keep playing and audio running while another application is active.

Video Mode offers the original picture with black bars, crop to fill, or stretch. It preserves the source picture’s proportions unless Stretch is selected. This setting does not enable larger replacement movie files.

Cutscene Borders applies to letterboxed cinematic views. Removing the bars crops the picture; pillarboxing keeps the original camera width. Subtitles remain above the bars. Other cinematic views keep their original framing. Bars switch immediately.

The frame limit can change during play; the game’s vertical sync may impose a lower limit. Vehicle camera smoothing keeps the original feel as the frame rate changes.

The native resolution picker uses detected Windows sizes supported by the active renderer instead of a fixed list. Native refresh queries select the highest matching rate. First launch and invalid saved dimensions use the physical desktop; valid saved sizes remain intact.

**Confine cursor** keeps the pointer inside the game window. It can be changed while playing and is enabled by default. The pointer is released when you leave the game, drag its window, open a system menu or turn the option off.

## Known limits

- Anti-aliasing offers Off and SMAA through `GRAPHICS.AntiAliasing`; it remains off by default and has no sharpness control. Turn off the game’s MSAA before enabling SMAA.
- Update notices and an in-game restart warning are not supported.

- Controller icons follow the **Gamepad Icons** setting; the icon family isn't detected from the connected pad.
- The default frame rate limit is 60 FPS: physics and scripted sequences weren't checked at higher frame rates.
- Movies keep the resolution of their source files.
- No in-game settings menu; FusionFixHub and fusionfix.io change the settings.

## Installation

Keep SilentPatch installed alongside FusionFix; it remains a required part of the installation.

1. Download and install [SilentPatch](https://github.com/CookiePLMonster/SilentPatchScarface/releases/latest). It includes required [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
2. Download and unpack [Scarface.FusionFix.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/scarface/Scarface.FusionFix.zip) to game's root folder.
   Keep the bundled `Xidi.32.dll` and `Xidi.ini` in the game folder so controller prompts can resolve the active profile's mappings.
3. After the first launch, use fusionfix.io or FusionFixHub to configure the fix.
4. Launch the game.



<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
