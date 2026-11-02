<div align="center">

<img src="https://thirteenag.github.io/screens/rdr/main2.jpg" width="760" alt="Red Dead Redemption Fusion Fix">

**Red Dead Redemption Fusion Fix** skips the startup logos.

[Website](https://thirteenag.github.io/wfp#rdr) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/RedDeadRedemption.FusionFix/dllmain.cpp)

</div>

---

## Options

- **Skip intro** — On by default: the game starts at the title screen, past the Rockstar Games and Double Eleven logos. Off shows them as in the original game. Needs a restart.

---

## Settings and hub

The fix creates `plugins/RedDeadRedemption.FusionFix.cfg` on first start; FusionFixHub and fusionfix.io can change it. Vanilla shows the logos; Recommended and Enhanced skip them. Texts are available in English, French, German, Italian and Spanish.

## Known limits

- The PC version already has widescreen, its own window modes, resolution list, anti-aliasing, frame rate options and controller support, so the fix adds nothing there.
- The "Precompiling shaders" screen still appears when the game needs it; it is real work, not an intro.
- The fix has no page in the game's own settings menu: change it in FusionFixHub or on fusionfix.io.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside `RDR.exe`, preserving the folders in the archive.
3. Launch the game as usual. `PlayRDR.exe` is a launcher and `RDRMessage.exe` is a companion tool; neither needs the fix.
