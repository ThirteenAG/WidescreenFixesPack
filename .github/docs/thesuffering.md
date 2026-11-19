<div align="center">

<img src="https://thirteenag.github.io/screens/thesuffering/main2.jpg" width="760" alt="The Suffering Fusion Fix">

**The Suffering Fusion Fix** corrects resolution handling, aspect ratio, HUD scaling, field of view and video scaling for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thesuffering) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheSuffering.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Resolution** — The game's own list offers every display mode, also in windowed mode; the disc check is removed. Startup setup accepts the saved selection automatically, or chooses the desktop resolution on a first run.
- **Hor+ field of view** — The view keeps the original vertical field of view at any aspect ratio; recomputed when the resolution changes.
- **HUD** — In play the HUD sits at the edges of the screen, or in a centred box of the ratio you choose. Menus, pause and loading screens keep their 4:3 layout.
- **Cutscene borders** — Off, the original letterbox bars, side bars that keep the picture at 4:3, or both; switched live.
- **Window modes** — Fullscreen, a window at the chosen resolution, or borderless covering the monitor.
- **Background** — The game can keep running while another window is active, and the cursor stays inside the window.
- **Skip intro** — The Surreal logo movie is skipped at startup.
- **Level loading hang** — Loading a level could freeze on multi-core processors; fixed.
- **Display scaling** — The game gets the full resolution on scaled desktops.
- **Controller** — The PC version reads only the keyboard and mouse. The fix comes with Xidi, which turns an Xbox-compatible controller into the game's default keys and mouse: left stick move, right stick look, RT fire, LT throw, A jump, B use, X reload, Y insanity, LB/RB previous/next weapon, D-pad up flashlight, down health, left/right thrown weapons, stick clicks run/walk and first person, Back map, Start pause. In menus the left stick moves the cursor and A clicks. The layout is in `Xidi.ini`; keep the game's default keyboard settings for it.

Applies to The Suffering and The Suffering: Ties That Bind.

## Options

In The Suffering, open **Options → Graphics** for field of view and cutscene borders, or **Options → Plugins → The Suffering** for everything else. Use Back to return to the game's options.

| Setting | Values | Default | Applies |
|---|---|---|---|
| Window mode | Fullscreen, Windowed, Borderless | Fullscreen | after a restart |
| Pause in background | On, Off | On | while playing |
| Confine cursor | On, Off | On | while playing |
| Skip intro logos | On, Off | On | after a restart |
| Check for updates | On, Off | Off | while playing |
| Settings language | Language of The Suffering’s Plugins page; FusionFixHub/settings file | Automatic | when the menu opens |
| HUD aspect ratio | Auto, 4:3, 16:9, 21:9, 32:9 | Auto | while playing |
| Cutscene borders | Off, Letterbox, Pillarbox, Both | Both | while playing |
| Field of view factor | 0.5–2.0 | 1.0 | while playing |
| Frame rate limit | 30–60 FPS | 60 | while playing |

Presets: Vanilla keeps the original letterbox bars and shows the intro; Recommended and Enhanced use both border types and skip the intro. Every preset limits the game to 60 FPS. The field of view factor is 1.0 and the HUD Auto in every preset.

Ctrl+F10 opens the developer overlay for supporters.

## Known limits

- Hold Shift while starting the game to open its setup dialog. If the desktop resolution is unavailable on a first run, the dialog remains open for a manual choice.
- Skip intro removes the Surreal logo; the Midway logo and the legal screen still show.
- The game is made for 60 FPS; its physics and timing go wrong above that, so the frame rate limit goes up to 60.
- Button prompts and tutorials name the keyboard keys also when a controller is used.
- In Ties That Bind the fix's settings are on fusionfix.io and in FusionFixHub; its in-game Plugins page isn't available there.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/TheSuffering.FusionFix.cfg`.
4. Configure the fix in the game's options, on fusionfix.io or in FusionFixHub. In Ties That Bind, use fusionfix.io or FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark&layout=mobile"><source media="(max-width: 768px)" srcset="https://fusionfix.io/credits/supporters.svg?layout=mobile"><source media="(prefers-color-scheme: dark)" srcset="https://fusionfix.io/credits/supporters.svg?theme=dark"><img width="100%" src="https://fusionfix.io/credits/supporters.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
