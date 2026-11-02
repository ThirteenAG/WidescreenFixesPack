module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module NativeMenuContracts;

import ComVars;


export namespace PunMenu
{
    // Native options screens, list widgets and menu input of pun.exe
    struct NativeAddresses
    {
        uintptr_t optionsInit{}, optionsInput{}, gameInit{}, gameInput{}, defaults{}, draw{};
        uintptr_t measure{}, select{}, press{}, repeat{}, hit{};
        uintptr_t options{}, optionsList{}, optionsBackground{}, gameSettings{}, gameLabels{}, gameValues{};
        uintptr_t listTable{}, dataTable{};
    };

    inline const NativeAddresses& Native()
    {
        static const auto addresses = []()
        {
            NativeAddresses a;
            auto operand = [](uintptr_t address) { return *reinterpret_cast<uintptr_t*>(address); };
            // OptionsMenu::Init: background, options screen and its list
            a.optionsInit = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? 84 C0 B9 ? ? ? ? 74 17 C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? C7 00 00 00 80 3F EB 15 A1 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? C7 00 00 00 00 00 B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A 02 B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 8B 4C 24 04 8B 01 83 F8 04").get_first());
            a.optionsBackground = operand(a.optionsInit + 8);
            a.options = operand(a.optionsInit + 0x3B);
            a.optionsList = operand(a.optionsInit + 0x5B);
            a.optionsInput = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 83 EC 08 85 C0 0F 85 5A 01 00 00 6A 00 6A 00 8D 44 24 0C").get_first());
            // GameSettingsMenu::Init: screen, label list and value list
            a.gameInit = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? 84 C0 B9 ? ? ? ? 74 1E C7 05 ? ? ? ? 00 00 00 00").get_first());
            a.gameSettings = operand(a.gameInit + 0x49);
            a.gameLabels = operand(a.gameInit + 0x70);
            a.gameValues = operand(a.gameInit + 0x7C);
            a.gameInput = reinterpret_cast<uintptr_t>(hook::pattern("83 EC 10 E8 ? ? ? ? 84 C0 0F 85 9D 04 00 00 A1 ? ? ? ? 85 C0 0F").get_first());
            a.defaults = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 53 BB 01 00 00 00 89 98 64 02 00 00 E8 ? ? ? ? 84 C0").get_first());
            a.draw = reinterpret_cast<uintptr_t>(hook::pattern("83 EC 38 53 55 56 57 8B F9 B9 ? ? ? ? E8 ? ? ? ? 89 44 24 1C DB").get_first());
            a.measure = reinterpret_cast<uintptr_t>(hook::pattern("83 EC 0C 57 8B 7C 24 1C 85 FF 75 11 8B 44 24 14 8B 4C 24 18 89 38 89 39").get_first());
            a.select = reinterpret_cast<uintptr_t>(hook::pattern("56 8B F1 8B 4E 64 85 C9 75 0B C7 46 60 FF FF FF FF 5E C2 04 00").get_first());
            a.press = reinterpret_cast<uintptr_t>(hook::pattern("E9 ? ? ? ? CC CC CC CC CC CC CC CC CC CC CC 8B 44 24 0C 8B 4C 24 10").get_first());
            a.repeat = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? 84 C0 74 1C E8 ? ? ? ? 84 C0 75 13 8B 44 24 08 6A 00").get_first());
            a.hit = reinterpret_cast<uintptr_t>(hook::pattern("51 8B 44 24 08 53 55 56 33 F6 85 C0 57 8B F9 74 06 C7 00 FF FF FF FF 8B").get_first());
            // Menu font id, stored when the fonts are loaded
            MenuFont.SetAddress(operand(reinterpret_cast<uintptr_t>(hook::pattern("6A 01 6A FF 68 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 6A 00 6A FF 68 ? ? ? ? A3").get_first(10))));
            // Vtables of the list widget and of its text row provider
            a.listTable = operand(reinterpret_cast<uintptr_t>(hook::pattern("89 46 58 89 46 54 C7 06 ? ? ? ? 89 4E 60 89 56 5C C6 46 23 01").get_first(8)));
            a.dataTable = operand(reinterpret_cast<uintptr_t>(hook::pattern("8D 8E 74 01 00 00 C7 06 ? ? ? ? 89 86 68 01 00 00 E8").get_first(8)));
            return a;
        }();
        return addresses;
    }
}
