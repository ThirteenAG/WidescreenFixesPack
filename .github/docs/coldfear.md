<div align="center">



<img src="https://thirteenag.github.io/screens/coldfear/main2.jpg" width="760" alt="Cold Fear Fusion Fix">



**Cold Fear Fusion Fix** corrects resolution handling, HUD scaling, field of view and video scaling for widescreen displays, for the retail and the GOG version.



[Website](https://thirteenag.github.io/wfp#coldfear) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ColdFear.FusionFix/dllmain.cpp)



</div>



---



## Fixes



- **Resolution** — The game's own Screen Configuration offers every resolution of the display instead of its five (retail 640x480 to 1600x1200, GOG 1280x720 to 3840x2160). The choice is kept in the game's `res.bin` and applies after a restart, as in the game; without the fix the game falls back to its default resolution.

- **HUD** — Correct HUD scaling for widescreen, with the health bars and the weapon display at the screen edges.

- **Field of view** — Widens with the screen (Hor+) at any aspect ratio; the shadow camera keeps its own, so character shadows stay in place.

- **Videos** — Correct FMV scaling for widescreen.

- **GOG version** — Works with GOG's version, whose own widescreen patch (a fixed 16:9 field of view and a stretched HUD canvas) the fix replaces.

- **Stability (retail version)** — The fixes of GOG's version for the retail game: Alt+Tab without pausing and resetting the game, the cursor kept in the game window, the window shown at once, frame rates above 120 fps on high refresh rate displays, an invalid blend state modern drivers reject, two crashes on missing objects, an uninitialized constructor, wrong members used in a few places, shadows of objects outside the view, the window's messages handled while videos play, and the brightness and contrast bars of the Screen Configuration showing their values.



## Options



- **In-game options** — The settings are in a new **Plugins** row in Settings; the game's own option screens are unchanged (the game has no options in its pause menu, and the Screen Configuration page is a fixed grid with no room for more rows). The row first lists the plugins, with this fix first, even when it is the only plugin installed. A plugin's page starts with its **Preset** (Vanilla, Recommended, Enhanced: left and right pick one, Enter twice applies it), then its settings and **Restore Defaults** (Enter twice); the row at the top switches between the parts of a long page. Left and right change the row selected, Enter too; the help of the row selected shows below. Most settings apply at once; after changing one that needs a restart, the help line says so. All settings are stored in `scripts/ColdFear.FusionFix.cfg`, which the fix writes with every setting and a comment above each; settings can also be changed on fusionfix.io or in FusionFixHub, which changes them in the running game too. The Plugins list supports multiple pages. Press **F1** on a selected setting to read its full help; use the top row for further help pages and Back to return.

- **Field of view** — A multiplier on top of the widescreen (Hor+) field of view.

- **HUD** — Keep the HUD at the screen edges or in a centred box such as 4:3, 16:9 or 21:9.

- **Cutscene borders** — Off, Letterbox (the black bars above and below of in-engine cutscenes), Pillarbox (black bars at the sides, the cutscenes at their original 4:3) or Both.

- **Videos** — Original keeps the videos' 4:3 picture; Fill enlarges the story movies past their black bars until the wide picture fits the screen, never cutting the picture.

- **Window mode** — Fullscreen, a framed window at the chosen resolution, or borderless at the desktop resolution.

- **Menu language** — Select English, French, German, Italian or Spanish in FusionFixHub. Automatic uses English; reopen Plugins after changing the language.

- **Resolution override** — A width and height set in FusionFixHub or on fusionfix.io override the game's choice.



---



**Window Mode** offers Fullscreen, Windowed and Borderless. Borderless uses the desktop resolution; a framed window keeps its size at high display scaling. **Skip Intro** skips the publisher, Darkworks and RenderWare logos; the story movies stay.



**Confine cursor** keeps the pointer inside the game window. It can be changed while playing and is enabled by default. The pointer is released when you leave the game, drag its window, open a system menu or turn the option off.



**Frame Rate Limit** caps the frame rate at the monitor's refresh rate, at a number of your choice, or not at all. **Pause in Background** pauses the game while another window is active and resumes it when you return; a pause you started yourself stays. **Check for Updates** shows a notice in the main menu when a newer version is out. **Ctrl+F10** opens the settings overlay.



**Skip Press Start** goes straight from the title prompt to the main menu; the language and difficulty choices stay. Fast mouse movements are no longer lost. Controllers: the retail version uses updated default control mappings and reads Xbox-compatible controllers through the included Xidi; bind the buttons in the game's Controls page. The GOG version keeps its own controller support.



## Known limits



- Resolution and window-mode changes require a restart.

- The PC interface uses its original button labels and has no controller icon-family selector.

- Raising the frame limit does not repair every possible timing issue.



---



## Installation



1. Download the fix archive from this release.

2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.

3. Launch the game once to create `scripts/ColdFear.FusionFix.cfg`.

4. Optionally change the settings in the game's Settings > Plugins, in FusionFixHub, or on fusionfix.io. Restart the game when an option requires it.



> [!NOTE]

> The archive's ASI loader is `winmm.dll`, for both versions: GOG's version ships its own `dinput8.dll` (GOG's controller support), which must stay. Earlier versions of this fix installed the loader as `dinput8.dll`: on the retail version, delete that `dinput8.dll` when updating. Don't copy a `dinput8.dll` into the GOG version.







<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>

