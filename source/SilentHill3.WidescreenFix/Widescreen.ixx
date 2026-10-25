module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/SilentHill/Display.hpp"

export module Widescreen;

// Presentation at any resolution and aspect ratio.
//
// The game draws the 3D scene into an off-screen render target, runs its post effects on it and composites it to
// the back buffer; 2D is laid out on the PS2's 512x448 screen and converted to render target pixels by two
// functions. Here the render target and the back buffer get the window's resolution, the 3D view widens (Hor+),
// and the 2D conversion places each element according to what it is:
//   - interface, text, menus, items, map markers: a centred 4:3 area,
//   - effects positioned from the 3D view (particles, blood, lens flares, light haze, screen noise) and
//     full-screen fades and backdrops: the whole screen,
//   - full-screen pictures (notes, pictures, the pause background): as released, or filling the screen.
// Movies keep their intended 14:9 shape (the PC files are anamorphic 512x384) or fill the screen; HD replacement
// movies use their own shape. In cutscenes the view never shows what the PS2's cinematic bars covered: with the
// bars on they are drawn as on the PS2, with them off the picture is enlarged to fill the screen instead.

namespace
{
    struct
    {
        int32_t Width = 0, Height = 0;
        int32_t RenderWidth = 0, RenderHeight = 0;
        float Aspect = 4.0f / 3.0f;
        float Scale = 1.0f;     // aspect / (4:3)
    } Screen;

    const int* CinematicBars = nullptr;
    const float* FOVFactor = nullptr;
    uint8_t** Player = nullptr;
    uint8_t* Aiming = nullptr;
    int32_t* EventId = nullptr;
    std::optional<float> HudAspectRatioConstraint;
    const int* Movies = nullptr;
    const int* FullscreenImages = nullptr;

    int32_t* RenderTargetSize = nullptr;    // w, h of the off-screen render target
    int32_t* PresentSize = nullptr;         // present parameters back buffer w, h
    int32_t* DisplaySize = nullptr;         // requested display mode w, h, then the current 2D target w, h
    int32_t* VideoSize = nullptr;           // movie frame w, h

    // Cutscenes: the cutscene player asks for its bars every frame
    uint32_t FrameCounter = 0;
    uint32_t CutsceneFrame = 0x80000000;
    constexpr float VisibleWithBars = 1.0f - 0.20089285f;   // height left between the native bars

    bool InCutscene()
    {
        return FrameCounter - CutsceneFrame <= 2;
    }

    float BorderHalf = 0.5f;    // the native bars' height factor, 0 hides them
    float Zero = 0.0f;

    // 3D view scale relative to the game's own values (Hor+, cutscene framing)
    void ViewScale(float& x, float& y)
    {
        x = 1.0f / Screen.Scale;
        y = 1.0f;
        if (InCutscene() && !(*CinematicBars & 1))
        {
            float zoom = (*CinematicBars & 2) ? 1.0f / VisibleWithBars : std::max(1.0f / VisibleWithBars, Screen.Scale);
            x = zoom / Screen.Scale;
            y = zoom;
        }
        else if (!InCutscene() && Player && *Player && Aiming && !*Aiming && EventId && *EventId == -1)
        {
            const float factor = std::isfinite(*FOVFactor) ? std::clamp(*FOVFactor, 0.5f, 2.0f) : 1.0f;
            x /= factor;
            y /= factor;
        }
    }

    SafetyHookInline ScaleXHook, ScaleYHook;

    float __cdecl ScaleX()
    {
        float x, y;
        ViewScale(x, y);
        return ScaleXHook.ccall<float>() * x;
    }

    float __cdecl ScaleY()
    {
        float x, y;
        ViewScale(x, y);
        return ScaleYHook.ccall<float>() * y;
    }

    // The game's own value, for 2D that is laid out in the 4:3 area (cursor, picked-up item)
    float __cdecl NativeScaleX()
    {
        return ScaleXHook.ccall<float>();
    }

    // 2D: PS2 screen coordinates (1/16 px, centre at 2048) to render target pixels
    enum class Layout2D { Interface, Stretch, Fill };
    Layout2D CurrentLayout = Layout2D::Interface;
    int(__cdecl* RenderWidth)() = nullptr;
    int(__cdecl* RenderHeight)() = nullptr;
    SafetyHookInline GsXHook, GsYHook, SpriteHook;

