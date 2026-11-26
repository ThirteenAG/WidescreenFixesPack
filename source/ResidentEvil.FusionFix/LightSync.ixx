module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "LEDEffects.h"

export module LightSync;

import EngineText;

namespace
{
    struct Ammo
    {
        int rounds{};
        int capacity{};
        bool operator==(const Ammo&) const = default;
    };

    struct LightingState
    {
        bool inGame{};
        int health{};
        bool poisoned{};
        Ammo ammo;
    };

    using ActorGetter = void* (__thiscall*)(void*);
    ActorGetter GetPlayer = nullptr;
    ActorGetter GetWeapon = nullptr;
    GameRef<void*> ActorManager;
    std::mutex StateMutex;
    LightingState State;

    void SetAmmoLights(Ammo ammo)
    {
        static constexpr std::array keys{
            LogiLed::KeyName::F1, LogiLed::KeyName::F2, LogiLed::KeyName::F3,
            LogiLed::KeyName::F4, LogiLed::KeyName::F5, LogiLed::KeyName::F6,
            LogiLed::KeyName::F7, LogiLed::KeyName::F8, LogiLed::KeyName::F9,
            LogiLed::KeyName::F10, LogiLed::KeyName::F11, LogiLed::KeyName::F12,
            LogiLed::KeyName::PRINT_SCREEN, LogiLed::KeyName::SCROLL_LOCK,
            LogiLed::KeyName::PAUSE_BREAK,
        };
        if (ammo.capacity == 255)
            ammo.capacity = ammo.rounds;
        const int count = static_cast<int>(keys.size());
        const int rounds = std::clamp(ammo.rounds, 0, std::max(ammo.capacity, 0));
        const int lit = ammo.capacity > 0 ? (rounds * count + ammo.capacity - 1) / ammo.capacity : 0;
        for (int i = 0; i < count; ++i)
        {
            const int brightness = i < lit ? 100 : 10;
            LogiLedSetLightingForKeyWithKeyName(keys[i], brightness, brightness, brightness);
        }
    }

    void UpdateLighting()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        LightingState state;
        {
            const std::lock_guard lock(StateMutex);
            state = State;
        }
        static std::optional<Ammo> lastAmmo;
        if (!state.inGame)
        {
            lastAmmo.reset();
            LogiLedStopEffects();
            LEDEffects::SetLighting(93, 12, 14);
            return;
        }
        if (state.health <= 1)
        {
            lastAmmo.reset();
            LEDEffects::SetLighting(26, 4, 4, false, true);
            LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
            return;
        }

        if (state.poisoned)
            LEDEffects::SetLighting(51, 4, 53, true, false, true);
        else if (state.health < 240)
            LEDEffects::SetLighting(26, 4, 4, true, false, true);
        else if (state.health < 720)
            LEDEffects::SetLighting(50, 30, 4, true, false, true);
        else
            LEDEffects::SetLighting(10, 30, 4, true, false, true);

        const int red = state.health < 240 ? 100 : state.health < 720 ? 67 : 0;
        LEDEffects::DrawCardiogram(red, red ? 0 : 100, 0, 0, 0, 0);
        if (lastAmmo != state.ammo)
        {
            SetAmmoLights(state.ammo);
            lastAmmo = state.ammo;
        }
    }
}

export namespace RELightSync
{
    void Sample()
    {
        if (!LEDEffects::bLogiLedInitialized.load())
            return;

        LightingState state;
        const auto manager = ActorManager.get();
        const auto player = manager ? GetPlayer(manager) : nullptr;
        if (player)
        {
            state.inGame = true;
            state.health = REText::Read<int>(player, 0x13BC);
            state.poisoned = REText::Read<int>(player, 0x1584) != 0;
            if (const auto weapon = GetWeapon(player))
            {
                state.ammo.rounds = REText::Read<int>(weapon, 0xFFC);
                state.ammo.capacity = REText::Read<int>(weapon, 0x1000);
            }
        }
        const std::lock_guard lock(StateMutex);
        State = state;
    }

    void Shutdown()
    {
        LEDEffects::Shutdown();
    }
}

class LightSync
{
public:
    LightSync()
    {
        WFP::onInitEvent() += []()
        {
            if (!StartupOption("MAIN.LightSyncRGB"))
                return;

            auto player = find_pattern("8B D1 56 8B 42 24", "FF 71 24 FF 71 20 E8 ? ? ? ? C3");
            auto weapon = hook::pattern("FF B1 D8 15 00 00 FF B1 D4 15 00 00 8B 0D ? ? ? ? E8 ? ? ? ? C3");
            auto manager = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B D0 89 54 24 18");
            GetPlayer = reinterpret_cast<ActorGetter>(player.get_first());
            GetWeapon = reinterpret_cast<ActorGetter>(weapon.get_first());
            ActorManager.SetAddress(*manager.get_first<void**>(2));
            LEDEffects::Inject(UpdateLighting, false);
        };
    }
} LightSync;
