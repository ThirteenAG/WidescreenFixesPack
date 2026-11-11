module;
#include <stdafx.h>
#include <ddraw.h>
#include <d3d.h>
#include "FixSettings.hpp"

export module Camera;
import ComVars;
import Window;


export namespace KnightRiderCamera
{
    inline float NativeFOV = 53.0f;
    inline bool Cinematic = false;
    inline thread_local bool DrawingMain = false;
    inline thread_local void* Caller = nullptr;
    inline thread_local void* CallerNext = nullptr;
    inline std::atomic<int> Borders{ 1 };
    inline SafetyHookMid AnimationHook, StaticHook;
    inline SafetyHookInline DrawHook;
    inline void (__thiscall* UpdateWorld)(void*) = nullptr;

    inline void BeginFrame()
    {
        Cinematic = false;
        KnightRiderHud::CockpitActive = false;
    }

    inline void ClearBorders(uint8_t* camera, int marginX, int marginY)
    {
        if (!marginX && !marginY)
            return;
        const auto renderer = *reinterpret_cast<uint8_t**>(camera + 432);
        const auto device = renderer ? *reinterpret_cast<IDirect3DDevice7**>(renderer + 40) : nullptr;
        D3DVIEWPORT7 previous{};
        if (!device || FAILED(device->GetViewport(&previous)))
            return;
        D3DVIEWPORT7 full{ 0, 0, static_cast<DWORD>(KnightRiderWindow::Width), static_cast<DWORD>(KnightRiderWindow::Height), 0.0f, 1.0f };
        if (FAILED(device->SetViewport(&full)))
            return;
        const LONG width = KnightRiderWindow::Width, height = KnightRiderWindow::Height;
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

    inline void __fastcall Draw(uint8_t* camera, void*)
    {
        struct CallScope
        {
            void* PreviousCaller;
            void* PreviousNext;
            bool PreviousMain;
            ~CallScope()
            {
                Caller = PreviousCaller;
                CallerNext = PreviousNext;
                DrawingMain = PreviousMain;
            }
        } scope{ Caller, CallerNext, DrawingMain };
        const auto returnAddress = reinterpret_cast<void**>(_AddressOfReturnAddress());
        Caller = returnAddress[0];
        CallerNext = returnAddress[1];
        DrawingMain = Wrapper.is_initialized() && Wrapper.get() && *reinterpret_cast<uint8_t**>(Wrapper.get() + 8) == camera;
        if (!DrawingMain)
            return DrawHook.thiscall<void>(camera);
        const float aspect = static_cast<float>(KnightRiderWindow::Width) / KnightRiderWindow::Height;
        const float nativeAspect = 4.0f / 3.0f;
        const bool preserve = Cinematic && Borders.load(std::memory_order_relaxed) != 0;
        const float factor = Cinematic ? 1.0f : std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f);
        const float halfHeight = std::tan(std::min(NativeFOV * factor, 170.0f) * 0.0087266462f) / nativeAspect;
        const float projectionAspect = preserve ? nativeAspect : aspect;
        auto frustum = reinterpret_cast<float*>(camera + 340);
        frustum[0] = -halfHeight * projectionAspect;
        frustum[1] = halfHeight * projectionAspect;
        frustum[2] = halfHeight;
        frustum[3] = -halfHeight;
        auto viewport = reinterpret_cast<float*>(camera + 364);
        const float previousViewport[4]{ viewport[0], viewport[1], viewport[2], viewport[3] };
        viewport[0] = 0.0f;
        viewport[1] = 1.0f;
        viewport[2] = 1.0f;
        viewport[3] = 0.0f;
        if (preserve)
        {
            if (aspect > nativeAspect)
            {
                const float margin = std::round((KnightRiderWindow::Width - KnightRiderWindow::Height * nativeAspect) * 0.5f);
                viewport[0] = margin / KnightRiderWindow::Width;
                viewport[1] = 1.0f - viewport[0];
                ClearBorders(camera, static_cast<int>(margin), 0);
            }
            else
            {
                const float margin = std::round((KnightRiderWindow::Height - KnightRiderWindow::Width / nativeAspect) * 0.5f);
                viewport[3] = margin / KnightRiderWindow::Height;
                viewport[2] = 1.0f - viewport[3];
                ClearBorders(camera, 0, static_cast<int>(margin));
            }
        }
        // Projection changes must also rebuild the engine's six world-space culling planes.
        UpdateWorld(camera);
        DrawHook.thiscall<void>(camera);
        std::copy(std::begin(previousViewport), std::end(previousViewport), viewport);
    }

    inline void Init()
    {
        const auto update = []()
        {
            Borders.store(std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 1), std::memory_order_relaxed);
        };
        update();
        FixSettings().OnChange("MAIN.CutsceneBorders", update);
        auto wrapper = hook::pattern("8B 0D ? ? ? ? D8 46 10 D9 1C 24 DD D8 E8 ? ? ? ? 5F 5E C2 04 00");
        Wrapper.SetAddress(*wrapper.get_first<uint8_t**>(2));
        auto animation = hook::pattern("8B 4E 28 53 55 57 E8 ? ? ? ? 83 C0 50 8B 28");
        AnimationHook = safetyhook::create_mid(animation.get_first(), [](SafetyHookContext&) { Cinematic = true; });
        auto stationary = hook::pattern("55 8B 2D ? ? ? ? 57 8B CE E8 ? ? ? ? 8A 88 94 00 00 00");
        StaticHook = safetyhook::create_mid(stationary.get_first(), [](SafetyHookContext&) { Cinematic = true; });
        auto world = hook::pattern("83 EC 70 53 55 8B D9 8B 43 18 85 C0 56 57 8D 6B 60 74 67");
        UpdateWorld = reinterpret_cast<decltype(UpdateWorld)>(world.get_first());
        auto draw = hook::pattern("56 8B F1 8B 8E B0 01 00 00 85 C9 0F 84 ? ? ? ? 8B 01 55 57");
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
    }
}
