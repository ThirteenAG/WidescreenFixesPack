module;

#include <stdafx.h>

export module WidescreenFix;

import ComVars;

SafetyHookInline shSetupMultiMon = {};
void __fastcall SetupMultiMon(uintptr_t device, void* edx)
{
    shSetupMultiMon.fastcall<void>(device, edx);

    auto width = *reinterpret_cast<int32_t*>(device + 0xD4);
    auto height = *reinterpret_cast<int32_t*>(device + 0xD8);
    if (width <= 0 || height <= 0)
        return;

    auto aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    if (std::abs(aspectRatio - fDefaultAspectRatio) < 0.01f)
        return;

    auto hudWidth = width;
    auto hudHeight = height;
    if (aspectRatio > fDefaultAspectRatio)
        hudWidth = static_cast<int32_t>(std::lround(height * fDefaultAspectRatio));
    else
        hudHeight = static_cast<int32_t>(std::lround(width / fDefaultAspectRatio));

    *reinterpret_cast<int32_t*>(device + 0x110) = 1;
    *reinterpret_cast<int32_t*>(device + 0x114) = (width - hudWidth) / 2;
    *reinterpret_cast<int32_t*>(device + 0x118) = (height - hudHeight) / 2;
    *reinterpret_cast<int32_t*>(device + 0x11C) = hudWidth;
    *reinterpret_cast<int32_t*>(device + 0x120) = hudHeight;
}

struct FlashString // FString, only read by the setters
{
    const char* data;
    int32_t count;
    int32_t max;
    int32_t pad;
};

SafetyHookInline shHealthFeedbackDraw = {};
void __fastcall HealthFeedbackDraw(uintptr_t healthFeedback, void* edx, float deltaTime)
{
    shHealthFeedbackDraw.fastcall<void>(healthFeedback, edx, deltaTime);

    auto ratio = GetAspectRatio() / fDefaultAspectRatio;
    if (ratio <= 1.0f)
        return;

    // same scale the game passes to SetHealthFeedback
    auto health = *reinterpret_cast<float*>(healthFeedback + 0x64);
    auto scale = health * 50.0f + 100.0f;

    static constexpr char path[] = "_root.HealthFeedback._xscale";
    FlashString str = { path, sizeof(path), sizeof(path), 0 };
    auto vtable = *reinterpret_cast<uintptr_t**>(healthFeedback);
    reinterpret_cast<void(__thiscall*)(uintptr_t, FlashString*, float)>(vtable[108 / 4])(healthFeedback, &str, scale * ratio);
}

// GFxValue, number type
struct FlashValue
{
    int32_t type = 3;
    int32_t pad = 0;
    double value = 0.0;
};

// GFxMovieView::SetVariable
bool SetFlashNumber(uintptr_t movieView, const char* path, double value)
{
    FlashValue flashValue = { .value = value };
    auto setVariable = reinterpret_cast<bool(__thiscall*)(uintptr_t, const char*, const FlashValue*, int32_t)>((*reinterpret_cast<uintptr_t**>(movieView))[44 / 4]);
    return setVariable(movieView, path, &flashValue, 1);
}

// Sticky camera PiP Flash frame (FlashHudPiP): the game sets _root.PiP_Bar._x = progress * 356 while it slides, progress goes from 0 (hidden)
// to 1 (shown) and the bar's graphics sit left of its origin, so hidden is just left of the 16:9 stage and nothing positions it before the first slide.
// Hide and slide it from the left screen edge instead.
float PiPBarProgress = 0.0f;
float GetPiPBarX(float progress)
{
    auto extra = 640.0f * (GetAspectRatio() / fDefaultAspectRatio - 1.0f);
    return progress * (356.0f + extra) - extra;
}

// Centered 16:9 rect of the backbuffer (x, y, w, h)
std::tuple<int32_t, int32_t, int32_t, int32_t> GetHudRect()
{
    auto width = BackBufferWidth;
    auto height = BackBufferHeight;
    if (GetAspectRatio() > fDefaultAspectRatio)
    {
        auto hudWidth = static_cast<int32_t>(std::lround(height * fDefaultAspectRatio));
        return { (width - hudWidth) / 2, 0, hudWidth, height };
    }
    auto hudHeight = static_cast<int32_t>(std::lround(width / fDefaultAspectRatio));
    return { 0, (height - hudHeight) / 2, width, hudHeight };
}