    void Box(Layout2D layout, float w, float h, float& boxW, float& boxH)
    {
        bool wider = w * 3.0f >= h * 4.0f;
        switch (layout)
        {
        case Layout2D::Stretch:
            boxW = w;
            boxH = h;
            break;
        case Layout2D::Fill:
            boxW = wider ? w : h * 4.0f / 3.0f;
            boxH = wider ? w * 0.75f : h;
            break;
        default:
            boxW = wider ? h * 4.0f / 3.0f : w;
            boxH = wider ? h : w * 0.75f;
            break;
        }
    }

    float MapX(Layout2D layout, int gs)
    {
        float w = float(RenderWidth()), h = float(RenderHeight()), boxW, boxH;
        Box(layout, w, h, boxW, boxH);
        return w * 0.5f + (gs / 16.0f - 2048.0f) * boxW / 512.0f;
    }

    float MapY(Layout2D layout, int gs)
    {
        float w = float(RenderWidth()), h = float(RenderHeight()), boxW, boxH;
        Box(layout, w, h, boxW, boxH);
        return h * 0.5f + (gs / 16.0f - 2048.0f) * boxH / 448.0f;
    }

    float __cdecl GsX(int gs) { return MapX(CurrentLayout, gs); }
    float __cdecl GsY(int gs) { return MapY(CurrentLayout, gs); }
    float __cdecl GsStretchX(int gs) { return MapX(Layout2D::Stretch, gs); }
    float __cdecl GsStretchY(int gs) { return MapY(Layout2D::Stretch, gs); }

    // Interface sprites: full-screen ones are fades, backdrops or pictures
    float* __cdecl DrawSprite(uint8_t* sprite)
    {
        auto coord = [&](int offset) { return int(*reinterpret_cast<int16_t*>(sprite + offset)); };
        uint16_t flags = *reinterpret_cast<uint16_t*>(sprite + 0x42);
        int x0 = coord(12), y0 = coord(14), x1 = coord(16), y1 = coord(18);
        if (flags & 0x80)
        {
            x0 = std::min({ coord(12), coord(16), coord(20), coord(24) });
            x1 = std::max({ coord(12), coord(16), coord(20), coord(24) });
            y0 = std::min({ coord(14), coord(18), coord(22), coord(26) });
            y1 = std::max({ coord(14), coord(18), coord(22), coord(26) });
        }
        if (x0 > x1) std::swap(x0, x1);
        if (y0 > y1) std::swap(y0, y1);
        bool textured = (flags & 1) != 0 && !(flags & 0x200);
        constexpr int Tolerance = 4 * 16;
        bool covers = x0 <= -256 * 16 + Tolerance && x1 >= 256 * 16 - Tolerance && y0 <= -224 * 16 + Tolerance && y1 >= 224 * 16 - Tolerance;
        bool exact = std::abs(x0 + 256 * 16) <= Tolerance && std::abs(x1 - 256 * 16) <= Tolerance &&
            std::abs(y0 + 224 * 16) <= Tolerance && std::abs(y1 - 224 * 16) <= Tolerance;

        auto saved = CurrentLayout;
        if (covers && !textured)
            CurrentLayout = Layout2D::Stretch;                  // fades, flashes, black backdrops
        else if (exact && textured && *FullscreenImages)
            CurrentLayout = Layout2D::Fill;                     // notes, pictures, the pause background
        auto result = SpriteHook.ccall<float*>(sprite);
        CurrentLayout = saved;
        return result;
    }

    // Movies: the picture's intended shape, fitted or filling the screen
    SafetyHookInline MovieRectHook;
    void(__cdecl* SetMovieRect)(float, float, float, float) = nullptr;

    void __cdecl MovieRect(float, float, float, float)
    {
        float rtW = float(RenderTargetSize[0]), rtH = float(RenderTargetSize[1]);
        float videoW = float(VideoSize[0]), videoH = float(VideoSize[1]);
        // The released movies are 512x384 shown at 14:9; replacements carry their own shape
        float content = 14.0f / 9.0f;
        if (videoW > 0 && videoH > 0 && std::abs(videoW * 3.0f - videoH * 4.0f) > 1.0f)
            content = videoW / videoH;
        float screen = float(Screen.Width) / float(Screen.Height);
        float wn = *Movies == 2 ? 1.0f : (*Movies == 1 ? std::max(1.0f, content / screen) : std::min(1.0f, content / screen));
        float hn = *Movies == 2 ? 1.0f : wn * screen / content;
        float x0 = rtW * (1.0f - wn) * 0.5f, y0 = rtH * (1.0f - hn) * 0.5f;
        SetMovieRect(x0, y0, rtW - x0, rtH - y0);
    }

