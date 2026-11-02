module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <stdexcept>

export module EchelonMenus;

import ComVars;

export void InitEchelonMenus()
{

    bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");

    if (bSkipIntro)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "75 0A C7 05 ? ? ? ? ? ? ? ? 8B 4D F4");
        injector::WriteMemory<int>(pattern.get_first<int>(8), 5, true);

    }

    static std::vector<std::string> list;
    GetResolutionsList(list);
    if (list.empty())
        list.emplace_back(std::format("{}x{}", Screen.Width, Screen.Height));
    const auto fallback = list.front();
    while (list.size() < 7)
        list.insert(list.begin(), fallback);
    const auto current = std::format("{}x{}", Screen.Width, Screen.Height);
    const auto currentMode = std::find(list.begin(), list.end(), current);
    if (currentMode != list.end())
        std::iter_swap(currentMode, list.end() - 5);
    else
    {
        std::swap(list[list.size() - 5], list[list.size() - 6]);
        list[list.size() - 5] = current;
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "C6 45 FC 10 33 F6 83 FE 07");
    auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(-850), (uintptr_t)pattern.get_first(0), "C7 45 E8");
    auto defaults = hook::range_pattern((uintptr_t)pattern.get_first(-850), (uintptr_t)pattern.get_first(0), "8D ? E4 68");
    if (rpattern.size() != 7 || defaults.size() != 6)
        throw std::runtime_error("Resolution-menu signatures not found");
    for (size_t i = 0; i < 7; ++i)
    {
        injector::WriteMemory<char*>(rpattern.get(i).get<char*>(3), &list[list.size() - 1 - i][0], true);
    }

    static const auto wsz0 = 0;
    for (size_t i = 0; i < 6; ++i)
    {
        injector::WriteMemory<const int*>(defaults.get(i).get<const int*>(4), &wsz0, true);
    }

    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ebp - 4) = 16;
            regs.esi = 0;
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(6));
}
