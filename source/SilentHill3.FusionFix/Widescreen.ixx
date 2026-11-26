module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"

export module Widescreen;

import ComVars;

// Presentation at any resolution and aspect ratio.
//
// The game draws the 3D scene into an off-screen render target, runs its post effects on it and composites it to
// the back buffer; 2D is laid out on the PS2's 512x448 screen and converted to render target pixels by two
// functions. Here the render target and the back buffer get the window's resolution, the 3D view widens (Hor+),
// and the 2D conversion places each element according to what it is:
//   - interface, text, menus, items, map markers: a centred 4:3 area,
//   - effects positioned from the 3D view (particles, blood, lens flares, light haze, screen noise) and
//     full-screen fades and backdrops: the whole screen,
//   - full-screen notes and pictures: as released, or filling the screen.
// Movies keep their intended 14:9 shape (the PC files are anamorphic 512x384), fitted whole into the game's 4:3 area or
// the whole screen; HD replacement movies use their own shape. Cutscenes preserve their authored framing with borders, or fill the screen Hor+.

namespace
{
    struct
    {
        int32_t Width = 0, Height = 0;
        int32_t RenderWidth = 0, RenderHeight = 0;
        float Aspect = 4.0f / 3.0f;
        float Scale = 1.0f;     // aspect / (4:3)
    } Screen;

    Size2D RenderOverride{};
    std::optional<Size2D> PendingSize;
    float HudScale = 1.0f;
    float MouseOffset = 0.0f;
    int32_t MouseBound = 0;
    float LogoRight = 0.0f;
    std::array<void*, 4> LogoLeftEdges{};
    struct DisplayMode
    {
        int32_t width, height;
        D3DFORMAT format;
    };
    struct DisplayModes
    {
        DisplayMode* modes;
        int32_t count;
        DisplayMode current, pending;
        int32_t selection;
        char label[64];
    };
    static_assert(sizeof(DisplayMode) == 12 && offsetof(DisplayModes, label) == 0x24);
    DisplayModes* NativeModes = nullptr;
    IDirect3D8* (__cdecl* GetDirect3D)() = nullptr;
    void* (__cdecl* GameAllocate)(size_t) = nullptr;
    void(__cdecl* GameFree)(void*) = nullptr;
    int(__cdecl* NativeSetSize)(int, int) = nullptr;
    uint8_t* ModesReady = nullptr;

