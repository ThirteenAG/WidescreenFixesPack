<div align="center">

**Mafia: Definitive Edition Fusion Fix** removes the stutter of high polling rate mice, makes the menu cursor follow the mouse directly and can skip the startup screens.

[Website](https://thirteenag.github.io/wfp#mafiade) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/MafiaDefinitiveEdition.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **High polling rate mice** — with a 4000 or 8000 Hz mouse the game stuttered heavily while the mouse moved, with single frames taking half a second. The game's message loop now handles the mouse moves of one frame at once instead of chasing every new one. Mouse look keeps exactly the same movement and sensitivity. Always on.
- **Menu cursor** — the game moved its menu cursor by the raw mouse movement through its own speed curve, and counted every move twice, so the cursor never matched the Windows pointer and felt sluggish on slow moves and too fast on quick ones. In menus, the pause menu and the map, the cursor now sits exactly where the Windows pointer is, with the pointer speed and acceleration set in Windows; hovering, clicking and dragging the map follow it. Controller cursor movement is unchanged. Always on.

## Options

- **Skip intro** — On by default: the game goes straight to the main menu, past the middleware logos, the disclaimer page, the 2K and Hangar 13 logo movie and the "Press Enter to start" screen. Signing in to the profile and loading still happen as before. Off shows the screens as in the original game. Needs a restart.

---

## Settings and hub

The fix creates `plugins/MafiaDefinitiveEdition.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it. Vanilla shows the startup screens; Recommended and Enhanced skip them. Texts are available in English, French, German, Italian and Spanish. The text language can be selected in the settings editor.

## Known limits

- The game already has widescreen, its own window modes, resolution list, field of view slider and controller support, so the fix adds nothing there.
- First-time settings and 2K account screens that need a choice are not skipped.
- The fix has no page in the game's own settings menu: change it in FusionFixHub or on fusionfix.io.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside `mafiadefinitiveedition.exe`, preserving the folders in the archive.
3. Launch the game as usual.
