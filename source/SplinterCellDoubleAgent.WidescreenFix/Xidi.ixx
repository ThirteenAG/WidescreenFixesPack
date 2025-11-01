module;

#include <stdafx.h>

export module Xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(unsigned int, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;

export void InitXidi()
{
    typedef bool (*RegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");

    if (xidiModule)
    {
        //auto RegisterProfileCallback = (RegisterProfileCallbackFunc)GetProcAddress(xidiModule, "RegisterProfileCallback");
        XidiSendVibration = (XidiSendVibrationFunc)GetProcAddress(xidiModule, "XidiSendVibration");

        //if (RegisterProfileCallback)
        //{
        //    RegisterProfileCallback([]() -> const wchar_t*
        //    {
        //        return L"Main";
        //    });
        //}
    }
}