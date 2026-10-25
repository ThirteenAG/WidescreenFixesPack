module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <thread>
#include <dolphin/dolphin.h>

export module Game;

// Resident Evil 2 and 3 for GameCube in Dolphin. A thread watches the emulated memory: when one of the supported
// discs runs, it finds the game's code by pattern and patches it (the patches are found again whenever the code
// they were found in changes, e.g. after a reset or a disc change).
// - MAIN.EnableDoorSkip: the door animations end at once.
// - MAIN.UnthrottleEmuDuringDoorSkip: while the game loads the next room (the door phase), Dolphin runs unthrottled.
// - The player's health for the LIGHTSYNC module.

export enum GameVersion
{
    UNDEFINED,
    RE2,
    RE3,
};

export namespace Game
{
    // Big-endian; the poison state follows at +0xC2 (RE2) or in +0x6 (RE3)
    int16_t* sPlayerHealth = nullptr;

    // bLightSync: the Japanese RE2 has no health lighting
    GameVersion Detect(bool bLightSync = false)
    {
        auto id = Dolphin::GameID();
        if (id == "GHAE08" || id == "GHAP08" || (!bLightSync && id == "GHAJ08"))
            return RE2;
        if (id == "GLEE08" || id == "GLEJ08" || id == "GLEP08")
            return RE3;
        return UNDEFINED;
    }
}

namespace
{
    bool bEnableDoorSkip;
    bool bUnthrottleEmuDuringDoorSkip;
    bool bLightSyncRGB;

    uint32_t convert(uint32_t n)
    {
        auto ptr = (unsigned char*)&n;
        return (ptr[1] << 24) | (ptr[0] << 16) | 0 | 0;
    };

