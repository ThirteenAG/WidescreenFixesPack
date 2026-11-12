<div align="center">

<img src="https://thirteenag.github.io/screens/re5/main2.jpg" width="760" alt="Resident Evil 5 Fusion Fix">

**Resident Evil 5 Fusion Fix** puts the HUD at the edges of wide screens, fixes camera near clipping in co-op and the menus' side panels, and adds modern controls, a knife parry, a colour filter switch, a field of view setting, intro skipping, window modes, play in the background and Logitech RGB lighting.

[Website](https://thirteenag.github.io/wfp#re5) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/ResidentEvil5.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Uroboros Mkono** — In Chapter 5-2, the weak spots open after the same flamethrower time at higher frame rates as at 30 FPS.
- **Co-op camera** — Correct near clipping that lets the camera see through walls.
- **Widescreen HUD** — On screens wider than 16:9 the game keeps its whole HUD in a centred 16:9 box. The fix moves the life gauges, ammo, partner status, action buttons, prompts (boxes and their text together) and the saving notice to the screen's edges, keeping their size. The inventory, map and menus stay centred as the game draws them; markers placed over characters and items stay on them.
- **Menu side panels** — The black panels beside the 16:9 menus covered only part of the sides at most resolutions; they now cover them fully.
- **Opening movie** — On screens between 16:9 and 2.35:1 (e.g. 21:9) the game enlarged the 2.35:1 opening movie past the screen's sides; it now shows the whole picture.

## Options

- **Field of view** — Adjusts the gameplay view; aiming and cutscenes keep their original framing.
- **HUD aspect ratio** — Auto spreads the HUD across the whole screen; 4:3, 16:9, 21:9 or 32:9 keep it in a centred box of that shape.
- **Frame rate limit** — 60 FPS by default, Unlimited, monitor refresh rate or a chosen rate. This works alongside the game’s own frame rate option.
- **Settings language** — Choose the Plugins-page language in FusionFixHub or the settings file; Automatic follows the game. Reopen the page to apply it.
- **Confine cursor** — On or Off; applies while playing.
- **Pause when unfocused** — Off keeps the game running while another window is active (windowed and borderless); it still ignores keys, mouse and pad meant for other windows.
- **Check for updates** — Off by default; when on, a newer version is announced in the Plugins list and FusionFixHub opens after the game exits.
- **Skip intro** — Skip the logos and the logo movie at start-up, confirm the start-up notices that only have an OK button ("Load successful", the auto-save notice) and go past the PRESS ANY KEY screen; story movies stay.
- **Window mode** — Windowed or borderless when the game's full screen option is off.
- **RGB lighting** — Support [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html).
- **Colour filter** — Off removes the yellow-orange colour grade of the levels; the game's own Special Settings filters still work.
- **Modern controls** — Controls like Resident Evil 6, for pad and keyboard/mouse:
  - A free over-the-shoulder camera: the right stick or the mouse turns it around the character and it stays where you leave it. It uses the game's own camera rig and wall handling; the mouse turns the view 1:1 in the same frame (the game's camera smoothing stays for the pad).
  - Movement relative to the camera; walking, running and stopping are the game's own.
  - Aiming turns the character to where the camera looks; the crosshair/laser is the game's.
  - Walking while aiming and shooting, with the knife out, while reloading and while switching weapons: the legs play the weapon's own walk clips while the upper body keeps the aim, shot, reload or switch. Reloading or switching outside the aim, the character also turns with the stick and runs (the weapon's run clip) when the run button is held, and the camera stays free.
  - One-button reload on the move: X on the pad when no X prompt is up (the keyboard's reload key already works outside the aim).
  - Quick turn: A with the stick toward the camera (keyboard: the quick turn key); the camera swings behind.
  - Dodges: while aiming, A with the stick to a side or back (keyboard: the quick turn key with a direction) plays the game's avoid actions (the dodges of the QTEs): a side roll or a jump back.
- **Laser sight with keyboard/mouse** — Keyboard/mouse aiming looks exactly like the pad's: the weapon's laser sight shows and the crosshair is hidden (the sniper scope keeps its reticle).
- **Turn speed while moving** — How fast the character turns while walking and running, with the mouse or a stick. The game turns much slower on the move than standing still, slowest while running; 1.0x keeps the game's speed, higher values turn faster. With modern controls the camera turns freely, so this doesn't apply there.
- **Knife parry** — Raise or swing the knife just before a Majini's melee attack or grab lands to block it: no damage (a grab doesn't latch on), the character plays the game's guard reaction (or keeps swinging) and the enemy reels as from a head shot, which opens the usual melee prompt for a follow-up.

