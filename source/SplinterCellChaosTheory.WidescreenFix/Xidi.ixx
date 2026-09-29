module;

#include <stdafx.h>

export module Xidi;

import ComVars;
import GUI;
import HudIDs;

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;

// Chosen on the game thread, which owns the menu state it depends on, and read by Xidi's polling
// thread. No preference (the configured mapper) until the first frame.
std::atomic<const wchar_t*> XidiProfile = nullptr;

const wchar_t* SelectXidiProfile()
{
    if (CMenusManager::IsMenuDisplayed(Page::P_Controls_joystick) && !CMenusManager::IsMenuDisplayed(Page::P_Controls_Popup_Joy_Selection))
        return L"P_Controls_joystick";
    else if (!CMenusManager::IsMenuDisplayed(Page::P_Map) && (CMenusManager::IsOpsatDisplayed() || CMenusManager::IsMenuDisplayed(Page::P_Controls_Popup_Joy_Selection)))
        return L"Opsat";
    else if (CMenusManager::IsMainMenuDisplayed())
        return L"Menu";
    return L"Main";
}

// Called once per frame from the game thread
export void UpdateXidiProfile()
{
    XidiProfile.store(SelectXidiProfile(), std::memory_order_relaxed);
}

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
                return XidiProfile.load(std::memory_order_relaxed);
            });
        }
    }
}
