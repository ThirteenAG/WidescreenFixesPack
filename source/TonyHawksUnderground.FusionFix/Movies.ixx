module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/Bink9.hpp"

export module Movies;

export void InstallMovie()
{
    TonyBink9::Install(&FixSettings().Int("MAIN.FMVWidescreenMode"),
        "81 EC 80 01 00 00 A1 ? ? ? ? 33 C4 89 84 24 7C 01 00 00 56 8B B4 24 88 01 00 00 85 F6",
        "D9 1D ? ? ? ? 8B D1 85 D2 89 54 24 ? DB 44 24 ? 7D ? D8 05 ? ? ? ? 8B 15 ? ? ? ? D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 00 8D 7C 24 ? 57 6A 01 D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 16 6A 00 6A 01 51 D9 1D",
        { 2, 34, 55, 74 });
}
