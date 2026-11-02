module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "RawMouse.hpp"
#include "Shared/PadInput.hpp"

export module Input;

import ComVars;

constexpr WORD XI_LEFT_THUMB = 0x0040;
constexpr WORD XI_RIGHT_THUMB = 0x0080;
constexpr WORD XI_BOTH_THUMBS = XI_LEFT_THUMB | XI_RIGHT_THUMB;

// Mouse delta tracking
POINT ptLastCursor = {};
bool  bFirstFrame = true;
int   nWheelAccum = 0;
float fDeadzone = 0.0f;

void Init()
{
    (void)PadInput::XInput();
}

void Update()
{
    g_RightStick.RawX = g_RightStick.RawY = 32768;
    g_RightStick.X = g_RightStick.Y = 0.0f;
    bBothSticksClicked = false;
    if (hWnd != GetForegroundWindow())
        return;

    XINPUT_STATE state{};
    DWORD slot{};
    if (PadInput::Read(state, slot))
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

        bBothSticksClicked = (state.Gamepad.wButtons & XI_BOTH_THUMBS) == XI_BOTH_THUMBS;
    }

    POINT ptCurrent;
    GetCursorPos(&ptCurrent);

    // The mouse's raw movement when raw input is on, else the cursor's
    int rawX = 0, rawY = 0;
    g_Mouse.Raw = RawMouse::Consume(rawX, rawY);
    if (g_Mouse.Raw)
    {
        g_Mouse.DeltaX = bFirstFrame ? 0 : rawX;
        g_Mouse.DeltaY = bFirstFrame ? 0 : rawY;
        bFirstFrame = false;
    }
    else if (!bFirstFrame)
    {
        g_Mouse.DeltaX = ptCurrent.x - ptLastCursor.x;
        g_Mouse.DeltaY = ptCurrent.y - ptLastCursor.y;
    }
    else
    {
        g_Mouse.DeltaX = 0;
        g_Mouse.DeltaY = 0;
        bFirstFrame = false;
    }

    if (bHideCursorForMouseLook)
    {
        RECT rect; GetWindowRect(hWnd, &rect);
        int cx = (rect.left + rect.right) / 2;
        int cy = (rect.top + rect.bottom) / 2;
        SetCursorPos(cx, cy);
        ptLastCursor = { cx, cy };
    }
    else
    {
        ptLastCursor = ptCurrent;
    }

    g_Mouse.Wheel = nWheelAccum;
    nWheelAccum = 0;
}

class Input
{
public:
    Input()
    {
        WFP::onInitEvent() += []()
        {
            float fRightStickDeadzone = FixSettings().Float("MISC.RightStickDeadzone");
            fDeadzone = (fRightStickDeadzone > 0.0f) ? fRightStickDeadzone / 200.0f : 0.0f;

            Init();

            WFP::onGameProcessEvent() += []()
            {
                fDeadzone = std::clamp(FixSettings().Float("MISC.RightStickDeadzone"), 0.0f, 50.0f) / 200.0f;
                RawMouse::Enabled = FixSettings().Bool("MAIN.RawInputMouse") && FixSettings().Bool("CAMERA.Enable");
                RawMouse::Update(hWnd.get());
                Update();
            };
        };
    }
} Input;