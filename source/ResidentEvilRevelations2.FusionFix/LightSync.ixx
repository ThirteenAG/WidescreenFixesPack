module;

#include <stdafx.h>
#include <array>
#include <mutex>
#include <optional>
#include "FixSettings.hpp"
#include "LEDEffects.h"

export module LightSync;

namespace
{
    struct Health
    {
        int left;
        int right;
    };

    using PlayerGetter = const uint8_t* (__thiscall*)(void*);
    GameRef<void*> players;
    PlayerGetter getPlayer = nullptr;
    std::mutex healthMutex;
    std::optional<Health> health;

    void UpdateSide(int value, bool left)
    {
        const bool dead = value <= 1;
        const bool danger = value <= 250;
        const bool caution = value <= 350;
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
            LEDEffects::SetLighting(90, 36, 3);
        }
    }
}

export namespace Rev2LightSync
{
    void Sample()
    {
        if (!LEDEffects::bLogiLedInitialized)
            return;

        std::optional<Health> snapshot;
        if (const auto manager = players.get())
        {
            if (const auto player = getPlayer(manager))
            {
                snapshot = Health{
                    *reinterpret_cast<const int32_t*>(player + 0x1A08),
                    *reinterpret_cast<const int32_t*>(player + 0x1A0C)
                };
            }
        }

        const std::lock_guard lock(healthMutex);
        health = snapshot;
    }

}

class LightSync
{
public:
    LightSync()
    {
        WFP::onInitEvent() += []()
        {
            if (!FixSettings().Bool("MAIN.LightSyncRGB"))
                return;

            auto manager = hook::pattern("8B 0D ? ? ? ? 56 E8 ? ? ? ? 85 C0 74 7B 8B C8");
            auto getter = hook::pattern("33 C0 8D 51 20 56 8B 32 85 F6 74 09 83 BE 20 79 00 00 00 74 0D 40 83 C2 04 83 F8 08");
            players.SetAddress(*manager.get_first<void**>(2));
            getPlayer = reinterpret_cast<PlayerGetter>(getter.get_first());

            LEDEffects::Inject(UpdateLighting);
        };
    }
} LightSync;
