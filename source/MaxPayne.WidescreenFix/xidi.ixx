module;

#include <stdafx.h>

export module xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(unsigned int, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;

export void InitXidi()
{
    typedef bool (*XidiRegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");

    if (xidiModule)
    {
        auto XidiRegisterProfileCallback = (XidiRegisterProfileCallbackFunc)GetProcAddress(xidiModule, "XidiRegisterProfileCallback");
        XidiSendVibration = (XidiSendVibrationFunc)GetProcAddress(xidiModule, "XidiSendVibration");

        if (XidiRegisterProfileCallback)
        {
            XidiRegisterProfileCallback([]() -> const wchar_t*
            {
                if (CurrentGameMode != eCurrentGameMode::GameMode)
                    return L"Menu";
                else if (CurrentGameMode == eCurrentGameMode::GameMode && bIsPaused)
                    return L"Pause";
                return L"Main";
            });
        }
    }
}