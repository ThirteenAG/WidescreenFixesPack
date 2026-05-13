module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module MenuMap;

import Skeleton;

class MenuMap
{
public:
    MenuMap()
    {
        WFP::onGameInitEvent() += []()
        {
            // Improved menu map page
            auto pattern = hook::pattern("0F 8A ? ? ? ? 81 F9");
            if (!pattern.empty())
                injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("0F 8A ? ? ? ? 81 FA");
            if (!pattern.empty())
                injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("0F 85 ? ? ? ? 81 F9 ? ? ? ? 75 ? ? ? ? ? ? ? ? ? EB");
            if (!pattern.empty())
                injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("0F 85 ? ? ? ? 81 F9 ? ? ? ? 75 ? ? ? ? ? EB");
            if (!pattern.empty())
                injector::MakeNOP(pattern.get_first(), 6, true);

            pattern = hook::pattern("68 ? ? ? ? 6A ? ? ? ? ? ? ? ? ? 6A ? 6A ? 8D 4C 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? ? ? ? ? 6A ? ? ? ? ? 6A ? ? ? ? ? ? ? 6A ? 8D 4C 24 ? C7 44 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 50 8D 54 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? ? ? ? ? 6A ? ? ? ? ? ? ? 6A ? 6A ? 8D 4C 24 ? ? ? ? ? C7 44 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? ? ? ? ? 6A ? ? ? ? ? 6A ? ? ? ? ? ? ? 6A ? 8D 4C 24 ? C7 44 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 50 8D 4C 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? 2B C6");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? ? ? ? ? 6A ? ? ? ? ? ? ? 6A ? 6A ? 8D 4C 24 ? ? ? ? ? ? ? ? ? ? ? C7 44 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? ? ? ? ? 6A ? 6A ? 6A ? ? ? ? ? 8D 4C 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? 6A ? ? ? ? ? C7 44 24 ? ? ? ? ? ? ? ? ? ? ? 6A");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), 0, true);

            pattern = hook::pattern("68 ? ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? DF E0 F6 C4 ? 0F 8A");
            if (!pattern.empty())
                injector::WriteMemory<float>(pattern.get_first(1), -9999.0f, true);

            pattern = hook::pattern("68 ? ? ? ? 8B CE C6 44 24");
            if (!pattern.empty())
                injector::WriteMemory<float>(pattern.get_first(1), -9999.0f, true);

            pattern = hook::pattern("C7 44 24 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 50 8D 4C 24");
            if (!pattern.empty())
                injector::WriteMemory<float>(pattern.get_first(4), -9999.0f, true);

            static float f64 = -9999.0f;
            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? D9 46 ? 8D 84 24 ? ? ? ? D8 25");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &f64, true);

            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? D9 46 ? 8D 84 24 ? ? ? ? D8 05");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &f64, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D9 C0 E8 ? ? ? ? D9 C9 D8 E1 8B F0");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &f64, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D9 C0 E8 ? ? ? ? D9 C9 D8 E1 8B E8");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &f64, true);

            static float f576 = 9999.0f;
            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? D9 46 ? 8D 94 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &f576, true);

            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 05 0F 8A ? ? ? ? D9 46 ? 8D 84 24 ? ? ? ? D8 25 ? ? ? ? 50 8D 4C 24 ? 51 D9 56 ? D9 46 ? D9 C9 D9 5C 24 ? 8B 54 24 ? 89 94 24 ? ? ? ? D9 9C 24 ? ? ? ? E8 ? ? ? ? 8D 54 24 ? 52 E8 ? ? ? ? DD D8 8D 44 24 ? 50 8D 4C 24 ? 51 E8 ? ? ? ? DB 86");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &f576, true);

            pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? ? ? ? ? DF E0 F6 C4 ? 0F 85 ? ? ? ? ? ? ? 8D 84 24");
            if (!pattern.empty())
                injector::WriteMemory<float>(pattern.get_first(1), 9999.0f, true);

            pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? ? ? ? ? DF E0 F6 C4 ? 0F 85 ? ? ? ? ? ? ? ? ? ? 68");
            if (!pattern.empty())
                injector::WriteMemory<float>(pattern.get_first(1), 9999.0f, true);

            pattern = hook::pattern("6A ? E8 ? ? ? ? 8B 44 24 ? 83 C4 ? 85 C0 74 ? 50 B9");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);

            // Zoom speed
            static float speed = 7.0f * 2.0f;

            pattern = hook::pattern("D8 0D ? ? ? ? D8 6E ? D9 5E ? D9 46 ? 8D 94 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &speed, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 46 ? D9 5E ? D9 46 ? 8D 8C 24 ? ? ? ? D9 46 ? 51 D9 9C 24 ? ? ? ? 8D 54 24 ? 52 D9 9C 24 ? ? ? ? E8 ? ? ? ? 8D 44 24 ? 50 E8 ? ? ? ? DD D8 8D 4C 24 ? 51 8D 54 24 ? 52 E8 ? ? ? ? D9 84 24 ? ? ? ? D8 1D ? ? ? ? 83 C4 14 DF E0 F6 C4 05 0F 8A ? ? ? ? EB 06 8D 9B ? ? ? ? D9 44 24 ? D8 1D ? ? ? ? DF E0 F6 C4 41 0F 85 ? ? ? ? D9 46 ? 8D 84 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &speed, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 6E ? D9 5E ? D9 46 ? 8D 84 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &speed, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 46 ? D9 5E ? D9 46 ? 8D 8C 24 ? ? ? ? D9 46 ? 51 D9 9C 24 ? ? ? ? 8D 54 24 ? 52 D9 9C 24 ? ? ? ? E8 ? ? ? ? 8D 44 24 ? 50 E8 ? ? ? ? DD D8 8D 4C 24 ? 51 8D 54 24 ? 52 E8 ? ? ? ? D9 84 24 ? ? ? ? D8 1D ? ? ? ? 83 C4 14 DF E0 F6 C4 05 0F 8A ? ? ? ? EB 06 8D 9B ? ? ? ? D9 44 24 ? D8 1D ? ? ? ? DF E0 F6 C4 41 0F 85 ? ? ? ? D9 46 ? 8D 8C 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &speed, true);

            pattern = hook::pattern("D8 05 ? ? ? ? 51 D9 5E");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &speed, true);

            pattern = hook::pattern("D8 25 ? ? ? ? 51 D9 5E");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(2), &speed, true);
        };
    }
} MenuMap;