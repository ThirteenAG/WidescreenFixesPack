module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module e2_d3d8_driver_mfc;

import ComVars;
import x_inputmfc;

BOOL WINAPI DllMainHook(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH || fdwReason == DLL_PROCESS_DETACH)
        return TRUE;

    return shDllMainHook.unsafe_stdcall<BOOL>(hinstDLL, fdwReason, lpvReserved);
}

// P_D3D::DRV_playVideo calculates the Bink rectangle before each frame.
SafetyHookMid MovieFrameHook;

export void InitE2_D3D8_DRIVER_MFC()
{
    // The game window as the driver opens a video mode: its frame for the window mode (MAIN.WindowMode 1 keeps the
    // game's own frame, the others have none) and square corners
    {
        WindowedModeWrapper::bBorderlessWindowed = FixSettings().Int("MAIN.WindowMode") != 1;
        auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "89 86 ? ? ? ? 8B 49");
        BorderlessWindowedHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            WindowedModeWrapper::GameHWND = (HWND)(regs.eax);
            WindowedModeWrapper::afterCreateWindow();
        });
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 08");
    shDllMainHook = safetyhook::create_inline(pattern.get_first(0), DllMainHook);

    // The Bink size is in EBP; the driver and the destination rectangle are on the game stack.
    pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"),
        "8B 54 24 54 8B 44 24 58 8B 4C 24 24 68 00 00 80 3F");
    MovieFrameHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        const auto driver = *reinterpret_cast<uint8_t**>(regs.esp + 0x2C);
        const auto movie = reinterpret_cast<const uint32_t*>(regs.ebp);
        const float width = static_cast<float>(*reinterpret_cast<const uint32_t*>(driver + 0x108));
        const float height = static_cast<float>(*reinterpret_cast<const uint32_t*>(driver + 0x10C));
        const float videoWidth = static_cast<float>(movie[0]);
        const float videoHeight = static_cast<float>(movie[1]);
        if (width <= 0.0f || height <= 0.0f || videoWidth <= 0.0f || videoHeight <= 0.0f)
            return;
        const bool original = FixSettings().Int("MAIN.FMVWidescreenMode") == 0;
        const float areaWidth = original ? std::min(width, height * 4.0f / 3.0f) : width;
        const float areaHeight = original ? std::min(height, width * 3.0f / 4.0f) : height;
        const float scale = std::min(areaWidth / videoWidth, areaHeight / videoHeight);
        *reinterpret_cast<float*>(regs.esp + 0x28) = (width - videoWidth * scale) * 0.5f;
        *reinterpret_cast<float*>(regs.esp + 0x24) = (height - videoHeight * scale) * 0.5f;
        *reinterpret_cast<float*>(regs.esp + 0x58) = scale;
        *reinterpret_cast<float*>(regs.esp + 0x54) = scale;
    });

    // Videos (intro, "previously" recap, credits), gamepad buttons skip them too
    static auto pVideoPeekMessageA = &VideoPeekMessageA;
    pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "FF 15 ? ? ? ? 85 C0 74 ? 8B 44 24 ? 3D 00 01 00 00 0F 84"); // P_D3D::DRV_playVideo
    injector::WriteMemory(pattern.get_first(2), &pVideoPeekMessageA, true); //0x1002456E
}
