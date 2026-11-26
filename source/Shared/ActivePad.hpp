#pragma once

#include "Shared/PadInput.hpp"
#include <array>
#include <cstdlib>

namespace PadInput
{
    // One controller owns an input stream until another controller supplies new input.
    class ActivePad
    {
    public:
        bool Read(XINPUT_STATE& state, int leftDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
            int rightDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
        {
            state = {};
            const auto fetch = XInput().getState;
            if (!fetch)
                return false;

            std::array<Sample, XUSER_MAX_COUNT> current{};
            DWORD first = XUSER_MAX_COUNT, changed = XUSER_MAX_COUNT;
            for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
            {
                auto& sample = current[slot];
                sample.connected = fetch(slot, &sample.state) == ERROR_SUCCESS;
                if (!sample.connected)
                    continue;
                if (first == XUSER_MAX_COUNT)
                    first = slot;
                if (NewActivity(sample, Previous[slot], leftDeadzone, rightDeadzone) && (changed == XUSER_MAX_COUNT || slot == Owner))
                    changed = slot;
            }

            const bool ownerConnected = Owner < XUSER_MAX_COUNT && current[Owner].connected;
            const DWORD selected = changed < XUSER_MAX_COUNT ? changed : (ownerConnected ? Owner : first);
            if (selected == XUSER_MAX_COUNT)
            {
                Reset();
                return false;
            }

            state = current[selected].state;
            auto& pad = state.Gamepad;
            if (selected != Owner)
            {
                // A fresh press can act immediately. Release controls held before handoff before they can act again.
                const auto& prior = Previous[selected];
                HeldButtons = prior.connected ? pad.wButtons & prior.state.Gamepad.wButtons : pad.wButtons;
                HeldLeftTrigger = TriggerDown(pad.bLeftTrigger) && (!prior.connected || TriggerDown(prior.state.Gamepad.bLeftTrigger));
                HeldRightTrigger = TriggerDown(pad.bRightTrigger) && (!prior.connected || TriggerDown(prior.state.Gamepad.bRightTrigger));
            }
            Owner = selected;
            Previous = current;

            HeldButtons &= pad.wButtons;
            pad.wButtons &= ~HeldButtons;
            HeldLeftTrigger &= TriggerDown(pad.bLeftTrigger);
            HeldRightTrigger &= TriggerDown(pad.bRightTrigger);
            if (HeldLeftTrigger)
                pad.bLeftTrigger = 0;
            if (HeldRightTrigger)
                pad.bRightTrigger = 0;
            return true;
        }

        void Reset()
        {
            *this = {};
        }

        DWORD Slot() const { return Owner; }

    private:
        struct Sample
        {
            XINPUT_STATE state{};
            bool connected = false;
        };

        static bool TriggerDown(BYTE value)
        {
            return value > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        }

        static bool TriggerMoved(BYTE value, BYTE prior)
        {
            return TriggerDown(value) && (!TriggerDown(prior) || std::abs(int(value) - prior) > 8);
        }

        static bool StickMoved(int x, int y, int priorX, int priorY, int deadzone)
        {
            const auto lengthSquared = [](int a, int b) { return int64_t(a) * a + int64_t(b) * b; };
            const auto threshold = int64_t(deadzone) * deadzone;
            return lengthSquared(x, y) > threshold &&
                (lengthSquared(priorX, priorY) <= threshold || lengthSquared(x - priorX, y - priorY) > 256 * 256);
        }

        static bool NewActivity(const Sample& sample, const Sample& baseline, int leftDeadzone, int rightDeadzone)
        {
            if (baseline.connected && baseline.state.dwPacketNumber == sample.state.dwPacketNumber)
                return false;
            const auto& pad = sample.state.Gamepad;
            const auto& prior = baseline.state.Gamepad;
            return (pad.wButtons & ~prior.wButtons) ||
                TriggerMoved(pad.bLeftTrigger, prior.bLeftTrigger) ||
                TriggerMoved(pad.bRightTrigger, prior.bRightTrigger) ||
                StickMoved(pad.sThumbLX, pad.sThumbLY, prior.sThumbLX, prior.sThumbLY, leftDeadzone) ||
                StickMoved(pad.sThumbRX, pad.sThumbRY, prior.sThumbRX, prior.sThumbRY, rightDeadzone);
        }

        std::array<Sample, XUSER_MAX_COUNT> Previous{};
        DWORD Owner = XUSER_MAX_COUNT;
        WORD HeldButtons = 0;
        bool HeldLeftTrigger = false, HeldRightTrigger = false;
    };
}
