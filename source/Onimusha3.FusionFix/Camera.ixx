module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;
import Borders;


export namespace OnimushaCamera
{
    inline std::atomic<float> Factor{ 1.0f };
    inline thread_local bool Gameplay = false;
    inline SafetyHookMid Begin, End;
    inline SafetyHookInline ProjectionHook;
    inline int(__thiscall* NativeProjection)(float*, int) = nullptr;
    inline std::function<float()> Aspect;

    inline float FocalLength(float focal, float factor, float aspect)
    {
        if (!std::isfinite(focal) || focal <= 0.0f || factor == 1.0f)
            return focal;
        constexpr float halfHeight = 224.0f;
        const float vertical = 2.0f * std::atan(halfHeight / focal);
        const float baseline = 2.0f * std::atan(halfHeight / focal * aspect);
        const float limit = std::max(baseline, 150.0f * float(M_PI) / 180.0f);
        const float scaled = std::clamp(vertical * factor, 0.01f, float(M_PI) - 0.01f);
        const float tangent = std::min(std::tan(scaled * 0.5f), std::tan(limit * 0.5f) / aspect);
        return halfHeight / tangent;
    }

    inline int __fastcall Projection(float* camera, void*, int mode)
    {
        const float original = camera[17];
        if (OnimushaBorders::Active())
            camera[17] *= OnimushaBorders::CameraScale(Aspect(), OnimushaBorders::Mode.load(std::memory_order_relaxed));
        else if (Gameplay)
            camera[17] = FocalLength(original, Factor.load(std::memory_order_relaxed), Aspect());
        const auto result = NativeProjection(camera, mode);
        camera[17] = original;
        return result;
    }

    inline void Init(std::function<float()> aspect)
    {
        Aspect = std::move(aspect);
        const auto update = []()
        {
            Factor = std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f);
        };
        update();
        FixSettings().OnChange("MAIN.FOVFactor", update);

        // This update runs only after the gameplay script dispatcher declines to take over the camera.
        // The loading update and the scripted camera builders remain outside the scope.
        auto updateCamera = hook::pattern("8D 8E D0 88 00 00 E8 ? ? ? ? F7 05 ? ? ? ? 00 00 00 10");
        Begin = safetyhook::create_mid(updateCamera.get_first(6), [](SafetyHookContext&) { Gameplay = true; });
        End = safetyhook::create_mid(updateCamera.get_first(11), [](SafetyHookContext&) { Gameplay = false; });

        // Level-specific gameplay cameras also publish through this dispatcher. The scope above excludes the
        // loading and scripted updates; the native builder keeps projection and culling on the same lens.
        auto projection = hook::pattern("8B 4E 08 6A 00 E8 ? ? ? ? 5F 5E 5B 8B E5 5D C3");
        const auto dispatcher = injector::GetBranchDestination(projection.get_first(5)).as_int();
        ProjectionHook = safetyhook::create_inline(reinterpret_cast<void*>(dispatcher), Projection);
        NativeProjection = ProjectionHook.original<decltype(NativeProjection)>();
    }
}
