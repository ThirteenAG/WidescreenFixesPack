#pragma once

#include <Windows.h>
#include <Xinput.h>
#include <initializer_list>
#include <cstdint>
#include <utility>

namespace PadInput
{
    struct XInputApi
    {
        decltype(&XInputGetState) getState = nullptr;
        decltype(&XInputSetState) setState = nullptr;
    };

    inline const XInputApi& XInput()
    {
        static XInputApi api = []
        {
            XInputApi found;
            for (auto name : { L"xinput1_4.dll", L"xinput9_1_0.dll", L"xinput1_3.dll" })
            {
                auto dll = LoadLibraryW(name);
                auto getState = dll ? GetProcAddress(dll, "XInputGetState") : nullptr;
                if (!getState)
                    continue;
                found.getState = reinterpret_cast<decltype(&XInputGetState)>(getState);
                found.setState = reinterpret_cast<decltype(&XInputSetState)>(GetProcAddress(dll, "XInputSetState"));
                break;
            }
            return found;
        }();
        return api;
    }

    inline bool Read(XINPUT_STATE& state, DWORD& slot)
    {
        state = {};
        auto getState = XInput().getState;
        if (!getState)
            return false;
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            if (getState(i, &state) == ERROR_SUCCESS)
            {
                slot = i;
                return true;
            }
        }
        return false;
    }

    // The developer overlay's pad bits (asi_overlay::Overlay::Gamepad): Select 0x1, L3 0x2, R3 0x4, Start 0x8,
    // d-pad up/right/down/left 0x10/0x20/0x40/0x80, LB 0x400, RB 0x800, Y 0x1000, B 0x2000, A 0x4000, X 0x8000.
    // XInput's own bits differ (d-pad up 0x1, right 0x8): passing them unconverted reads up + right as Select + Start.
    inline uint16_t OverlayButtons(WORD buttons)
    {
        static constexpr std::pair<WORD, uint16_t> Mapping[] =
        {
            { XINPUT_GAMEPAD_BACK, 0x1 }, { XINPUT_GAMEPAD_LEFT_THUMB, 0x2 }, { XINPUT_GAMEPAD_RIGHT_THUMB, 0x4 },
            { XINPUT_GAMEPAD_START, 0x8 }, { XINPUT_GAMEPAD_DPAD_UP, 0x10 }, { XINPUT_GAMEPAD_DPAD_RIGHT, 0x20 },
            { XINPUT_GAMEPAD_DPAD_DOWN, 0x40 }, { XINPUT_GAMEPAD_DPAD_LEFT, 0x80 },
            { XINPUT_GAMEPAD_LEFT_SHOULDER, 0x400 }, { XINPUT_GAMEPAD_RIGHT_SHOULDER, 0x800 },
            { XINPUT_GAMEPAD_Y, 0x1000 }, { XINPUT_GAMEPAD_B, 0x2000 }, { XINPUT_GAMEPAD_A, 0x4000 }, { XINPUT_GAMEPAD_X, 0x8000 },
        };
        uint16_t mapped = 0;
        for (auto [button, bit] : Mapping)
        {
            if (buttons & button)
                mapped |= bit;
        }
        return mapped;
    }

    // A stick axis for the overlay: -127..127, positive right and down (pass -sThumbLY for Y). Dividing by 256
    // would turn -sThumbLY = 32768 (stick fully down) into int8 -128, fully up.
    inline int8_t OverlayAxis(int value)
    {
        const int scaled = value / 258;
        return static_cast<int8_t>(scaled < -127 ? -127 : scaled > 127 ? 127 : scaled);
    }

    // The overlay's pad from an XInput state: buttons and the left stick
    template <typename Overlay>
    inline void ToOverlay(Overlay& overlay, const XINPUT_GAMEPAD& pad)
    {
        overlay.Gamepad(OverlayButtons(pad.wButtons), OverlayAxis(pad.sThumbLX), OverlayAxis(-int(pad.sThumbLY)));
    }

    enum class Family { Xbox = 1, PlayStation = 2, Nintendo = 3 };

    inline Family ResolveFamily(int selected)
    {
        return selected >= 1 && selected <= 3 ? Family(selected) : Family::Xbox;
    }

    inline const wchar_t* ButtonName(WORD button, Family family)
    {
        bool ps = family == Family::PlayStation;
        bool nintendo = family == Family::Nintendo;
        switch (button)
        {
            case XINPUT_GAMEPAD_A: return ps ? L"CROSS" : nintendo ? L"B" : L"A";
            case XINPUT_GAMEPAD_B: return ps ? L"CIRCLE" : nintendo ? L"A" : L"B";
            case XINPUT_GAMEPAD_X: return ps ? L"SQUARE" : nintendo ? L"Y" : L"X";
            case XINPUT_GAMEPAD_Y: return ps ? L"TRIANGLE" : nintendo ? L"X" : L"Y";
            case XINPUT_GAMEPAD_START: return ps ? L"OPTIONS" : nintendo ? L"PLUS" : L"MENU";
            case XINPUT_GAMEPAD_BACK: return ps ? L"SHARE" : nintendo ? L"MINUS" : L"VIEW";
        }
        return L"?";
    }
}
