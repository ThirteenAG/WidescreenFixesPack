module;
#include <stdafx.h>
#include <ddraw.h>
#include <d3d.h>
#include <array>
#include "FixSettings.hpp"

export module Camera;
import ComVars;
import Window;


export namespace KnightRider2Camera
{
    inline float NativeFOV = 53.0f;
    inline bool Cinematic = false;
    inline thread_local bool DrawingMain = false;
    inline thread_local void* Caller = nullptr;
    inline std::atomic<int> Borders{ 1 };
    inline SafetyHookMid AnimationHook, StaticHook;
    inline SafetyHookInline DrawHook, DirectorHook;
    inline SafetyHookInline FOVHook;
    inline void (__thiscall* UpdateWorld)(void*) = nullptr;
    inline void (*FrontendDraw)(uint8_t*) = nullptr;

    inline void BeginFrame()
    {
        Cinematic = false;
        KnightRider2Hud::CockpitActive = false;
    }

    inline void ClearBorders(uint8_t* camera, int marginX, int marginY)
    {
        if (!marginX && !marginY)
            return;
        const auto renderer = *reinterpret_cast<uint8_t**>(camera + 392);
        const auto device = renderer ? *reinterpret_cast<IDirect3DDevice7**>(renderer + 48) : nullptr;
        D3DVIEWPORT7 previous{};
        if (!device || FAILED(device->GetViewport(&previous)))
            return;
        D3DVIEWPORT7 full{ 0, 0, static_cast<DWORD>(KnightRider2Window::Width), static_cast<DWORD>(KnightRider2Window::Height), 0.0f, 1.0f };
        if (FAILED(device->SetViewport(&full)))
            return;
        const LONG width = KnightRider2Window::Width, height = KnightRider2Window::Height;
        D3DRECT bars[2]{};
        if (marginX)
        {
            bars[0] = { 0, 0, marginX, height };
            bars[1] = { width - marginX, 0, width, height };
        }
        else
        {
            bars[0] = { 0, 0, width, marginY };
            bars[1] = { 0, height - marginY, width, height };
        }
        device->Clear(2, bars, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        device->SetViewport(&previous);
    }

    inline uint8_t* MainCamera()
    {
        return Wrapper.is_initialized() && Wrapper.get() ? *reinterpret_cast<uint8_t**>(Wrapper.get() + 8) : nullptr;
    }

    // Widens the main camera's projection for this frame; returns its viewport to restore after drawing.
    inline std::array<float, 4> Prepare(uint8_t* camera)
    {
        const float aspect = static_cast<float>(KnightRider2Window::Width) / KnightRider2Window::Height;
        const float nativeAspect = 4.0f / 3.0f;
        const bool preserve = Cinematic && Borders.load(std::memory_order_relaxed) != 0;
        const float factor = Cinematic ? 1.0f : std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f);
        const float halfHeight = std::tan(std::min(NativeFOV * factor, 170.0f) * 0.0087266462f) / nativeAspect;
        const float projectionAspect = preserve ? nativeAspect : aspect;
        auto frustum = reinterpret_cast<float*>(camera + 324);
        frustum[0] = -halfHeight * projectionAspect;
        frustum[1] = halfHeight * projectionAspect;
        frustum[2] = halfHeight;
        frustum[3] = -halfHeight;
        auto viewport = reinterpret_cast<float*>(camera + 348);
        const std::array<float, 4> previousViewport{ viewport[0], viewport[1], viewport[2], viewport[3] };
        viewport[0] = 0.0f;
        viewport[1] = 1.0f;
        viewport[2] = 1.0f;
        viewport[3] = 0.0f;
        if (preserve)
        {
            if (aspect > nativeAspect)
            {
                const float margin = std::round((KnightRider2Window::Width - KnightRider2Window::Height * nativeAspect) * 0.5f);
                viewport[0] = margin / KnightRider2Window::Width;
                viewport[1] = 1.0f - viewport[0];
                ClearBorders(camera, static_cast<int>(margin), 0);
            }
            else
            {
                const float margin = std::round((KnightRider2Window::Height - KnightRider2Window::Width / nativeAspect) * 0.5f);
                viewport[3] = margin / KnightRider2Window::Height;
                viewport[2] = 1.0f - viewport[3];
                ClearBorders(camera, 0, static_cast<int>(margin));
            }
        }
        // Projection changes must also rebuild the engine's six world-space culling planes.
        UpdateWorld(camera);
        return previousViewport;
    }