    void __cdecl FillDisplayModes(DisplayModes* list, int)
    {
        NativeModes = list;
        std::vector<DisplayMode> modes{ list->current };
        auto add = [&](DWORD width, DWORD height)
        {
            if (width >= 640 && width <= 16384 && height >= 480 && height <= 16384)
                modes.push_back({ static_cast<int32_t>(width), static_cast<int32_t>(height), list->current.format });
        };
        const auto& display = SHDisplay::Get();
        if (display.mode == SHDisplay::Mode::Windowed)
        {
            MONITORINFOEXW monitor{};
            monitor.cbSize = sizeof(monitor);
            const auto device = GetMonitorInfoW(MonitorFromWindow(display.window, MONITOR_DEFAULTTOPRIMARY), &monitor) ? monitor.szDevice : nullptr;
            for (DWORD index = 0;; ++index)
            {
                DEVMODEW mode{};
                mode.dmSize = sizeof(mode);
                if (!EnumDisplaySettingsW(device, index, &mode))
                    break;
                if (mode.dmBitsPerPel >= 16)
                    add(mode.dmPelsWidth, mode.dmPelsHeight);
            }
        }
        else if (display.mode == SHDisplay::Mode::Fullscreen)
        {
            auto d3d = GetDirect3D();
            const auto count = d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT);
            for (UINT index = 0; index < count; ++index)
            {
                D3DDISPLAYMODE mode{};
                if (SUCCEEDED(d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, index, &mode)) &&
                    SUCCEEDED(d3d->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mode.Format, list->current.format, FALSE)))
                    add(mode.Width, mode.Height);
            }
        }
        auto dimensions = [](const DisplayMode& mode) { return std::tie(mode.width, mode.height); };
        std::ranges::sort(modes, {}, dimensions);
        modes.erase(std::unique(modes.begin(), modes.end(), [&](const auto& a, const auto& b)
        {
            return dimensions(a) == dimensions(b);
        }), modes.end());
        auto replacement = static_cast<DisplayMode*>(GameAllocate(modes.size() * sizeof(DisplayMode)));
        if (!replacement)
            return;
        std::copy(modes.begin(), modes.end(), replacement);
        GameFree(list->modes);
        list->modes = replacement;
        list->count = static_cast<int32_t>(modes.size());
        auto selected = std::ranges::find(modes, dimensions(list->pending), dimensions);
        if (selected == modes.end())
            selected = std::ranges::find(modes, dimensions(list->current), dimensions);
        list->selection = static_cast<int32_t>(std::distance(modes.begin(), selected));
        list->pending = *selected;
        list->label[0] = '\0';
        *ModesReady = 1;
    }

    void SelectAppliedMode()
    {
        if (!NativeModes || !NativeModes->modes)
            return;
        const auto first = NativeModes->modes;
        const auto last = first + NativeModes->count;
        const auto applied = std::find_if(first, last, [](const DisplayMode& mode)
        {
            return mode.width == Screen.Width && mode.height == Screen.Height;
        });
        if (applied != last)
        {
            NativeModes->selection = static_cast<int32_t>(std::distance(first, applied));
            NativeModes->pending = *applied;
        }
    }

    int __cdecl SelectDisplaySize(int width, int height)
    {
        if (SHDisplay::Get().mode != SHDisplay::Mode::Borderless &&
            (width != Screen.Width || height != Screen.Height))
            PendingSize = Size2D{ width, height };
        return NativeSetSize(width, height);
    }

    void RefreshGeometry()
    {
        Screen.Aspect = float(Screen.Width) / float(Screen.Height);
        Screen.Scale = Screen.Aspect / (4.0f / 3.0f);
        HudScale = std::min(1.0f, 1.0f / Screen.Scale);
        MouseOffset = (512.0f / HudScale - 512.0f) * 0.5f;
        MouseBound = int32_t(std::floor(float(Screen.Width) * HudScale));
        const float left = (float(Screen.RenderWidth) - float(Screen.RenderHeight) * 4.0f / 3.0f) * 0.5f;
        LogoRight = float(Screen.RenderWidth) - left;
        for (const auto edge : LogoLeftEdges)
        {
            if (edge)
                injector::WriteMemory<float>(edge, left - 0.5f, true);
        }
    }

    int CinematicBars = 3;
    float FOVFactor = 1.0f;
    std::optional<float> HudAspectRatioConstraint;
    bool FillFullscreenImages = false;

    void RefreshLiveFraming()
    {
        CinematicBars = FixSettingNumber("MAIN.CutsceneBorders", 3);
        FOVFactor = FixSettingNumber("MAIN.FOVFactor", 1.0f);
        FillFullscreenImages = FixSettingNumber("MAIN.FullscreenImages", 0) == 1;
        const auto hud = FixSettings().Get("MAIN.HudAspectRatioConstraint");
        HudAspectRatioConstraint = ParseWidescreenHudOffset(hud);
    }

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
        if (InCutscene())
        {
            if (CinematicBars && Screen.Scale < 1.0f)
            {
                x = 1.0f;
                y = Screen.Scale;
            }
        }
    }

    SafetyHookInline ScaleXHook, ScaleYHook;

    float GameplayFactor(float nativeX)
    {
        if (InCutscene() || !pPlayer || bAiming || nEventId != -1 || nCameraMode != 0 || (nCameraFlags & 3) ||
            nRenderMode == 2 || bPauseActive)
            return 1.0f;
        const float factor = std::isfinite(FOVFactor) ? std::clamp(FOVFactor, 0.5f, 2.0f) : 1.0f;
        if (factor == 1.0f)
            return factor;
        // Native projection: width = 512 / base / scaleX, m00 = 2 * distance / width.
        const float horizontal = fProjectionDistance * fProjectionBase * nativeX / (256.0f * Screen.Scale);
        if (!std::isfinite(horizontal) || horizontal <= 0.0f)
            return 1.0f;
        constexpr float halfLimit = 1.3089969389957472f;
        const float halfAngle = std::atan(1.0f / horizontal);
        const float scaled = std::min(halfAngle * factor, std::max(halfAngle, halfLimit));
        return std::tan(scaled) * horizontal;
    }

    float __cdecl ScaleX()
    {
        float x, y;
        ViewScale(x, y);
        const float native = ScaleXHook.ccall<float>();
        return native * x / GameplayFactor(native);
    }

    float __cdecl ScaleY()
    {
        float x, y;
        ViewScale(x, y);
        return ScaleYHook.ccall<float>() * y / GameplayFactor(ScaleXHook.ccall<float>());
    }

    // The game's own value, for 2D that is laid out in the 4:3 area (cursor, picked-up item)
    float __cdecl NativeScaleX()
    {
        return ScaleXHook.ccall<float>();
    }

    // 2D: PS2 screen coordinates (1/16 px, centre at 2048) to render target pixels
    enum class Layout2D { Interface, Stretch, Fill, FillWidePicture };
    Layout2D CurrentLayout = Layout2D::Interface;
    bool DrawingMemoPicture = false;
    int(__cdecl* RenderWidth)() = nullptr;
    int(__cdecl* RenderHeight)() = nullptr;
    SafetyHookInline GsXHook, GsYHook, SpriteHook, MemoPictureHook;

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
        case Layout2D::FillWidePicture:
            boxW = std::max(w, h * (layout == Layout2D::FillWidePicture ? 14.0f / 9.0f : 4.0f / 3.0f));
            boxH = boxW * 0.75f;
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
        const auto centredSpan = [](int low, int high, int halfPixels)
        {
            return std::abs(low + halfPixels * 16) <= Tolerance && std::abs(high - halfPixels * 16) <= Tolerance;
        };
        const bool fullWidth = centredSpan(x0, x1, 256);
        const bool memoPicture = DrawingMemoPicture && centredSpan(y0, y1, 256);

        auto saved = CurrentLayout;
        if (covers && !textured)
            CurrentLayout = Layout2D::Stretch;                  // fades, flashes, black backdrops
        else if (textured && FillFullscreenImages && fullWidth && (memoPicture || centredSpan(y0, y1, 224)))
            CurrentLayout = memoPicture ? Layout2D::FillWidePicture : Layout2D::Fill;
        auto result = SpriteHook.ccall<float*>(sprite);
        CurrentLayout = saved;
        return result;
    }

    int __cdecl DrawMemoPicture(int picture, int nextPicture)
    {
        // The memo sprite is square, with 384 lines of artwork between its black bands.
        const auto saved = std::exchange(DrawingMemoPicture, true);
        const auto result = MemoPictureHook.ccall<int>(picture, nextPicture);
        DrawingMemoPicture = saved;
        return result;
    }

    // Movies: the picture's intended shape, fitted into the game's 4:3 area (Original) or the whole screen (Fill)
    SafetyHookInline MovieRectHook;
    void(__cdecl* SetMovieRect)(float, float, float, float) = nullptr;

    void __cdecl MovieRect(float, float, float, float)
    {
        float rtW = float(RenderTargetSize->w), rtH = float(RenderTargetSize->h);
        float videoW = float(VideoSize->w), videoH = float(VideoSize->h);
        // The released movies are 512x384 shown at 14:9; replacements carry their own shape
        float content = 14.0f / 9.0f;
        if (videoW > 0 && videoH > 0 && std::abs(videoW * 3.0f - videoH * 4.0f) > 1.0f)
            content = videoW / videoH;
        float screen = float(Screen.Width) / float(Screen.Height);
        const auto mode = FixSettingNumber("MAIN.FMVWidescreenMode", 1);
        const float area = mode == 0 ? 4.0f / 3.0f : screen;
        float wn = std::min(1.0f, area / screen) * std::min(1.0f, content / area);
        float hn = wn * screen / content;
        float x0 = rtW * (1.0f - wn) * 0.5f, y0 = rtH * (1.0f - hn) * 0.5f;
        SetMovieRect(x0, y0, rtW - x0, rtH - y0);
    }

    // Resolution of the window, the render target and the back buffer
    void ApplyResolution()
    {
        const auto size = PendingSize.value_or(Size2D{ Screen.Width, Screen.Height });
        *RenderTargetSize = RenderOverride.w ? RenderOverride : size;
        *PresentSize = size;
        DisplaySize->mode = DisplaySize->target = size;
    }

    void ConfirmResolution(IDirect3DDevice8* device)
    {
        IDirect3DSurface8* buffer = nullptr;
        if (FAILED(device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &buffer)))
            return;
        D3DSURFACE_DESC desc{};
        const auto result = buffer->GetDesc(&desc);
        buffer->Release();
        if (FAILED(result) || !desc.Width || !desc.Height)
            return;

        const auto request = std::exchange(PendingSize, std::nullopt);
        Screen.Width = static_cast<int32_t>(desc.Width);
        Screen.Height = static_cast<int32_t>(desc.Height);
        Screen.RenderWidth = RenderOverride.w ? RenderOverride.w : Screen.Width;
        Screen.RenderHeight = RenderOverride.h ? RenderOverride.h : Screen.Height;
        ApplyResolution();
        RefreshGeometry();
        if (request)
            SelectAppliedMode();
        if (request && request->w == Screen.Width && request->h == Screen.Height)
        {
            FixSettings().Set("MAIN.ResX", std::to_string(Screen.Width));
            FixSettings().Set("MAIN.ResY", std::to_string(Screen.Height));
        }
    }

    struct SetResolution
    {
        void operator()(injector::reg_pack&)
        {
            ApplyResolution();
        }
    };

    struct RestoreResolution
    {
        void operator()(injector::reg_pack&)
        {
            PendingSize.reset();
            auto& display = SHDisplay::Get();
            display.backBufferWidth = Screen.Width;
            display.backBufferHeight = Screen.Height;
            ApplyResolution();
            SelectAppliedMode();
        }
    };

    void Hook(const char* signature, int end)
    {
        auto pattern = hook::pattern(signature);
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
        return reinterpret_cast<uintptr_t>(hook::get_pattern(signature, offset));
    }
}

