module;

#include <stdafx.h>
#include <mutex>
#include <optional>
#include "FixSettings.hpp"
#include "LEDEffects.h"

export module LightSync;

namespace
{
    struct Health
    {
        int16_t left;
        int16_t right;
    };

    SafetyHookInline updateHook, resetHook;
    std::mutex healthMutex;
    std::optional<Health> health;

    void Publish(std::optional<Health> value)
    {
        const std::lock_guard lock(healthMutex);
        health = value;
    }

    int __fastcall Update(const uint8_t* manager, void*)
    {
        const auto result = updateHook.unsafe_thiscall<int>(manager);
        if (!LEDEffects::bLogiLedInitialized)
            return result;

        std::optional<Health> snapshot;
        if (const auto player = *reinterpret_cast<const uint8_t* const*>(manager + 0x28))
        {
            if (const auto partner = *reinterpret_cast<const uint8_t* const*>(player + 0x2A74))
            {
                snapshot = Health{
                    *reinterpret_cast<const int16_t*>(partner + 0x1364),
                    *reinterpret_cast<const int16_t*>(player + 0x1364)
                };
            }
        }
        Publish(snapshot);
        return result;
    }

    int __fastcall Reset(void* manager, void*)
    {
        const auto result = resetHook.unsafe_thiscall<int>(manager);
        Publish(std::nullopt);
        return result;
    }

    void UpdateSide(int value, bool left)
    {
        const bool dead = value <= 1;
        const bool danger = value <= 2;
        const bool caution = value <= 250;
        const auto [red, green, blue] = danger ? std::array{ 26, 4, 4 } :
            caution ? std::array{ 50, 30, 4 } : std::array{ 10, 30, 4 };

        if (left)
            LEDEffects::SetLightingLeftSide(red, green, blue, !dead, dead);
        else
            LEDEffects::SetLightingRightSide(red, green, blue, !dead, dead);

        const auto draw = left ? LEDEffects::DrawCardiogram : LEDEffects::DrawCardiogramNumpad;
        draw(danger ? 100 : caution ? 67 : 0, caution ? 0 : 100, 0, 0, 0, 0, dead);
    }

    void UpdateLighting()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::optional<Health> snapshot;
        {
            const std::lock_guard lock(healthMutex);
            snapshot = health;
        }

        if (snapshot)
        {
            UpdateSide(snapshot->left, true);
            UpdateSide(snapshot->right, false);
        }
        else
        {
            LogiLedStopEffects();
            LEDEffects::SetLighting(98, 87, 24);
        }
    }
}

export namespace LightSync
{
    void Init()
    {
        if (!FixSettings().Bool("MAIN.LightSyncRGB"))
            return;

        auto update = hook::pattern("53 55 57 8B F9 8B 87 E0 05 00 00 89 87 DC 05 00 00 8B 0D ? ? ? ? E8 ? ? ? ? 8D 6F 24 BB 04 00 00 00");
        auto reset = hook::pattern("53 56 8B F1 57 8D 7E 24 BB 04 00 00 00 8D 49 00 8B 07 50 8B CE E8 ? ? ? ? 83 C7 04 83 EB 01 75 ? 8B 46 3C");
        updateHook = safetyhook::create_inline(update.get_first(), Update);
        resetHook = safetyhook::create_inline(reset.get_first(), Reset);
        LEDEffects::Inject(UpdateLighting);
    }
}