    // Resolution of the window, the render target and the back buffer
    void ApplyResolution()
    {
        RenderTargetSize[0] = Screen.RenderWidth;
        RenderTargetSize[1] = Screen.RenderHeight;
        PresentSize[0] = Screen.Width;
        PresentSize[1] = Screen.Height;
        for (int i = 0; i < 4; i += 2)
        {
            DisplaySize[i] = Screen.Width;
            DisplaySize[i + 1] = Screen.Height;
        }
    }

    struct SetResolution
    {
        void operator()(injector::reg_pack&)
        {
            ApplyResolution();
        }
    };

    void Hook(const char* signature, int end)
    {
        auto pattern = hook::pattern(signature);
        if (pattern.size() == 1)
            injector::MakeInline<SetResolution>(pattern.get_first(0), pattern.get_first(end));
    }

    // Redirects the calls to the 2D conversion functions in [start, end) to their full-width versions
    void StretchCalls(uintptr_t start, uintptr_t end, uintptr_t gsx, uintptr_t gsy)
    {
        auto calls = hook::range_pattern(start, end, "E8 ? ? ? ?");
        for (size_t i = 0; i < calls.size(); ++i)
        {
            auto at = calls.get(i).get<void>(0);
            auto target = injector::GetBranchDestination(at, true).as_int();
            if (target == gsx)
                injector::MakeCALL(at, &GsStretchX, true);
            else if (target == gsy)
                injector::MakeCALL(at, &GsStretchY, true);
        }
    }

    uintptr_t FunctionAt(const char* signature, int offset = 0)
    {
        auto pattern = hook::pattern(signature);
        return pattern.size() == 1 ? reinterpret_cast<uintptr_t>(pattern.get_first(offset)) : 0;
    }
}

