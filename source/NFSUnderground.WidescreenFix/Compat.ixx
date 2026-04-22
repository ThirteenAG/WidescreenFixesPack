module;

#include <stdafx.h>

export module Compat;

import ComVars;

export namespace XtendedInput
{
    HMODULE mhXtendedInput = NULL;
    float(__cdecl* SetFEScale)(float val) = nullptr;
    bool(__cdecl* GetUseWin32Cursor)() = nullptr;
    bool bLookedForXInput = false;
    bool bFoundXInput = false;
}

namespace XtendedInputCompat
{
    struct XI_GAMEPAD
    {
        WORD  wButtons;
        BYTE  bLeftTrigger;
        BYTE  bRightTrigger;
        SHORT sThumbLX;
        SHORT sThumbLY;
        SHORT sThumbRX;
        SHORT sThumbRY;
    };
    struct XI_STATE
    {
        DWORD      dwPacketNumber;
        XI_GAMEPAD Gamepad;
    };

    typedef DWORD(WINAPI* PFN_XInputGetState)(DWORD dwUserIndex, XI_STATE* pState);

    HMODULE            hXInputLib = nullptr;
    PFN_XInputGetState pfnXInputGetState = nullptr;
    bool               bActive = false;

    float fDeadzone = 0.0f;

    void Init()
    {
        const char* xinputDlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
        for (auto dll : xinputDlls)
        {
            hXInputLib = LoadLibraryA(dll);
            if (hXInputLib) break;
        }

        if (hXInputLib)
            pfnXInputGetState = reinterpret_cast<PFN_XInputGetState>(
                GetProcAddress(hXInputLib, "XInputGetState"));

        bActive = (pfnXInputGetState != nullptr);
    }

    void Update()
    {
        if (hWnd != GetForegroundWindow())
            return;

        // --- Right stick via XInput (port 0) ---
        XI_STATE state = {};
        if (bActive && pfnXInputGetState(0, &state) == 0 /*ERROR_SUCCESS*/)
        {
            // Convert XInput signed (-32768..32767) to DInput unsigned (0..65535)
            // DInput Y axis is inverted (down = higher value), XInput Y up = positive
            g_RightStick.RawX = (int)state.Gamepad.sThumbRX + 32768;
            g_RightStick.RawY = -(int)state.Gamepad.sThumbRY + 32768;

            // Normalise to -1..1 matching the DInput hook convention
            double rx = (double)state.Gamepad.sThumbRX / 32767.5;
            double ry = -(double)state.Gamepad.sThumbRY / 32767.5;

            if (std::abs(rx) <= fDeadzone) rx = 0.0;
            if (std::abs(ry) <= fDeadzone) ry = 0.0;

            g_RightStick.X = (float)rx;
            g_RightStick.Y = (float)ry;
        }
    }

    HCURSOR WINAPI SetCursor_Hook(HCURSOR hCursor)
    {
        if (bHideCursorForMouseLook)
            return SetCursor(nullptr);
        return SetCursor(hCursor);
    }
}

void Init()
{
    CIniReader iniReader("");
    static float fRightStickDeadzone = iniReader.ReadFloat("MISC", "RightStickDeadzone", 10.0f);

    XtendedInput::mhXtendedInput = GetModuleHandleA("NFSU_XtendedInput.asi");
    XtendedInput::SetFEScale = reinterpret_cast<decltype(XtendedInput::SetFEScale)>(GetProcAddress(XtendedInput::mhXtendedInput, "SetFEScale"));
    XtendedInput::GetUseWin32Cursor = reinterpret_cast<decltype(XtendedInput::GetUseWin32Cursor)>(GetProcAddress(XtendedInput::mhXtendedInput, "GetUseWin32Cursor"));
    XtendedInput::bFoundXInput = (XtendedInput::SetFEScale != nullptr) && (XtendedInput::GetUseWin32Cursor != nullptr);

    XtendedInputCompat::fDeadzone = (fRightStickDeadzone > 0.0f) ? fRightStickDeadzone / 200.0f : 0.0f;

    XtendedInputCompat::Init();

    WFP::onGameProcessEvent() += []()
    {
        XtendedInputCompat::Update();
    };
}

class Compat
{
public:
    Compat()
    {
        WFP::onInitEvent() += []()
        {
            CallbackHandler::RegisterCallback(L"NFSU_XtendedInput.asi", Init);
        };
    }
} Compat;