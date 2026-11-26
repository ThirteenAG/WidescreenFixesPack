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

void ResetMouseSample()
{
    g_Mouse.DeltaX = g_Mouse.DeltaY = g_Mouse.Wheel = 0;
    nWheelAccum = 0;
    bFirstFrame = true;
}

void Update()
{
    g_RightStick.RawX = g_RightStick.RawY = 32768;
    g_RightStick.X = g_RightStick.Y = 0.0f;
    bBothSticksClicked = false;
    if (hWnd != GetForegroundWindow())
    {
        ResetMouseSample();
        RawMouse::Clear();
        return;
    }

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

        // One round dead zone; past it the stick's travel is rescaled to the full range
        double magnitude = std::hypot(rx, ry);
        if (magnitude <= fDeadzone)
            rx = ry = 0.0;
        else
        {
            double scale = (std::min(magnitude, 1.0) - fDeadzone) / (1.0 - fDeadzone) / magnitude;
            rx *= scale;
            ry *= scale;
        }

        g_RightStick.X = (float)rx;
        g_RightStick.Y = (float)ry;

        bBothSticksClicked = (state.Gamepad.wButtons & XI_BOTH_THUMBS) == XI_BOTH_THUMBS;
    }

    int rawX = 0, rawY = 0;
    const bool wasRaw = g_Mouse.Raw;
    g_Mouse.Raw = RawMouse::Consume(rawX, rawY);

    POINT ptCurrent{};
    if (!GetCursorPos(&ptCurrent))
        return ResetMouseSample();

    POINT ptNext = ptCurrent;
    if (bHideCursorForMouseLook)
    {
        RECT rect{};
        if (!GetWindowRect(hWnd, &rect))
            return ResetMouseSample();
        ptNext = { rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2 };
        // The OS may constrain the warp to a position other than the requested centre.
        if (!SetCursorPos(ptNext.x, ptNext.y) || !GetCursorPos(&ptNext))
            return ResetMouseSample();
    }

    const bool haveBaseline = !bFirstFrame && wasRaw == g_Mouse.Raw;
    g_Mouse.DeltaX = haveBaseline ? (g_Mouse.Raw ? rawX : ptCurrent.x - ptLastCursor.x) : 0;
    g_Mouse.DeltaY = haveBaseline ? (g_Mouse.Raw ? rawY : ptCurrent.y - ptLastCursor.y) : 0;
    ptLastCursor = ptNext;
    bFirstFrame = false;
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
            fDeadzone = std::clamp(FixSettings().Float("GAMEPAD.RightStickDeadzone"), 0.0f, 50.0f) / 100.0f;

            Init();

            WFP::onGameProcessEvent() += []()
            {
                fDeadzone = std::clamp(FixSettings().Float("GAMEPAD.RightStickDeadzone"), 0.0f, 50.0f) / 100.0f;
                const bool useRawMouse = FixSettings().Bool("MAIN.RawInputMouse") && FixSettings().Bool("CAMERA.FreeLook");
                RawMouse::Enabled.store(useRawMouse);
                RawMouse::Update(hWnd.get());
                Update();
            };
        };
    }
} Input;
