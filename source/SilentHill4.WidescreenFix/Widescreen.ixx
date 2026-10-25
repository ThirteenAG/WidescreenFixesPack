module;

#include <stdafx.h>
#include <intrin.h>
#include <minidx8/d3d8.h>
#include "FixSettings.hpp"
#include "../Shared/SilentHill/Display.hpp"

export module Widescreen;

// Presentation at any resolution and aspect ratio.
//
// The game draws all 2D through vertex shaders whose constants map the PS2's 512x448 screen (and a 640x480 space
// for text and screen effects) to the screen. The released fix squeezed those constants once, so everything 2D
// became 4:3 - including effects that are positioned from the 3D view or sample the whole screen, which it then
// had to switch off (the darkness ribbons, decals and blood stains) or widen by guessing. Here each batch of 2D
// gets the right constants when it is drawn: the interface, text, menus and pictures stay in a centred 4:3 area,
// while world-anchored effects, screen-copy effects (blur, depth of field, glow, film, map zoom), full-screen fades
// and backdrops cover the whole screen, and masks that covered the PS2 screen (peep holes, the door scope) get black
// sides, so the world doesn't show beside them (#556). The mouse hit areas follow the 4:3 menus (#554).
//
// The 3D view widens (Hor+). In cutscenes the PS2's cinematic bars hid part of the picture: with the bars on they
// are drawn full width as on the PS2, with them off the picture is enlarged so nothing the bars hid is shown.
// Movies keep their shape or fill the screen.

namespace
{
    struct
    {
        int32_t Width = 0, Height = 0;
        float Aspect = 4.0f / 3.0f;
        float Scale = 1.0f;     // aspect / (4:3)
    } Screen;

    const int* CinematicBars = nullptr;
    const float* FOVFactor = nullptr;
    std::optional<float> HudAspectRatioConstraint;
    void* HealthSprite = nullptr;
    SafetyHookInline HealthPositionHook;

    void* __cdecl HealthPosition(void* sprite, float x, float y, float width, float height)
    {
        if (sprite == HealthSprite)
        {
            const float ratio = HudAspectRatioConstraint ? ClampHudAspectRatio(*HudAspectRatioConstraint, Screen.Aspect) : Screen.Aspect;
            // The health indicator is left-anchored in the native 512-wide interface space.
            x -= 256.0f * (ratio / (4.0f / 3.0f) - 1.0f);
        }
        return HealthPositionHook.ccall<void*>(sprite, x, y, width, height);
    }

    float* GameplayIntent = nullptr;
    uint8_t* PlayerStatus = nullptr;
    int32_t* GameplayMode = nullptr;
    const int* Movies = nullptr;

    // 2D vertex shader constants: c2/c3 for the 512x448 space, c94/c95 for the 640x480 space
    struct Constants
    {
        float c2[4], c3[4], c94[4], c95[4];
    } Native, Interface;
    bool HaveConstants = false;
    bool InterfaceSet = false;

    IDirect3DDevice8* Device()
    {
        return SHDisplay::Get().device;
    }

    void Apply(const Constants& set)
    {
        auto device = Device();
        if (!device)
            return;
        device->SetVertexShaderConstant(2, set.c2, 1);
        device->SetVertexShaderConstant(3, set.c3, 1);
        device->SetVertexShaderConstant(94, set.c94, 1);
        device->SetVertexShaderConstant(95, set.c95, 1);
        InterfaceSet = &set == &Interface;
    }

    // The game's own constants, read back once it set them; the interface set squeezes x into the 4:3 area
    SafetyHookInline ConstantsHook;
    void __cdecl SetConstants()
    {
        ConstantsHook.ccall<void>();
        auto device = Device();
        if (!device)
            return;
        device->GetVertexShaderConstant(2, Native.c2, 1);
        device->GetVertexShaderConstant(3, Native.c3, 1);
        device->GetVertexShaderConstant(94, Native.c94, 1);
        device->GetVertexShaderConstant(95, Native.c95, 1);
        Interface = Native;
        float s = 1.0f / Screen.Scale;
        Interface.c2[0] = Native.c2[0] * s;
        Interface.c3[0] = Interface.c2[0] * -2048.0f;
        Interface.c94[0] = Native.c94[0] * s;
        Interface.c95[0] = Native.c95[0] * s;
        HaveConstants = true;
        Apply(Interface);
    }

