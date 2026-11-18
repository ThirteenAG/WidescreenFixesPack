<div align="center">

<img src="https://thirteenag.github.io/screens/gtalcs/main2.jpg" width="760" alt="Grand Theft Auto: Liberty City Stories Fusion Fix (PCSX2F)">

**Grand Theft Auto: Liberty City Stories Fusion Fix (PCSX2F)** corrects widescreen aspect ratio, field of view, HUD and frontend layout, and adds 60 FPS, modern pad controls, keyboard and mouse controls with cheats and Xbox or Nintendo button icons.

[Website](https://thirteenag.github.io/wfp#gtalcs) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTALCS.PCSX2F.FusionFix/main.cpp)

</div>

---

## Fixes

- **Aspect ratio** - follows the emulator's aspect ratio or window size, at any width; Force Aspect Ratio (FusionFixHub) sets a ratio instead.
- **Field of view** - Widens the gameplay view without stretching. Field of View (0.5–2.0) adjusts the gameplay view further without changing cutscenes.
- **HUD** - Keeps HUD elements in proportion at the screen edges. HUD Size changes their size. HUD Aspect Ratio (Auto, 4:3, 16:9, 21:9, 32:9 or a custom ratio in FusionFixHub) keeps the HUD inside a centered area on wide screens; Auto uses the whole screen.
- **Frontend and map** - kept at 4:3 proportions; full-screen artwork is contained with black sides.
- **Cutscenes** - always keep their 16:9 framing at any aspect ratio: nothing cut off, nothing stretched. Cutscene Borders chooses which bars cover the rest: Off, Letterbox (top and bottom, on screens narrower than 16:9), Pillarbox (sides, on wider screens) or Both. With the bars off the cutscene fills the screen. The bars slide in and out.
- **Free-aim reticle and light glows** - stay round at any aspect ratio.
- **Skip intro** - skips the logo movies at startup.
- **Options** - frame rate (`MAIN.FPSLimit` in the cfg; 30 FPS by default), fast loading (unthrottled emulator while loading), blur on/off.
- **60 FPS sound** - at 60 FPS, sound effects and streamed audio keep their original rate.
- **Modern controls** - drive, aim and shoot with the triggers: R2 accelerates and fires, L2 brakes and targets, Cross is the handbrake, the d-pad switches weapons, targets and radio stations, the right stick aims, and L1/R1 look left and right (both or R3 look behind). Help texts name the new buttons. On by default; the Vanilla preset turns it off.
- **Pad camera** - Pad Camera Speed and Invert Pad Y for the right stick camera.
- **Keyboard and mouse** - mouse camera and aim, rebindable actions, key pictures in help text and menu footers, and typed cheats. A pad keeps working next to them: the device used last drives the game and its prompts.
- **Vehicle lights** - lit headlights, tail, brake and reverse lights on car and bike bodies. Vehicle Lights is on by default and switches at once; the Vanilla preset turns it off.
- **Button icons** - Xbox or Nintendo button pictures in help text and menu footers while you play with a pad; Auto uses Xbox buttons; PlayStation keeps the game's own PlayStation 2 buttons. The emulator can't tell which pad you hold, so choose the set yourself.

<details>
<summary>Typed cheats (with keyboard and mouse controls on, type the word while playing)</summary>

| Cheat | Word |
| --- | --- |
| Weapons 1, 2, 3 | THUGSTOOLS, PROFESSIONALTOOLS, NUTTERTOOLS |
| Money | ALWAYSCARRYCASH |
| Armor | PRECIOUSPROTECTION |
| Health | ASPIRINE |
| Wanted level up | YOUWONTTAKEMEALIVE |
| Never wanted | LEAVEMEALONE |
| Weather: sunny, clear, overcast, rain, fog | SKINCANCERFORME, TOODAMNHOT, ILIKESCOTLAND, ILOVESCOTLAND, PEASOUP |
| Rhino | GIVEUSATANK |
| Trashmaster | RUBBISHCAR |
| Destroy all vehicles | BANGBANGBANG |
| Change outfit | ILIKEDRESSINGUP |
| Faster clock | TIMEFLIESWHENYOU |
| Pedestrians attack you | NOBODYLIKESME |
| Pedestrians riot | ITSALLGOINGMAAAD |
| Pedestrians have weapons | WEAPONSFORALL |
| Faster, slower gameplay | ONSPEED, BOOOOORING |
| Perfect traction | CORNERSLIKEMAD |
| Cars drive on water | SEAWAYS |
| Green traffic lights | GREENLIGHT |
| Aggressive traffic | MIAMITRAFFIC |
| Suicide | ICANTTAKEITANYMORE |

</details>

## Installation

1. Download the fix archive from this release.
2. Use [PCSX2F](https://github.com/ASI-Factory/PCSX2-Fork-With-Plugins/releases/tag/latest) (PCSX2 Fork With Plugins; the fix does not work in regular PCSX2).
3. Extract the plugin archive into the emulator folder, preserving its `PLUGINS` directory.
4. Set the emulator’s aspect ratio or window size, then launch a supported version of the game.
5. If an older `PLUGINS/GTALCS.PCSX2F.WidescreenFix.elf` or `PLUGINS/GTALCS.PCSX2F.ImVehLM.elf` is still there, delete it. Both are part of this Fusion Fix now; while one of them is installed, the Fusion Fix stays off and says so on screen.

## Useful information

One version of the game is compatible, **SLUS-21423** (crc: **7EA439F5**).

## Settings

The settings are stored in `PLUGINS/GTALCS.PCSX2F.FusionFix.cfg`, which the fix creates on the first start. Change them in FusionFixHub, on fusionfix.io, or in the game: the Display menu has one **Plugins** entry that opens this fix's page. It has its preset (Vanilla, Recommended, Enhanced), frame rate, field of view, HUD size and aspect ratio, cutscene borders, blur, fast loading, skip intro, vehicle lights, button icons, a Controls page (modern controls, pad camera speed, invert pad Y, keyboard and mouse controls with bindings), and Restore Defaults. A line under the menu describes the selected row. Menu changes apply at once and are saved when the menu closes; Skip Intro applies the next time the game starts, and the menu says so. Changes made on fusionfix.io or in FusionFixHub apply the next time the emulated game starts, or when a save state is loaded.

- **Presets** - Vanilla keeps the original game (cutscene letterbox, blur, intros, normal loading, no vehicle lights, original controls, 30 FPS, PlayStation icons); Recommended is the default (modern controls, 30 FPS, Auto icons), and Enhanced adds 60 FPS. Applying a preset changes only the settings that differ between presets. Field of view, HUD, pad camera, keyboard and mouse controls, mouse sensitivity and bindings keep your choice.
- **Save states** - after loading a state, the fix reads its settings file again, so your current settings apply instead of the ones saved in the state. Skip intro keeps the value the game started with.

## Not covered

- **Resolution, anti-aliasing, window modes and Alt+Tab** - handled by the emulator.
- **Videos** - the only videos are the four logo movies at startup, which Skip Intro skips. They are decoded straight to the screen; PCSX2F's own FMV aspect ratio setting shows them at 4:3.
- **Controller layout picture** - the Controls page keeps the game's PlayStation 2 controller picture.

# Project2DFX

[Project2DFX](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/tag/gtalcsps2) is available as a separate plugin.

## Presets and FusionFixHub

- FusionFixHub text: English, French, German, Italian and Spanish.
- The added in-game settings menu is in English only.
- Blur patch failure notices support English, French, German, Italian and Spanish through Text Language; Automatic uses English.
