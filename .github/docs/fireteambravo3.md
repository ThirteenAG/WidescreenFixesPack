<div align="center">

<img src="https://thirteenag.github.io/screens/fireteambravo3/main2.jpg" width="760" alt="SOCOM: U.S. Navy SEALs Fireteam Bravo 3 Fusion Fix (PPSSPP)">

**SOCOM: U.S. Navy SEALs Fireteam Bravo 3 Fusion Fix (PPSSPP)** adds widescreen camera and HUD layouts, native settings menus, dual analog controls, controller artwork and a 60 FPS mode.

[Website](https://thirteenag.github.io/wfp#fireteambravo3) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SOCOM.FireteamBravo3.PPSSPP.FusionFix/main.cpp)

</div>

---

## Installation

1. Use PPSSPP with plugin loading enabled.
2. Extract the archive beside PPSSPP's `memstick` folder, preserving its directory structure.
3. Start the game once. The complete configuration is created at `memstick/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionFix/SOCOM.FireteamBravo3.PPSSPP.FusionFix.cfg`.
4. Open **Profile Settings > Settings > Plugins** from the main lobby, or **Options > Plugins** from the pause menu. Select the Fusion Fix, then use Triangle for full help and Circle to go back.

Settings can also be edited in FusionFixHub or the configuration file. Restart the emulated game after editing the file.

If the game menu data is not recognized, the original menu stays available and a notice reports that the Plugins row could not be added.

If a button texture cannot be replaced, the game keeps its original icon. If the Plugins page's button artwork is unavailable, the page does not open and a notice appears.

## Display and camera

- **Force aspect ratio:** Auto follows PPSSPP's display shape. A ratio such as `21:9` overrides it. Gameplay keeps its vertical field of view and gains horizontal view on wider displays.
- **Field of view:** 0.5-2.0, with 1.0 retaining the intended gameplay angle. Changes in Plugins apply when gameplay updates. Aiming, binoculars and scripted cameras keep their own angles.
- **HUD box:** Auto anchors side elements to the screen edges. The 4:3, 16:9, 21:9 and 32:9 choices constrain them to a centered box. Custom ratios are accepted in the configuration. **HUD scale** adjusts element size from 0.5 to 1.5.
- Menus and loading screens retain the PSP canvas proportions. Map content, marker art, subtitles and text backdrops follow their relevant screen or HUD layout.
- **Cutscene borders:** Off, Letterbox, Pillarbox or Both. Both preserves the original framing on narrow and wide displays; Off uses a full-screen view with the normal aspect correction. The game has no original animated cinematic bars, so there is no animation setting.

Movies are shown whole at their own aspect ratio, including subtitles, with black bars at the sides on wider screens. The PSP movies match the PSP screen, so there is no separate movie setting.

## Controls and frame rate

**Dual analog controls** assigns movement to the left stick and camera/aim to the right stick. Xbox-compatible controllers use stock Windows PPSSPP's XInput axes without additional right-stick mapping; explicit PPSSPP right-analog mappings are also supported. Camera inversion and binocular sensitivity follow the game's settings. Reduced camera smoothing requires dual analog controls. Restart after changing either control option.

**Controller icons** offers Auto, Xbox, PlayStation and Nintendo. Auto uses Xbox art because PPSSPP does not tell the game which controller is connected. PlayStation retains the original PSP button art. Separate button textures in the HUD and menus change with this option; the original illustration of PSP hardware remains native. This setting changes artwork, so control bindings are still configured in PPSSPP.

**Frame rate** offers the original 30 FPS limit and a 60 FPS mode. Effects, camera-shake decay and text timing use elapsed time. The Plugins menu switches the limit immediately. **Faster loading** temporarily unthrottles loading, then returns to normal speed.

**Skip intro** skips startup logos and routine notices while retaining story movies and genuine choices. **Skip press start** advances the title screen through the game's normal completion path, preserving profile creation. These options require restarting the emulated game.

## Presets and language

Vanilla keeps the original intros, title screen, controls, PSP icons and 30 FPS. Recommended and Enhanced enable startup skipping, dual analog controls, reduced camera smoothing, faster loading, Xbox icons and 60 FPS. Settings shared by all presets, including your FOV, HUD, aspect and language choices, retain your values when switching presets.

Plugin text is available in English, French, German, Italian and Spanish. Auto follows the emulated PSP's language when supported, otherwise English. Text language, icon style, aspect, HUD, borders, FOV and frame rate changes made through Plugins apply without restarting. The menu shows a restart notice for changes to startup or control options.

## Compatibility

Supported game IDs: UCUS98716, UCES01242, ULES01242, UCKS45128 and NPHG00032.

Resolution, rendering scale, anti-aliasing, window modes, display scaling, focus behavior and host controller support belong to PPSSPP. Configure mouse and keyboard controls there too; button prompts show controller buttons. The PSP has no controller vibration or split-screen mode. Updates are managed through FusionFixHub.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
