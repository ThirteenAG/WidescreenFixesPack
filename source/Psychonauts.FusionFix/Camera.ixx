module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Camera;

import ComVars;

SafetyHookInline CameraUpdateHook, CameraViewHook;

struct GameplayFovScope
{
    float* value = nullptr;
    float original = 0.0f, adjusted = 0.0f;

    explicit GameplayFovScope(void* camera)
    {
        auto bytes = static_cast<uint8_t*>(camera);
        const auto type = *reinterpret_cast<int*>(bytes);
        // Native types: Fly, Script, Chase, Rail, Map, Sphere. Preserve special
        // views, zoom and FOV interpolation; 104 is GetCameraDefaultFOV's value.
        if ((type != 2 && type != 5) || *reinterpret_cast<float*>(bytes + 944) != 0.0f)
            return;
        auto fov = reinterpret_cast<float*>(bytes + 48);
        const float factor = FixSettings().Float("MAIN.FOVFactor");
        if (*fov != 104.0f || !std::isfinite(factor) || factor == 1.0f)
            return;
        value = fov;
        original = *fov;
        // The value is twice the vertical angle; widening stops at 150 degrees across the screen.
        const float aspect = Screen.fAspectRatio > 0.0f ? Screen.fAspectRatio : 4.0f / 3.0f;
        const float widest = 4.0f * std::atan(std::tan(75.0f * 3.14159265f / 180.0f) / aspect) * 180.0f / 3.14159265f;
        adjusted = std::clamp(original * std::clamp(factor, 0.5f, 2.0f), 1.0f, std::max(original, widest));
        *value = adjusted;
    }

    ~GameplayFovScope()
    {
        // Keep the camera's stored value native, so smoothing never compounds
        // the multiplier. Preserve a new native value if the update changed it.
        if (value && *value == adjusted)
            *value = original;
    }
};

void __fastcall UpdateGameplayCamera(void* camera, void*, int target, float delta)
{
    GameplayFovScope fov(camera);
    CameraUpdateHook.thiscall<void>(camera, target, delta);
}

void __fastcall BuildCameraView(void* camera, void*, int view, int position, int rotation)
{
    GameplayFovScope fov(camera);
    CameraViewHook.thiscall<void>(camera, view, position, rotation);
}

export void InstallGameplayFov()
{
    auto update = hook::pattern("55 8B EC 83 E4 F0 81 EC 58 0B 00 00 56 57 89 8C 24 5C 0B 00 00 A1 ? ? ? ? 8B 88 CC 01 00 00");
    auto view = hook::pattern("55 8B EC 83 E4 F0 81 EC B8 0B 00 00 56 57 89 8C 24 BC 0B 00 00 B9 10 00 00 00 BE ? ? ? ? 8D BC 24 70 0B 00 00 F3 A5");
    CameraUpdateHook = safetyhook::create_inline(update.get_first(), UpdateGameplayCamera);
    CameraViewHook = safetyhook::create_inline(view.get_first(), BuildCameraView);
}

