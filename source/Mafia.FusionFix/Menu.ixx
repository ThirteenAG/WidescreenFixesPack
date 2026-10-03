module;

#include <stdafx.h>
#include <cmath>

export module Menu;

import ComVars;

uintptr_t menuRawCoordinates;
std::array<uintptr_t, 5> menuCallers;
bool menuHooksInstalled;

void InstallMenuHooks()
{
    auto pMenuImagesOld = find_pattern("E8 ? ? ? ? D9 45 10 8B 45 34 C7 45 00 ? ? ? ? D9 55 38 D9 45 14 3B C3 D9 54 24 20 D9 5D 3C 74 ? D8 40 38 D9 5D 38 D9 44 24 20 D8 40 3C C7 45 00 ? ? ? ? 8B C5 D9 5D 3C E9 ? ? ? ? DD D8 C7 45 00 ? ? ? ? 8B C5 E9 ? ? ? ? 81 E9 64 6E 69 77");
    auto pMenuPanelsOld = find_pattern("E8 ? ? ? ? D9 45 10 8B 45 34 C7 45 00 ? ? ? ? D9 55 38 D9 45 14 3B C3 D9 54 24 20 D9 5D 3C 74 ? D8 40 38 D9 5D 38 D9 44 24 20 D8 40 3C D9 5D 3C EB ? DD D8 C7 45 00 ? ? ? ? 8B C5");
    auto pMenuGarageOld = find_pattern("E8 ? ? ? ? D9 45 10 8B 45 34 C7 45 00 ? ? ? ? D9 55 38 D9 45 14 3B C3 D9 54 24 20 D9 5D 3C 74 ? D8 40 38 D9 5D 38 D9 44 24 20 D8 40 3C 8B C5");
    auto pMenuRaceOld = find_pattern("E8 ? ? ? ? D9 46 10 8B 46 34 C7 06 ? ? ? ?");
    auto pMenuCarsOld = find_pattern("E8 ? ? ? ? 8B 46 08 C7 06 ? ? ? ? 0D 01 00 01 00");
    auto pMenuPanelsNew = find_pattern("E8 ? ? ? ? C7 06 ? ? ? ? 8B C6 8B 4C 24 10 5F 5E 5B 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 90 90 90 B8 64 6E 69 77");
    auto pMenuImagesNew = find_pattern("E8 ? ? ? ? C7 06 ? ? ? ? 8B C6 8B 4C 24 10 5F 5E 5B 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 90 90 90 B8 63 6E 69 77");
    if (menuHooksInstalled) return;
    menuHooksInstalled = true;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        if (pMenuImagesOld.size() != 1 ||
            pMenuPanelsOld.size() != 1 ||
            pMenuGarageOld.size() != 1 ||
            pMenuRaceOld.size() != 1 ||
            pMenuCarsOld.size() != 1)
            return;
        menuCallers = {reinterpret_cast<uintptr_t>(pMenuImagesOld.get_first()) + 5, reinterpret_cast<uintptr_t>(pMenuPanelsOld.get_first()) + 5,
            reinterpret_cast<uintptr_t>(pMenuGarageOld.get_first()) + 5, reinterpret_cast<uintptr_t>(pMenuRaceOld.get_first()) + 5, reinterpret_cast<uintptr_t>(pMenuCarsOld.get_first()) + 5};
        auto pCoordinateMode = find_pattern("8A 15 ? ? ? ? 3A D1 74 ? 8B 50 08 89 56 10");
        auto flag = pCoordinateMode.size() == 1 ? reinterpret_cast<uintptr_t>(pCoordinateMode.get_first()) : 0;
        auto pPosition = find_pattern("8B 50 1C 89 4C 24 08 89 56 20 8B 50 20");
        auto position = pPosition.size() == 1 ? reinterpret_cast<uintptr_t>(pPosition.get_first()) : 0;
        if (!flag || !position) return;
        menuRawCoordinates = Field<uintptr_t>(flag, 2);
        static auto MenuPositionHook = safetyhook::create_mid(position, [](SafetyHookContext& regs)
        {
            if (Field<uint8_t>(menuRawCoordinates, 0)) return;
            auto caller = Field<uintptr_t>(regs.esp, 0xC);
            auto templateOffset = regs.eax - menuRawCoordinates;
            // Offsets within the original menu-template buffer, whose base is
            // discovered from the constructor's native coordinate-mode flag.
            static constexpr std::array<uint32_t, 23> panels = {
                0x2B4, 0x200, 0x638, 0x2474, 0x2840, 0x695C, 0x4148, 0x4484, 0x45C8,
                0xDF4, 0xECC, 0xF14, 0xF5C, 0x8E4, 0x6CBC, 0x80C, 0x2A80, 0x2AC8,
                0x4E80, 0x4F58, 0x2E4C, 0x65D8, 0x6470};
            bool shift = caller == menuCallers[1] && std::ranges::find(panels, templateOffset) != panels.end();
            shift |= caller == menuCallers[0] && (templateOffset == 0x6788 || templateOffset == 0x266C);
            shift |= (caller == menuCallers[2] || caller == menuCallers[3] || caller == menuCallers[4]) &&
                (templateOffset == 0x5030 || templateOffset == 0x4970 || templateOffset == 0x278C);
            if (shift) Field<float>(regs.esi, 0x10) += fMenuOffset;
        });
    }
    else
    {
        if (pMenuPanelsNew.size() != 1 ||
            pMenuImagesNew.size() != 1)
            return;
        menuCallers[0] = reinterpret_cast<uintptr_t>(pMenuPanelsNew.get_first()) + 5;
        menuCallers[1] = reinterpret_cast<uintptr_t>(pMenuImagesNew.get_first()) + 5;
        // Both X copies and Y are stored here; the x87 stack is empty. Shift
        // before the native constructor attaches the object to its parent.
        auto pPosition = find_pattern("D9 5E 18 D9 05 ? ? ? ? D8 48 10 D9 5E 1C D9 05 ? ? ? ? D8 48 14");
        auto position = pPosition.size() == 1 ? reinterpret_cast<uintptr_t>(pPosition.get_first()) : 0;
        if (!position) return;
        static auto MenuPositionHook = safetyhook::create_mid(position + 3, [](SafetyHookContext& regs)
        {
            auto caller = Field<uintptr_t>(regs.esp, 0x18);
            auto type = Field<uint32_t>(regs.eax, 0);
            auto flags = Field<uint32_t>(regs.eax, 0x1C);
            bool shift = caller == menuCallers[0] && (type == 1 || type == 3 || type == 0x82 ||
                (type == 2 && (flags == 0xFF00B2 || flags == 0xFF0032 || flags == 0x1F0032)));
            shift |= caller == menuCallers[1] && type >= 1 && type <= 3;
            if (shift)
            {
                Field<float>(regs.esi, 0x14) += fMenuOffset;
                Field<float>(regs.esi, 0x24) += fMenuOffset;
            }
        });
    }
}

