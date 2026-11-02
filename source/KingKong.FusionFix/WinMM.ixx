module;

#include <stdafx.h>
#include <joystickapi.h>

export module WinMM;

import ComVars;
import Xidi;

decltype(&joyGetNumDevs) joyGetNumDevsOriginal = nullptr;
UINT WINAPI joyGetNumDevsHook()
{
    const auto result = joyGetNumDevsOriginal();
    // The loader serves the joystick functions from Xidi, loaded with the original WinMM
    InitXidi();
    return result;
}

decltype(&joySetCapture) joySetCaptureOriginal = nullptr;
MMRESULT WINAPI joySetCaptureHook(HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged)
{
    joySetCaptureOriginal(hwnd, uJoyID, uPeriod, fChanged);
    return JOYERR_NOERROR;
}

decltype(&joyGetPosEx) joyGetPosExOriginal = nullptr;
MMRESULT WINAPI joyGetPosExHook(UINT uJoyID, LPJOYINFOEX pji)
{
    auto ret = joyGetPosExOriginal(uJoyID, pji);
    constexpr DWORD GAMEPAD_DEADZONE = 10;
    constexpr DWORD centerX = 0x8000;
    constexpr DWORD centerY = 0x8000;
    if (ret == JOYERR_NOERROR && pji && (abs((int)pji->dwXpos - (int)centerX) > GAMEPAD_DEADZONE || abs((int)pji->dwYpos - (int)centerY) > GAMEPAD_DEADZONE))
        wasUsingLeftStick = true;

    return ret;
}

class WinMM
{
public:
    WinMM()
    {
        WFP::onInitEvent() += []()
        {
            auto winmm = GetModuleHandleW(L"winmm.dll");
            joyGetNumDevsOriginal = reinterpret_cast<decltype(joyGetNumDevsOriginal)>(GetProcAddress(winmm, "joyGetNumDevs"));
            joySetCaptureOriginal = reinterpret_cast<decltype(joySetCaptureOriginal)>(GetProcAddress(winmm, "joySetCapture"));
            joyGetPosExOriginal = reinterpret_cast<decltype(joyGetPosExOriginal)>(GetProcAddress(winmm, "joyGetPosEx"));
            IATHook::Replace(GetModuleHandleA(NULL), "WINMM.DLL",
                std::forward_as_tuple("joyGetNumDevs", joyGetNumDevsHook),
                std::forward_as_tuple("joySetCapture", joySetCaptureHook),
                std::forward_as_tuple("joyGetPosEx", joyGetPosExHook)
            );
            InitXidi();
        };
    }
} WinMM;
