<div align="center">

**Mafia III: Definitive Edition Fusion Fix** removes the stutter of high polling rate mice, adds a borderless window for the game's windowed mode and starts straight at the main menu.

[Website](https://thirteenag.github.io/wfp#mafia3de) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Mafia3DefinitiveEdition.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **High polling rate mice** — with a 4000 or 8000 Hz mouse the game hitched while the mouse moved. The fix reads the mouse in batches instead of report by report and stops the game's message loop from stalling on mouse moves. Mouse look keeps exactly the same movement and sensitivity. Always on.

## Options

- **Borderless window** — On by default: with FULLSCREEN off in the game's display options, the game runs in a borderless window covering its monitor at the desktop resolution, sharp on monitors with display scaling. Off shows the game's framed window. FULLSCREEN on is unchanged. Applies at once.
- **Skip intro** — On by default: the game starts straight at the main menu, without the middleware and studio logos and the save warning screen. Off shows them as in the original game. Needs a restart.

---

## Settings and hub

The fix creates `plugins/Mafia3DefinitiveEdition.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it. Vanilla shows the intro screens; Recommended and Enhanced skip them. The window setting remains your choice when switching presets. Texts are available in English, French, German, Italian and Spanish. The text language can be selected in the settings editor.

## Known limits

- The game already has widescreen, fullscreen, a resolution list, a field of view slider and controller support, so the fix adds nothing there.
- While the borderless window is in use the game renders at the desktop resolution, whatever resolution the options show.
- The fix has no page in the game's own settings menu: change it in FusionFixHub or on fusionfix.io.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside `mafia3definitiveedition.exe`, preserving the folders in the archive.
3. Launch the game as usual.
