<div align="center">

<img src="https://thirteenag.github.io/screens/da4/main2.jpg" width="760" alt="Deer Avenger 4 Widescreen Fix">

**Deer Avenger 4 Widescreen Fix** corrects the main camera aspect ratio for widescreen displays.

[Website](https://thirteenag.github.io/wfp#da4) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/DeerAvenger4.WidescreenFix/dllmain.cpp)

</div>

---

## Fixes

- **Aspect ratio** - Correct widescreen aspect ratio scaling.

## Options

- **Aspect correction** - Widen the main gameplay view for the selected resolution. Restart after changing this option.
- **Gameplay Field of View** - Use 1.0 for the intended view, a larger value for a wider view or a smaller value for a narrower view. Changes apply immediately during gameplay. Aiming, scripted views and the hub keep their own framing.
- **HUD proportions** - Keep the main gameplay HUD proportional. Restart after changing this option.
- **HUD width** - Keep side-anchored HUD elements within the chosen screen shape. Automatic uses the full screen width. Requires HUD proportions and applies immediately.
- **Window Mode** - Choose Fullscreen, a fixed-size Windowed mode or Borderless, then restart. Windowed modes require support from the selected graphics adapter. Oversized windowed resolutions are reduced to keep the full window and title bar inside the desktop work area.
- **Skip Intro** - Skip startup movies while retaining later movies.
- **Video Scaling** - Choose Original aspect with black bars, cropped Fill (the default) or Stretch. This can be changed while playing.

## Known limits

The correction applies to the main gameplay camera. The hub, menus, exhibition previews and scope overlay keep their original layout. Some native rendering issues remain visible in widescreen. Accents in added menu text are simplified to match the game font. If video cropping is unavailable, playback uses the original fitted view.

## Configuration

The launcher button and settings window title read "Plugins" in every language. The added settings text is available in English, French, German, Italian and Spanish. Automatic uses English; select Text Language in FusionFixHub to use another language.

Recommended uses the default setup. Vanilla and Enhanced offer alternative presets. Settings marked for restart take effect on the next launch. If Plugins is unavailable in your game version, use FusionFixHub or the generated `.cfg` file.

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch once to create `scripts/DeerAvenger4.WidescreenFix.cfg`, then configure the fix in FusionFixHub or on fusionfix.io.