    void FindGame(GameVersion gameVersion, uint32_t*& data, uint32_t*& memcheck, uint32_t& bytes)
    {
        using namespace powerpc;

        // The door phase flag
        auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 ? 00 01 3c 60 ? ? 38 ? ? ? 90 ? ? ? 3c ? ? ? 38 63 ? ? 80 63 00 00 88 ? 00 09 28 ? 00 0b ? ? ? ? 3c 60 ? ? 38 ? ? ? 54 ? ? ? 7c ? ? ? 7c");
        if (gameVersion == RE3)
            pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 00 00 01 3c 60 ? ? 38 63 ? ? 98 03 00 00 3c 60 ? ? 38 63 ? ? 93 83 21 58 3c 60");
        if (PatternAvailable(pattern, "ResidentEvil2.RE3.Dolphin.FusionFix: Game"))
        {
            memcheck = pattern.get(0).get<uint32_t>(4);
            bytes = injector::ReadMemory<uint32_t>(memcheck, true);
            uint16_t a = swap16(*pattern.get(0).get<uint16_t>(6));
            uint16_t b = swap16(*pattern.get(0).get<uint16_t>(10));
            uint16_t c = swap16(*pattern.get(0).get<uint16_t>(14));
            if (a != 0 && b != 0)
                data = (uint32_t*)((convert(a) - int16_t(0 - b) + c) - Dolphin::ImageBase + Dolphin::GameMemoryStart);
        }

        if (bLightSyncRGB)
        {
            if (gameVersion == RE3)
            {
                auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "7c 7e 1b 78 7c 9d 23 78 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 2c 03 00 00 ? ? ? ? 2c 1e 00 0a ? ? ? ? 2c 1e 00 64 ? ? ? ? 3b de 00 0a");
                if (PatternAvailable(pattern, "ResidentEvil2.RE3.Dolphin.FusionFix: Game"))
                {
                    uint16_t a = swap16(*pattern.get(0).get<uint16_t>(10));
                    uint16_t b = swap16(*pattern.get(0).get<uint16_t>(14));
                    uint16_t c = swap16(*pattern.get(0).get<uint16_t>(18));
                    uint16_t d = swap16(*pattern.get(0).get<uint16_t>(54));
                    if (a != 0 && b != 0)
                        Game::sPlayerHealth = (int16_t*)((convert(a) + b + c + d) - Dolphin::ImageBase + Dolphin::GameMemoryStart);
                }
            }
            else if (gameVersion == RE2)
            {
                auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "3c 60 ? ? 38 63 ? ? ? ? ? ? 2c 00 00 00 ? ? ? ? 38 00 ? ? 3c 60 ? ? 38 63 ? ? ? ? ? ? 3c 60 ? ? 38 63 ? ? ? ? ? ? 54 00 02 94");
                if (PatternAvailable(pattern, "ResidentEvil2.RE3.Dolphin.FusionFix: Game"))
                {
                    uint16_t a = swap16(*pattern.get(0).get<uint16_t>(2));
                    uint16_t b = swap16(*pattern.get(0).get<uint16_t>(6));
                    uint16_t c = swap16(*pattern.get(0).get<uint16_t>(10));
                    if (a != 0 && b != 0)
                        Game::sPlayerHealth = (int16_t*)((convert(a) + b + c) - Dolphin::ImageBase + Dolphin::GameMemoryStart);
                }
            }
        }

        if (bEnableDoorSkip)
        {
            if (gameVersion == RE3)
            {
                auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "94 21 ff f0 7c 08 02 a6 ? ? ? ? ? ? ? ? 38 60 00 01");
                if (pattern.size() > 1)
                {
                    ProtectedGameRef<uint32_t>{pattern.get(0).get<uint32_t>(0)} = blr(); //return
                    Dolphin::MenuBarClearCache();
                }
            }
            else if (gameVersion == RE2)
            {
                auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "28 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 54 00 03 9c 28 00 00 00 ? ? ? ? 38 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 48 00 ? ? 48 00");
                if (PatternAvailable(pattern, "ResidentEvil2.RE3.Dolphin.FusionFix: Game"))
                {
                    ProtectedGameRef<uint16_t>{pattern.get(0).get<uint16_t>(4)} = 0x0048; // bne -> b
                    Dolphin::MenuBarClearCache();
                }
            }
        }
    }

    void PluginThread()
    {
        [&]()
        {
            __try
            {
                static uint32_t* data = nullptr;
                static uint32_t* memcheck = nullptr;
                static uint32_t bytes = 0;
                static std::array<char, 6> previousDisc{};
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if (!Dolphin::MemoryValid())
                    {
                        data = nullptr;
                        Game::sPlayerHealth = nullptr;
                        continue;
                    }

                    auto gameVersion = Game::Detect();
                    auto disc = Dolphin::GameID();
                    if (!gameVersion || disc.size() != previousDisc.size() ||
                        std::memcmp(disc.data(), previousDisc.data(), previousDisc.size()) != 0)
                    {
                        data = nullptr;
                        memcheck = nullptr;
                        Game::sPlayerHealth = nullptr;
                        if (!gameVersion)
                        {
                            previousDisc.fill(0);
                            continue;
                        }
                        std::memcpy(previousDisc.data(), disc.data(), previousDisc.size());
                    }

                    if (data == nullptr || !Dolphin::MemoryAddrValid(data) || !Dolphin::MemoryAddrValid(memcheck) || *memcheck != bytes)
                    {
                        data = nullptr;
                        Game::sPlayerHealth = nullptr;
                        FindGame(gameVersion, data, memcheck, bytes);
                    }
                    else if (bUnthrottleEmuDuringDoorSkip)
                    {
                        // The flag is a word in RE3 and the first byte of a word in RE2
                        const uint32_t doorPhase = gameVersion == RE3 ? 1 : 0x01000000;
                        while (Dolphin::MemoryAddrValid(data) && data[0] == doorPhase)
                        {
                            Dolphin::SetIsThrottlerTempDisabled(true);
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                    }
                }
            }
            __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
            }
        }();
    }
}

export namespace Game
{
    void Init()
    {
        bEnableDoorSkip = FixSettings().Bool("MAIN.EnableDoorSkip");
        bUnthrottleEmuDuringDoorSkip = FixSettings().Bool("MAIN.UnthrottleEmuDuringDoorSkip");
        bLightSyncRGB = FixSettings().Bool("MAIN.LightSyncRGB");
        if (!bEnableDoorSkip && !bUnthrottleEmuDuringDoorSkip && !bLightSyncRGB)
            return;

        std::thread(PluginThread).detach();
    }
}
