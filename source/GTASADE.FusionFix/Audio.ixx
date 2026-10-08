module;

#include "stdafx.h"

export module Audio;

namespace
{
    SafetyHookInline rainHook, playerEngineHook;
    const uint32_t* gameTime = nullptr;
    struct ParkedSound
    {
        uint8_t* sound = nullptr;
        uint32_t since = 0;
    };
    struct VehicleSounds
    {
        void* vehicle = nullptr;
        uint32_t lastSeen = 0, lastRain = 0;
        bool rainStarted = false;
        std::array<ParkedSound, 7> parked{};
    };
    std::map<uint8_t*, VehicleSounds> vehicles;
    uint32_t lastCleanup = 0;

    VehicleSounds& GetSounds(uint8_t* entity)
    {
        const auto now = *gameTime;
        if (uint32_t(now - lastCleanup) >= 2000)
        {
            std::erase_if(vehicles, [now](const auto& item) { return uint32_t(now - item.second.lastSeen) > 2000; });
            lastCleanup = now;
        }
        auto& sounds = vehicles[entity];
        auto vehicle = *reinterpret_cast<void**>(entity + 8);
        if (sounds.vehicle != vehicle)
        {
            sounds = {};
            sounds.vehicle = vehicle;
        }
        sounds.lastSeen = now;
        return sounds;
    }
    uintptr_t RainOnVehicle(uint8_t* entity)
    {
        auto& sounds = GetSounds(entity);
        const auto now = *gameTime;
        // Original 30 fps / every third frame = ten drops per second. Count
        // simulation milliseconds so high fps cannot consume the audio pool.
        // Do not catch up after stalls: that would enqueue another burst.
        if (sounds.rainStarted && uint32_t(now - sounds.lastRain) < 100)
            return 0;
        sounds.lastRain = now;
        sounds.rainStarted = true;
        *reinterpret_cast<int16_t*>(entity + 254) = 2;
        return rainHook.ccall<uintptr_t>(entity);
    }
    uintptr_t PlayerEngine(uint8_t* entity, void* parameters)
    {
        const auto result = playerEngineHook.ccall<uintptr_t>(entity, parameters);
        auto& sounds = GetSounds(entity);
        const auto now = *gameTime;
        for (size_t i = 0; i < sounds.parked.size(); ++i)
        {
            auto& slot = *reinterpret_cast<uint8_t**>(entity + 976 + i * sizeof(void*));
            auto& parked = sounds.parked[i];
            if (parked.sound != slot)
                parked = {slot, now};
            if (slot && uint32_t(now - parked.since) >= 500)
            {
                // DE revives these cancelled engine sounds until a successor
                // reaches the hardware. If allocation fails, that can last
                // forever. End an abandoned handover with the same stop flags
                // used by CAEVehicleAudioEntity::StoppedUsingAsPlayerVehicle.
                *reinterpret_cast<uint16_t*>(slot + 102) &= ~uint16_t(4);
                *reinterpret_cast<uint16_t*>(slot + 124) = 1;
                slot = nullptr;
                parked = {};
            }
        }
        return result;
    }
}

class AudioModule
{
public:
    AudioModule()
    {
        WFP::onInitEvent() += []()
        {
            auto rain = hook::pattern("40 57 48 81 EC 80 00 00 00 48 8B 05 ? ? ? ? 48 8B F9 48 8B 48 30 48 85 C9 0F 84 ? ? ? ? 83 79 68 69").get_first();
            auto engine = hook::pattern("40 53 56 57 48 83 EC 60 4C 0F BF 81 2C 01 00 00 48 8B D9 48 8B 05 ? ? ? ? 48 8B FA 48 8B 72 10 4A 8B 0C C0").get_first();
            auto timer = hook::pattern("8B 05 ? ? ? ? 03 C5 89 7B 28 F3 48 0F 2A C0 B8 04 00 00 00").get_first<uint8_t>(2);
            if (!rain || !engine || !timer)
                return;
            gameTime = reinterpret_cast<const uint32_t*>(injector::ReadRelativeOffset(timer).as_int());
            rainHook = safetyhook::create_inline(rain, RainOnVehicle);
            playerEngineHook = safetyhook::create_inline(engine, PlayerEngine);
            if (!rainHook || !playerEngineHook)
            {
                rainHook = {};
                playerEngineHook = {};
            }
        };
        WFP::onGameInitEvent() += []() { vehicles.clear(); lastCleanup = 0; };
    }
} AudioModuleInstance;
