<div align="center">

**Mafia II: Definitive Edition Fusion Fix** removes the lag of high polling rate mice and can start the game straight at the main menu.

[Website](https://thirteenag.github.io/wfp#mafia2de) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Mafia2DefinitiveEdition.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **High polling rate mice** — with a 4000 or 8000 Hz mouse the frame rate dropped and the camera lagged while the mouse moved, because Windows also turned every mouse report into window messages for the game, which it doesn't use. While the game window is in front and the cursor is over it, those messages are switched off; the game's own mouse input stays complete, so mouse look keeps exactly the same movement and sensitivity. The title bar of a window and other programs keep working as usual. Always on.

## Options

- **Skip intro** — On by default: the game starts at the main menu, past the 2K, Hangar 13 and d3t logo videos, the legal screen and "Press any key or button to continue". Off shows them as in the original game. Needs a restart.

---

## Settings and hub

The fix creates `pc/scripts/Mafia2DefinitiveEdition.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it. Vanilla shows the intro; Recommended and Enhanced skip it. Texts are available in English, French, German, Italian and Spanish. The text language can be selected in the settings editor.

## Known limits

- The game already has widescreen, its own window modes, resolution list, frame rate limit and controller support, so the fix adds nothing there.
- The first loading screen stays on screen as long as the game needs to load; with Skip intro it is a plain loading screen instead of the legal notices.
- A 2K account prompt for a game without a linked account was not seen while testing; if it appears, it is shown as usual.
- The fix has no page in the game's own settings menu: change it in FusionFixHub or on fusionfix.io.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder (the one with the `pc` folder), preserving the folders in the archive.
3. Launch the game as usual.