// UEPLGameplayManager sticky camera / camera remote PiP: the game lays it out in screen pixels (7.5% / 7.2% offset, 20% size)
// and the multi-monitor mode then squeezes it into the HUD rect, clipping it at the rect edge while it slides in from x = -width.
// Lay it out inside the HUD rect instead and keep it in screen space, so it slides in from the screen edge together with its Flash frame.
void LayoutPiP(uintptr_t manager, bool shown)
{
    if (!BackBufferWidth || !BackBufferHeight || std::abs(GetAspectRatio() - fDefaultAspectRatio) < 0.01f)
        return;

    auto [rectX, rectY, rectWidth, rectHeight] = GetHudRect();
    auto x0 = std::clamp(static_cast<int32_t>(rectWidth * 0.075f), 0, rectWidth);
    auto x1 = std::clamp(static_cast<int32_t>(rectWidth * 0.2f + x0), 0, rectWidth);
    auto y0 = std::clamp(static_cast<int32_t>(rectHeight * 0.072f), 0, rectHeight);
    auto y1 = std::clamp(static_cast<int32_t>(rectHeight * 0.2f + y0), 0, rectHeight);
    if (y1 - y0 > 480)
        y1 = 480;

    auto x = rectX + x0;
    auto y = rectY + y0;
    auto width = x1 - x0;
    auto height = y1 - y0;

    *reinterpret_cast<float*>(manager + 1244) = static_cast<float>(x);          // shown x
    *reinterpret_cast<float*>(manager + 1240) = static_cast<float>(y);
    *reinterpret_cast<float*>(manager + 1232) = static_cast<float>(width + 4);  // slide distance
    *reinterpret_cast<float*>(manager + 1236) = static_cast<float>(height);

    // current slide position (UEPLGameplayManager +0x4A8 progress, same formula as the slide animation), not the end position
    auto progress = std::clamp(*reinterpret_cast<float*>(manager + 0x4A8), 0.0f, 1.0f);
    auto viewport = *reinterpret_cast<uintptr_t*>(manager + 1212);
    *reinterpret_cast<int32_t*>(viewport + 232) = shown ? static_cast<int32_t>(x - (width + 4 + x) * (1.0f - progress)) : -width;
    *reinterpret_cast<int32_t*>(viewport + 236) = y;
    *reinterpret_cast<int32_t*>(viewport + 216) = width;
    *reinterpret_cast<int32_t*>(viewport + 220) = height;
}

SafetyHookInline shSetupPiPHidden = {};
void __fastcall SetupPiPHidden(uintptr_t manager, void* edx)
{
    shSetupPiPHidden.fastcall<void>(manager, edx);
    LayoutPiP(manager, false);
}

SafetyHookInline shSetupPiPShown = {};
void __fastcall SetupPiPShown(uintptr_t manager, void* edx)
{
    shSetupPiPShown.fastcall<void>(manager, edx);
    LayoutPiP(manager, true);
}

