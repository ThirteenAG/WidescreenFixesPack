module;

#include "stdafx.h"

export module EchelonMenus;

import ComVars;

export void InitEchelonMenus()
{
    CIniReader iniReader("");
    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 1);

    if (bSkipIntro)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "75 0A C7 05 ? ? ? ? ? ? ? ? 8B 4D F4");
        injector::WriteMemory(pattern.get_first(8), 5, true);

#ifdef DEBUG
        {
            //uint8_t asm_code[13] = { 0 };
            //auto pattern = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "83 F8 05 0F 87");
            //injector::ReadMemoryRaw(pattern.get_first(3), asm_code, sizeof(asm_code), true);
            //injector::MakeNOP(pattern.get_first(3), 13);
            keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
            keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
            //injector::WriteMemoryRaw(pattern.get_first(3), asm_code, sizeof(asm_code), true);
        }
#endif
    }

    static std::vector<std::string> list;
    GetResolutionsList(list);
    std::swap(list[list.size() - 5], list[list.size() - 6]);
    list[list.size() - 5] = std::format("{}x{}", Screen.Width, Screen.Height);

    auto pattern = hook::module_pattern(GetModuleHandle(L"EchelonMenus"), "C6 45 FC 10 33 F6 83 FE 07");
    auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(-850), (uintptr_t)pattern.get_first(0), "C7 45 E8");
    for (size_t i = 0; i < rpattern.size(); ++i)
    {
        injector::WriteMemory(rpattern.get(i).get<uint32_t>(3), &list[list.size() - 1 - i][0], true);
    }

    static const auto wsz0 = 0;
    rpattern = hook::range_pattern((uintptr_t)pattern.get_first(-850), (uintptr_t)pattern.get_first(0), "8D ? E4 68");
    for (size_t i = 0; i < rpattern.size(); ++i)
    {
        injector::WriteMemory(rpattern.get(i).get<uint32_t>(4), &wsz0, true);
    }

    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ebp - 4) = 16;
            regs.esi = 0;
            list.clear();
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(6));
}
