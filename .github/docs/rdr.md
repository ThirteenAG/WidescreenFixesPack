<div align="center">

<img src="https://thirteenag.github.io/screens/rdr/main2.jpg" width="760" alt="Red Dead Redemption Fusion Fix">

**Red Dead Redemption Fusion Fix** skips startup intros and legal screens.

[Website](https://thirteenag.github.io/wfp#rdr) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/RedDeadRedemption.FusionFix/dllmain.cpp)



</div>

---

## Settings

FusionFixHub and fusionfix.io change the setting; the first launch creates `plugins/RedDeadRedemption.FusionFix.cfg`.
**Skip intro** (on by default) skips the logos and legal screens at start-up and applies after a
restart. Presets: Vanilla shows them, Recommended and Enhanced skip them. An old `plugins/RedDeadRedemption.FusionFix.ini` isn't used anymore and can be deleted.

## Known limits

- The fix adds only startup-screen skipping. It does not add FOV, HUD-width, movie-framing, cutscene-border or raw-mouse controls.
- Ultrawide cutscene framing and high-polling mouse behavior have not been checked by this fix.
- Update notices and an in-game restart warning are not supported.

- The PC port is native widescreen with its own window modes, resolution list, anti-aliasing (DLSS), frame rate
  options and controller support. These remain controlled by the game.
- No in-game settings menu.

## Installation

Extract the release archive into the game directory, preserving folders, then launch `RDR.exe`. `PlayRDR.exe` is a launcher and `RDRMessage.exe` is a companion tool.
