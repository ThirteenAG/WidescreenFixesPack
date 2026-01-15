module;

#include <stdafx.h>

export module Xidi;

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
                if (bPlayingVideo || bPressStartToContinue)
                    return L"Video";

                if (IsEnhanced())
                    return L"EnhancedMain";

                bool bIsMainMenu = UObject::GetState(L"EPCConsole") == L"UWindow";
                if (bIsMainMenu)
                    return L"Menu";

                return L"Main";
            });
        }
    }
}
