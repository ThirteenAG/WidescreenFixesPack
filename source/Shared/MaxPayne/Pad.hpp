#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include "Shared/PadInput.hpp"
#include "Shared/MaxPayne/GamepadButtons.hpp"

// The controller of both Max Payne games, read through XInput once per input update of the game. The buttons use the
// numbers of MaxPayneGamepadButtons::Button, the triggers count as buttons. The sticks give -1..1 with a round deadzone,
// y pointing down as the game's own axes do.
namespace MaxPaynePad
{
    using Button = MaxPayneGamepadButtons::Button;

    struct Stick
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct State
    {
        XINPUT_STATE current{};
        XINPUT_STATE previous{};
        POINT leftActivity{};
        POINT rightActivity{};
        DWORD slot = 0;
        bool connected = false;
        bool enabled = false;
        ULONGLONG nextProbe = 0;
    };

    inline State& Get()
    {
        static State state;
        return state;
    }

    constexpr float Deadzone = 0.15f;

    inline bool Held(const XINPUT_GAMEPAD& pad, int button)
    {
        static constexpr WORD Masks[] =
        {
            XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
            XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, 0, 0,
            XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
            XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
        };
        if (button == Button::LT)
            return pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        if (button == Button::RT)
            return pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        if (button < 0 || button >= Button::Count)
            return false;
        return (pad.wButtons & Masks[button]) != 0;
    }

    inline bool Pressed(int button)
    {
        return Held(Get().current.Gamepad, button);
    }

    inline bool JustPressed(int button)
    {
        auto& s = Get();
        return Held(s.current.Gamepad, button) && !Held(s.previous.Gamepad, button);
    }

    inline bool JustReleased(int button)
    {
        auto& s = Get();
        return !Held(s.current.Gamepad, button) && Held(s.previous.Gamepad, button);
    }

    // Round deadzone: the stick's travel past it is scaled back to 0..1, the direction kept
    inline Stick ReadStick(SHORT x, SHORT y)
    {
        Stick stick{ std::clamp(x / 32767.0f, -1.0f, 1.0f), std::clamp(-y / 32767.0f, -1.0f, 1.0f) };
        float length = std::sqrt(stick.x * stick.x + stick.y * stick.y);
        if (length <= Deadzone)
            return {};
        float scale = (std::min(length, 1.0f) - Deadzone) / (1.0f - Deadzone) / length;
        return { stick.x * scale, stick.y * scale };
    }

    inline Stick LeftStick(bool previous = false)
    {
        auto& pad = previous ? Get().previous.Gamepad : Get().current.Gamepad;
        return ReadStick(pad.sThumbLX, pad.sThumbLY);
    }

    inline Stick RightStick()
    {
        auto& pad = Get().current.Gamepad;
        return ReadStick(pad.sThumbRX, pad.sThumbRY);
    }

    // A stick used as four buttons: a direction counts within 67.5 degrees of it, so diagonals press two
    inline bool Direction(const Stick& stick, float x, float y)
    {
        float length = std::sqrt(stick.x * stick.x + stick.y * stick.y);
        return length > 0.0f && (stick.x * x + stick.y * y) > 0.38f * length;
    }

    // From the game's input update. Without a controller, XInput is asked again once a second. heldSticksAreInput: the
    // controller's mapping turns a held stick into keyboard or mouse input (the menus), which keeps the controller in use.
    inline void Update(bool heldSticksAreInput = false)
    {
        auto& s = Get();
        s.previous = s.current;
        if (!s.enabled)
        {
            s.current = {};
            return;
        }
        auto getState = PadInput::XInput().getState;
        bool read = s.connected && getState && getState(s.slot, &s.current) == ERROR_SUCCESS;
        if (!read)
        {
            s.leftActivity = {};
            s.rightActivity = {};
            const bool wasConnected = s.connected;
            bool probe = s.connected || GetTickCount64() >= s.nextProbe;
            s.connected = probe && PadInput::Read(s.current, s.slot);
            if (!s.connected)
            {
                if (wasConnected)
                    MaxPayneGamepadButtons::SelectController(XUSER_MAX_COUNT);
                s.current = {};
                if (probe)
                    s.nextProbe = GetTickCount64() + 1000;
                return;
            }
            s.previous = {};
            MaxPayneGamepadButtons::SelectController(s.slot);
        }

        // Held input must not take the device back from a newer keyboard or mouse action. Releases count: the
        // controller's mapping sends a key release for them.
        auto& now = s.current.Gamepad;
        auto changed = [heldSticksAreInput](SHORT x, SHORT y, POINT& lastActivity)
        {
            const auto stick = ReadStick(x, y);
            if (stick.x == 0.0f && stick.y == 0.0f)
            {
                const bool released = lastActivity.x != 0 || lastActivity.y != 0;
                lastActivity = {};
                return released;
            }
            if (!heldSticksAreInput && std::abs(x - lastActivity.x) <= 128 && std::abs(y - lastActivity.y) <= 128)
                return false;
            lastActivity = { x, y };
            return true;
        };
        const bool leftChanged = changed(now.sThumbLX, now.sThumbLY, s.leftActivity);
        const bool rightChanged = changed(now.sThumbRX, now.sThumbRY, s.rightActivity);
        bool active = leftChanged || rightChanged;
        for (int i = 0; i < Button::Count && !active; ++i)
            active = JustPressed(i) || JustReleased(i);
        if (active)
            MaxPayneGamepadButtons::OnPadActivity();
    }

    // The game's input starts and ends
    inline void Enable(bool enable)
    {
        auto& s = Get();
        s.enabled = enable;
        s.nextProbe = 0;
        if (!enable)
        {
            MaxPayneGamepadButtons::SelectController(XUSER_MAX_COUNT);
            s.connected = false;
            s.current = {};
            s.previous = {};
        }
    }

    // The developer overlay's controller state: its button bits and the left stick
    template<typename Report>
    inline void ToOverlay(Report report)
    {
        static constexpr uint16_t Bits[] =
        {
            0x4000, 0x2000, 0x8000, 0x1000, 0x400, 0x800, 0x100, 0x200,
            0x1, 0x8, 0x2, 0x4, 0x10, 0x40, 0x80, 0x20,
        };
        uint16_t held = 0;
        for (int i = 0; i < Button::Count; ++i)
        {
            if (Pressed(i))
                held |= Bits[i];
        }
        auto& pad = Get().current.Gamepad;
        auto axis = [](int value)
        {
            return static_cast<int8_t>(std::clamp(value / 256, -127, 127));
        };
        report(held, axis(pad.sThumbLX), axis(-pad.sThumbLY));
    }

    // Buttons that skip videos, pressed on any controller. The intro plays before the game's input starts.
    inline WORD VideoSkipButtons()
    {
        auto getState = PadInput::XInput().getState;
        WORD buttons = 0;
        for (DWORD i = 0; getState && i < XUSER_MAX_COUNT; ++i)
        {
            XINPUT_STATE state{};
            if (getState(i, &state) == ERROR_SUCCESS)
                buttons |= state.Gamepad.wButtons;
        }
        return buttons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK);
    }
}
