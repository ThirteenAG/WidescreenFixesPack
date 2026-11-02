module;

#include <stdafx.h>
#include <random>

export module Misc;

import ComVars;

export void InstallControls()
{
    hook::pattern pattern("");
    // Without the launcher's saved controls the keyboard has no bindings at all; use the launcher's defaults then
    pattern = hook::pattern("83 C8 FF B9 98 00 00 00 BF ? ? ? ? F3 AB");
    auto readControls = hook::pattern("E8 ? ? ? ? 8B 44 24 0C 83 C4 04 6A 00");
    keyboardMap.SetAddress(*pattern.get_first<int32_t*>(9));
    static auto defaultKeysHook = safetyhook::create_mid(readControls.get_first(5), [](SafetyHookContext&)
    {
        static constexpr int32_t defaults[24] = { 17, 31, 30, 32, 44, 45, 46, 47, 80, 77, 75, 72, -1, 15, 79, 81, 71, 73, 82, -1, 199, 207, 211, 209 };
        if (std::all_of(keyboardMap.get().begin(), keyboardMap.get().end(), [](int32_t key) { return key == -1; }))
            std::copy(std::begin(defaults), std::end(defaults), keyboardMap.get().begin());
    });
}

export void InstallMusic()
{
    hook::pattern pattern("");
    {
        pattern = hook::pattern("83 C4 08 ? D0 07 00 00");
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
