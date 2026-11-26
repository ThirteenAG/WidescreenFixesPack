#pragma once

#include <windows.h>
#include <xinput.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <mutex>
#include <optional>

class MaxPayneVibration
{
public:
    using Send = bool (*)(short, unsigned short, unsigned short);

    void Pulse(Send send, DWORD selected, bool enabled, int strength, std::chrono::milliseconds duration)
    {
        if (!send || !enabled || selected >= XUSER_MAX_COUNT || strength < 0)
            return;

        std::lock_guard lock(mutex);
        const auto slot = static_cast<short>(selected);
        if (sentThisFrame || (activeSlot == slot && strength < activeStrength))
            return;
        sentThisFrame = true;

        const auto now = Clock::now();
        if (activeSlot >= 0 && activeSlot != slot)
        {
            Stop(send, activeSlot, now);
            activeSlot = -1;
        }
        constexpr int MaxStrength = 100;
        constexpr WORD MaxMotorSpeed = 0xFFFF;
        const auto motor = static_cast<WORD>(static_cast<float>(std::clamp(strength, 0, MaxStrength)) /
            MaxStrength * MaxMotorSpeed);
        if (!send(slot, motor, motor))
        {
            // The request can be stored even when the controller is unavailable.
            Stop(send, slot, now);
            activeSlot = -1;
            return;
        }

        pendingStops[slot].reset();
        activeSlot = slot;
        activeStrength = strength;
        stopAt = now + duration;
    }

    void Update(Send send, DWORD selected, bool enabled)
    {
        std::lock_guard lock(mutex);
        sentThisFrame = false;
        const auto now = Clock::now();
        if (activeSlot >= 0 && (!enabled || selected != static_cast<DWORD>(activeSlot) || now >= stopAt))
        {
            Stop(send, activeSlot, now);
            activeSlot = -1;
        }
        if (!send)
            return;
        for (short slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            if (pendingStops[slot] && now >= *pendingStops[slot])
                Stop(send, slot, now);
        }
    }

private:
    using Clock = std::chrono::steady_clock;

    void Stop(Send send, short slot, Clock::time_point now)
    {
        if (send && send(slot, 0, 0))
            pendingStops[slot].reset();
        else
            pendingStops[slot] = now + std::chrono::milliseconds{ 100 };
    }

    std::mutex mutex;
    std::array<std::optional<Clock::time_point>, XUSER_MAX_COUNT> pendingStops{};
    Clock::time_point stopAt{};
    short activeSlot = -1;
    int activeStrength = 0;
    bool sentThisFrame = false;
};
