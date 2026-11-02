<div align="center">

<img src="https://thirteenag.github.io/screens/thegodfather/main2.jpg" width="760" alt="The Godfather Fusion Fix">

**The Godfather Fusion Fix** corrects resolution handling, HUD scaling and field of view for widescreen displays.

[Website](https://thirteenag.github.io/wfp#thegodfather) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/TheGodfather.FusionFix/dllmain.cpp)

</div>

---

Adds widescreen resolution selection, correct menu proportions, native Plugins settings and a configurable frame rate limit.

Extract the release beside the game executable, preserving its folders. The fix uses `scripts/TheGodfather.FusionFix.cfg`. Configure it in the game, on fusionfix.io or in FusionFixHub.

Open **Options → Video → Plugins → The Godfather Fusion Fix**. F1 opens the complete help for the selected option. Escape returns to the same page and selection. Vanilla, Recommended and Enhanced presets retain essential corrections; a restart notice identifies options that need a new game session.

The native Video Resolution list includes all supported 32-bit display modes, sorted by width and height. Duplicate dimensions use the highest supported refresh rate. Select a mode and use the game's own OK button to apply it. A framed window follows the selected startup render size, including on a scaled desktop. Resizing the window by dragging its border changes the render size at once, in menus and in gameplay.

Frame Rate Limit supports 30–1000 FPS, Unlimited (0) and Monitor Refresh Rate (-1). Vertical Synchronization requires a restart and can cap the frame rate at the display refresh rate; turn it off to use a higher limit. Recommended uses 60 FPS with VSync off.

Menus retain a centered 4:3 layout on wider displays and fit inside narrower windows. Character creation keeps the complete character preview visible.

HUD Aspect Ratio places the complete health, cash, weapon and radar groups at the screen edges with Auto, or inside a centered box with 4:3, 16:9, 21:9 or 32:9. Their sizes and proportions stay unchanged, including the 3D weapon picture beside the weapon name. Field of View Factor changes the normal gameplay view immediately; 1.0 is neutral, and larger values widen the view.

Cutscene Borders offers Off, Letterbox, Pillarbox and Both. Both preserves the original framed view; Off fills the screen with a wider view. The change applies immediately during a cinematic.

Movie Framing offers Original Aspect, which shows the whole movie inside the game's centred 4:3 frame, and Fill, which fits the whole movie to the screen. Skip Intro controls the startup logo sequence.

Controller Icons offers Automatic (Xbox), Xbox, PlayStation and Nintendo artwork. Keyboard and mouse keep their own prompts.

Pause When Unfocused stops simulation while the game is inactive. Ctrl+F10 opens or closes the settings overlay at the title screen or during gameplay. Its Info page shows the version, build time, renderer and render size.

## Known limits

- Restart after changing Window Mode.
- On some NVIDIA drivers, resizing the window during gameplay can crash the game inside the driver, most often right after switching weapons. Resizing in the menus is not affected; choose the window size there or in the Video options.
- A small part of the original tab border remains visible on Plugins pages.

The game keeps its native multisampling.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>