    // Batches drawn with the native (full screen) constants, by the game's call site
    std::vector<uintptr_t> ScreenCallSites;
    std::vector<IDirect3DBaseTexture8**> ScreenTextures;
    IDirect3DSurface8* BackBuffer = nullptr;

    bool RenderingToScreenTexture()
    {
        auto device = Device();
        IDirect3DSurface8* target = nullptr;
        if (!device || FAILED(device->GetRenderTarget(&target)) || !target)
            return false;
        target->Release();
        return BackBuffer && target != BackBuffer;
    }

    // The flush's two vertex batches: {prim, used, count} headers 12 bytes apart, 280000 bytes of 28 byte vertices
    // {x, y, z, w, colour, u, v} each
    uint8_t* BatchCounts = nullptr;
    uint8_t* BatchVertices = nullptr;
    uintptr_t LensSite = 0;

    // The door scope's lens shows the screen copy: the 4:3 lens samples the middle of the widened picture
    void FixLensUVs()
    {
        if (!BatchCounts || Screen.Scale <= 1.0f)
            return;
        for (int batch = 0; batch < 2; ++batch)
        {
            auto count = *reinterpret_cast<uint32_t*>(BatchCounts + batch * 12);
            auto vertex = reinterpret_cast<float*>(BatchVertices + batch * 280000);
            if (!count)
                continue;
            float centre = vertex[5];       // a fan around the lens centre
            for (uint32_t i = 0; i < count; ++i, vertex += 7)
                vertex[5] = centre + (vertex[5] - centre) / Screen.Scale;
        }
    }

    SafetyHookInline FlushHook;
    void __cdecl Flush(int a0, int a1, int a2, int a3, int a4, int a5, int a6, IDirect3DBaseTexture8* texture, int a8, int a9,
        IDirect3DBaseTexture8* texture2, int a11, int a12, int mode)
    {
        auto site = reinterpret_cast<uintptr_t>(_ReturnAddress());
        uint8_t space = uint8_t(mode);
        bool screen = false;
        if (HaveConstants && space != 3)
        {
            screen = space == 2 || std::find(ScreenCallSites.begin(), ScreenCallSites.end(), site) != ScreenCallSites.end();
            if (!screen && space == 1)
            {
                for (auto slot : ScreenTextures)
                {
                    if (*slot && (*slot == texture || *slot == texture2))
                    {
                        screen = true;
                        break;
                    }
                }
            }
            if (!screen)
                screen = RenderingToScreenTexture();
        }
        if (screen)
            Apply(Native);
        else if (HaveConstants && !InterfaceSet)
            Apply(Interface);
        if (site == LensSite)
            FixLensUVs();
        FlushHook.ccall<void>(a0, a1, a2, a3, a4, a5, a6, texture, a8, a9, texture2, a11, a12, mode);
        if (screen)
            Apply(Interface);
    }

