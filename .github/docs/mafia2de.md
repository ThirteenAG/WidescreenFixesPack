<div align="center">

**Mafia II: Definitive Edition Fusion Fix** removes the lag of high polling rate mice and can start the game straight at the main menu.

[Website](https://thirteenag.github.io/wfp#mafia2de) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Mafia2DefinitiveEdition.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **High polling rate mice** — the fix moves the game's DirectInput mouse registration to a separate thread that reads raw reports in batches. While the game window is in front and the cursor is over its client area, the fix also stops Windows from generating legacy mouse messages for those reports. Mouse look keeps the same movement and sensitivity. The window's title bar and other programs keep their usual mouse behaviour. Always on.

## Options

- **Skip intro** — On by default: the game starts at the main menu, past the 2K, Hangar 13 and d3t logo videos, the legal screen and "Press any key or button to continue". Off shows them as in the original game. Needs a restart.

---

## Settings and hub

The fix creates `pc/scripts/Mafia2DefinitiveEdition.FusionFix.cfg` on first start. You can change it in the standalone FusionFixHub settings editor. Vanilla shows the intro; Recommended and Enhanced skip it. Texts are available in English, French, German, Italian and Spanish. The text language can be selected in the settings editor.

## Known limits

- The game already has widescreen, its own window modes, resolution list, frame rate limit and controller support, so the fix adds nothing there.
- The first loading screen stays on screen as long as the game needs to load; with Skip intro it is a plain loading screen instead of the legal notices.
- A 2K account prompt for a game without a linked account was not seen while testing; if it appears, it is shown as usual.
- The fix has no page in the game's own settings menu: change its options in FusionFixHub.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder (the one with the `pc` folder), preserving the folders in the archive.
3. Launch the game as usual.
