module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"

export module PadLabels;
import ComVars;
import Window;


export namespace LARushPadLabels
{
    inline SafetyHookMid DrawHook, MenuButtonsHook;

    inline const char* Button(int index, int family)
    {
        static constexpr const char* names[3][12]
        {
            { "[X]", "[B]", "[A]", "[Y]", "[LT]", "[RT]", "[LB]", "[RB]", "[VIEW]", "[MENU]", "[LS]", "[RS]" },
            { "[SQUARE]", "[CIRCLE]", "[CROSS]", "[TRIANGLE]", "[L2]", "[R2]", "[L1]", "[R1]", "[SHARE]", "[OPTIONS]", "[L3]", "[R3]" },
            { "[Y]", "[A]", "[B]", "[X]", "[ZL]", "[ZR]", "[L]", "[R]", "[-]", "[+]", "[LS]", "[RS]" }
        };
        return index >= 0 && index < 12 ? names[family][index] : nullptr;
    }

    inline void Init()
    {
        // The gamepad menu defaults accept with Button 2 and go back with Button 3, which are B and A on the
        // bundled controller layout: swap the two records so A accepts and B goes back.
        auto menu = hook::pattern("8B 15 ? ? ? ? 6A 09 6A 00 6A 06 6A 02");
        MenuTable.SetAddress(*menu.get_first<uint8_t**>(2));
        MenuButtonsHook = safetyhook::create_mid(menu.get_first(), [](SafetyHookContext&)
        {
            constexpr size_t record = 148;
            auto table = MenuTable.get();
            std::swap_ranges(table + 4 * record, table + 5 * record, table + 5 * record);
        });

        auto records = hook::pattern("8D 8E ? ? ? ? 8B F9 8B D0 2B FA 8A 10 88 14 07 40 3A D3 75 ? 6A 40 8D 86");
        Bindings.SetAddress(*records.get_first<uint8_t*>(2));
        auto draw = hook::pattern("83 EC 5C 53 55 56 8B F0 57 8D 44 24 24 50 8D 4C 24 24 51 33 ED");
        DrawHook = safetyhook::create_mid(draw.get_first(), [](SafetyHookContext& regs)
        {
            if (LARushWindow::GameState != 165)
                return;
            const auto text = static_cast<uintptr_t>(regs.eax);
            const auto first = reinterpret_cast<uintptr_t>(Bindings.get_ptr() + 64);
            if (text < first || text >= first + 13 * 208 || (text - first) % 208 != 0)
                return;
            const auto record = Bindings.get_ptr() + ((text - first) / 208) * 208;
            const int index = *reinterpret_cast<int32_t*>(record + 128);
            const int kind = *reinterpret_cast<int32_t*>(record + 132);
            const auto family = PadInput::ResolveFamily(std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3));
            const char* label = kind == 2 ? Button(index, static_cast<int>(family) - 1) :
                kind == 3 ? "[LS LEFT]" : kind == 4 ? "[LS RIGHT]" : nullptr;
            if (label)
                regs.eax = reinterpret_cast<uintptr_t>(label);
        });
    }
}