    // Masks that hid the whole PS2 screen (the wall hole, the door scope, the Water Prison peep holes) are 4:3
    // sprites drawn outside the flush: the sides are cleared to black after them (#556)
    void BlackSides()
    {
        auto device = Device();
        if (!device || !BackBuffer || Screen.Scale <= 1.0f || RenderingToScreenTexture())
            return;
        D3DSURFACE_DESC desc = {};
        BackBuffer->GetDesc(&desc);
        LONG w = LONG(desc.Width), h = LONG(desc.Height);
        LONG side = LONG(std::ceil((float(w) - float(w) / Screen.Scale) * 0.5f));
        if (side <= 0)
            return;
        D3DRECT rects[2] = { { 0, 0, side, h }, { w - side, 0, w, h } };
        D3DVIEWPORT8 viewport = {}, full = { 0, 0, DWORD(w), DWORD(h), 0.0f, 1.0f };
        device->GetViewport(&viewport);
        device->SetViewport(&full);
        device->Clear(2, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        device->SetViewport(&viewport);
    }

    SafetyHookInline PeepHook, PrisonPeepHook;
    int __cdecl Peep(int hole)
    {
        int result = PeepHook.ccall<int>(hole);
        BlackSides();
        return result;
    }
    int __cdecl PrisonPeep(uint8_t* object)
    {
        int result = PrisonPeepHook.ccall<int>(object);
        if (*reinterpret_cast<void**>(object + 0x408))
            BlackSides();
        return result;
    }

    // Cinematic bars: sprites drawn full width
    void(__cdecl* DrawCinemaBars)() = nullptr;
    void __cdecl CinemaBars()
    {
        if (HaveConstants)
            Apply(Native);
        DrawCinemaBars();
        if (HaveConstants)
            Apply(Interface);
    }

    // Cutscene framing: the camera's zoom
    uint8_t* RenderCamera = nullptr;    // +0x44 zoom, +0x280 dirty flags
    int32_t* SceneFlags = nullptr;      // bit 0: cutscene
    constexpr float VisibleWithBars = 329.0f / 448.0f;

    void SetZoom(float zoom)
    {
        auto& current = *reinterpret_cast<float*>(RenderCamera + 0x44);
        if (current != zoom)
        {
            current = zoom;
            *reinterpret_cast<uint32_t*>(RenderCamera + 0x280) |= 1;
        }
    }

    // Movies filling the screen
    void(__cdecl* Quad2D)(float, float, float, float, float, float, float, float, int, int) = nullptr;
    void __cdecl MovieQuad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, int top, int bottom)
    {
        if (*Movies == 2)
        {
            x0 = 320.0f + (x0 - 320.0f) * Screen.Scale;
            x1 = 320.0f + (x1 - 320.0f) * Screen.Scale;
        }
        else if (*Movies == 1 && Screen.Scale > 1.0f)
        {
            // 640x480 space, the interface constants squeeze x by 1/scale: widen to the screen, crop top and bottom
            float extraX = (640.0f * Screen.Scale - 640.0f) * 0.5f;
            float extraY = (480.0f * Screen.Scale - 480.0f) * 0.5f;
            x0 -= extraX; x1 += extraX; y0 -= extraY; y1 += extraY;
        }
        Quad2D(x0, y0, x1, y1, u0, v0, u1, v1, top, bottom);
    }

    // Mouse: menus are drawn in the 4:3 area
    SafetyHookInline CursorHook, SetCursorHook, HitRectFHook, HitRectIHook, PreviousPageHook, NextPageHook;
    void(__cdecl* GetCursor)(float*, float*) = nullptr;

    // Menus are squeezed into the 4:3 area: a menu x maps to 256 + (x - 256) / scale across the screen's 512
    float ToInterface(float x) { return 256.0f + (x - 256.0f) * Screen.Scale; }
    float FromInterface(float x) { return 256.0f + (x - 256.0f) / Screen.Scale; }

    void __cdecl MenuCursor(float* x, float* y)
    {
        CursorHook.ccall<void>(x, y);
        if (x)
            *x = ToInterface(*x);
    }

    void __cdecl SetMenuCursor(double x, double y)
    {
        SetCursorHook.ccall<void>(double(FromInterface(float(x))), y);
    }

    int __cdecl HitRectF(float* rect)
    {
        float r[4] = { FromInterface(rect[0]), rect[1], rect[2] / Screen.Scale, rect[3] };
        return HitRectFHook.ccall<int>(r);
    }

    int __cdecl HitRectI(int32_t* rect)
    {
        int32_t r[4] = { int32_t(std::lround(FromInterface(float(rect[0])))), rect[1], int32_t(std::lround(rect[2] / Screen.Scale)), rect[3] };
        return HitRectIHook.ccall<int>(r);
    }

    bool CursorIn(float x0, float y0, float w, float h)
    {
        float x = 0, y = 0;
        MenuCursor(&x, &y);
        return x >= x0 && x < x0 + w && y >= y0 && y < y0 + h;
    }
}

namespace
{
    // The Resolution row of the options shows the real resolution: its six value messages become one of ours
    constexpr int32_t ResolutionMessage = 0x7F00;
    SafetyHookInline MessageHook;
    const uint16_t* __cdecl Message(int bank, int id)
    {
        if (bank == 0 && id == ResolutionMessage)
        {
            static std::vector<uint16_t> text;
            if (text.empty())
            {
                // Longer than the native "640x480": tighten the letters (0xFCnn moves back n units) to stay clear of the arrow
                auto value = std::format("{}x{}", Screen.Width, Screen.Height);
                for (size_t i = 0; i < value.size(); ++i)
                {
                    if (i && value.size() > 7)
                        text.push_back(0xFC02);
                    text.push_back(uint16_t(256 - uint8_t(value[i])));
                }
                text.push_back(0xFFFF);
                text.push_back(0x8000);
            }
            return text.data();
        }
        return MessageHook.ccall<const uint16_t*>(bank, id);
    }

