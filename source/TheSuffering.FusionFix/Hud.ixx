module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module Hud;

import ComVars;

export
{
bool InGameplay()
{
    return GameFlags.is_initialized() && (GameFlags.get() & 0x81) == 0x80;
}

// MAIN.HudAspectRatioConstraint: the HUD's box; Auto (a very wide ratio) uses the whole screen
std::atomic<float> fHudAspect = 100.0f;
void UpdateHudAspect()
{
    const auto parsed = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
    fHudAspect = parsed ? std::clamp(*parsed, 4.0f / 3.0f, 32.0f / 9.0f) : 100.0f;
}

void __cdecl sub_44A390(uintptr_t a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, uintptr_t a12, uintptr_t a13, uintptr_t a14)
{
    if (!a1 || !Memory.is_initialized() || !Memory.get() ||
        (!a14 && (!std::isfinite(a10) || !std::isfinite(a11) || a10 == 0.0f || a11 == 0.0f)))
        return;
    uintptr_t rendererMemory = Memory.get();

    if (a2 == 0.0f && a4 == Screen.fWidth)
    {
        if (a12)
        {
            if (*(uint32_t*)(a12 + 0x00) == 0xffffffff && *(uint32_t*)(a12 + 0x04) == 0xffffffff && *(uint32_t*)(a12 + 0x08) == 0xffffffff && *(uint32_t*)(a12 + 0x0C) == 0xffffffff)
            {
                a2 /= Screen.fAspectRatioDiff;
                a4 /= Screen.fAspectRatioDiff;
                a2 += Screen.fHudOffsetReal;
            }
        }
    }
    else if (InGameplay())
    {
        // The HUD: each element keeps its 4:3 size and its distance from the side it belongs to (left, right or
        // centred by where it sits) inside the HUD box, the whole screen with Auto
        const float centre = (a2 + a4 * 0.5f) / Screen.fWidth;
        const float boxWidth = std::min(Screen.fWidth, Screen.fHeight * fHudAspect.load());
        const float boxLeft = (Screen.fWidth - boxWidth) * 0.5f;
        const float right = Screen.fWidth - a2;
        a4 /= Screen.fAspectRatioDiff;
        if (centre < 1.0f / 3.0f)
            a2 = boxLeft + a2 / Screen.fAspectRatioDiff;
        else if (centre > 2.0f / 3.0f)
            a2 = boxLeft + boxWidth - right / Screen.fAspectRatioDiff;
        else
            a2 = a2 / Screen.fAspectRatioDiff + Screen.fHudOffsetReal;
    }
    else
    {
        a2 /= Screen.fAspectRatioDiff;
        a4 /= Screen.fAspectRatioDiff;
        a2 += Screen.fHudOffsetReal;
    }

    float v14 = a6 + 0.5f;
    float v22 = a11 - a7 - 0.5f;
    float v15 = 1.0f / a10;
    float v23 = 1.0f / a11;

    if (a14)
    {
        *reinterpret_cast<float*>(a1 + 0x20) = a2 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_288) + a4;
        *reinterpret_cast<float*>(a1 + 0x38) = a8;
        *reinterpret_cast<float*>(a1 + 0x24) = a3 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_290);
        *reinterpret_cast<float*>(a1 + 0x3C) = a7;
        *reinterpret_cast<float*>(a1 + 0x18) = a6;
        *reinterpret_cast<float*>(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_288);
        *reinterpret_cast<float*>(a1 + 0x1C) = a9;
        *reinterpret_cast<float*>(a1 + 0x04) = a5 + *(float*)(a1 + 0x24);
    }
    else
    {
        *reinterpret_cast<float*>(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_288);
        *reinterpret_cast<float*>(a1 + 0x04) = a3 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_290);
        *reinterpret_cast<float*>(a1 + 0x18) = v15 * v14;
        *reinterpret_cast<float*>(a1 + 0x1C) = v23 * v22;
        *reinterpret_cast<float*>(a1 + 0x20) = a4 + *(float*)a1;
        *reinterpret_cast<float*>(a1 + 0x24) = a5 + *(float*)(a1 + 4);
        *reinterpret_cast<float*>(a1 + 0x38) = (v14 + a8) * v15;
        *reinterpret_cast<float*>(a1 + 0x3C) = (v22 - a9) * v23;
    }
    *reinterpret_cast<uint32_t*>(a1 + 0x40) = *(uint32_t*)(a1 + 0x20);
    *reinterpret_cast<uint32_t*>(a1 + 0x44) = *(uint32_t*)(a1 + 0x24);
    *reinterpret_cast<uint32_t*>(a1 + 0x48) = *(uint32_t*)(a1 + 0x28);
    *reinterpret_cast<uint32_t*>(a1 + 0x4C) = *(uint32_t*)(a1 + 0x2C);
    *reinterpret_cast<uint32_t*>(a1 + 0x50) = *(uint32_t*)(a1 + 0x30);
    *reinterpret_cast<uint32_t*>(a1 + 0x54) = *(uint32_t*)(a1 + 0x34);
    *reinterpret_cast<uint32_t*>(a1 + 0x58) = *(uint32_t*)(a1 + 0x38);
    *reinterpret_cast<uint32_t*>(a1 + 0x5C) = *(uint32_t*)(a1 + 0x3C);
    *reinterpret_cast<uint32_t*>(a1 + 0x60) = *(uint32_t*)(a1 + 0x00);
    *reinterpret_cast<uint32_t*>(a1 + 0x64) = *(uint32_t*)(a1 + 0x04);
    *reinterpret_cast<uint32_t*>(a1 + 0x68) = *(uint32_t*)(a1 + 0x08);
    *reinterpret_cast<uint32_t*>(a1 + 0x6C) = *(uint32_t*)(a1 + 0x0C);
    *reinterpret_cast<uint32_t*>(a1 + 0x70) = *(uint32_t*)(a1 + 0x10);
    *reinterpret_cast<uint32_t*>(a1 + 0x74) = *(uint32_t*)(a1 + 0x14);
    *reinterpret_cast<uint32_t*>(a1 + 0x78) = *(uint32_t*)(a1 + 0x18);
    *reinterpret_cast<uint32_t*>(a1 + 0x7C) = *(uint32_t*)(a1 + 0x1C);
    *reinterpret_cast<uint32_t*>(a1 + 0x3C) = *(uint32_t*)(a1 + 0x1C);
    *reinterpret_cast<uint32_t*>(a1 + 0x24) = *(uint32_t*)(a1 + 0x04);
    *reinterpret_cast<uint32_t*>(a1 + 0x7C) = *(uint32_t*)(a1 + 0x5C);
    *reinterpret_cast<uint32_t*>(a1 + 0x64) = *(uint32_t*)(a1 + 0x44);
    *reinterpret_cast<uint32_t*>(a1 + 0x08) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x0C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x10) = *(uint32_t*)(a12 + 0x00);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x10) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x14) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x14) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x28) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x2C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x30) = *(uint32_t*)(a12 + 0x04);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x30) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x34) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x34) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x48) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x4C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x50) = *(uint32_t*)(a12 + 0x08);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x50) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x54) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x54) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x68) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x6C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x70) = *(uint32_t*)(a12 + 0x0C);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x70) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x74) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x74) = 0xFF000000;
    if (a13)
        *reinterpret_cast<uint32_t*>(a13) = 0;
}

void InitHud()
{
    hook::pattern pattern;
    //HUD
    UpdateHudAspect();
    FixSettings().OnChange("MAIN.HudAspectRatioConstraint", UpdateHudAspect);
    GameFlags.SetAddress(*hook::get_pattern<uint32_t*>("F6 05 ? ? ? ? 01 75 0C D9 41 18", 2)); //0x5C6E70
    Memory.SetAddress(*(uintptr_t**)hook::get_pattern("8B 0D ? ? ? ? DD D8 8B 51 4C DD D8", 2)); //0x5C5D7C
    off_288 = *hook::get_pattern<uint32_t>("D8 84 0A ? ? ? ? D8 44 24 14 D9 58 ? 8B 0D", 3);
    off_290 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -4);
    off_2C0 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -11);
    pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 38 50");
    injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44A6C0
    pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 38 51");
    injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44AB58


}

}
