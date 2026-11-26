#pragma once

#include "Shared/PadInput.hpp"
#include <array>

class ActivePad
{
public:
    struct Sample
    {
        XINPUT_GAMEPAD current{}, previous{};
        DWORD slot = XUSER_MAX_COUNT;
    };

    template<typename HasInput>
    Sample Read(HasInput hasInput)
    {
        std::array<XINPUT_GAMEPAD, XUSER_MAX_COUNT> current{};
        std::array<bool, XUSER_MAX_COUNT> connected{};
        const auto getState = PadInput::XInput().getState;
        for (DWORD slot = 0; getState && slot < XUSER_MAX_COUNT; ++slot)
        {
            XINPUT_STATE state{};
            connected[slot] = getState(slot, &state) == ERROR_SUCCESS;
            current[slot] = state.Gamepad;
        }

        const bool activeConnected = active < XUSER_MAX_COUNT && connected[active];
        // Keep a controller while it has input. An idle controller cannot hide an active one.
        if (!activeConnected || !hasInput(current[active]))
        {
            DWORD selected = activeConnected ? active : XUSER_MAX_COUNT;
            for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
            {
                if (!connected[slot])
                    continue;
                if (hasInput(current[slot]))
                {
                    selected = slot;
                    break;
                }
                if (selected == XUSER_MAX_COUNT)
                    selected = slot;
            }
            active = selected;
        }

        Sample sample{};
        // A newly connected controller starts with its current state as its edge baseline.
        if (active < XUSER_MAX_COUNT)
            sample = { current[active], previousConnected[active] ? previous[active] : current[active], active };
        previous = current;
        previousConnected = connected;
        return sample;
    }

private:
    DWORD active = XUSER_MAX_COUNT;
    std::array<XINPUT_GAMEPAD, XUSER_MAX_COUNT> previous{};
    std::array<bool, XUSER_MAX_COUNT> previousConnected{};
};
