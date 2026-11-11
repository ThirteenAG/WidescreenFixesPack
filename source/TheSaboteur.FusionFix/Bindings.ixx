module;

#include "stdafx.h"

export module Bindings;

import ComVars;

export namespace SaboteurBindings
{
    inline GameRef<uintptr_t>* HUD()
    {
        static const auto address = []() -> GameRef<uintptr_t>*
        {
            auto pattern = hook::pattern("8B 0D ? ? ? ? 6A 1F E8 ? ? ? ? 83 78 60 05 74 07 B8 0B 00 00 00");
            auto ptr = *pattern.get_first<uintptr_t*>(2);
            gHUD.SetAddress(ptr);
            return &gHUD;
        }();
        return address;
    }

    inline GameRef<uintptr_t>* Movies()
    {
        static const auto address = []() -> GameRef<uintptr_t>*
        {
            auto pattern = hook::pattern("8B 0D ? ? ? ? 33 C0 83 B9 24 02 00 00 07 0F 95 C0 C2 04 00");
            auto ptr = *pattern.get_first<uintptr_t*>(2);
            gMovies.SetAddress(ptr);
            return &gMovies;
        }();
        return address;
    }

    inline GameRef<uintptr_t>* Players()
    {
        static const auto address = []() -> GameRef<uintptr_t>*
        {
            auto pattern = hook::pattern("A1 ? ? ? ? D9 44 24 0C 51 8D 0C 06 D9 1C 24 E8 ? ? ? ? 83 C7 01 81 C6 60 30 00 00");
            auto ptr = *pattern.get_first<uintptr_t*>(1);
            gPlayers.SetAddress(ptr);
            return &gPlayers;
        }();
        return address;
    }

    inline GameRef<uintptr_t>* Renderer()
    {
        static const auto address = []() -> GameRef<uintptr_t>*
        {
            auto ptr = *hook::get_pattern<uintptr_t*>("A1 ? ? ? ? 8B 88 74 06 00 00 DB 80 74 06 00 00", 1);
            gRenderer.SetAddress(ptr);
            return &gRenderer;
        }();
        return address;
    }
}