class Widescreen
{
public:
    Widescreen()
    {
        WFP::onInitEvent() += []()
        {
            RefreshLiveFraming();
            // The player, aiming, event, camera mode/flags, render mode, pause and projection variables are bound in Init
            // Borders: 1 letterbox (the native bars), 2 pillarbox (the 4:3 picture), 3 both. Screens narrower than 4:3
            // show the whole 4:3 picture with bars above and below whenever there are borders.
            SHDisplay::OnPresent().push_back([]()
            {
                auto device = SHDisplay::Get().device;
                const int mode = std::clamp(CinematicBars, 0, 3);
                if (!device || !InCutscene() || !mode)
                    return;
                const auto& display = SHDisplay::Get();
                const LONG width = static_cast<LONG>(display.backBufferWidth);
                const LONG height = static_cast<LONG>(display.backBufferHeight);
                const float boxWidth = std::min(float(width), float(height) * (4.0f / 3.0f));
                const float visible = (mode & 1) ? VisibleWithBars : 1.0f;
                const LONG side = (mode & 2) ? std::max(0L, LONG((width - boxWidth) * 0.5f)) : 0;
                const LONG top = std::max(0L, LONG((height - boxWidth * 0.75f * visible) * 0.5f));
                D3DRECT borders[4] = { { 0, 0, side, height }, { width - side, 0, width, height },
                    { 0, 0, width, top }, { 0, height - top, width, height } };
                D3DVIEWPORT8 previous = {}, full = { 0, 0, DWORD(width), DWORD(height), 0.0f, 1.0f };
                if (SUCCEEDED(device->GetViewport(&previous)))
                {
                    device->SetViewport(&full);
                    for (const auto& border : borders)
                    {
                        if (border.x2 > border.x1 && border.y2 > border.y1)
                            device->Clear(1, &border, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
                    }
                    device->SetViewport(&previous);
                }
            });
            Screen.Width = FixSettings().Int("MAIN.ResX");
            Screen.Height = FixSettings().Int("MAIN.ResY");
            if (FixSettings().Int("MAIN.WindowMode") == 2 || Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
            Screen.RenderWidth = FixSettings().Int("MAIN.RenderResX");
            Screen.RenderHeight = FixSettings().Int("MAIN.RenderResY");
            if (Screen.RenderWidth > 0 && Screen.RenderHeight > 0 && Screen.RenderWidth <= 16384 && Screen.RenderHeight <= 16384)
                RenderOverride = { Screen.RenderWidth, Screen.RenderHeight };
            else
            {
                Screen.RenderWidth = Screen.Width;
                Screen.RenderHeight = Screen.Height;
            }
            RefreshGeometry();

            SHDisplay::OnPresent().push_back([]() { ++FrameCounter; });

            // Resolution
            auto renderTarget = hook::pattern("A1 ? ? ? ? 53 8B 5C 24 0C 3B C3 55 8B 6C 24 14");
            auto present = hook::pattern("BF ? ? ? ? F3 AB 52");
            auto display = hook::pattern("8B 0D ? ? ? ? 8B 44 24 04 3B C8 8B 4C 24 08 75 08 39 0D");
            RenderTargetSize.SetAddress(*renderTarget.get_first<Size2D*>(1));
            PresentSize.SetAddress(*present.get_first<Size2D*>(1));
            DisplaySize.SetAddress(*display.get_first<DisplaySizes*>(2));

            // The render target size is the fix's: the game's setters and its options list of sizes are disabled
            auto setRenderTarget = hook::pattern("51 A1 ? ? ? ? 53 8B 5C 24 0C 3B C3");
            injector::MakeRET(setRenderTarget.get_first());
            auto storeRenderTarget = hook::pattern(pattern_str(0x8B, 0x44, 0x24, 0x04, 0x8B, 0x4C, 0x24, 0x08, 0xA3, to_bytes(reinterpret_cast<int32_t*>(RenderTargetSize.get_ptr()))));
            injector::MakeRET(storeRenderTarget.get_first());
            auto renderTargetList = hook::pattern("8B 07 83 EC 0C 53 33 DB 3B C3 56");
            injector::MakeRET(renderTargetList.get_first());

            Hook("89 15 ? ? ? ? 8B 10 50 FF 52 3C 85 C0", 6);                                                   // CreateDevice
            Hook("89 0D ? ? ? ? 89 15 ? ? ? ? E8 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 84 C0 75", 12);                   // Reset
            auto failed = hook::pattern("89 35 ? ? ? ? 5E 33 C0 5B 83 C4 10");
            injector::MakeInline<RestoreResolution>(failed.get_first(), failed.get_first(6));
            Hook("A3 ? ? ? ? 89 0D ? ? ? ? A3 ? ? ? ? C7 05 ? ? ? ? 3C 00", 16);                                // defaults
            ApplyResolution();
            SHDisplay::OnDeviceReady().push_back(&ConfirmResolution);

            auto enumerate = hook::pattern("81 EC D8 00 00 00 55 8B AC 24 E0 00 00 00 56 57 E8");
            GetDirect3D = reinterpret_cast<decltype(GetDirect3D)>(injector::GetBranchDestination(enumerate.get_first(0x3C)).as_int());
            GameFree = reinterpret_cast<decltype(GameFree)>(injector::GetBranchDestination(enumerate.get_first(0x27)).as_int());
            GameAllocate = reinterpret_cast<decltype(GameAllocate)>(injector::GetBranchDestination(enumerate.get_first(0x5F)).as_int());
            ModesReady = *enumerate.get_first<uint8_t*>(0x34);
            injector::MakeJMP(enumerate.get_first(), &FillDisplayModes, true);
            auto applyMode = hook::pattern("8B 56 04 8B 06 52 50 E8 ? ? ? ? E8 ? ? ? ? 8B 0D");
            auto retryMode = hook::pattern("8B 44 3E 04 8B 0C 3E 50 51 E8 ? ? ? ? E8 ? ? ? ? 8B 15");
            NativeSetSize = reinterpret_cast<decltype(NativeSetSize)>(injector::GetBranchDestination(applyMode.get_first(7)).as_int());
            injector::MakeCALL(applyMode.get_first(7), &SelectDisplaySize, true);
            injector::MakeCALL(retryMode.get_first(9), &SelectDisplaySize, true);

            // 3D view: Hor+, keeping the game's zoom effects; cutscene framing
            auto scale = hook::pattern("6A 02 E8 ? ? ? ? 83 C4 04 85 C0 74 07 D9 05 ? ? ? ? C3 D9 05 ? ? ? ? D8 0D ? ? ? ? C3");
            fProjectionBase.SetAddress(*scale.get(0).get<float*>(23));
            ScaleXHook = safetyhook::create_inline(scale.get(0).get<void>(0), &ScaleX);
            ScaleYHook = safetyhook::create_inline(scale.get(1).get<void>(0), &ScaleY);
            for (auto signature : { "E8 ? ? ? ? D9 9D 9C FE FF FF E8", "E8 ? ? ? ? D9 9D A0 FE FF FF" })
            {
                auto call = hook::pattern(signature);
                injector::MakeCALL(call.get_first(), &NativeScaleX, true);
            }

            auto cutscene = hook::pattern("6A 06 E8 ? ? ? ? 66 8B 44 24 08");
            static auto CutsceneHook = safetyhook::create_mid(cutscene.get_first(), [](SafetyHookContext&)
            {
                CutsceneFrame = FrameCounter;
            });

            // The native cinematic bars on or off, and no 2-pixel side bars
            auto bars = hook::pattern("D9 05 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 D8 C9");
            BorderHalf = **bars.get_first<float*>(2);
            static float Native = BorderHalf;
            static float Current = (CinematicBars & 1) ? Native : 0.0f;
            injector::WriteMemory(bars.get_first(2), &Current, true);
            SHDisplay::OnPresent().push_back([]()
            {
                RefreshLiveFraming();
                Current = (CinematicBars & 1) ? Native : 0.0f;
            });
            for (auto signature : { "D8 0D ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 6A 00 D9 5C 24 14", "D8 0D ? ? ? ? DC C0 D9 54 24 0C D9 5C 24 04 E8" })
            {
                auto side = hook::pattern(signature);
                injector::WriteMemory(side.get_first(2), &Zero, true);
            }

            // 2D conversion
            // The X function directly followed by the Y function
            uintptr_t gsx = FunctionAt("51 E8 ? ? ? ? 89 44 24 00 DB 44 24 00 DB 44 24 08 D8 0D ? ? ? ? D8 25 ? ? ? ? D8 05 ? ? ? ? DE C9 D8 0D ? ? ? ? 59 C3 90 90 "
                "51 E8 ? ? ? ? 89 44 24 00 DB 44 24 00 DB 44 24 08 D8 0D ? ? ? ? D8 25 ? ? ? ? D8 05 ? ? ? ? DE C9 D8 0D ? ? ? ? 59 C3 90 90 E9");
            uintptr_t gsy = gsx + 0x30;
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
                    auto start = FunctionAt(signature);
                    StretchCalls(start, start + size, gsx, gsy);
                }
                for (auto signature : { "E8 ? ? ? ? 8B 0D ? ? ? ? 6B C9 1C D9 99", "E8 ? ? ? ? D9 5E FC D9 07 D8 0D ? ? ? ? E8 ? ? ? ? 50" })
                {
                    auto call = hook::pattern(signature);
                    injector::MakeCALL(call.get_first(), &GsStretchX, true);
                }

                GsXHook = safetyhook::create_inline(gsx, &GsX);
                GsYHook = safetyhook::create_inline(gsy, &GsY);
                SpriteHook = safetyhook::create_inline(FunctionAt("83 EC 20 53 55 56 8B 74 24 30 66 8B 4E 42"), &DrawSprite);
                MemoPictureHook = safetyhook::create_inline(FunctionAt("51 56 E8 ? ? ? ? 33 F6 E8 ? ? ? ? 83 F8 0B 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 8B 44 24 0C 50 E8"), &DrawMemoPicture);
            }

            // Mouse in the 4:3 interface area
            auto cursorWidth = hook::pattern("D8 05 ? ? ? ? D9 5C 24 04 E8 ? ? ? ? D8 0D ? ? ? ? 8B 4C 24 04 68 80 00 00 00 6A 01");
            auto cursorStore = hook::pattern("DB 44 24 00 D9 1D ? ? ? ? E8 ? ? ? ? 89 44 24 00 DB 44 24 00 D9 1D ? ? ? ? 83 C4 14 C3");
            fCursorX.SetAddress(*cursorWidth.get_first<float*>(2));
            struct CursorWidth
            {
                void operator()(injector::reg_pack&)
                {
                    float x = fCursorX;
                    _asm fadd dword ptr[x]
                    _asm fmul dword ptr[HudScale]
                }
            };
            injector::MakeInline<CursorWidth>(cursorWidth.get_first(0), cursorWidth.get_first(6));
            struct CursorStore
            {
                void operator()(injector::reg_pack& regs)
                {
                    fCursorX = float(*(int32_t*)&regs.eax) * HudScale;
                }
            };
            injector::MakeInline<CursorStore>(cursorStore.get_first(0), cursorStore.get_first(10));
            auto mouseX = hook::pattern("E8 ? ? ? ? 8B F0 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9");
            static injector::hook_back<int32_t(*)()> GetMouseX;
            GetMouseX.fun = injector::MakeCALL(mouseX.get_first(), static_cast<int32_t(*)()>([]() -> int32_t
            {
                return int32_t(float(GetMouseX.fun()) - MouseOffset);
            }), true).get();
            static SafetyHookInline MouseYHook;
            auto mouseY = injector::GetBranchDestination(mouseX.get_first(7)).get<void>();
            MouseYHook = safetyhook::create_inline(mouseY, static_cast<int32_t(__cdecl*)()>([]() -> int32_t
            {
                const int32_t y = MouseYHook.ccall<int32_t>();
                return Screen.Scale < 1.0f ? int32_t((y - 224.0f) / Screen.Scale + 224.0f) : y;
            }));
            auto mouseBounds = hook::pattern("E8 ? ? ? ? 8B C8 8B C6 C1 E0 09 99");
            injector::MakeCALL(mouseBounds.get_first(), static_cast<int32_t(*)()>([]() -> int32_t { return MouseBound; }), true);

            // Low health cross in the lower right corner of the screen
            auto health = hook::pattern("D9 59 E8 D9 C0 D8 48 F8 D9 59 EC 7C DE 6A 01");
            struct HealthCross
            {
                void operator()(injector::reg_pack& regs)
                {
                    float x = 0.0f;
                    _asm fstp dword ptr[x]
                    const float ratio = HudAspectRatioConstraint ? ClampHudAspectRatio(*HudAspectRatioConstraint, Screen.Aspect) : Screen.Aspect;
                    const float width = float(Screen.Height) * ratio;
                    const float offset = (float(Screen.Width) + width) * 0.5f - float(Screen.Width) * HudScale;
                    *(float*)(regs.ecx - 0x18) = x * HudScale + offset;
                    _asm fld st
                }
            };
            injector::MakeInline<HealthCross>(health.get_first(0));

            // Black inventory backdrop wide enough for any screen
            auto backdrop = hook::pattern("B8 00 01 00 00 BA 00 FF FF FF 8B E8");
            injector::WriteMemory(backdrop.get_first(1), 1024, true);
            injector::WriteMemory(backdrop.get_first(6), -1024, true);

            // Logos and credits pictures in the 4:3 area
            {
                auto logoLeft = hook::pattern("A1 ? ? ? ? 6A 05 C7 44 24 18");
                auto logoLeft2 = hook::pattern("C7 44 24 50 ? ? ? ? C7 44 24 58 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 64");
                auto logoRight = hook::pattern("D8 25 ? ? ? ? 8D 54 24 0C 52 6A 02");
                injector::WriteMemory<uint16_t>(logoRight.get_first(0), 0x05D9, true);   // fsub -> fld
                injector::WriteMemory(logoRight.get_first(2), &LogoRight, true);
                auto creditsLeft = hook::pattern("6A 05 C7 44 24 1C");
                auto creditsLeft2 = hook::pattern("C7 44 24 54 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 60 ? ? ? ? C7 44 24 68");
                auto creditsRight = hook::pattern("D8 25 ? ? ? ? 8D 4C 24 10 51 6A 02 D9 54 24 34");
                injector::WriteMemory<uint16_t>(creditsRight.get_first(0), 0x05D9, true);
                injector::WriteMemory(creditsRight.get_first(2), &LogoRight, true);
                LogoLeftEdges = { logoLeft.get_first(11), logoLeft2.get_first(4), creditsLeft.get_first(6), creditsLeft2.get_first(4) };
                RefreshGeometry();
            }

            // Movies
            auto movieRect = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 10 83 E8 00");
            auto videoSize = hook::pattern("8B 0D ? ? ? ? 8B 15 ? ? ? ? 68 ? ? ? ? 50 A1 ? ? ? ? 50 6A 00 6A 01 51 52");
            VideoSize.SetAddress(*videoSize.get_first<Size2D*>(8));
            SetMovieRect = reinterpret_cast<decltype(SetMovieRect)>(injector::GetBranchDestination(movieRect.get_first()).as_int());
            injector::MakeCALL(movieRect.get_first(), &MovieRect, true);

            // Half-pixel alignment of the final picture: every edge of the present quad at -0.5, not only the far ones
            auto presentQuad = hook::pattern("89 15 ? ? ? ? A3 ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ?");
            fPresentQuadRight.SetAddress(*presentQuad.get_first<float*>(2));                      // v2.x
            aPresentQuad.SetAddress(*presentQuad.get_first<std::array<PresentVertex, 4>*>(7));     // v0.x, stride 24 bytes
            fPresentQuadBottom.SetAddress(*presentQuad.get_first<float*>(13));                     // v3.y
            fPresentQuadBottom2.SetAddress(*presentQuad.get_first<float*>(19));                    // v2.y
            struct PresentQuad
            {
                void operator()(injector::reg_pack&)
                {
                    float w = float(RenderTargetSize->w) - 0.5f, h = float(RenderTargetSize->h) - 0.5f;
                    auto& quad = *aPresentQuad;
                    quad[0].x = w;   quad[0].y = -0.5f;                     // v0
                    quad[1].x = -0.5f; quad[1].y = -0.5f;                   // v1
                    fPresentQuadRight = w;    fPresentQuadBottom2 = h;      // v2
                    quad[3].x = -0.5f; fPresentQuadBottom = h;              // v3
                    float temp = 0.0f;
                    __asm fstp temp
                }
            };
            injector::MakeInline<PresentQuad>(presentQuad.get_first(0), presentQuad.get_first(23));
            // The frame blend quad: its far edges like the present quad (its near edges are static zeros)
            auto blendX = hook::pattern("D9 15 ? ? ? ? D9 1D ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ? DB");
            auto blendY = hook::pattern("D9 15 ? ? ? ? D9 1D ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ? 8B 10");
            for (int i = 0; i < 4; ++i)
            {
                aBlendQuadX[i].SetAddress(*blendX.get_first<float*>(2 + 6 * i));
                aBlendQuadY[i].SetAddress(*blendY.get_first<float*>(2 + 6 * i));
            }
            struct BlendX
            {
                void operator()(injector::reg_pack&)
                {
                    for (auto& x : aBlendQuadX)
                        x = float(RenderTargetSize->w) - 0.5f;
                    float temp = 0.0f;
                    __asm fstp temp
                    __asm fstp temp
                }
            };
            struct BlendY
            {
                void operator()(injector::reg_pack&)
                {
                    for (auto& y : aBlendQuadY)
                        y = float(RenderTargetSize->h) - 0.5f;
                    float temp = 0.0f;
                    __asm fstp temp
                    __asm fstp temp
                }
            };
            injector::MakeInline<BlendX>(blendX.get_first(0), blendX.get_first(24));
            injector::MakeInline<BlendY>(blendY.get_first(0), blendY.get_first(24));
        };
    }
} Widescreen;
