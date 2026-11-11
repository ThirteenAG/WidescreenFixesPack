module;

#include <stdafx.h>
#include <intrin.h>
#include <atomic>
#include <minidx8/d3d8.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"

export module Widescreen;

import ComVars;
import Camera;

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
// The 3D view widens (Hor+). Cutscene borders preserve the original shot; turning them off fills the screen Hor+.
// Movies keep their shape and are fitted whole into the game's 4:3 area or the whole screen.

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
    bool FramingCleared = false;
    float InterfaceX() { return 1.0f / std::max(1.0f, Screen.Scale); }
    float InterfaceY() { return std::min(1.0f, Screen.Scale); }
    std::atomic<float> HudAspectRatioConstraint{ 0.0f };
    std::array<void*, 4> HudSprites{};
    SafetyHookInline HudSpriteHook;
    SafetyHookMid HudCircleHook;

    float HudOffset()
    {
        const float requested = HudAspectRatioConstraint.load(std::memory_order_relaxed);
        const float ratio = requested > 0.0f ? ClampHudAspectRatio(requested, Screen.Aspect) : Screen.Aspect;
        return 256.0f * (std::max(1.0f, ratio / (4.0f / 3.0f)) - 1.0f);
    }

    uintptr_t __cdecl HudSprite(void* sprite, int mode)
    {
        if (!sprite || mode != 2 || std::find(HudSprites.begin(), HudSprites.end(), sprite) == HudSprites.end())
            return HudSpriteHook.ccall<uintptr_t>(sprite, mode);
        const float offset = HudOffset();
        if (offset == 0.0f)
            return HudSpriteHook.ccall<uintptr_t>(sprite, mode);
        auto x = reinterpret_cast<float*>(static_cast<uint8_t*>(sprite) + 0x30);
        const float original = *x;
        *x = original - offset;
        const auto result = HudSpriteHook.ccall<uintptr_t>(sprite, mode);
        *x = original;
        return result;
    }



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
        const float s = InterfaceX(), t = InterfaceY();
        Interface.c2[0] = Native.c2[0] * s;
        Interface.c3[0] = Native.c3[0] * s;
        Interface.c94[0] = Native.c94[0] * s;
        Interface.c95[0] = Native.c95[0] * s;
        Interface.c2[1] = Native.c2[1] * t;
        Interface.c3[1] = Native.c3[1] * t;
        Interface.c94[1] = Native.c94[1] * t;
        Interface.c95[1] = Native.c95[1] * t;
        HaveConstants = true;
        Apply(Interface);
    }

    // Batches drawn with the native (full screen) constants, by the game's call site
    std::vector<uintptr_t> ScreenCallSites;
    std::vector<IDirect3DBaseTexture8**> ScreenTextures;     // slots of pScreenCopyTexture and aRenderTargetTextures
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

    bool ClearOutside(float left, float top, float right, float bottom)
    {
        auto device = Device();
        if (!device || !BackBuffer || RenderingToScreenTexture())
            return false;
        D3DSURFACE_DESC surface{};
        if (FAILED(BackBuffer->GetDesc(&surface)))
            return false;
        const LONG width = LONG(surface.Width), height = LONG(surface.Height);
        const LONG x0 = std::clamp(LONG(std::ceil(left)), 0L, width);
        const LONG x1 = std::clamp(LONG(std::floor(right)), 0L, width);
        const LONG y0 = std::clamp(LONG(std::ceil(top)), 0L, height);
        const LONG y1 = std::clamp(LONG(std::floor(bottom)), 0L, height);
        D3DRECT borders[] = { { 0, 0, x0, height }, { x1, 0, width, height },
            { 0, 0, width, y0 }, { 0, y1, width, height } };
        D3DVIEWPORT8 previous{}, full{ 0, 0, DWORD(width), DWORD(height), 0.0f, 1.0f };
        if (FAILED(device->GetViewport(&previous)))
            return false;
        if (FAILED(device->SetViewport(&full)))
            return false;
        for (const auto& border : borders)
        {
            if (border.x2 > border.x1 && border.y2 > border.y1)
                device->Clear(1, &border, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        }
        device->SetViewport(&previous);
        return true;
    }

    bool ClearBox(float visible)
    {
        const auto& display = SHDisplay::Get();
        const float width = float(display.backBufferWidth), height = float(display.backBufferHeight);
        const float boxWidth = std::min(width, height * (4.0f / 3.0f));
        const float boxHeight = boxWidth * 0.75f * visible;
        return ClearOutside((width - boxWidth) * 0.5f, (height - boxHeight) * 0.5f,
            (width + boxWidth) * 0.5f, (height + boxHeight) * 0.5f);
    }

    void ClearFraming();

    // The flush's two vertex batches: {prim, used, count} headers 12 bytes apart, 280000 bytes of 28 byte vertices
    // {x, y, z, w, colour, u, v} each

    uintptr_t LensSite = 0;

    // The door scope's lens shows the screen copy: the 4:3 lens samples the middle of the widened picture
    void FixLensUVs()
    {
        if (!aFlushBatchCounts.is_initialized() || Screen.Scale <= 1.0f)
            return;
        for (int batch = 0; batch < 2; ++batch)
        {
            auto count = (*aFlushBatchCounts)[batch].count;
            auto vertex = (*aFlushBatchVertices)[batch].data();
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
        if (!screen)
            ClearFraming();
        FlushHook.ccall<void>(a0, a1, a2, a3, a4, a5, a6, texture, a8, a9, texture2, a11, a12, mode);
        if (screen)
            Apply(Interface);
    }

    // Masks that hid the whole PS2 screen (the wall hole, the door scope, the Water Prison peep holes) are 4:3
    // sprites drawn outside the flush: the sides are cleared to black after them (#556)
    void BlackSides()
    {
        ClearBox(1.0f);
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
        ClearFraming();
        if (HaveConstants)
            Apply(Native);
        DrawCinemaBars();
        if (HaveConstants)
            Apply(Interface);
    }

    // Cutscene framing: the camera's zoom
    SafetyHookInline ProjectionHook;
    constexpr float VisibleWithBars = 329.0f / 448.0f;

    void ClearFraming()
    {
        if (FramingCleared || !(nSceneFlags & 1) || !CinematicBars)
            return;
        const int mode = std::clamp(*CinematicBars, 0, 3);
        if (!mode)
            return;
        const auto& display = SHDisplay::Get();
        const float width = float(display.backBufferWidth), height = float(display.backBufferHeight);
        const float boxWidth = std::min(width, height * (4.0f / 3.0f));
        const float visible = (mode & 1) ? VisibleWithBars : 1.0f;
        const float side = (mode & 2) ? std::max(0.0f, (width - boxWidth) * 0.5f) : 0.0f;
        const float top = std::max(0.0f, (height - boxWidth * 0.75f * visible) * 0.5f);
        FramingCleared = ClearOutside(side, top, width - side, height - top);
    }

    void SetZoom(float zoom)
    {
        if (RenderCamera->fZoom != zoom)
        {
            RenderCamera->fZoom = zoom;
            RenderCamera->nFlags |= 1;
        }
    }

    float BaseZoom()
    {
        const bool contained = (nSceneFlags & 1) && CinematicBars && *CinematicBars;
        return contained ? 1.0f / std::max(1.0f, Screen.Scale) : 1.0f / Screen.Scale;
    }

    float GameplayZoom()
    {
        const float base = BaseZoom();
        if (!IsGameplayView() || nGameMode != 0)
            return base;
        const int state = *reinterpret_cast<int32_t*>(aPlayerStatus->data() + 0xC4);
        if (state == 3 || state == 4 || state == 5 || (*aMoveIntent)[0x50 / 4] != 0.0f)
            return base;
        const float factor = std::isfinite(*FOVFactor) ? std::clamp(*FOVFactor, 0.5f, 2.0f) : 1.0f;
        if (factor == 1.0f)
            return base;
        const float angle = RenderCamera->fAngle;
        const float tangent = std::tan(angle * 0.5f);
        if (!std::isfinite(tangent) || tangent <= 0.0f)
            return base;
        // The native perspective builder uses m00 = zoom / tan(horizontal / 2).
        constexpr float tan75 = 3.7320508075688772f;
        const float maximum = std::max(1.0f, base * tan75 / tangent);
        return base / std::min(factor, maximum);
    }

    int __cdecl ApplyProjection()
    {
        if (pCurrentCamera.is_initialized() && pCurrentCamera == reinterpret_cast<uint8_t*>(RenderCamera.get_ptr()))
            SetZoom(GameplayZoom());
        return ProjectionHook.ccall<int>();
    }

    // Movies at their own aspect, fitted into the game's 4:3 area (Original) or the whole screen (Fill)
    void(__cdecl* Quad2D)(float, float, float, float, float, float, float, float, int, int) = nullptr;
    void __cdecl MovieQuad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, int top, int bottom)
    {
        const float width = (x1 - x0) * float(Screen.Width) / 640.0f * InterfaceX();
        const float height = (y1 - y0) * float(Screen.Height) / 480.0f * InterfaceY();
        if (width > 0.0f && height > 0.0f && std::isfinite(width) && std::isfinite(height))
        {
            static const auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
            const float areaWidth = mode == 0 ? std::min(float(Screen.Width), float(Screen.Height) * 4.0f / 3.0f) : float(Screen.Width);
            const float areaHeight = mode == 0 ? std::min(float(Screen.Height), float(Screen.Width) * 3.0f / 4.0f) : float(Screen.Height);
            float scaleX = areaWidth / width, scaleY = areaHeight / height;
            scaleX = scaleY = std::min(scaleX, scaleY);
            const float halfX = (x1 - x0) * scaleX * 0.5f, halfY = (y1 - y0) * scaleY * 0.5f;
            x0 = 320.0f - halfX; x1 = 320.0f + halfX;
            y0 = 240.0f - halfY; y1 = 240.0f + halfY;
            ClearOutside((float(Screen.Width) - width * scaleX) * 0.5f, (float(Screen.Height) - height * scaleY) * 0.5f,
                (float(Screen.Width) + width * scaleX) * 0.5f, (float(Screen.Height) + height * scaleY) * 0.5f);
        }
        Quad2D(x0, y0, x1, y1, u0, v0, u1, v1, top, bottom);
    }

    // Mouse: menus are drawn in the 4:3 area
    SafetyHookInline CursorHook, SetCursorHook, HitRectFHook, HitRectIHook, PreviousPageHook, NextPageHook;
    void(__cdecl* GetCursor)(float*, float*) = nullptr;

    // Menus are squeezed into the 4:3 area: a menu x maps to 256 + (x - 256) / scale across the screen's 512
    float ToInterface(float x) { return 256.0f + (x - 256.0f) / InterfaceX(); }
    float FromInterface(float x) { return 256.0f + (x - 256.0f) * InterfaceX(); }
    float ToInterfaceY(float y) { return 224.0f + (y - 224.0f) / InterfaceY(); }
    float FromInterfaceY(float y) { return 224.0f + (y - 224.0f) * InterfaceY(); }

    void __cdecl MenuCursor(float* x, float* y)
    {
        CursorHook.ccall<void>(x, y);
        if (x)
            *x = ToInterface(*x);
        if (y)
            *y = ToInterfaceY(*y);
    }

    void __cdecl SetMenuCursor(double x, double y)
    {
        SetCursorHook.ccall<void>(double(FromInterface(float(x))), double(FromInterfaceY(float(y))));
    }

    int __cdecl HitRectF(float* rect)
    {
        float r[4] = { FromInterface(rect[0]), FromInterfaceY(rect[1]), rect[2] * InterfaceX(), rect[3] * InterfaceY() };
        return HitRectFHook.ccall<int>(r);
    }

    int __cdecl HitRectI(int32_t* rect)
    {
        int32_t r[4] = { int32_t(std::lround(FromInterface(float(rect[0])))), int32_t(std::lround(FromInterfaceY(float(rect[1])))),
            int32_t(std::lround(rect[2] * InterfaceX())), int32_t(std::lround(rect[3] * InterfaceY())) };
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
        auto resolutionRow = hook::pattern("89 BF A8 00 00 00 C7 87 AC 00 00 00 ? ? ? ? 89 9F F8 00 00 00");
        auto row = *resolutionRow.get_first<uint32_t*>(12);
        auto values = reinterpret_cast<uint32_t*>(row[3]);
        for (int v = 0; v < 6; ++v)
            injector::WriteMemory(&values[v], ResolutionMessage, true);
        MessageHook = safetyhook::create_inline(lookup.get_first(), &Message);
    }
}

