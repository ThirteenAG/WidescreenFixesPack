module;

#include <stdafx.h>
#include <joystickapi.h>

export module WinMM;

import ComVars;

SafetyHookInline shjoySetCapture = {};
MMRESULT WINAPI joySetCaptureHook(HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged)
{
    shjoySetCapture.unsafe_stdcall(hwnd, uJoyID, uPeriod, fChanged);
    return JOYERR_NOERROR;
}

SafetyHookInline shjoyGetPosEx = {};
MMRESULT WINAPI joyGetPosExHook(UINT uJoyID, LPJOYINFOEX pji)
{
    auto ret = shjoyGetPosEx.unsafe_stdcall<MMRESULT>(uJoyID, pji);
    constexpr DWORD GAMEPAD_DEADZONE = 10;
    constexpr DWORD centerX = 0x8000;
    constexpr DWORD centerY = 0x8000;
    if (abs((int)pji->dwXpos - (int)centerX) > GAMEPAD_DEADZONE || abs((int)pji->dwYpos - (int)centerY) > GAMEPAD_DEADZONE)
        wasUsingLeftStick = true;

    return ret;
}

export void InitWinMM()
{
    shjoySetCapture = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"winmm"), "joySetCapture"), joySetCaptureHook);
    shjoyGetPosEx = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"winmm"), "joyGetPosEx"), joyGetPosExHook);
}