    void ShowResolution()
    {
        auto lookup = hook::pattern("56 8B 74 24 08 85 F6 7D 06 8B 44 24 0C");
        if (lookup.size() != 1)
            return;
        auto module = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(module + reinterpret_cast<IMAGE_DOS_HEADER*>(module)->e_lfanew);
        auto section = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
        {
            if ((section->Characteristics & IMAGE_SCN_MEM_EXECUTE) || !(section->Characteristics & IMAGE_SCN_MEM_WRITE))
                continue;
            auto begin = reinterpret_cast<uint32_t*>(module + section->VirtualAddress);
            auto end = begin + section->Misc.VirtualSize / 4 - 7;
            for (auto row = begin; row < end; ++row)
            {
                // {activate, get, flash, values, count 6, label "Resolution", help}
                if (row[2] == 0 && row[4] == 6 && row[5] == 0x71 && row[3] > uintptr_t(module) && row[3] < uintptr_t(module) + nt->OptionalHeader.SizeOfImage)
                {
                    auto values = reinterpret_cast<uint32_t*>(row[3]);
                    if (values[1] != 0x72)
                        continue;
                    for (int v = 0; v < 6; ++v)
                        injector::WriteMemory(&values[v], ResolutionMessage, true);
                    MessageHook = safetyhook::create_inline(lookup.get_first(), &Message);
                    return;
                }
            }
        }
    }
}

