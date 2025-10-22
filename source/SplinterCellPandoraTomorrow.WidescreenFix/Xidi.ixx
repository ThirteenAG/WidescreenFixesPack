module;

#include <stdafx.h>

export module Xidi;

import ComVars;

export void InitXidi()
{
    typedef bool (*RegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    auto xidiModule = GetModuleHandleW(L"Xidi.dll");

    if (xidiModule)
    {
        auto RegisterProfileCallback = (RegisterProfileCallbackFunc)GetProcAddress(xidiModule, "RegisterProfileCallback");
        if (RegisterProfileCallback)
        {
            RegisterProfileCallback([]() -> const wchar_t*
            {
                constexpr auto s_Turret = 8330;
                constexpr auto s_KeyPadInteract = 8338;
                constexpr auto s_Zooming = 6942;
                constexpr auto s_PlayerSniping = 7059;
                constexpr auto s_UsingPalm = 8274;
                constexpr auto s_LaserMicTargeting = 10015;
                constexpr auto s_GameInteractionMenu = 8122;
                constexpr auto s_FirstPersonTargeting = 7060;
                constexpr auto s_RappellingTargeting = 7294;
                constexpr auto s_PlayerBTWTargeting = 7548;

                if (bPlayingVideo || bPressStartToContinue)
                    return L"Video";

                if (EchelonMainHUDState == 8707 || EchelonMainHUDState == 8708)
                {
                    switch (EPlayerControllerState)
                    {
                    case s_KeyPadInteract:
                    {
                        if (bElevatorPanel)
                            return L"Elevator";
                        else if (bKeyPad)
                            return L"Keypad";
                        break;
                    }
                    case s_Zooming:
                    case s_UsingPalm:
                    case s_LaserMicTargeting:
                        return L"Zooming";
                    case s_Turret:
                        return L"Turret";
                    default:
                    {
                        if (EGameInteractionState == s_GameInteractionMenu)
                            return L"GameInteractionMenu";
                        return L"Main";
                    }
                    }
                }

                return L"Main";
            });
        }
    }
}