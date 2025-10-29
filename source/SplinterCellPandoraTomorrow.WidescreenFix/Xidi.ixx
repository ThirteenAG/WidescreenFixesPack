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
        auto RegisterProfileCallback = (RegisterProfileCallbackFunc)GetProcAddress(xidiModule, "RegisterProfileCallback");
        XidiSendVibration = (XidiSendVibrationFunc)GetProcAddress(xidiModule, "XidiSendVibration");

        if (RegisterProfileCallback)
        {
            RegisterProfileCallback([]() -> const wchar_t*
            {
                if (bPlayingVideo || bPressStartToContinue)
                    return L"Video";

                auto EchelonMainHUDState = UObject::GetState(L"EchelonMainHUD");
                if (EchelonMainHUDState == L"MainHUD" || EchelonMainHUDState == L"s_Slavery")
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
            });
        }
    }
}