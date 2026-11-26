#pragma once

#include <Xinput.h>

namespace RWGTA
{
    class MenuPadButtons
    {
        XINPUT_GAMEPAD previous{};
        WORD blocked = 0;
        bool menuWasActive = false;
        bool blockLT = false, blockRT = false;

    public:
        WORD Update(XINPUT_GAMEPAD& state, bool menuActive)
        {
            const WORD pressed = state.wButtons & ~previous.wButtons;
            if (menuWasActive && !menuActive)
            {
                blocked = previous.wButtons;
                blockLT = previous.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
                blockRT = previous.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            }
            menuWasActive = menuActive;
            previous = state;
            blocked &= state.wButtons;
            blockLT &= state.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            blockRT &= state.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            state.wButtons &= ~blocked;
            if (blockLT) state.bLeftTrigger = 0;
            if (blockRT) state.bRightTrigger = 0;
            return pressed;
        }
    };
}
