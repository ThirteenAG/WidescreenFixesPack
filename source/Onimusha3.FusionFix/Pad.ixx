module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Pad;
import ComVars;
import NativeMenu;
import DeveloperOverlay;


export namespace OnimushaPad
{
    inline std::atomic<int> Icons{ 0 };
    inline SafetyHookInline FontHook;
    inline SafetyHookMid ActionHook;
    using Action = std::array<uint32_t, 10>;
    inline std::map<const Action*, std::array<Action, 71>> ActionMaps;

    inline void ExtendActions(uint32_t* format)
    {
        const auto original = reinterpret_cast<const Action*>(format[4]);
        auto& actions = ActionMaps[original];
        std::memcpy(actions.data(), original, 70 * sizeof(Action));
        // The native Tab action has no controller semantic. Keep it and add RS for the same action.
        actions[70] = original[68];
        actions[70][1] = 0xFF00440F;
        actions[70][2] = 0;
        std::fill(actions[70].begin() + 4, actions[70].end(), 0);
        format[2] = 71 * sizeof(uint32_t);
        format[3] = 71;
        format[4] = reinterpret_cast<uintptr_t>(actions.data());
    }

    inline void Names()
    {
        using SetLabel = void(__cdecl*)(int, const char*);
        const auto module = GetModuleHandleW(L"Xidi.32.dll");
        const auto label = module ? reinterpret_cast<SetLabel>(GetProcAddress(module, "XidiSetButtonLabel")) : nullptr;
        if (!label)
            return;
        static constexpr const char* xbox[] = { "LS X", "LS Y", "RS X", "RS Y", "D-pad Up", "D-pad Down",
            "D-pad Left", "D-pad Right", "LT", "RT", "A", "B", "X", "Y", "LB", "RB", "Back", "Start", "LS", "RS", "Guide", "Share" };
        static constexpr const char* playstation[] = { "LS X", "LS Y", "RS X", "RS Y", "D-pad Up", "D-pad Down",
            "D-pad Left", "D-pad Right", "L2", "R2", "Cross", "Circle", "Square", "Triangle", "L1", "R1", "Select", "Start", "L3", "R3", "PS", "Share" };
        static constexpr const char* nintendo[] = { "LS X", "LS Y", "RS X", "RS Y", "D-pad Up", "D-pad Down",
            "D-pad Left", "D-pad Right", "ZL", "ZR", "B", "A", "Y", "X", "L", "R", "-", "+", "LS", "RS", "Home", "Capture" };
        const int family = Icons.load(std::memory_order_relaxed);
        for (int i = 0; i < int(std::size(xbox)); ++i)
            label(i, family == 2 ? playstation[i] : family == 3 ? nintendo[i] : xbox[i]);
    }

    inline void Glyph(uint16_t* quad, int glyph)
    {
        const int x = (glyph & 15) * CellWidth;
        const int y = (glyph >> 4) * CellHeight;
        quad[8] = uint16_t(x * 16 + 8);
        quad[9] = uint16_t(y * 16 + 8);
        quad[10] = uint16_t((x + CellWidth) * 16);
        quad[11] = uint16_t((y + CellHeight) * 16);
    }

    inline void Remap(uint8_t* owner, unsigned first, unsigned last)
    {
        auto& count = *reinterpret_cast<uint16_t*>(owner + 258320);
        for (unsigned i = first; i < last; ++i)
        {
            auto quad = reinterpret_cast<uint16_t*>(owner + 49216 + i * 32);
            if (quad[0] != 0 || quad[1] != 0)
                continue;
            const int glyph = (int(quad[8]) - 8) / (CellWidth * 16) +
                16 * ((int(quad[9]) - 8) / (CellHeight * 16));
            if (glyph < 103 || glyph > 110)
                continue;
            // The PS2 icons Circle, Cross, Triangle, Square, R1, R2, L1, L2 as letters of the font (A = 33): Xbox
            // B A Y X RB RT LB LT, Nintendo A B X Y R ZR L ZL. Two letters share the icon's cell.
            const bool nintendo = Icons.load(std::memory_order_relaxed) == 3;
            static constexpr int xbox[8][2] = { { 34, 0 }, { 33, 0 }, { 57, 0 }, { 56, 0 }, { 50, 34 }, { 50, 52 }, { 44, 34 }, { 44, 52 } };
            static constexpr int switchPad[8][2] = { { 33, 0 }, { 34, 0 }, { 56, 0 }, { 57, 0 }, { 50, 0 }, { 58, 50 }, { 44, 0 }, { 58, 44 } };
            const auto& letters = (nintendo ? switchPad : xbox)[glyph - 103];
            if (letters[1] && count >= 6144)
                continue;
            // Keep the original icon's advance, colour and alpha; labels stay inside its cell.
            Glyph(quad, letters[0]);
            if (letters[1])
            {
                auto second = reinterpret_cast<uint16_t*>(owner + 49216 + count++ * 32);
                std::memcpy(second, quad, 32);
                const auto middle = uint16_t((int16_t(quad[12]) + int16_t(quad[14])) / 2);
                quad[14] = middle;
                second[12] = middle;
                const int top = int16_t(quad[13]);
                const int height = int16_t(quad[15]) - top;
                quad[13] = second[13] = uint16_t(top + height / 4);
                quad[15] = second[15] = uint16_t(top + height / 4 + (height + 1) / 2);
                Glyph(second, letters[1]);
            }
        }
    }

    inline unsigned __fastcall Font(uint8_t* owner, void*, uint8_t* text, unsigned position, int16_t alpha, int16_t size)
    {
        auto& count = *reinterpret_cast<uint16_t*>(owner + 258320);
        const auto first = count;
        const auto result = FontHook.unsafe_thiscall<unsigned>(owner, text, position, alpha, size);
        if (Icons.load(std::memory_order_relaxed) == 2 || OnimushaMenu::NativeLanguage < 2 || OnimushaMenu::NativeLanguage > 7 || CellWidth <= 0 || CellHeight <= 0)
            return result;
        Remap(owner, first, count);
        return result;
    }

    inline void Init()
    {
        auto& settings = FixSettings();
        Icons = std::clamp(settings.Int("MAIN.GamepadIcons"), 0, 3);
        settings.OnChange("MAIN.GamepadIcons", []()
        {
            Icons = std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3);
            Names();
        });
        auto metrics = hook::pattern("A1 ? ? ? ? 8B 1D ? ? ? ? EB 0B A1 ? ? ? ? 8B 1D");
        CellWidth.SetAddress(*metrics.get_first<int*>(1));
        CellHeight.SetAddress(*metrics.get_first<int*>(7));
        FontHook = safetyhook::create_inline(hook::pattern("83 EC 24 53 55 56 57 8B D9 89 5C 24 30 0F B7 44 24 40 D9 EE").get_first(), Font);
        ActionHook = safetyhook::create_mid(hook::pattern("57 68 68 01 00 00 88 4E 5C E8 ? ? ? ? 8B F8 83 C4 04 85 FF").get_first(),
            [](SafetyHookContext& context)
            {
                ExtendActions(reinterpret_cast<uint32_t*>(context.ebx));
            });
        Names();
        OnimushaOverlay::InputCreated = Names;
    }
}
