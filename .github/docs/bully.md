<div align="center">

<img src="https://thirteenag.github.io/screens/bully/main2.jpg" width="760" alt="Bully: Scholarship Edition Fusion Fix">

**Bully: Scholarship Edition Fusion Fix** adds Hor+ widescreen with a field of view option, keeps the HUD at the screen edges, frames cutscenes, and adds window modes, raw mouse input, modern controller support and in-game settings.

[Website](https://thirteenag.github.io/wfp#bully) · [Source Code](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Bully.FusionFix/dllmain.cpp)

</div>

---

## Fixes

- **Field of view** — Hor+: the picture keeps the camera height and vertical framing of the original 4:3 game and shows more at the sides. The game's own widescreen mode moved the follow camera lower; the fix keeps it where 4:3 has it. The FOV multiplier affects normal on-foot and vehicle views and stops at 150 degrees across; first-person, cutscene, conversation and shop cameras keep their original framing.
- **HUD** — Auto uses the whole screen width; a HUD aspect ratio moves the side elements into a centered box. Centered text, character health bars and the full map keep their place.
- **Cutscenes** — Cutscenes keep the height of the original letterboxed 4:3 picture; screens up to 16:9 show more at the sides, wider screens get side borders. Off removes the borders and widens the view to fill the screen. Normal gameplay framing returns after the cutscene.
- **Videos** — the 16:9 videos are shown whole at their own aspect, with black bars where the screen is wider or narrower.
- **Resolution** — ultrawide resolutions are supported and kept between launches. The Video menu lists the display modes your adapter reports; it has room for 128 entries, and if there are more it keeps 640×480, your desktop and current sizes and the largest modes.
- **Window modes** — fullscreen, a framed window or desktop borderless. Alt+Enter switches between fullscreen and the last window mode and remembers the framed position. Switching to another application leaves the game window open.
- **Cursor** — Confine Cursor keeps the cursor inside the active game window and lets it go when you switch applications or move the window.
- **Focus** — Pause When Unfocused (on by default) pauses the game while another window is active. Turned off, the game keeps running in the background and ignores controls while another application has focus.
- **Mouse** — Raw Mouse Input uses raw relative mouse movement and removes the game's horizontal movement bias; the game's sensitivity and bindings stay.
- **Controller** — XInput controllers connected during play are picked up; bindings and button prompts follow your latest input, and disconnecting returns to keyboard and mouse. Gamepad Deadzone (15% by default) and Gamepad Camera Speed apply immediately. The game's invert-Y and vibration options stay. Gamepad Icons selects Xbox, PlayStation or Nintendo pictures; Auto uses Xbox.
- **Shadows** — Shadow Resolution sets the shadow map size: 1024 (the original), 2048 or 4096. Higher sizes give sharper shadows and use more video memory. It applies at the next start and needs Shadows turned on in the game's Video menu.
- **Skip intro** — skips the Rockstar and Mad Doc logos; story and attract-mode movies stay.
- **Updates** — Check for Updates is off by default. When enabled, a notice appears in the game's menus if a newer fix is available. If FusionFixHub is installed beside the game, it opens after the game closes; otherwise the notice points to fusionfix.io.

## Options

Open **Display Settings → Plugins** from the title menu, or **Options → Display → Plugins** while paused. It lists this fix first, then other loaded plugins with settings; each has its own preset and Restore Defaults, applied with confirmation. Keyboard, controller and mouse navigation work. F1, the Help button or Y on an Xbox controller (Triangle on PlayStation, X on Nintendo) opens the full option help; Up and Down scroll it. The settings can also be changed on fusionfix.io and in FusionFixHub, also while the game runs.

| Setting | Values | Default |
|---|---|---|
| Field of view | 0.5–2.0, live | 1.0 |
| HUD aspect ratio | Auto (whole screen width), 4:3, 16:9, 21:9, 32:9 or any width:height, live | Auto |
| Cutscene borders | Off, Letterbox, Pillarbox, Both, live | Both |
| Animated cutscene borders | On or off | On |
| Window mode | Fullscreen, Windowed, Borderless (restart; Alt+Enter while playing) | Fullscreen |
| Shadow resolution | 1024, 2048, 4096 (restart) | 1024 |
| Skip intro | On or off (restart) | On |
| Raw mouse input | On or off | On |
| Gamepad camera speed | 0.1–5.0 | 1.0 |
| Gamepad deadzone | 0–50% | 15% |
| Gamepad icons | Auto (Xbox), Xbox, PlayStation, Nintendo | Auto |
| Pause when unfocused | On or off | On |
| Confine cursor | On or off | On |
| Check for updates | On or off | Off |

## Known limits

- The game keeps its 30 FPS limit.

---

## Installation

1. Extract the archive into the game folder, beside `Bully.exe`, keeping its folders. Keep `plugins/Bully.FusionFix/buttons.nft` with the fix: it holds the button artwork.
2. The fix creates `plugins/Bully.FusionFix.cfg` on the first start.
3. Change the settings in the game's Plugins menu, on fusionfix.io or in FusionFixHub.
