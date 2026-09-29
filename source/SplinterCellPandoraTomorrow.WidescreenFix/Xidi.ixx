module;

#include <stdafx.h>

export module Xidi;

import ComVars;

typedef bool (*XidiSendVibrationFunc)(short, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;

// Chosen on the game thread, which owns the state it depends on, and read by Xidi's polling thread.
// No preference (the configured mapper) until the first frame.
std::atomic<const wchar_t*> XidiProfile = nullptr;

const wchar_t* SelectXidiProfile()
{
    if (bPlayingVideo || bPressStartToContinue)
        return L"Video";

    auto EchelonMainHUDState = UObject::GetState(L"EchelonMainHUD");
    if (EchelonMainHUDState == L"s_MainMenu" || EchelonMainHUDState == L"s_GameMenu")
    {
        return L"Menu";
    }
    else if (EchelonMainHUDState == L"MainHUD" || EchelonMainHUDState == L"s_Slavery")
    {
        auto EPlayerControllerState = UObject::GetState(L"EPlayerController");
        if (EPlayerControllerState == L"s_KeyPadInteract")
        {
            auto EKeyPadState = UObject::GetState(L"EKeyPad");
            auto EElevatorPanelState = UObject::GetState(L"EElevatorPanel");

            if (EElevatorPanelState == L"s_Use")
                return L"Elevator";
            else if (EKeyPadState == L"s_Use")
                return L"Keypad";
        }
        else if (EPlayerControllerState == L"s_Zooming" || EPlayerControllerState == L"s_UsingPalm" || EPlayerControllerState == L"s_LaserMicTargeting")
        {
            return L"Zooming";
        }
        else if (EPlayerControllerState == L"s_Turret")
        {
            return L"Turret";
        }
        else
        {
            auto EGameInteractionState = UObject::GetState(L"EGameInteraction");
            if (EGameInteractionState == L"s_GameInteractionMenu")
                return L"GameInteractionMenu";
            return L"Main";
        }
    }

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
