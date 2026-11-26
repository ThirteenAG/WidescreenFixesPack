#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <Xinput.h>

namespace RWGTA
{
    struct PadCheat
    {
        std::string_view buttons;
        std::string_view keyword;
    };

    class PadCheats
    {
        std::array<char, 32> history{};
        uint32_t previous = 0;
        int previousSlot = -1;

    public:
        template<class Cheat>
        const Cheat* Find(const XINPUT_GAMEPAD& pad, int slot, std::span<const Cheat> cheats)
        {
            constexpr uint32_t leftTrigger = 1u << 16, rightTrigger = 1u << 17;
            uint32_t held = pad.wButtons;
            if (pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                held |= leftTrigger;
            if (pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                held |= rightTrigger;
            const uint32_t pressed = held & ~previous;
            previous = held;
            if (slot != previousSlot || slot < 0)
            {
                history.fill(0);
                previousSlot = slot;
                return {};
            }
            if (!pressed)
                return {};

            // Face buttons, D-pad, then shoulders in the native controller's polling order.
            constexpr struct { uint32_t mask; char code; } buttons[] =
            {
                { XINPUT_GAMEPAD_Y, 'T' }, { XINPUT_GAMEPAD_B, 'C' },
                { XINPUT_GAMEPAD_A, 'X' }, { XINPUT_GAMEPAD_X, 'S' },
                { XINPUT_GAMEPAD_DPAD_UP, 'U' }, { XINPUT_GAMEPAD_DPAD_DOWN, 'D' },
                { XINPUT_GAMEPAD_DPAD_LEFT, 'L' }, { XINPUT_GAMEPAD_DPAD_RIGHT, 'R' },
                { XINPUT_GAMEPAD_LEFT_SHOULDER, '1' }, { leftTrigger, '2' },
                { XINPUT_GAMEPAD_RIGHT_SHOULDER, '3' }, { rightTrigger, '4' },
            };
            for (const auto& button : buttons)
            {
                if (!(pressed & button.mask))
                    continue;
                std::move(history.begin() + 1, history.end(), history.begin());
                history.back() = button.code;
                const std::string_view entered(history.data(), history.size());
                for (const auto& cheat : cheats)
                {
                    if (entered.ends_with(cheat.buttons))
                    {
                        history.fill(0);
                        return &cheat;
                    }
                }
            }
            return {};
        }

        std::string_view Update(const XINPUT_GAMEPAD& pad, int slot, std::span<const PadCheat> cheats)
        {
            const auto match = Find(pad, slot, cheats);
            return match ? match->keyword : std::string_view{};
        }
    };
}