export namespace Widescreen
{
    void Init()
    {
        CinematicBars = &FixSettings().Int("MAIN.CutsceneBorders");
        FOVFactor = &FixSettings().Float("MAIN.FOVFactor");
        auto updateHUD = []()
        {
            HudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        };
        updateHUD();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", updateHUD);
        auto health = hook::pattern("57 33 C0 B9 26 00 00 00 BF ? ? ? ? F3 AB 6A 02 68 ? ? ? ? A3 ? ? ? ? A2");
        auto position = hook::pattern("8B 44 24 04 85 C0 74 1C 8B 4C 24 08 8B 54 24 0C 89 48 30 8B 4C 24 10 89 50 34 8B 54 24 14 89 48 38 89 50 3C C3");
        if (health.size() == 1 && position.size() == 1)
        {
            HealthSprite = *health.get_first<void*>(9);
            HealthPositionHook = safetyhook::create_inline(position.get_first(), HealthPosition);
        }
        else
            OutputDebugStringA("Silent Hill 4: health HUD position patterns unavailable.\n");

        auto intent = hook::pattern("B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 8B 15");
        auto status = hook::pattern("55 8B EC 83 E4 F0 83 EC 14 8B 0D ? ? ? ? A1 ? ? ? ? 8B 15 ? ? ? ? 53 56 89 0D");
        auto mode = hook::pattern("A1 ? ? ? ? 8B 34 85 ? ? ? ? 33 FF");
        if (intent.size() == 1 && status.size() == 1 && mode.size() == 1)
        {
            GameplayIntent = *intent.get_first<float*>(1);
            PlayerStatus = *status.get_first<uint8_t*>(0x10);
            GameplayMode = *mode.get_first<int32_t*>(1);
        }
        else
            OutputDebugStringA("SilentHill4: gameplay FOV state patterns unavailable.\n");
        Movies = &FixSettings().Int("MAIN.FMVWidescreenMode");
        SHDisplay::OnPresent().push_back([]()
        {
            auto device = SHDisplay::Get().device;
            if (!device || !(SceneFlags && (*SceneFlags & 1)) || !(*CinematicBars & 2))
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

        Screen.Width = FixSettings().Int("MAIN.ResX");
        Screen.Height = FixSettings().Int("MAIN.ResY");
        if (FixSettings().Int("MAIN.WindowMode") == 2 || Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
            std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
        if (Screen.Width <= 0 || Screen.Height <= 0)
            std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
        Screen.Aspect = float(Screen.Width) / float(Screen.Height);
        Screen.Scale = Screen.Aspect / (4.0f / 3.0f);

        // Resolution: the game's six resolutions all become the screen's
        auto size = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 6A 00 50 51 E8");
        auto height = hook::pattern("8B 0D ? ? ? ? 89 44 24 1C 39");
        if (size.size() == 1 && height.size() == 1)
        {
            auto width = *size.get_first<int32_t*>(7);
            auto heightPtr = *height.get_first<int32_t*>(2);
            for (auto [address, value] : { std::pair{ width, Screen.Width }, std::pair{ heightPtr, Screen.Height } })
            {
                auto writes = hook::pattern(pattern_str(0xC7, 0x05, to_bytes(address)));
                for (size_t i = 0; i < writes.size(); ++i)
                    injector::WriteMemory(writes.get(i).get<void>(6), value, true);
                *address = value;
            }
        }

        // Hor+: the vertical field of view stays, the horizontal one widens
        auto fov = hook::pattern("83 C4 14 68 ? ? ? ? E8 ? ? ? ? 68");
        auto aspect = hook::pattern("C7 46 48 ? ? ? ? C7 46 44");
        if (fov.size() == 1 && aspect.size() == 1)
        {
            RenderCamera = *fov.get_first<uint8_t*>(4);
            injector::MakeNOP(fov.get_first(8), 5, true);
            injector::WriteMemory<float>(fov.get_first(14), Screen.Scale, true);
            injector::WriteMemory<float>(aspect.get_first(3), 0.78125f * Screen.Scale, true);
        }

        // Cutscene framing, every frame where the game applies its camera
        auto sceneFlags = hook::pattern("A1 ? ? ? ? 83 E0 01 C3 90 90 90 90 90 90 90 55 8B EC 83 EC 1C");
        auto eventCamera = hook::pattern("55 8B EC 83 E4 F0 83 EC 60 8D 44 24 00 50 E8");
        auto gameCamera = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 10 E9");
        if (RenderCamera && sceneFlags.size() == 1 && eventCamera.size() == 1 && gameCamera.size() == 1)
        {
            SceneFlags = *sceneFlags.get_first<int32_t*>(1);
            static SafetyHookMid EventHook = safetyhook::create_mid(eventCamera.get_first(), [](SafetyHookContext&)
            {
                // Without the bars the picture is enlarged until nothing they hid is visible
                float zoom = Screen.Scale;
                if ((*SceneFlags & 1) && !(*CinematicBars & 1))
                    zoom *= (*CinematicBars & 2) ? 1.0f / VisibleWithBars : std::max(Screen.Scale, 1.0f / VisibleWithBars);
                SetZoom(zoom);
            });
            static SafetyHookMid GameHook = safetyhook::create_mid(gameCamera.get_first(), [](SafetyHookContext&)
            {
                float factor = 1.0f;
                if (GameplayIntent && PlayerStatus && GameplayMode && *GameplayMode == 0 && !(*SceneFlags & 1))
                {
                    const int state = *reinterpret_cast<int32_t*>(PlayerStatus + 0xC4);
                    const bool aiming = state == 3 || state == 4 || state == 5 || GameplayIntent[0x50 / 4] != 0.0f;
                    if (!aiming && std::isfinite(*FOVFactor))
                        factor = std::clamp(*FOVFactor, 0.5f, 2.0f);
                }
                SetZoom(Screen.Scale / factor);
            });
        }

        // The native cinematic bars on or off
        auto barFlags = hook::pattern("A1 ? ? ? ? 85 C0 74 ? 6A 02 68");
        if (barFlags.size() == 2)
        {
            static int32_t* Flags[2] = { *barFlags.get(0).get<int32_t*>(1), *barFlags.get(1).get<int32_t*>(1) };
            static int32_t Gates[2] = {};
            for (int i = 0; i < 2; ++i)
                injector::WriteMemory(barFlags.get(i).get<void>(1), &Gates[i], true);
            SHDisplay::OnPresent().push_back([]()
            {
                for (int i = 0; i < 2; ++i)
                    Gates[i] = (*CinematicBars & 1) ? *Flags[i] : 0;
            });
        }
        auto bars = hook::pattern("E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? E8 ? ? ? ? 6A 01 E8");
        if (bars.size() == 1)
        {
            DrawCinemaBars = reinterpret_cast<decltype(DrawCinemaBars)>(injector::GetBranchDestination(bars.get_first()).as_int());
            injector::MakeCALL(bars.get_first(), &CinemaBars, true);
        }

        // 2D constants per batch
        auto constants = hook::pattern("83 EC 48 E8 ? ? ? ? D9 5C 24 00 E8 ? ? ? ? D9 5C 24 04 E8");
        auto flush = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 03 C1 83 EC 10 85 C0 75 04 83 C4 10 C3");
        if (constants.size() == 1 && flush.size() == 1)
        {
            ConstantsHook = safetyhook::create_inline(constants.get_first(), &SetConstants);
            FlushHook = safetyhook::create_inline(flush.get_first(), &Flush);
            SHDisplay::OnBeforeReset().push_back([](IDirect3DDevice8*)
            {
                if (BackBuffer)
                {
                    BackBuffer->Release();
                    BackBuffer = nullptr;
                }
                HaveConstants = false;
            });
            SHDisplay::OnDeviceReady().push_back([](IDirect3DDevice8* device)
            {
                if (BackBuffer)
                    BackBuffer->Release();
                BackBuffer = nullptr;
                device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &BackBuffer);
            });

            // Screen-sized textures: the screen copy and the game's render targets
            auto copy = hook::pattern("83 EC 14 A1 ? ? ? ? 8B 0D ? ? ? ? 8D 54 24 00 52 89 44 24 10 A1");
            if (copy.size() == 1)
                ScreenTextures.push_back(*copy.get_first<IDirect3DBaseTexture8**>(0x18));
            auto targets = hook::pattern("8B 44 24 04 85 C0 7C 0D 83 F8 02 7D 08 8B 04 85");
            if (targets.size() == 1)
            {
                auto first = *targets.get_first<IDirect3DBaseTexture8**>(0x10);
                for (int i : { 0, 1, 11, 12, 13, 14 })      // 6E8E44, 6E8E48, 6E8E70 .. 6E8E7C
                    ScreenTextures.push_back(first + i);
            }

            // World-anchored effects and full-screen backdrops, by their call into the batch flush
            static const std::pair<const char*, int> Sites[] =
            {
                { "E8 ? ? ? ? 83 C4 5C C6 05 ? ? ? ? 00 B8", 0 },                    // darkness object: cap fan
                { "E8 ? ? ? ? 83 C4 5C C6 05 ? ? ? ? 00 A1", 0 },                    // world decals
                { "50 51 52 C6 05 ? ? ? ? 01 E8", 10 },                              // blood stains
            };
            for (auto [signature, offset] : Sites)
            {
                auto site = hook::pattern(signature);
                if (site.size() == 1)
                    ScreenCallSites.push_back(reinterpret_cast<uintptr_t>(site.get_first(offset + 5)));
            }
            // Calls into the flush inside whole functions: darkness ribbons, light halos, the light grid, the
            // 512x448 fade, and the document viewer's black backdrop (its first batch; the page itself stays 4:3)
            auto flushAddress = reinterpret_cast<uintptr_t>(flush.get_first());
            auto addCalls = [&](const char* signature, size_t size, int count)
            {
                auto start = hook::pattern(signature);
                if (start.size() != 1)
                    return;
                auto from = reinterpret_cast<uintptr_t>(start.get_first());
                auto calls = hook::range_pattern(from, from + size, "E8 ? ? ? ?");
                int found = 0;
                for (size_t i = 0; i < calls.size() && found < count; ++i)
                {
                    auto at = calls.get(i).get<uint8_t>(0);
                    if (injector::GetBranchDestination(at, true).as_int() != flushAddress)
                        continue;
                    ScreenCallSites.push_back(reinterpret_cast<uintptr_t>(at + 5));
                    ++found;
                }
            };
            addCalls("55 8B EC 83 E4 F0 83 EC 64 D9 45 18", 0x377, 8);                                          // darkness ribbons
            addCalls("55 8B EC 83 E4 F0 81 EC 34 01 00 00 A0 ? ? ? ? 84 C0 53 56 57 0F 85", 0x6E4, 8);           // light halos
            addCalls("55 8B EC 83 E4 F0 81 EC A4 00 00 00 8B 45 0C 99 B9 03 00 00 00 F7 F9", 0x2E7, 8);          // light halos
            addCalls("55 8B EC 83 E4 F0 83 EC 44 53 56 33", 0x281, 8);                                         // light grid
            addCalls("E8 ? ? ? ? 8B 44 24 0C 50 50 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F E8 ? ? ? ? 89 44 24 24", 0x81, 8);  // fade
            addCalls("8B 44 24 04 0F B7 80 10 04 00 00 83 EC 10 83 F8 06 0F 8C", 0x2CA, 1);                    // document backdrop

            // The wobbling grid behind the Options and the other in-game menus covers the whole screen
            auto backdrop = hook::pattern("6A 01 6A 00 68 0D 00 03 00 6A 00 68 5B 01 00 00 6A 00 6A 44 E8");
            if (backdrop.size() == 1)
                ScreenCallSites.push_back(reinterpret_cast<uintptr_t>(backdrop.get_first(25)));

            // Peep holes and the door scope
            BatchCounts = *flush.get_first<uint8_t*>(7);
            BatchVertices = BatchCounts + 0x18;
            auto lens = hook::pattern("68 01 10 03 00 6A 00 68 14 01 00 00 6A 00 68 88 00 00 00 E8");
            if (lens.size() == 1)
                LensSite = reinterpret_cast<uintptr_t>(lens.get_first(24));
            auto peep = hook::pattern("E8 ? ? ? ? 85 C0 74 24 D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1D ? ? ? ? 56 8B 74 24 08 85 F6 75 05");
            if (peep.size() == 1)
                PeepHook = safetyhook::create_inline(peep.get_first(), &Peep);
            auto prison = hook::pattern("55 8B EC 83 E4 F0 83 EC 74 8B 45 08 53 56 57 8B B8 08 04 00 00");
            if (prison.size() == 1)
                PrisonPeepHook = safetyhook::create_inline(prison.get_first(), &PrisonPeep);
        }

        // Movies
        auto movie = hook::pattern("E8 ? ? ? ? 8B 14 B5 ? ? ? ? A1");
        if (movie.size() == 1)
        {
            Quad2D = reinterpret_cast<decltype(Quad2D)>(injector::GetBranchDestination(movie.get_first()).as_int());
            injector::MakeCALL(movie.get_first(), &MovieQuad, true);
        }

        // Mouse
        auto cursor = hook::pattern("83 EC 18 DB 05 ? ? ? ? 8B 0D ? ? ? ? 8D 44 24 08 50 D9 5C 24 04");
        auto setCursor = hook::pattern("83 EC 18 8B 0D ? ? ? ? 33 C0 56 89 44 24 04 89 44 24 08");
        auto hitF = hook::pattern("83 EC 10 8B 0D ? ? ? ? 53 55 56 57 8D 44 24 10 50 51 FF 15");
        auto hitI = hook::pattern("83 EC 20 8B 0D ? ? ? ? 53 55 56 57 8D 44 24 10 50 51 FF 15");
        auto pages = hook::pattern("83 EC 14 8B 0D ? ? ? ? 53 55 56 57 8D 44 24 14 50 51 FF 15 ? ? ? ? DB 44 24 1C D9 05 ? ? ? ? D8 C9");
        if (cursor.size() == 1 && setCursor.size() == 1 && hitF.size() == 1 && hitI.size() == 1 && pages.size() == 2)
        {
            CursorHook = safetyhook::create_inline(cursor.get_first(), &MenuCursor);
            SetCursorHook = safetyhook::create_inline(setCursor.get_first(), &SetMenuCursor);
            HitRectFHook = safetyhook::create_inline(hitF.get_first(), &HitRectF);
            HitRectIHook = safetyhook::create_inline(hitI.get_first(), &HitRectI);
            // The options page arrows, in the 512x448 space
            PreviousPageHook = safetyhook::create_inline(pages.get(0).get<void>(0), static_cast<int(__cdecl*)()>([]() -> int
            {
                return CursorIn(33.6f, 39.2f, 48.0f, 28.0f) ? 1 : 0;
            }));
            NextPageHook = safetyhook::create_inline(pages.get(1).get<void>(0), static_cast<int(__cdecl*)()>([]() -> int
            {
                return CursorIn(429.6f, 39.2f, 48.0f, 28.0f) ? 1 : 0;
            }));
        }

        ShowResolution();

        // Soft shadow blur offsets in pixels: scaled with the resolution
        auto taps = hook::pattern("8B 0D ? ? ? ? 05 A8 00 00 00 89 44 24 20 89 08 D9 05");
        if (taps.size() == 4)
        {
            float k = std::max(1.0f, float(Screen.Height) / 480.0f);
            for (size_t i = 0; i < 4; ++i)
            {
                auto x = *taps.get(i).get<float*>(2);
                auto y = *taps.get(i).get<float*>(0x13);
                injector::WriteMemory<float>(x, *x * k, true);
                injector::WriteMemory<float>(y, *y * k, true);
            }
        }
    }
}
