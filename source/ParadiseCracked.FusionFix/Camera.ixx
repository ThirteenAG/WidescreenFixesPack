module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;
import ComVars;
import Display;


// The scene camera keeps its vertical field of view (60 degrees) and takes the aspect ratio of the current mode, so
// wider screens see more at the sides (Hor+). Its frustum and culling planes come from the same projection.
export namespace ParadiseCamera
{
    inline SafetyHookMid ProjectionHook;
    inline void (__thiscall* SetProjection)(void*, float, float, float, float) = nullptr;
    inline void* Camera = nullptr;
    inline float NativeFov = 3.1415927f / 3.0f;
    inline float Near = 1.0f;
    inline float Far = 420.0f;
    inline std::atomic<float> Factor{ 1.0f };
    inline std::atomic<bool> Dirty{ false };

    inline void Projection(SafetyHookContext& context)
    {
        auto& fov = *reinterpret_cast<float*>(context.esp + 4);
        auto& aspect = *reinterpret_cast<float*>(context.esp + 16);
        Camera = reinterpret_cast<void*>(context.ecx);
        NativeFov = fov;
        Near = *reinterpret_cast<float*>(context.esp + 8);
        Far = *reinterpret_cast<float*>(context.esp + 12);
        const auto width = ParadiseDisplay::Logical.get_ptr()[0], height = ParadiseDisplay::Logical.get_ptr()[1];
        if (width > 0 && height > 0)
            aspect = float(height) / float(width);
        if (ParadiseDisplay::GameRequest)
            fov = (std::min)(fov * Factor.load(std::memory_order_relaxed), 170.0f * 3.1415927f / 180.0f);
    }

    // On the game thread, before the frame is shown: a changed factor rebuilds the projection
    inline void Frame()
    {
        if (Dirty.exchange(false) && Camera)
            SetProjection(Camera, NativeFov, Near, Far, 0.75f);
    }

    inline void Init()
    {
        const auto read = []()
        {
            const float factor = FixSettings().Float("MAIN.FOVFactor");
            Factor.store(std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f);
            Dirty.store(true);
        };
        read();
        FixSettings().OnChange("MAIN.FOVFactor", read);
        auto projection = hook::pattern("55 8B EC 8B 45 14 D9 45 10 56 8B F1 89 86 CC 00 00 00 8B 45 08");
        SetProjection = reinterpret_cast<decltype(SetProjection)>(projection.get_first());
        ProjectionHook = safetyhook::create_mid(projection.get_first(), Projection);
    }
}