    inline void Restore(uint8_t* camera, const std::array<float, 4>& previousViewport)
    {
        std::copy(previousViewport.begin(), previousViewport.end(), reinterpret_cast<float*>(camera + 348));
    }

    inline void __fastcall Draw(uint8_t* camera, void*)
    {
        struct CallScope
        {
            void* PreviousCaller;
            bool PreviousMain;
            ~CallScope()
            {
                Caller = PreviousCaller;
                DrawingMain = PreviousMain;
            }
        } scope{ Caller, DrawingMain };
        const auto returnAddress = reinterpret_cast<void**>(_AddressOfReturnAddress());
        Caller = returnAddress[0];
        DrawingMain = camera == MainCamera();
        if (!DrawingMain)
        {
            if (FrontendDraw)
                return FrontendDraw(camera);
            return DrawHook.thiscall<void>(camera);
        }
        const auto previousViewport = Prepare(camera);
        DrawHook.thiscall<void>(camera);
        Restore(camera, previousViewport);
    }

    inline char __fastcall Direct(uint8_t* director, void*, uint8_t* frame, int pass)
    {
        // Levels with a visibility manager (indoor levels) draw the main camera through it, not through the camera.
        const auto camera = MainCamera();
        const auto level = *reinterpret_cast<uint8_t**>(frame + 12);
        if (pass != 0 || !camera || !*reinterpret_cast<void**>(level + 104))
            return DirectorHook.thiscall<char>(director, frame, pass);
        const auto previousViewport = Prepare(camera);
        const auto result = DirectorHook.thiscall<char>(director, frame, pass);
        Restore(camera, previousViewport);
        return result;
    }

    inline void* __fastcall FieldOfView(uint8_t* wrapper, void*, float fov, float aspect)
    {
        if (Wrapper.is_initialized() && wrapper == Wrapper.get())
            NativeFOV = fov;
        return FOVHook.thiscall<void*>(wrapper, fov, aspect);
    }

    inline void Init()
    {
        const auto update = []()
        {
            Borders.store(std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 1), std::memory_order_relaxed);
        };
        update();
        FixSettings().OnChange("MAIN.CutsceneBorders", update);
        auto fov = hook::pattern("83 EC 18 D9 44 24 1C 53 D8 0D ? ? ? ? 8B D9 8B 43 08 56 D9 F2 57 8D B0 44 01 00 00");
        FOVHook = safetyhook::create_inline(fov.get_first(), FieldOfView);
        auto animation = hook::pattern("83 EC 4C 53 8B D9 8B 43 0C 85 C0 0F 84 93 01 00 00 8B 48 08 85 C9 0F 84 88 01 00 00");
        AnimationHook = safetyhook::create_mid(animation.get_first(), [](SafetyHookContext& context)
        {
            const auto property = reinterpret_cast<uint8_t*>(context.ecx);
            Cinematic = Cinematic || (*reinterpret_cast<uint32_t*>(property + 12) && *reinterpret_cast<uint32_t*>(property + 56));
        });
        auto stationary = hook::pattern("51 53 55 56 57 8B F1 6A 00 68 ? ? ? ? 68 ? ? ? ? 8B 46 38 6A 00");
        StaticHook = safetyhook::create_mid(stationary.get_first(), [](SafetyHookContext& context)
        {
            Cinematic = Cinematic || reinterpret_cast<uint8_t*>(context.ecx)[156];
        });
        auto world = hook::pattern("83 EC 64 53 55 8B D9 8B 43 1C 85 C0 56 57 74 6E 8D 48 64 8D 43 30 50 8D 54 24 44");
        UpdateWorld = reinterpret_cast<decltype(UpdateWorld)>(world.get_first());
        auto draw = hook::pattern("A1 ? ? ? ? 85 C0 55 8B E9 0F 84 CE 01 00 00 53 56 8B 35 ? ? ? ?");
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        auto director = hook::pattern("A1 ? ? ? ? 83 EC 68 85 C0 53 55 56 57 8B E9 75 33"); // CinematicDirector click
        auto wrapperAddress = *director.get_first<uint8_t**>(1);
        if (wrapperAddress)
            Wrapper.SetAddress(wrapperAddress);
        DirectorHook = safetyhook::create_inline(director.get_first(), Direct);
    }
}