class Widescreen
{
public:
    Widescreen()
    {
        WFP::onInitEvent() += []()
        {
            CinematicBars = &FixSettings().Int("MAIN.CutsceneBorders");
            FOVFactor = &FixSettings().Float("MAIN.FOVFactor");
            auto updateHUD = []()
            {
                const auto aspect = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
                HudAspectRatioConstraint.store(aspect && std::isfinite(*aspect) && *aspect > 0.0f ? *aspect : 0.0f, std::memory_order_relaxed);
            };
            updateHUD();
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", updateHUD);
            auto indicator = hook::pattern("57 33 C0 B9 26 00 00 00 BF ? ? ? ? F3 AB 6A 02 68 ? ? ? ? A3 ? ? ? ? A2");
            auto gauge = hook::pattern("A0 ? ? ? ? 83 EC 08 A8 04 0F 84 ? ? ? ? A1 ? ? ? ? 85 C0 0F 84");
            auto drawSprite = hook::pattern("8B 44 24 08 83 F8 03 77 ? 56 FF 24 85 ? ? ? ? 53 8B 5C 24 0C");
            const auto start = reinterpret_cast<uintptr_t>(gauge.get_first());
            auto sprites = hook::range_pattern(start, start + 0x14E, "6A 00 6A 01 56 6A 02 68 ? ? ? ? E8 ? ? ? ?");
            auto circle = hook::range_pattern(start, start + 0x14E, "DB 05 ? ? ? ? D8 05 ? ? ? ? D9 1C 24 E8 ? ? ? ? 83 C4 0C");
            HudSprites[0] = *indicator.get_first<void*>(9);
            for (size_t i = 0; i < 3; ++i)
                HudSprites[i + 1] = *sprites.get(i).get<void*>(8);
            HudSpriteHook = safetyhook::create_inline(drawSprite.get_first(), &HudSprite);
            HudCircleHook = safetyhook::create_mid(circle.get_first(15), [](SafetyHookContext& regs)
            {
                auto x = reinterpret_cast<float*>(regs.esp);
                *x -= HudOffset();
            });
            if (!HudSpriteHook || !HudCircleHook)
                std::terminate();

            // The move intent, player status and game mode are bound in Init
            SHDisplay::OnPresent().push_back([]() { FramingCleared = false; });

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
            nDisplayWidth.SetAddress(*size.get_first<int32_t*>(7));
            nDisplayHeight.SetAddress(*height.get_first<int32_t*>(2));
            for (auto [variable, value] : { std::pair{ &nDisplayWidth, Screen.Width }, std::pair{ &nDisplayHeight, Screen.Height } })
            {
                auto address = variable->get_ptr();
                auto signature = pattern_str(0xC7, 0x05, to_bytes(address));
                // get_first stops scanning after one match; use a fresh pattern for all writes.
                hook::pattern(signature).get_first();
                auto writes = hook::pattern(signature);
                for (size_t i = 0; i < writes.size(); ++i)
                    injector::WriteMemory(writes.get(i).get<void>(6), value, true);
                *variable = value;
            }

            // Hor+: the vertical field of view stays, the horizontal one widens
            auto fov = hook::pattern("83 C4 14 68 ? ? ? ? E8 ? ? ? ? 68");
            auto aspect = hook::pattern("C7 46 48 ? ? ? ? C7 46 44");
            RenderCamera.SetAddress(*fov.get_first<RenderCameraData*>(4));
            injector::MakeNOP(fov.get_first(8), 5, true);
            injector::WriteMemory<float>(fov.get_first(14), 1.0f / Screen.Scale, true);
            injector::WriteMemory<float>(aspect.get_first(3), 0.78125f / Screen.Scale, true);

            // Cutscene framing, every frame where the game applies its camera
            auto sceneFlags = hook::pattern("A1 ? ? ? ? 83 E0 01 C3 90 90 90 90 90 90 90 55 8B EC 83 EC 1C");
            auto gameCamera = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 10 E9");
            // nSceneFlags is bound in Init
            auto target = injector::GetBranchDestination(gameCamera.get_first(39)).as_int();
            auto apply = hook::range_pattern(target, target + 15, "68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ?").get_first<uint8_t>();
            pCurrentCamera.SetAddress(*reinterpret_cast<uint8_t***>(apply + 11));
            ProjectionHook = safetyhook::create_inline(apply, &ApplyProjection);

            // The native cinematic bars on or off
            auto barFlags = hook::pattern("A1 ? ? ? ? 85 C0 74 ? 6A 02 68").count(2);
            aCinemaBarFlags[0].SetAddress(*barFlags.get(0).get<int32_t*>(1));
            aCinemaBarFlags[1].SetAddress(*barFlags.get(1).get<int32_t*>(1));
            static int32_t Gates[2] = {};
            for (int i = 0; i < 2; ++i)
                injector::WriteMemory(barFlags.get(i).get<void>(1), &Gates[i], true);
            SHDisplay::OnPresent().push_back([]()
            {
                for (int i = 0; i < 2; ++i)
                    Gates[i] = (*CinematicBars & 1) ? aCinemaBarFlags[i].get() : 0;
            });
            auto bars = hook::pattern("E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? E8 ? ? ? ? 6A 01 E8");
            DrawCinemaBars = reinterpret_cast<decltype(DrawCinemaBars)>(injector::GetBranchDestination(bars.get_first()).as_int());
            injector::MakeCALL(bars.get_first(), &CinemaBars, true);

            // 2D constants per batch
            auto constants = hook::pattern("83 EC 48 E8 ? ? ? ? D9 5C 24 00 E8 ? ? ? ? D9 5C 24 04 E8");
            auto flush = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 03 C1 83 EC 10 85 C0 75 04 83 C4 10 C3");
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
            pScreenCopyTexture.SetAddress(*copy.get_first<IDirect3DBaseTexture8**>(0x18));
            ScreenTextures.push_back(pScreenCopyTexture.get_ptr());
            auto targets = hook::pattern("8B 44 24 04 85 C0 7C 0D 83 F8 02 7D 08 8B 04 85");
            aRenderTargetTextures.SetAddress(*targets.get_first<IDirect3DBaseTexture8**>(0x10));
            for (int i : { 0, 1, 11, 12, 13, 14 })      // 6E8E44, 6E8E48, 6E8E70 .. 6E8E7C
                ScreenTextures.push_back(&(*aRenderTargetTextures)[i]);

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
                ScreenCallSites.push_back(reinterpret_cast<uintptr_t>(site.get_first(offset + 5)));
            }
            // Calls into the flush inside whole functions: darkness ribbons, light halos, the light grid, the
            // 512x448 fade, and the document viewer's black backdrop (its first batch; the page itself stays 4:3)
            auto flushAddress = reinterpret_cast<uintptr_t>(flush.get_first());
            auto addCalls = [&](const char* signature, size_t size, int count, const char* requiredCall)
            {
                auto start = hook::pattern(signature);
                auto from = reinterpret_cast<uintptr_t>(start.get_first());
                auto required = hook::range_pattern(from, from + size, requiredCall).get_first<uint8_t>();
                auto requiredAddress = reinterpret_cast<uintptr_t>(required);
                auto displacement = static_cast<uint32_t>(flushAddress - requiredAddress - 5);
                hook::range_pattern(requiredAddress, requiredAddress + 5, pattern_str(0xE8, to_bytes(displacement))).get_first();
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
            addCalls("55 8B EC 83 E4 F0 83 EC 64 D9 45 18", 0x377, 8, "E8 ? ? ? ? 8B 8C 24 C8 00 00 00 33 4D 04 83 C4 5C C6 05"); // darkness ribbons
            addCalls("55 8B EC 83 E4 F0 81 EC 34 01 00 00 A0 ? ? ? ? 84 C0 53 56 57 0F 85", 0x6E4, 8, "E8 ? ? ? ? 8B BC 24 CC 00 00 00 83 C4 60 C6 05"); // light halos
            addCalls("55 8B EC 83 E4 F0 81 EC A4 00 00 00 8B 45 0C 99 B9 03 00 00 00 F7 F9", 0x2E7, 8, "E8 ? ? ? ? 83 C4 60 C6 05 ? ? ? ? 00 B8 01 00 00 00"); // light halos
            addCalls("55 8B EC 83 E4 F0 83 EC 44 53 56 33", 0x281, 8, "E8 ? ? ? ? 8B 44 24 64 8B 7C 24 60 8B 54 24 68 83 C4 38"); // light grid
            addCalls("E8 ? ? ? ? 8B 44 24 0C 50 50 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F E8 ? ? ? ? 89 44 24 24", 0x81, 8, "E8 ? ? ? ? 83 C4 60"); // fade
            addCalls("8B 44 24 04 0F B7 80 10 04 00 00 83 EC 10 83 F8 06 0F 8C", 0x2CA, 1, "E8 ? ? ? ? 83 C4 60 68 DF F1 03 F0 E8"); // document backdrop

            // The wobbling grid behind the Options and the other in-game menus covers the whole screen
            auto backdrop = hook::pattern("6A 01 6A 00 68 0D 00 03 00 6A 00 68 5B 01 00 00 6A 00 6A 44 E8");
            ScreenCallSites.push_back(reinterpret_cast<uintptr_t>(backdrop.get_first(25)));

            // Peep holes and the door scope
            aFlushBatchCounts.SetAddress(*flush.get_first<uint8_t*>(7));
            aFlushBatchVertices.SetAddress(*flush.get_first<uint8_t*>(7) + 0x18);
            auto lens = hook::pattern("68 01 10 03 00 6A 00 68 14 01 00 00 6A 00 68 88 00 00 00 E8");
            LensSite = reinterpret_cast<uintptr_t>(lens.get_first(24));
            auto peep = hook::pattern("E8 ? ? ? ? 85 C0 74 24 D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1D ? ? ? ? 56 8B 74 24 08 85 F6 75 05");
            PeepHook = safetyhook::create_inline(peep.get_first(), &Peep);
            auto prison = hook::pattern("55 8B EC 83 E4 F0 83 EC 74 8B 45 08 53 56 57 8B B8 08 04 00 00");
            PrisonPeepHook = safetyhook::create_inline(prison.get_first(), &PrisonPeep);

            // Movies
            auto movie = hook::pattern("E8 ? ? ? ? 8B 14 B5 ? ? ? ? A1");
            Quad2D = reinterpret_cast<decltype(Quad2D)>(injector::GetBranchDestination(movie.get_first()).as_int());
            injector::MakeCALL(movie.get_first(), &MovieQuad, true);

            // Mouse
            auto cursor = hook::pattern("83 EC 18 DB 05 ? ? ? ? 8B 0D ? ? ? ? 8D 44 24 08 50 D9 5C 24 04");
            auto setCursor = hook::pattern("83 EC 18 8B 0D ? ? ? ? 33 C0 56 89 44 24 04 89 44 24 08");
            auto hitF = hook::pattern("83 EC 10 8B 0D ? ? ? ? 53 55 56 57 8D 44 24 10 50 51 FF 15");
            auto hitI = hook::pattern("83 EC 20 8B 0D ? ? ? ? 53 55 56 57 8D 44 24 10 50 51 FF 15");
            auto pages = hook::pattern("83 EC 14 8B 0D ? ? ? ? 53 55 56 57 8D 44 24 14 50 51 FF 15 ? ? ? ? DB 44 24 1C D9 05 ? ? ? ? D8 C9").count(2);
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

            ShowResolution();

            // Soft shadow blur offsets in pixels: scaled with the resolution
            auto taps = hook::pattern("8B 0D ? ? ? ? 05 A8 00 00 00 89 44 24 20 89 08 D9 05").count(4);
            float k = std::max(1.0f, float(Screen.Height) / 480.0f);
            for (size_t i = 0; i < 4; ++i)
            {
                auto x = *taps.get(i).get<float*>(2);
                auto y = *taps.get(i).get<float*>(0x13);
                injector::WriteMemory<float>(x, *x * k, true);
                injector::WriteMemory<float>(y, *y * k, true);
            }
        };
    }
} Widescreen;