---

## Installation

1. Download the fix archive from this release.
2. Extract the archive into the game folder, beside the game executable, preserving the folders in the archive.
3. Launch the game once to create `scripts/ResidentEvil5.FusionFix.cfg`.
4. Configure the fix in the game's Options (GAME SETTINGS and PLUGINS), on fusionfix.io or in FusionFixHub. Restart the game when an option requires it.

## Settings

The game's Options menu (at the title screen and in the pause menu) has the fix's settings: **GAME SETTINGS** gets
Field of view, Modern controls, Laser sight with keyboard/mouse and Knife parry under the game's own rows (its Restore
to default resets them too), and the added **PLUGINS** row lists the fix and every other loaded plugin with settings.
Each plugin's page has its preset, its settings and Restore defaults for that page. FusionFixHub and fusionfix.io change
the same settings; the first launch creates `scripts/ResidentEvil5.FusionFix.cfg`. **Vanilla** shows the logos; **Recommended** skips them; **Enhanced** also turns off the colour filter. Modern controls, laser sight, turn speed while moving and knife parry remain player choices. The field of view, HUD aspect ratio, frame rate limit, cursor confinement, pause when unfocused, colour filter, modern
controls, laser sight, turn speed while moving, knife parry and update check apply at once. Reopen the Plugins page after changing its language;
window mode, intro skipping and RGB lighting apply after a restart. The window mode and update check are the player's choice. Ctrl+F10 opens the developer overlay (supporters).

## Known limits

- The game is already Hor+ at any aspect.
- Movies keep their proportions as the game fits them (the 16:9 logo movies with black bars at the sides on wider
  screens; the 2.35:1 opening movie with its baked bars zoomed past), so there is no movie setting.
- Cutscenes keep the game's own 2.35:1 frame: black bars above and below on screens narrower than 2.35:1, at the sides on
  wider ones. There is no setting to remove the bars: the game renders the cutscene into that frame, and widening it
  would stretch the picture.
- The chapter title cards and the PAUSED label stay where the game places them in the 16:9 area.
- The fix doesn't force a window mode: **Window mode** applies when the game's own full screen option is off.
- Split screen stays top and bottom: the per-player HUD layout is computed for those halves in many places, and the
  HUD keeps the game's layout there.
- The controls and controller icons are the game's own (tutorial texts still describe the original controls).
- Modern controls: the camera and the legs are local. In online co-op the partner's game sees the character's actions,
  position and facing as usual; walking while aiming, reloading or switching is off online (the partner's game would
  not move the character), so these stand still there as in the original game. RE6's quick shot is not added: RE5 has
  no unaimed shot animation. Dodges use the QTE dodge animations (side steps and a back step), not RE6's rolls.
- Walking while aiming uses the forward, back and strafe clips of the weapon in hand; diagonals play the nearest one,
  turned toward the real direction. There are no footstep effects for these steps.
- Knife parry: only the Majini family's melee attacks (punches, kicks, weapon swings, shield and spear attacks, Plaga
  whips) and grab attempts; a grab that already holds the character, kill moves, knock-downs, fire and explosions, bosses and other enemies hit as usual. Off in online
  sessions (the enemy's reaction belongs to the machine that owns it). No extra spark effect: the feedback is the
  guard reaction, the enemy's flinch and pad rumble.
- The camera's up/down follows the game's camera invert option; the up/down range is the game's own look-up and
  look-down camera poses.

<p align="center"> <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-wfp.svg"></picture></a> <br /> <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a> <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a> <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a> <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a> <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br> </p>
