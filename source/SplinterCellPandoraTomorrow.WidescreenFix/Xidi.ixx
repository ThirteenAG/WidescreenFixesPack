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