module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module e2_d3d8_driver_mfc;

import ComVars;

BOOL WINAPI DllMainHook(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH || fdwReason == DLL_PROCESS_DETACH)
        return TRUE;

    return shDllMainHook.unsafe_stdcall<BOOL>(hinstDLL, fdwReason, lpvReserved);
}

export void InitE2_D3D8_DRIVER_MFC()
{
    // The game window as the driver opens a video mode: its frame for the window mode (MAIN.WindowMode 1 keeps the
    // game's own frame, the others have none) and square corners
    {
        WindowedModeWrapper::bBorderlessWindowed = FixSettings().Int("MAIN.WindowMode") != 1;
        auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "89 86 ? ? ? ? 8B 49");
        if (!pattern.empty())
        {
            BorderlessWindowedHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                WindowedModeWrapper::GameHWND = (HWND)(regs.eax);
                WindowedModeWrapper::afterCreateWindow();
            });
        }
        else
            OutputDebugStringA("Max Payne: window decoration pattern unavailable.\n");
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 28 8B 45 ? 53 56 33 F6");
    if (!pattern.empty())
        shDllMainHook = safetyhook::create_inline(pattern.get_first(0), DllMainHook);
    else
        OutputDebugStringA("Max Payne: renderer entry pattern unavailable.\n");
}
