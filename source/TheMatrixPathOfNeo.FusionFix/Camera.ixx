module;

#include <stdafx.h>
#include "FixSettings.hpp"

// The game's lens takes the vertical half angle and the camera's width and height. The camera width is widened with
// the screen (Hor+); cinematics get the camera height of their band; the gameplay camera gets the FOV factor.

export module Camera;

import ComVars;
import Layout;
import Cinematics;

export namespace NeoCamera
{
    inline SafetyHookInline LensHook;
    inline std::atomic<float> Factor{ 1.0f };

    inline bool Gameplay(uintptr_t camera)
    {
        const auto world = *reinterpret_cast<uintptr_t*>(camera + 1148);
        if (!world || !*reinterpret_cast<uintptr_t*>(world + 324) ||
            *reinterpret_cast<uintptr_t*>(camera + 656))
            return false;
        const auto controller = *reinterpret_cast<uintptr_t*>(camera + 648);
        if (!controller)
            return false;
        const auto current = *reinterpret_cast<int*>(controller + 724);
        const auto next = *reinterpret_cast<int*>(controller + 728);
        return current >= 0 && current <= 2 && next >= 0 && next <= 2;
    }

    inline int __cdecl Lens(float halfVertical, uintptr_t camera, float pixelAspect)
    {
        if (!camera || Screen.nAspectRatioDiff <= 0 || Screen.fAspectRatio <= 0.0f || !std::isfinite(halfVertical) ||
            halfVertical <= 0.0f || halfVertical >= 90.0f)
            return LensHook.ccall<int>(halfVertical, camera, pixelAspect);
        injector::WriteMemory<int>(reinterpret_cast<int*>(camera + 72), Screen.nAspectRatioDiff, true);
        const auto band = NeoCinema::Current(camera);
        if (band.Active)
        {
            injector::WriteMemory<int>(reinterpret_cast<int*>(camera + 76), NeoCinema::CameraHeight(band), true);
            return LensHook.ccall<int>(halfVertical, camera, pixelAspect);
        }
        const float factor = Gameplay(camera) ? Factor.load() : 1.0f;
        return LensHook.ccall<int>(NeoLayout::HalfVertical(halfVertical, Screen.fAspectRatio, factor), camera, pixelAspect);
    }

    inline void Install()
    {
        Factor = std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f);
        FixSettings().OnChange("MAIN.FOVFactor", []()
        {
            Factor = std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f);
        });
        auto lens = hook::pattern("D9 05 ? ? ? ? 8B 44 24 08 D8 64 24 04 8B 4C 24 04 89 88 88 00 00 00");
        LensHook = safetyhook::create_inline(lens.get_first(), Lens, SafetyHookInline::StartDisabled);
        (void)LensHook.enable();
    }
}
