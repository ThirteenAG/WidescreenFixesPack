module;

#include <stdafx.h>

export module e2_d3d8_driver_mfc;

import ComVars;
import x_inputmfc;

BOOL WINAPI DllMainHook(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH || fdwReason == DLL_PROCESS_DETACH)
        return TRUE;

    return shDllMainHook.unsafe_stdcall<BOOL>(hinstDLL, fdwReason, lpvReserved);
}

export void InitE2_D3D8_DRIVER_MFC()
{
    CIniReader iniReader("");
    bool BorderlessWindowedMode = iniReader.ReadInteger("MAIN", "BorderlessWindowedMode", 1) != 0;
    if (BorderlessWindowedMode)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "89 86 ? ? ? ? 8B 49");
        BorderlessWindowedHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
        {
            WindowedModeWrapper::GameHWND = (HWND)(regs.eax);
            WindowedModeWrapper::afterCreateWindow();
        });
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 08");
    shDllMainHook = safetyhook::create_inline(pattern.get_first(0), DllMainHook);

    // Videos (intro, "previously" recap, credits), gamepad buttons skip them too
    static auto pVideoPeekMessageA = &VideoPeekMessageA;
    pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "FF 15 ? ? ? ? 85 C0 74 ? 8B 44 24 ? 3D 00 01 00 00 0F 84"); // P_D3D::DRV_playVideo
    injector::WriteMemory(pattern.get_first(2), &pVideoPeekMessageA, true); //0x1002456E
}
