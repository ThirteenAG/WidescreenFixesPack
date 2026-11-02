module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <format>

export module Camera;

import ComVars;
import Cutscene;

export
{
const float* pFOVFactor = nullptr;

// The game's field of view constant for the current aspect (Hor+) and MAIN.FOVFactor; cutscenes keep factor 1
float CurrentFieldOfView()
{
    const float factor = !pFOVFactor || !std::isfinite(*pFOVFactor) || bInCutsceneView() ? 1.0f : std::clamp(*pFOVFactor, 0.5f, 2.0f);
    return NativeFOV.get() / (Screen.fAspectRatio / (4.0f / 3.0f)) / factor;
}

// The game's state flags: 0x80 while playing, 1 while a menu is over the game; menus, loading and front-end screens
// keep their 4:3 box
// A camera keeps its lens (the FOV constant over the tangent of half its field of view) from when its field of view
// was last set, so a changed MAIN.FOVFactor reached the view only at the next camera change. The renderer's cameras
// (an array in the renderer object) now get their lens rescaled through the game's own setter, which rebuilds the view.
using SetLensFn = void(__thiscall*)(void* camera, float lens);
SetLensFn SetCameraLens = nullptr;
uint32_t nCameraLensOffset = 0;
float fAppliedFieldOfView = 0.0f;

void UpdateCameraLenses()
{
    if (!SetCameraLens || !Memory.is_initialized() || !Memory.get() || !off_2C0)
        return;
    if (fAppliedFieldOfView <= 0.0f)
        fAppliedFieldOfView = Screen.fFieldOfView;
    if (Screen.fFieldOfView == fAppliedFieldOfView || !std::isfinite(Screen.fFieldOfView) || Screen.fFieldOfView <= 0.0f)
        return;
    const float ratio = Screen.fFieldOfView / fAppliedFieldOfView;
    fAppliedFieldOfView = Screen.fFieldOfView;
    const auto renderer = reinterpret_cast<uint32_t*>(Memory.get());
    const auto base = static_cast<uintptr_t>(renderer[0x10]);
    const auto count = static_cast<int>(renderer[0x12]);
    for (int i = 0; base && i < count && i < 16; ++i)
    {
        auto camera = reinterpret_cast<void*>(base + off_2C0 * i);
        auto methods = *reinterpret_cast<void***>(camera);
        if (!methods || methods[2] != reinterpret_cast<void*>(SetCameraLens))
            continue;
        const float lens = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(camera) + nCameraLensOffset);
        if (std::isfinite(lens) && lens > 0.0f)
            SetCameraLens(camera, lens * ratio);
    }
}

void InitCameraLenses()
{
    auto pattern = hook::pattern("8B 44 24 04 8B 11 89 81 ? ? ? ? 8B 81 ? ? ? ? 50 8B 81 ? ? ? ? 50 8B 81 ? ? ? ? 50 8B 81 ? ? ? ? 50");
    SetCameraLens = reinterpret_cast<SetLensFn>(pattern.get_first()); //0x4B1F00
    nCameraLensOffset = *pattern.get_first<uint32_t>(8); //0x27C
}

void InitFOV()
{
    hook::pattern pattern;
    //FOV
    for (const auto offset : { nCameraLensOffset, nCameraLensOffset - 12 })
    {
        const auto signature = std::format("D9 05 ? ? ? ? D8 B1 {:02X} {:02X} {:02X} {:02X} D9 E8 D9 F3",
            offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF, (offset >> 24) & 0xFF);
        pattern = hook::pattern(signature);
        injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fFieldOfView, true);
    }
    pattern = hook::pattern("D8 0D ? ? ? ? D8 B6 ? ? ? ? 5E D9 E8 D9 F3");
    injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fFieldOfView, true); //fmul
    pattern = hook::pattern("D8 0D ? ? ? ? D9 96");
    injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fFieldOfView, true); //fmul
    pattern = hook::pattern("D8 3D ? ? ? ? C2 04 00");
    injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fFieldOfView, true); //fdiv
    pattern = hook::pattern("D8 3D ? ? ? ? D9 99 ? ? ? ? FF");
    injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fFieldOfView, true); //fdiv
    pattern = hook::pattern("D8 3D ? ? ? ? D9 99 ? ? ? ? C2 04 00");
    NativeFOV.SetAddress(*pattern.get_first<float*>(2));
    Screen.fFieldOfView = NativeFOV.get();
    FixSettings().OnChange("MAIN.FOVFactor", []()
    {
        Screen.fFieldOfView = CurrentFieldOfView();
    });
    injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fFieldOfView, true); //fdiv


}

}
