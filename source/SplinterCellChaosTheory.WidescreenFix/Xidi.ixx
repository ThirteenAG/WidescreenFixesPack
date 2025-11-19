module;

#include <stdafx.h>

export module Xidi;

import ComVars;
import GUI;
import HudIDs;

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
                if (CMenusManager::IsMenuDisplayed(Page::P_Controls_joystick))
                    return L"P_Controls_joystick";
                else if (!CMenusManager::IsMenuDisplayed(Page::P_Map) && (CMenusManager::IsOpsatDisplayed() || CMenusManager::IsMenuDisplayed(Page::P_Controls_Popup_Joy_Selection)))
                    return L"Opsat";
                else if (CMenusManager::IsMainMenuDisplayed())
                    return L"Menu";
                return L"Main";
            });
        }
    }
}