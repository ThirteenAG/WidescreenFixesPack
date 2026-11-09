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

    // ElectricPickLock: 1.00 lists it among the gadgets unlocked by secondary objectives, although it can't be earned,
    // and shows it in the pause menu equipment. Later builds already drop it from both.
    pattern = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "83 FF 11 8B 4E 70 0F 83");
    if (!pattern.empty())
    {
        // Unlock list rows and their descriptions: 17 gadget names, one fewer without it
        auto desc = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "66 3D 11 00 0F 83 ? ? ? ? 0F BF C0 8B 0C 85");
        auto gadgets = *desc.get_first<const wchar_t**>(16);
        for (size_t i = 0; i < 17; ++i)
        {
            if (_wcsicmp(gadgets[i], L"ElectricPickLock") == 0)
            {
                std::copy(gadgets + i + 1, gadgets + 17, gadgets + i); // the name table is in writable data
                gadgets[16] = nullptr;
                break;
            }
        }
        injector::WriteMemory<uint8_t>(pattern.get_first(2), 16, true);
        injector::WriteMemory<uint8_t>(desc.get_first(2), 16, true);

        // Gadget unlock state: report it as not unlockable instead of mapping it to a profile bit
        auto bit = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "85 C0 74 0A B8 10 00 00 00 E9");
        injector::WriteMemory<uint8_t>(bit.get_first(2), 0xEB, true);

        // Pause menu equipment is read from the ini one entry at a time; drop the ElectricPickLock entry
        static uintptr_t equipmentList = 0;
        static uintptr_t equipmentRemove = 0;
        auto equip = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "8B 0D ? ? ? ? 03 CE FF 15 ? ? ? ? 85 C0 0F 85 ? ? ? ? 6A 01 57 B9");
        equipmentList = *equip.get_first<uintptr_t>(2);
        equipmentRemove = (uintptr_t)injector::GetBranchDestination(equip.get_first(30)).as_int();

        struct EquipmentEntryHook
        {
            void operator()(injector::reg_pack& regs)
            {
                struct FString
                {
                    wchar_t* Data;
                    int32_t ArrayNum;
                    int32_t ArrayMax;
                };

                // esi = byte offset of the entry just read, edi = its index
                auto entry = (FString*)(*(uintptr_t*)equipmentList + regs.esi);
                regs.eax = entry->ArrayNum ? entry->ArrayNum - 1 : 0;
                if (entry->Data && _wcsicmp(entry->Data, L"ElectricPickLock") == 0)
                {
                    reinterpret_cast<void(__fastcall*)(uintptr_t, void*, int32_t, int32_t)>(equipmentRemove)(equipmentList, nullptr, regs.edi, 1);
                    regs.eax = 1; // not the end of the list, go on with the next entry
                }
            }
        }; injector::MakeInline<EquipmentEntryHook>(equip.get_first(0), equip.get_first(14));
    }
}
