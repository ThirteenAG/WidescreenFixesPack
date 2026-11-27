module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <thread>
#include <dolphin/dolphin.h>

export module Game;

// Resident Evil 2 and 3 for GameCube in Dolphin. A thread watches the emulated memory: when one of the supported
// discs runs, it finds the game's code by pattern and patches it (the patches are found again whenever the code
// they were found in changes, e.g. after a reset or a disc change).
// - MAIN.SkipIntro: Resident Evil 2 goes past the violence warning and the title movie, Resident Evil 3 past the
//   warning, the Nintendo licence notice, the Capcom logo and the title movie, to the title screen.
// - MAIN.DoorSkip: the door animations end at once.
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
    bool bSkipIntro;
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
        if (pattern.size() >= 1)
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
                if (pattern.size() >= 1)
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
                if (pattern.size() >= 1)
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
                auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "94 21 ff f0 7c 08 02 a6 ? ? ? ? ? ? ? ? 38 60 00 01 ? ? ? ? 3c 60 ? ? 38 63 ? ? 80 ? ? ? 54 ? ? ? 28 ? 00 00");
                if (!pattern.empty())
                {
                    injector::WriteMemory<uint32_t>(pattern.get(0).get<uint32_t>(0), blr(), true);
                    Dolphin::MenuBarClearCache();
                }
            }
            else if (gameVersion == RE2)
            {
                auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "28 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 54 00 03 9c 28 00 00 00 ? ? ? ? 38 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 48 00 ? ? 48 00");
                if (pattern.size() >= 1)
                {
                    injector::WriteMemory<uint16_t>(pattern.get(0).get<uint16_t>(4), 0x0048, true); // beq -> b
                    Dolphin::MenuBarClearCache();
                }
            }
        }
    }


    // Big-endian instruction words in the emulated memory
    uint32_t Word(const uint32_t* p)
    {
        return powerpc::swap32(*p);
    }

    void SetWord(uint32_t* p, uint32_t word)
    {
        injector::WriteMemory<uint32_t>(p, powerpc::swap32(word), true);
    }

    // Sets the immediate of an addi/li when the instruction is the expected one (any target register with anyRegister)
    bool SetImmediate(uint32_t* p, uint32_t expected, int16_t value, bool anyRegister = false)
    {
        const uint32_t mask = anyRegister ? 0xFC1FFFFF : 0xFFFFFFFF;
        if ((Word(p) & mask) != (expected & mask))
            return false;
        SetWord(p, (Word(p) & 0xFFFF0000) | uint16_t(value));
        return true;
    }

    // A patched instruction: when the game's code is loaded again (reset, disc change) the patches are made again
    uint32_t* pIntroCheck = nullptr;
    uint32_t nIntroValue = 0;

    // Resident Evil 2's start-up screens (+0x1C state, +0x2C frame counter): the warning (state 1) and the licence
    // notice (state 5) may already be counting their 120 frames when the code is patched; they end at the next frame
    uint32_t* pStartupScreens = nullptr;

    void EndStartupScreen()
    {
        if (!pStartupScreens || !Dolphin::MemoryAddrValid(pStartupScreens))
            return;
        const auto state = Word(pStartupScreens + 0x1C / 4);
        auto counter = pStartupScreens + 0x2C / 4;
        if ((state == 1 || state == 5) && int32_t(Word(counter)) > 1)
            SetWord(counter, 1);
    }

    // True once the start-up patches are in place for the running disc
    bool SkipIntro(GameVersion gameVersion)
    {
        using namespace powerpc;

        if (gameVersion == RE2)
        {
            // The violence warning (and the licence notice) is shown for 120 frames each: one frame
            auto warning = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 00 00 78 3c 60 ? ? 38 63 ? ? 90 03 00 2c");
            // The title movie stops when START is pressed: it stops at once
            auto movie = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "54 00 04 e6 28 00 00 00 41 82 00 08 3b e0 00 00 2c 1f 00 00 40 82");
            // The warning's code runs before the title's is loaded: it is patched as soon as it is there
            if (!warning.empty())
            {
                warning.for_each_result([](hook::pattern_match match)
                {
                    auto w = match.get<uint32_t>(0);
                    SetImmediate(w, 0x38000078, 1);
                    // The screen fades in and out by 0x400 of 0x8000 per frame (li r4 of the fade call): two frames each
                    bool fadeIn = false, fadeOut = false;
                    for (int i = 1; i < 48; ++i)
                    {
                        if (!fadeIn)
                            fadeIn = SetImmediate(w + i, 0x3880FC00, -0x4000);
                        else if (!fadeOut)
                            fadeOut = SetImmediate(w + i, 0x38800400, 0x4000);
                    }
                });
                // lis r3, hi / addi r3, r3, lo: the start-up screens' state
                const uint16_t hi = swap16(*warning.get_first<uint16_t>(6));
                const uint16_t lo = swap16(*warning.get_first<uint16_t>(10));
                pStartupScreens = (uint32_t*)(((uint32_t(hi) << 16) + int32_t(int16_t(lo))) - Dolphin::ImageBase + Dolphin::GameMemoryStart);
                Dolphin::MenuBarClearCache();
            }
            EndStartupScreen();
            if (!pStartupScreens || movie.empty())
                return false;
            pIntroCheck = movie.get_first<uint32_t>(8);
            SetWord(pIntroCheck, 0x60000000);
            nIntroValue = Word(pIntroCheck);
            Dolphin::MenuBarClearCache();
            return true;
        }

        // RE3. The title's first state waits for the warning screen, then fades the Capcom logo in, shows it for 120
        // frames and fades it out at the first start (one frame each), then plays the title movie (skipped)
        auto title = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 00 00 02 3c 60 ? ? 38 63 ? ? 98 03 ? ? 38 60 00 01 ? ? ? ? 3b e0 00 00");
        // The warning screen waits for the title in steps of 60 frames, then fades out, shows the licence notice for
        // 120 frames and fades to black: one frame each
        auto wait = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 60 00 3c ? ? ? ? 3c 60 ? ? 38 63 ? ? 88 03 ? ? 28 00 00 02 40 82 ff e8");
        if (title.empty() || wait.empty())
            return false;
        pIntroCheck = nullptr;
        auto t = title.get_first<uint32_t>(0);
        bool logoTime = false;
        for (int i = 7; i < 200; ++i)
        {
            if (SetImmediate(t + i, 0x3920001e, 1) && !pIntroCheck)
                pIntroCheck = t + i;
            if (!logoTime)
                logoTime = SetImmediate(t + i, 0x3bc00078, 1, true);
            if (Word(t + i) == 0x54000630 && Word(t + i + 1) == 0x28000000 && Word(t + i + 2) == 0x40820030)
                SetWord(t + i + 2, 0x48000030);
        }
        auto w = wait.get_first<uint32_t>(0);
        SetImmediate(w, 0x3860003c, 1);
        if (!pIntroCheck)
            pIntroCheck = w;
        nIntroValue = Word(pIntroCheck);
        SetImmediate(w - 14, 0x3920001e, 1);            // fade in
        SetImmediate(w + 14, 0x39200032, 1);            // fade out
        SetImmediate(w + 36, 0x39200032, 1);            // fade in of the licence notice
        SetImmediate(w + 45, 0x3bc00078, 0, true);      // licence notice frames (r30 or r31)
        for (int i = 46; i < 46 + 64; ++i)
        {
            if (SetImmediate(w + i, 0x3920001e, 1))     // fade to black (li r9 only)
                break;
        }
        Dolphin::MenuBarClearCache();
        return true;
    }

    struct Watch
    {
        uint32_t* data = nullptr;
        uint32_t* memcheck = nullptr;
        uint32_t bytes = 0;
        std::array<char, 6> previousDisc{};
        bool introPatched = false;
        ULONGLONG bootUntil = 0;   // Resident Evil 2: the first seconds of the disc, until its warning screen is patched
        int searchDelay = 0;
    };

    void Update(Watch& watch)
    {
        if (!Dolphin::MemoryValid())
        {
            watch.data = nullptr;
            watch.introPatched = false;
            watch.previousDisc.fill(0);
            pStartupScreens = nullptr;
            Game::sPlayerHealth = nullptr;
            return;
        }

        auto gameVersion = Game::Detect();
        auto disc = Dolphin::GameID();
        if (!gameVersion || disc.size() != watch.previousDisc.size() ||
            std::memcmp(disc.data(), watch.previousDisc.data(), watch.previousDisc.size()) != 0)
        {
            watch.data = nullptr;
            watch.memcheck = nullptr;
            watch.introPatched = false;
            watch.searchDelay = 0;
            pStartupScreens = nullptr;
            Game::sPlayerHealth = nullptr;
            if (!gameVersion)
            {
                watch.previousDisc.fill(0);
                return;
            }
            std::memcpy(watch.previousDisc.data(), disc.data(), watch.previousDisc.size());
            watch.bootUntil = gameVersion == RE2 ? GetTickCount64() + 20000 : 0;
        }

        if (bSkipIntro)
        {
            if (watch.introPatched && (!Dolphin::MemoryAddrValid(pIntroCheck) || Word(pIntroCheck) != nIntroValue))
                watch.introPatched = false;
            if (!watch.introPatched)
                watch.introPatched = SkipIntro(gameVersion);
            else if (gameVersion == RE2)
                EndStartupScreen();
        }

        // Door skip, unthrottled doors and the health lighting need the game's room code, which Resident Evil 2 loads
        // only when a game starts: until it is there, look for it once a second
        if (!bEnableDoorSkip && !bUnthrottleEmuDuringDoorSkip && !bLightSyncRGB)
            return;
        if (watch.data == nullptr || !Dolphin::MemoryAddrValid(watch.data) || !Dolphin::MemoryAddrValid(watch.memcheck) || *watch.memcheck != watch.bytes)
        {
            watch.data = nullptr;
            Game::sPlayerHealth = nullptr;
            if (watch.searchDelay > 0)
            {
                --watch.searchDelay;
                return;
            }
            watch.searchDelay = 9;
            FindGame(gameVersion, watch.data, watch.memcheck, watch.bytes);
        }
        else if (bUnthrottleEmuDuringDoorSkip)
        {
            // The flag is a word in RE3 and the first byte of a word in RE2
            const uint32_t doorPhase = gameVersion == RE3 ? 1 : 0x01000000;
            while (Dolphin::MemoryAddrValid(watch.data) && watch.data[0] == doorPhase)
            {
                Dolphin::SetIsThrottlerTempDisabled(true);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    // The emulated memory can go away while it is read (the emulation stops): that pass ends and the next one starts
    // over
    void GuardedUpdate(Watch& watch)
    {
        __try
        {
            Update(watch);
        }
        __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            watch.data = nullptr;
            watch.introPatched = false;
            Game::sPlayerHealth = nullptr;
        }
    }

    void PluginThread()
    {
        Watch watch;
        while (true)
        {
            // Resident Evil 2 shows its warning screen as soon as its code is loaded: until then it is looked for often
            const bool booting = bSkipIntro && !pStartupScreens && GetTickCount64() < watch.bootUntil;
            std::this_thread::sleep_for(std::chrono::milliseconds(booting ? 2 : 100));
            GuardedUpdate(watch);
        }
    }
}

export namespace Game
{
    void Init()
    {
        bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
        bEnableDoorSkip = FixSettings().Bool("MAIN.DoorSkip");
        bUnthrottleEmuDuringDoorSkip = FixSettings().Bool("MAIN.UnthrottleEmuDuringDoorSkip");
        bLightSyncRGB = FixSettings().Bool("MAIN.LightSyncRGB");
        if (!bSkipIntro && !bEnableDoorSkip && !bUnthrottleEmuDuringDoorSkip && !bLightSyncRGB)
            return;

        std::thread(PluginThread).detach();
    }
}
