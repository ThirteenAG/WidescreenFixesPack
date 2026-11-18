module;

#include <stdafx.h>
#include <random>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/PadPrompts.hpp"

export module Misc;

import ComVars;

export void InstallControls()
{
    auto& settings = FixSettings();
    TonyPad::Install(
        {
            "THUG",
            "83 EC 54 8B 44 24 58 0F B6 40 24 55 56 33 ED 83 F8 15",
            "BB ? ? ? ? C7 44 24 10 02 00 00 00", 1,
            304,
            "53 8B 5C 24 08 8B 04 9D ? ? ? ? 85 C0 74 ? 8B 08 8B 51 64 55 50 FF D2",
            "8D A8 ? ? ? ? 8D B8 ? ? ? ? 55 B9 44 00 00 00", 8,
            "A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 08 8B 51 64 50 FF D2 85 C0 7D",
            "8B 15 ? ? ? ? A3 ? ? ? ? A1", 7,
            // Sticks, A B X Y, Start Back, LT RT LB RB, LS RS, the D-pad as buttons (the bundled Xidi profile)
            { 2, 2, 1, 1, 8, 8, 3, 3, 11, 12, 13, 14, 20, 19, 17, 18, 15, 16, 21, 22, 23, 24, 25, 26 },
            "68 60 02 00 00 6A FF 68 ? ? ? ? A3", 8,
            "E8 ? ? ? ? 8B 4C 24 2C 83 C4 28 6A 00", 5,
            { 17, 31, 30, 32, 44, 45, 46, 47, 80, 77, 75, 72, -1, 15, 79, 81, 71, 73, 82, 83, 199, 207, 211, 209 },
            "8B 4C 24 04 83 EC 4C 53 8B 1C 8D ? ? ? ? 57 33 FF 3B DF 0F 84",
            "E8 ? ? ? ? D9 5C 24 0C 80 7E 18 00 0F 85",
            "D9 87 14 02 00 00 D9 5C 24 08 D9 EE",
            "8B 44 24 04 50 E8 ? ? ? ? 8A 80 ? ? ? ? 83 C4 04 C3",
            "83 C5 01 55 E8 ? ? ? ? 83 C4 04 EB ? 83 C5 01 55 E8", 4,
            "8D 54 24 17 52 88 44 24 1B E8 ? ? ? ? 83 C4 08 8A C8 80 E9 04", 9,
            "83 C3 01 53 89 5C 24 28 E8 ? ? ? ? 83 C4 04 89 44 24 20 EB", 8,
            "88 44 24 17 8D 44 24 17 50 E8 ? ? ? ? 83 C4 08 89 44", 9,
            "8B 0D ? ? ? ? 0F B6 84 08 0C 01 00 00 8D 14 40 8B 41 08 89 4C 24 1C",
            "8B 86 78 01 00 00 8B 8E 74 01 00 00 50 51 6A 00 E8", 16,
            0x174,
            20, 64, 0x15C,
            0x18
        }, &settings.Int("MAIN.Vibration"), &settings.Float("MAIN.GamepadCameraSpeed"), &settings.Int("MAIN.InvertGamepadY"),
        &settings.Int("MAIN.GamepadIcons"));
}

export void InstallMusic()
{
    hook::pattern pattern("");
    {
        pattern = hook::pattern("83 C4 08 ? D0 07 00 00"); //0x424BDD or 423CDB
        auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(110), "75 ? A1");
        dword_69B718.SetAddress(*rpattern.get_first<int32_t*>(3));
        dword_681D14.SetAddress(*hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(60), "A1 ? ? ? ?").get_first<int32_t*>(1));
        struct RandomHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto num_songs = dword_681D14.get();
                int32_t* sp_xbox_randomized_songs = dword_69B718.get_ptr();

                std::vector<int32_t> songs;
                songs.assign(std::addressof(sp_xbox_randomized_songs[0]), std::addressof(sp_xbox_randomized_songs[num_songs]));
                std::mt19937 r{ std::random_device{}() };
                std::shuffle(std::begin(songs), std::end(songs), r);

                std::copy(songs.begin(), songs.end(), sp_xbox_randomized_songs);
            }
        }; injector::MakeInline<RandomHook>(pattern.get_first(3), rpattern.get_first(2));
    }
}
