module;

#include <stdafx.h>
#include <mutex>
#include <optional>
#include "FixSettings.hpp"
#include "LEDEffects.h"

export module LightSync;

namespace
{
    using PlayerGetter = const uint8_t* (__thiscall*)(void*, int);

    GameRef<void*> players;
    GameRef<void*> gameState;
    PlayerGetter getPlayer = nullptr;
    std::mutex healthMutex;
    std::optional<float> health;

    void SampleHealth()
    {
        if (!LEDEffects::bLogiLedInitialized)
            return;

        std::optional<float> snapshot;
        const auto manager = players.get();
        if (manager && gameState.get())
        {
            if (const auto player = getPlayer(manager, 0))
                snapshot = *reinterpret_cast<const float*>(player + 0xE38);
        }

        const std::lock_guard lock(healthMutex);
        health = snapshot;
    }

    void UpdateLighting()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::optional<float> snapshot;
        {
            const std::lock_guard lock(healthMutex);
            snapshot = health;
        }

        if (!snapshot)
        {
            LogiLedStopEffects();
            LEDEffects::SetLighting(90, 36, 3);
            return;
        }

        if (*snapshot <= 0)
        {
            LEDEffects::SetLighting(26, 4, 4, false, true);
            LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
        }
        else if (*snapshot <= 400)
        {
            LEDEffects::SetLighting(26, 4, 4, true);
            LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0);
        }
        else if (*snapshot <= 600)
        {
            LEDEffects::SetLighting(50, 30, 4, true);
            LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0);
        }
        else
        {
            LEDEffects::SetLighting(10, 30, 4, true);
            LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0);
        }
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

            auto player = hook::pattern("8B 0D ? ? ? ? 85 C9 75 14 68 ? ? ? ? 6A 3E 68 ? ? ? ? E8 ? ? ? ? 83 C4 0C 8B 0D ? ? ? ? 8D 54 24 0A 52 E8 ? ? ? ? 8B F8 A1");
            const auto getter = injector::GetBranchDestination(player.get_first(41)).get_raw<uint8_t>();
            players.SetAddress(*player.get_first<void**>(2));
            gameState.SetAddress(injector::ReadMemory<void*>(getter + 17));
            getPlayer = reinterpret_cast<PlayerGetter>(getter);

            auto update = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 06 8B 50 18 8B CE FF D2 A1 ? ? ? ? 80 B8 ? 26 00 00 00");
            static auto sample = safetyhook::create_mid(update.get_first(20), [](SafetyHookContext&)
            {
                SampleHealth();
            });

            LEDEffects::Inject(UpdateLighting);
        };
    }
} LightSync;
