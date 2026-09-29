module;

#include <stdafx.h>

export module xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
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
                switch (GamepadProfile.load(std::memory_order_relaxed))
                {
                case eGamepadProfile::Main:
                    return L"Main";
                case eGamepadProfile::Pause:
                    return L"Pause";
                default:
                    return L"Menu";
                }
            });
        }
    }
}