export void InitMenu()
{
    auto pOnePCurBaseWidth4 = find_pattern("D8 0D ? ? ? ? D9 1D ? ? ? ? 8B 10 FF 52 70",
        "D8 0D ? ? ? ? EB ? 8B 10 FF 52 70 89 44 24 18");
    auto pOnePCurBaseWidth3 = find_pattern("D8 0D ? ? ? ? EB ? 8B 10 FF 52 70 89 44 24 1C");
    auto pHudPleaseWaitTextX = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 10",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 08");
    auto pHudPleaseWaitTextX2 = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 08",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? A1 ? ? ? ? 50 8B 10");
    if (pOnePCurBaseWidth4.size() != 1 ||
        pHudPleaseWaitTextX.size() != 1 ||
        pHudPleaseWaitTextX2.size() != 1)
        return;
    if (nGameVersion != MAFIA_1_0_ENG && (pOnePCurBaseWidth3.size() != 1)) return;


    if (nGameVersion == MAFIA_1_0_ENG)
    {
        injector::WriteMemory(pOnePCurBaseWidth4.get_first(0x2), &Screen.fInvBaseWidth, true);
    }
    else
    {
        injector::WriteMemory(pOnePCurBaseWidth3.get_first(0x2), &Screen.fInvBaseWidth, true);
        injector::WriteMemory(pOnePCurBaseWidth4.get_first(0x2), &Screen.fInvBaseWidth, true);
    }

    float fixMenuRight = (Screen.fBaseWidth - 800.0f) / 2.0f;

    static float menuPleaseWaitTextX = 780.0f + fixMenuRight;
    injector::WriteMemory(pHudPleaseWaitTextX.get_first(0x2), &menuPleaseWaitTextX, true);
    injector::WriteMemory(pHudPleaseWaitTextX2.get_first(0x2), &menuPleaseWaitTextX, true);
    fMenuOffset = (Screen.fWidth - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
    InstallMenuHooks();
}