export void InitWidescreenFix()
{
    if (bUltraWideSupport)
    {
        // Fullscreen: don't letterbox the viewport to 16:9 of the desktop aspect and treat pixels as square, render to the whole backbuffer like windowed mode
        auto pattern = hook::pattern("74 73 39 3D ? ? ? ? F3 0F 2A 86");
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);

        // The engine has a multi-monitor mode (UD3DRenderDevice +0x110, set for Eyefinity or when aspect > MinTripleDispRatio):
        // the viewport gets Hor+ FOV from the 16:9 horizontal FOV (UViewport +0x2B0 -> sub_87B3A3), Flash menus/HUD are laid out
        // in the HUD rect at +0x114 (x, y, w, h) and mouse input is offset into it, HUD beacons project into it too.
        // Its own triple display setup stays disabled (it puts the HUD rect on the middle third), a centered 16:9 rect is used for any non 16:9 aspect instead.
        pattern = hook::pattern("8D 86 ? ? ? ? 8B 08 3B CD");
        static auto IniHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esi + 0x104) = 0;     // ForceMultiMon
            *(float*)(regs.esi + 0x108) = 99.0f;    // MinTripleDispRatio
            *(uint32_t*)(regs.esi + 0x10C) = 0;     // EyefinityMode
        });

        pattern = hook::pattern("55 8B EC 83 EC 20 53 56 8B F1 8B 86 D4 00 00 00 33 DB");
        shSetupMultiMon = safetyhook::create_inline(pattern.get_first(), SetupMultiMon);

        // The multi-monitor mode also restricts the Flash render pass to the HUD rect (see InitLeadD3DRender), which clips everything the movies
        // draw outside the 16:9 stage (menu backgrounds, full screen overlays). That pass is kept fullscreen and the stage is placed into
        // the GViewport rect here instead, the same rect Scaleform uses for mouse input.
        // FlashMenuRenderer::BeginDisplay: the stage -> clip space matrix maps the frame rect to the whole render target and ignores the viewport rect
        pattern = hook::pattern("8D 74 24 20 F3 A5 8D 8B 90 00 00 00");
        static auto FlashBeginDisplay = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto viewport = *reinterpret_cast<int32_t**>(regs.ebp + 0xC); // GViewport: BufferWidth, BufferHeight, Left, Top, Width, Height
            auto [bufferWidth, bufferHeight, left, top, width, height] = std::tuple(viewport[0], viewport[1], viewport[2], viewport[3], viewport[4], viewport[5]);
            if (bufferWidth <= 0 || bufferHeight <= 0 || width <= 0 || height <= 0 || left < 0 || top < 0)
                return;
            if (left == 0 && top == 0 && width == bufferWidth && height == bufferHeight)
                return;

            auto scaleX = static_cast<float>(width) / bufferWidth;
            auto scaleY = static_cast<float>(height) / bufferHeight;
            auto offsetX = static_cast<float>(2 * left + width) / bufferWidth - 1.0f;
            auto offsetY = 1.0f - static_cast<float>(2 * top + height) / bufferHeight;

            auto matrix = reinterpret_cast<float*>(regs.esp + 0x20);
            matrix[0] *= scaleX;
            matrix[3] = matrix[3] * scaleX + offsetX;
            matrix[5] *= scaleY;
            matrix[7] = matrix[7] * scaleY + offsetY;

            // GMatrix2D copy (a, b, tx, c, d, ty): x row is already stored, y row is stored from xmm2/xmm4 right after
            auto matrix2D = reinterpret_cast<float*>(regs.esp + 0x68);
            matrix2D[0] = matrix[0];
            matrix2D[2] = matrix[3];
            regs.xmm2.f32[0] = matrix[5];
            regs.xmm4.f32[0] = matrix[7];
        });

        // BeginDisplay also draws the movie background color as a tile over the frame rect (menu dimming), extend it over the whole screen
        pattern = hook::pattern("D9 1C 24 E8 ? ? ? ? 8D 8C 24 80 00 00 00 E8");
        static auto FlashBackgroundTile = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
        {
            auto viewport = *reinterpret_cast<int32_t**>(regs.ebp + 0xC);
            auto [bufferWidth, bufferHeight, width, height] = std::tuple(viewport[0], viewport[1], viewport[4], viewport[5]);
            if (width <= 0 || height <= 0)
                return;

            auto tile = reinterpret_cast<float*>(regs.esp); // x0, y0, x1, y1 in frame (stage) coordinates
            auto extendX = (tile[2] - tile[0]) * (static_cast<float>(bufferWidth) / width - 1.0f) / 2.0f;
            auto extendY = (tile[3] - tile[1]) * (static_cast<float>(bufferHeight) / height - 1.0f) / 2.0f;
            tile[0] -= extendX;
            tile[2] += extendX;
            tile[1] -= extendY;
            tile[3] += extendY;
        });

        // Sticky camera PiP
        {
            auto pattern = hook::pattern("55 8B EC 51 51 53 56 57 8B F1 E8 ? ? ? ? 8B 40 44 8B 10 8B C8 FF 92");
            if (pattern.size() == 2)
            {
                shSetupPiPHidden = safetyhook::create_inline(pattern.get(0).get<void>(), SetupPiPHidden);
                shSetupPiPShown = safetyhook::create_inline(pattern.get(1).get<void>(), SetupPiPShown);

                // UWindowsViewport: don't remap the PiP viewport into the HUD rect, it's already laid out in it
                pattern = hook::pattern("E8 ? ? ? ? 3B C3 89 45 3C 0F 84");
                static auto PiPViewportMultiMon = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
                {
                    regs.eax = 0;
                });
            }

            // Flash frame, see GetPiPBarX
            pattern = hook::pattern("F3 0F 10 45 08 F3 0F 59 05 ? ? ? ? 8B 06 51 8D 4D EC");
            static auto PiPBarSlide = safetyhook::create_mid(pattern.get_first(13), [](SafetyHookContext& regs)
            {
                PiPBarProgress = *reinterpret_cast<float*>(regs.ebp + 8);
                if (GetAspectRatio() > fDefaultAspectRatio)
                    regs.xmm0.f32[0] = GetPiPBarX(PiPBarProgress);
            });

            // FlashHudPiP reset: _x = 0 (hidden)
            pattern = hook::pattern("D9 EE 8B 06 51 8D 4D EC D9 1C 24 51 8B CE FF 50 6C");
            static auto PiPBarReset = safetyhook::create_mid(pattern.get_first(14), [](SafetyHookContext& regs)
            {
                PiPBarProgress = 0.0f;
                if (GetAspectRatio() > fDefaultAspectRatio)
                    *reinterpret_cast<float*>(regs.esp + 4) = GetPiPBarX(PiPBarProgress);
            });
        }

        // Health feedback (blood vignette): HUD_Single's _root.SetHealthFeedback(fScale, fAlpha) sets HealthFeedback._xscale/_yscale to fScale,
        // the HUD is laid out in the centered 16:9 rect, so widen the vignette horizontally to reach the screen edges.
        {
            auto pattern = hook::pattern("55 8B EC 83 EC 50 56 8B F1 E8 ? ? ? ? 85 C0 0F 84");
            shHealthFeedbackDraw = safetyhook::create_inline(pattern.get_first(), HealthFeedbackDraw);
        }
    }

    // GFxMovieView::SetViewport call sites in RenderPhases (esi the renderer: buffer size at +4, viewport size at +0Ch, edi the movie),
    // the arguments are pushed: buffer width, height, left, top, width, height.
    {
        static auto MovieFixes = [](SafetyHookContext& regs)
        {
            // Loading after a split screen match: the buffer is still one player's half, the stage is laid out for the whole screen
            // and ends up off the left side (loading hints), use the whole screen
            auto args = reinterpret_cast<int32_t*>(regs.esp);
            if (args[4] > args[0] || args[5] > args[1])
            {
                args[0] = std::max(args[0], args[4]);
                args[1] = std::max(args[1], args[5]);
                args[2] = (args[0] - args[4]) / 2;
                args[3] = (args[1] - args[5]) / 2;
            }

            // Menu pages (sc5 components) dim the screen with Background.BG, a stage sized clip: stretch it over the whole screen.
            // The sticky camera PiP frame is kept at its remapped position (see GetPiPBarX).
            auto ratio = GetAspectRatio() / fDefaultAspectRatio;
            auto movieView = *reinterpret_cast<uintptr_t*>(regs.edi + 0x28);
            if (!bUltraWideSupport || ratio <= 1.0f || !movieView)
                return;
            if (SetFlashNumber(movieView, "_root.Background.BG._width", 1280.0 * ratio))
                SetFlashNumber(movieView, "_root.Background.BG._x", -640.0 * (ratio - 1.0));
            SetFlashNumber(movieView, "_root.PiP_Bar._x", GetPiPBarX(PiPBarProgress));
        };

        auto pattern = hook::pattern("8D 4D C0 E8 ? ? ? ? 8B 4F 28");
        static auto RenderPhasesMovieFixes = safetyhook::create_mid(pattern.get_first(), MovieFixes);

        pattern = hook::pattern("8D 4D C4 E8 ? ? ? ? 8B 4F 28");
        static auto RenderMovieFixes = safetyhook::create_mid(pattern.get_first(), MovieFixes);
    }
}
