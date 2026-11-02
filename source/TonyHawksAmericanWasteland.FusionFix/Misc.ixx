module;

#include <stdafx.h>
#include <random>

export module Misc;

import ComVars;

export void InstallMusic()
{
    hook::pattern pattern("");
    {
        pattern = hook::pattern("BD D0 07 00 00");
        auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(0), (uintptr_t)pattern.get_first(110), "75 ? A1");
        dword_69B718.SetAddress(*rpattern.get_first<int32_t*>(3));
        dword_681D14.SetAddress(*hook::range_pattern((uintptr_t)pattern.get_first(0), (uintptr_t)pattern.get_first(60), "8B 15 ? ? ? ?").get_first<int32_t*>(2));
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
        }; injector::MakeInline<RandomHook>(pattern.get_first(0), rpattern.get_first(2));
    }
}