export namespace Widescreen
{
    void Init()
    {
        CinematicBars = &FixSettings().Int("MAIN.CutsceneBorders");
        FOVFactor = &FixSettings().Float("MAIN.FOVFactor");
        auto player = hook::pattern("FF A1 ? ? ? ? 89 35 ? ? ? ? 89 86 80 01 00 00 8B 0D ? ? ? ?");
        auto aiming = hook::pattern("A1 ? ? ? ? 83 EC 08 85 C0 0F 84 ? ? ? ? 8A 0D ? ? ? ? 84 C9 0F 84 ? ? ? ?");
        auto event = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 53 52 57 E8 ? ? ? ? 83 C4 0C 83 F8 FF A3 ? ? ? ? 89 1D ? ? ? ?");
        if (player.size() == 1 && aiming.size() == 1 && event.size() == 1)
        {
            Player = *player.get_first<uint8_t**>(8);
            Aiming = *aiming.get_first<uint8_t*>(18);
            EventId = *event.get_first<int32_t*>(26);
        }
        else
            OutputDebugStringA("SilentHill3: gameplay FOV state patterns unavailable.\n");
        Movies = &FixSettings().Int("MAIN.FMVWidescreenMode");
        SHDisplay::OnPresent().push_back([]()
        {
            auto device = SHDisplay::Get().device;
            if (!device || !(InCutscene()) || !(*CinematicBars & 2))
                return;
            const auto& display = SHDisplay::Get();
            const LONG width = static_cast<LONG>(display.backBufferWidth);
            const LONG height = static_cast<LONG>(display.backBufferHeight);
            const LONG side = std::max(0L, (width - static_cast<LONG>(height * (4.0f / 3.0f))) / 2);
            if (side > 0)
            {
                D3DRECT borders[2] = { { 0, 0, side, height }, { width - side, 0, width, height } };
                D3DVIEWPORT8 previous = {}, full = { 0, 0, DWORD(width), DWORD(height), 0.0f, 1.0f };
                device->GetViewport(&previous);
                device->SetViewport(&full);
                device->Clear(2, borders, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
                device->SetViewport(&previous);
            }
        });
        FullscreenImages = &FixSettings().Int("MAIN.FullscreenImages");
        auto updateHUD = []()
        {
            HudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        };
        updateHUD();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", updateHUD);

        Screen.Width = FixSettings().Int("MAIN.ResX");
        Screen.Height = FixSettings().Int("MAIN.ResY");
        if (FixSettings().Int("MAIN.WindowMode") == 2 || Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
            std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
        if (Screen.Width <= 0 || Screen.Height <= 0)
            std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
        Screen.RenderWidth = FixSettings().Int("MAIN.RenderResX");
        Screen.RenderHeight = FixSettings().Int("MAIN.RenderResY");
        if (Screen.RenderWidth <= 0 || Screen.RenderHeight <= 0 || Screen.RenderWidth > 16384 || Screen.RenderHeight > 16384)
        {
            Screen.RenderWidth = Screen.Width;
            Screen.RenderHeight = Screen.Height;
        }
        Screen.Aspect = float(Screen.Width) / float(Screen.Height);
        Screen.Scale = Screen.Aspect / (4.0f / 3.0f);

        SHDisplay::OnPresent().push_back([]() { ++FrameCounter; });

        // Resolution
        auto renderTarget = hook::pattern("A1 ? ? ? ? 53 8B 5C 24 0C 3B C3 55 8B 6C 24 14");
        auto present = hook::pattern("BF ? ? ? ? F3 AB 52");
        auto display = hook::pattern("8B 0D ? ? ? ? 8B 44 24 04 3B C8 8B 4C 24 08 75 08 39 0D");
        if (renderTarget.size() != 1 || present.size() != 1 || display.empty())
            return;
        RenderTargetSize = *renderTarget.get_first<int32_t*>(1);
        PresentSize = *present.get_first<int32_t*>(1);
        DisplaySize = *display.get_first<int32_t*>(2);

        // The render target size is the fix's: the game's setters and its options list of sizes are disabled
        auto setRenderTarget = hook::pattern("51 A1 ? ? ? ? 53 8B 5C 24 0C 3B C3");
        if (setRenderTarget.size() == 1)
            injector::MakeRET(setRenderTarget.get_first());
        auto storeRenderTarget = hook::pattern(pattern_str(0x8B, 0x44, 0x24, 0x04, 0x8B, 0x4C, 0x24, 0x08, 0xA3, to_bytes(RenderTargetSize)));
        if (storeRenderTarget.size() == 1)
            injector::MakeRET(storeRenderTarget.get_first());
        auto renderTargetList = hook::pattern("8B 07 83 EC 0C 53 33 DB 3B C3 56");
        if (renderTargetList.size() == 1)
            injector::MakeRET(renderTargetList.get_first());

        Hook("89 15 ? ? ? ? 8B 10 50 FF 52 3C 85 C0", 6);                                                   // CreateDevice
        Hook("89 0D ? ? ? ? 89 15 ? ? ? ? E8 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 84 C0 75", 12);                   // Reset
        Hook("89 35 ? ? ? ? 5E 33 C0 5B 83 C4 10", 6);                                                      // Reset failed
        Hook("A3 ? ? ? ? 89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? C6 05 ? ? ? ? 01", 22);                     // display mode
        Hook("A3 ? ? ? ? 89 0D ? ? ? ? A3 ? ? ? ? C7 05 ? ? ? ? 3C 00", 16);                                // defaults
        ApplyResolution();

        // 3D view: Hor+, keeping the game's zoom effects; cutscene framing
        auto scale = hook::pattern("6A 02 E8 ? ? ? ? 83 C4 04 85 C0 74 07 D9 05 ? ? ? ? C3 D9 05 ? ? ? ? D8 0D ? ? ? ? C3");
        if (scale.size() == 2)
        {
            ScaleXHook = safetyhook::create_inline(scale.get(0).get<void>(0), &ScaleX);
            ScaleYHook = safetyhook::create_inline(scale.get(1).get<void>(0), &ScaleY);
            for (auto signature : { "E8 ? ? ? ? D9 9D 9C FE FF FF E8", "E8 ? ? ? ? D9 9D A0 FE FF FF" })
            {
                auto call = hook::pattern(signature);
                if (call.size() == 1)
                    injector::MakeCALL(call.get_first(), &NativeScaleX, true);
            }
        }

        auto cutscene = hook::pattern("6A 06 E8 ? ? ? ? 66 8B 44 24 08");
        if (cutscene.size() == 1)
        {
            static auto CutsceneHook = safetyhook::create_mid(cutscene.get_first(), [](SafetyHookContext&)
            {
                CutsceneFrame = FrameCounter;
            });
        }

        // The native cinematic bars on or off, and no 2-pixel side bars
        auto bars = hook::pattern("D9 05 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 D8 C9");
        if (bars.size() == 1)
        {
            BorderHalf = **bars.get_first<float*>(2);
            static float Native = BorderHalf;
            static float Current = (*CinematicBars & 1) ? Native : 0.0f;
            injector::WriteMemory(bars.get_first(2), &Current, true);
            SHDisplay::OnPresent().push_back([]() { Current = (*CinematicBars & 1) ? Native : 0.0f; });
        }
        for (auto signature : { "D8 0D ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 6A 00 D9 5C 24 14", "D8 0D ? ? ? ? DC C0 D9 54 24 0C D9 5C 24 04 E8" })
        {
            auto side = hook::pattern(signature);
            if (side.size() == 1)
                injector::WriteMemory(side.get_first(2), &Zero, true);
        }

        // 2D conversion
        auto gsy = FunctionAt("51 E8 ? ? ? ? 89 44 24 00 DB 44 24 00 DB 44 24 08 D8 0D ? ? ? ? D8 25 ? ? ? ? D8 05 ? ? ? ? DE C9 D8 0D ? ? ? ? 59 C3 90 90 E9");
        uintptr_t gsx = gsy ? gsy - 0x30 : 0;
        if (gsx && *reinterpret_cast<uint16_t*>(gsx) == 0xE851)
        {
            RenderWidth = reinterpret_cast<decltype(RenderWidth)>(injector::GetBranchDestination(gsx + 1).as_int());
            RenderHeight = reinterpret_cast<decltype(RenderHeight)>(injector::GetBranchDestination(gsy + 1).as_int());

            // Effects positioned from the 3D view use the whole screen: lens flares, light haze, the fog layer,
            // the lens flare occlusion test, and the PS2 packet interpreter (particles, blood, screen effects)
            static const std::pair<const char*, size_t> Effects[] =
            {
                { "83 EC 70 8B 48 0C 8B 50 08 C1 E1 08", 0x1A1 },                                               // lens flare
                { "83 EC 70 8B 48 0C 53 8B 9C 24 84 00 00 00", 0x185 },                                         // lens flare, rotated
                { "55 8B EC 83 E4 F0 81 EC D4 00 00 00 A1 ? ? ? ?", 0xA71 },                                     // light haze
                { "55 8B EC 83 E4 F0 83 EC 34 D9 05 ? ? ? ? A1 ? ? ? ? D8 25 ? ? ? ? 89 44 24 30", 0x394 },       // fog layer
                { "55 8B EC 83 E4 F0 83 EC 74 53 56 57 68 ? ? ? ?", 0x90D },                                     // lens flare occlusion
            };
            for (auto [signature, size] : Effects)
            {
                if (auto start = FunctionAt(signature))
                    StretchCalls(start, start + size, gsx, gsy);
            }
            for (auto signature : { "E8 ? ? ? ? 8B 0D ? ? ? ? 6B C9 1C D9 99", "E8 ? ? ? ? D9 5E FC D9 07 D8 0D ? ? ? ? E8 ? ? ? ? 50" })
            {
                auto call = hook::pattern(signature);
                if (call.size() == 1)
                    injector::MakeCALL(call.get_first(), &GsStretchX, true);
            }

            GsXHook = safetyhook::create_inline(gsx, &GsX);
            GsYHook = safetyhook::create_inline(gsy, &GsY);
            if (auto sprite = FunctionAt("83 EC 20 53 55 56 8B 74 24 30 66 8B 4E 42"))
                SpriteHook = safetyhook::create_inline(sprite, &DrawSprite);
        }

        // Mouse in the 4:3 interface area
        float hudScale = 1.0f / Screen.Scale;
        auto cursorWidth = hook::pattern("D8 05 ? ? ? ? D9 5C 24 04 E8 ? ? ? ? D8 0D ? ? ? ? 8B 4C 24 04 68 80 00 00 00 6A 01");
        auto cursorStore = hook::pattern("DB 44 24 00 D9 1D ? ? ? ? E8 ? ? ? ? 89 44 24 00 DB 44 24 00 D9 1D ? ? ? ? 83 C4 14 C3");
        if (cursorWidth.size() == 1 && cursorStore.size() == 1)
        {
            static float* CursorX = *cursorWidth.get_first<float*>(2);
            static float HudScale = hudScale;
            struct CursorWidth
            {
                void operator()(injector::reg_pack&)
                {
                    float x = *CursorX;
                    _asm fadd dword ptr[x]
                    _asm fmul dword ptr[HudScale]
                }
            };
            injector::MakeInline<CursorWidth>(cursorWidth.get_first(0), cursorWidth.get_first(6));
            struct CursorStore
            {
                void operator()(injector::reg_pack& regs)
                {
                    *CursorX = float(*(int32_t*)&regs.eax) * HudScale;
                }
            };
            injector::MakeInline<CursorStore>(cursorStore.get_first(0), cursorStore.get_first(10));
        }
        auto mouseX = hook::pattern("E8 ? ? ? ? 8B F0 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9");
        if (mouseX.size() == 1)
        {
            static injector::hook_back<int32_t(*)()> GetMouseX;
            static float Offset = ((512.0f / hudScale) - 512.0f) / 2.0f;
            GetMouseX.fun = injector::MakeCALL(mouseX.get_first(), static_cast<int32_t(*)()>([]() -> int32_t
            {
                return int32_t(float(GetMouseX.fun()) - Offset);
            }), true).get();
        }
        auto mouseBounds = hook::pattern("E8 ? ? ? ? 8B C8 8B C6 C1 E0 09 99");
        if (mouseBounds.size() == 1)
        {
            static int32_t Bound = int32_t(std::floor(float(Screen.Width) * hudScale));
            injector::MakeCALL(mouseBounds.get_first(), static_cast<int32_t(*)()>([]() -> int32_t { return Bound; }), true);
        }

        // Low health cross in the lower right corner of the screen
        auto health = hook::pattern("D9 59 E8 D9 C0 D8 48 F8 D9 59 EC 7C DE 6A 01");
        if (health.size() == 1)
        {
            static float HudScale = hudScale;
            struct HealthCross
            {
                void operator()(injector::reg_pack& regs)
                {
                    float x = 0.0f;
                    _asm fstp dword ptr[x]
                    const float ratio = HudAspectRatioConstraint ? ClampHudAspectRatio(*HudAspectRatioConstraint, Screen.Aspect) : Screen.Aspect;
                    const float width = float(Screen.Height) * ratio;
                    const float offset = (float(Screen.Width) + width) * 0.5f - float(Screen.Height) * (4.0f / 3.0f);
                    *(float*)(regs.ecx - 0x18) = x * HudScale + offset;
                    _asm fld st
                }
            };
            injector::MakeInline<HealthCross>(health.get_first(0));
        }

        // Black inventory backdrop wide enough for any screen
        auto backdrop = hook::pattern("B8 00 01 00 00 BA 00 FF FF FF 8B E8");
        if (backdrop.size() == 1)
        {
            injector::WriteMemory(backdrop.get_first(1), 1024, true);
            injector::WriteMemory(backdrop.get_first(6), -1024, true);
        }

        // Logos and credits pictures in the 4:3 area
        {
            static float Left = (float(Screen.RenderWidth) - float(Screen.RenderHeight) * 4.0f / 3.0f) / 2.0f;
            static float Right = float(Screen.RenderWidth) - Left;
            auto logoLeft = hook::pattern("A1 ? ? ? ? 6A 05 C7 44 24 18");
            auto logoLeft2 = hook::pattern("C7 44 24 50 ? ? ? ? C7 44 24 58 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 64");
            auto logoRight = hook::pattern("D8 25 ? ? ? ? 8D 54 24 0C 52 6A 02");
            if (logoLeft.size() == 1 && logoLeft2.size() == 1 && logoRight.size() == 1)
            {
                injector::WriteMemory<float>(logoLeft.get_first(11), Left - 0.5f, true);
                injector::WriteMemory<float>(logoLeft2.get_first(4), Left - 0.5f, true);
                injector::WriteMemory<uint16_t>(logoRight.get_first(0), 0x05D9, true);   // fsub -> fld
                injector::WriteMemory(logoRight.get_first(2), &Right, true);
            }
            auto creditsLeft = hook::pattern("6A 05 C7 44 24 1C");
            auto creditsLeft2 = hook::pattern("C7 44 24 54 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 60 ? ? ? ? C7 44 24 68");
            auto creditsRight = hook::pattern("D8 25 ? ? ? ? 8D 4C 24 10 51 6A 02 D9 54 24 34");
            if (creditsLeft.size() == 1 && creditsLeft2.size() == 1 && creditsRight.size() == 1)
            {
                injector::WriteMemory<float>(creditsLeft.get_first(6), Left - 0.5f, true);
                injector::WriteMemory<float>(creditsLeft2.get_first(4), Left - 0.5f, true);
                injector::WriteMemory<uint16_t>(creditsRight.get_first(0), 0x05D9, true);
                injector::WriteMemory(creditsRight.get_first(2), &Right, true);
            }
        }

        // Movies
        auto movieRect = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 10 83 E8 00");
        auto videoSize = hook::pattern("8B 0D ? ? ? ? 8B 15 ? ? ? ? 68 ? ? ? ? 50 A1 ? ? ? ? 50 6A 00 6A 01 51 52");
        if (movieRect.size() == 1 && videoSize.size() == 1)
        {
            VideoSize = *videoSize.get_first<int32_t*>(8);
            SetMovieRect = reinterpret_cast<decltype(SetMovieRect)>(injector::GetBranchDestination(movieRect.get_first()).as_int());
            injector::MakeCALL(movieRect.get_first(), &MovieRect, true);
        }

        // Half-pixel alignment of the final picture: every edge of the present quad at -0.5, not only the far ones
        auto presentQuad = hook::pattern("89 15 ? ? ? ? A3 ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ?");
        if (presentQuad.size() == 1)
        {
            static float* Right = *presentQuad.get_first<float*>(2);        // v2.x
            static float* Quad = *presentQuad.get_first<float*>(7);         // v0.x, stride 24 bytes
            static float* Bottom = *presentQuad.get_first<float*>(13);      // v3.y
            static float* Bottom2 = *presentQuad.get_first<float*>(19);     // v2.y
            struct PresentQuad
            {
                void operator()(injector::reg_pack&)
                {
                    float w = float(RenderTargetSize[0]) - 0.5f, h = float(RenderTargetSize[1]) - 0.5f;
                    Quad[0] = w;   Quad[1] = -0.5f;     // v0
                    Quad[6] = -0.5f; Quad[7] = -0.5f;   // v1
                    *Right = w;    *Bottom2 = h;        // v2
                    Quad[18] = -0.5f; *Bottom = h;      // v3
                    float temp = 0.0f;
                    __asm fstp temp
                }
            };
            injector::MakeInline<PresentQuad>(presentQuad.get_first(0), presentQuad.get_first(23));
        }
        // The frame blend quad: its far edges like the present quad (its near edges are static zeros)
        auto blendX = hook::pattern("D9 15 ? ? ? ? D9 1D ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ? DB");
        auto blendY = hook::pattern("D9 15 ? ? ? ? D9 1D ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ? 8B 10");
        if (blendX.size() == 1 && blendY.size() == 1)
        {
            static float* X[4];
            static float* Y[4];
            for (int i = 0; i < 4; ++i)
            {
                X[i] = *blendX.get_first<float*>(2 + 6 * i);
                Y[i] = *blendY.get_first<float*>(2 + 6 * i);
            }
            struct BlendX
            {
                void operator()(injector::reg_pack&)
                {
                    for (auto x : X)
                        *x = float(RenderTargetSize[0]) - 0.5f;
                    float temp = 0.0f;
                    __asm fstp temp
                    __asm fstp temp
                }
            };
            struct BlendY
            {
                void operator()(injector::reg_pack&)
                {
                    for (auto y : Y)
                        *y = float(RenderTargetSize[1]) - 0.5f;
                    float temp = 0.0f;
                    __asm fstp temp
                    __asm fstp temp
                }
            };
            injector::MakeInline<BlendX>(blendX.get_first(0), blendX.get_first(24));
            injector::MakeInline<BlendY>(blendY.get_first(0), blendY.get_first(24));
        }
    }
}
