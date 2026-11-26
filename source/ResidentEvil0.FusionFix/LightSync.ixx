module;

#include <stdafx.h>
#include <array>
#include <mutex>
#include "FixSettings.hpp"
#include "LEDEffects.h"

export module LightSync;

import EngineText;

namespace
{
    struct PlayerStatus
    {
        bool present{};
        int health{};
        bool poisoned{};
    };

    GameRef<void*> players;
    std::mutex stateMutex;
    std::array<PlayerStatus, 2> playerStatus;

    void SamplePlayers()
    {
        if (!LEDEffects::bLogiLedInitialized)
            return;

        std::array<PlayerStatus, 2> snapshot{};
        if (const auto manager = players.get())
        {
            // The stage owns the player and partner slots. Identify each character after a control swap.
            for (const auto slot : { 0x2C, 0x3C })
            {
                const auto player = RE0Text::Read<void*>(manager, slot);
                if (!player)
                    continue;
                const auto id = RE0Text::Read<uint32_t>(player, 0x100);
                if (id == 3 || id == 5)
                {
                    snapshot[id == 5] = {
                        true,
                        RE0Text::Read<int32_t>(player, 0x1030),
                        RE0Text::Read<int32_t>(player, 0x68C4) == 1
                    };
                }
            }
        }

        const std::lock_guard lock(stateMutex);
        playerStatus = snapshot;
    }

    void UpdateSide(const PlayerStatus& player, bool left)
    {
        const bool dead = player.present && player.health <= 1;
        const bool danger = player.health < 40;
        const bool caution = player.health < 130;
        const auto [red, green, blue] = !player.present ? std::array{ 64, 20, 8 } :
            dead ? std::array{ 26, 4, 4 } :
            player.poisoned ? std::array{ 51, 4, 53 } :
            danger ? std::array{ 26, 4, 4 } :
            caution ? std::array{ 50, 30, 4 } : std::array{ 10, 30, 4 };

        if (left)
            LEDEffects::SetLightingLeftSide(red, green, blue, player.present && !dead, dead);
        else
            LEDEffects::SetLightingRightSide(red, green, blue, player.present && !dead, dead);

        if (player.present)
        {
            const auto draw = left ? LEDEffects::DrawCardiogram : LEDEffects::DrawCardiogramNumpad;
            draw(danger ? 100 : caution ? 67 : 0, caution ? 0 : 100, 0, 0, 0, 0, dead);
        }
    }

    void UpdateLighting()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::array<PlayerStatus, 2> snapshot;
        {
            const std::lock_guard lock(stateMutex);
            snapshot = playerStatus;
        }

        if (snapshot[0].present || snapshot[1].present)
        {
            UpdateSide(snapshot[0], true);
            UpdateSide(snapshot[1], false);
        }
        else
        {
            LogiLedStopEffects();
            LEDEffects::SetLighting(64, 20, 8);
        }
    }
}

export namespace RE0LightSync
{
    void Sample()
    {
        SamplePlayers();
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

            auto manager = hook::pattern("8B 0D ? ? ? ? 83 EC 08 85 C9 75 1B 68 ? ? ? ? 68 8B 01 00 00 68 ? ? ? ? E8 ? ? ? ? 8B 4C 24 0C 83 C4 0C E8 ? ? ? ? 8B C8 E8 ? ? ? ? 85 C0 7F 06 32 C0 83 C4 08 C3");
            players.SetAddress(*manager.get_first<void**>(2));

            LEDEffects::Inject(UpdateLighting);
        };
    }
} LightSync;
