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
        for (int i = 0; i < int(std::size(xbox)); ++i)
            label(i, Icons.load(std::memory_order_relaxed) == 2 ? playstation[i] : xbox[i]);
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
            if (glyph >= 107 && count >= 6144)
                continue;
            // Keep the original icon's advance, colour and alpha; labels stay inside its cell.
            static constexpr int letters[] = { 34, 33, 57, 56, 50, 50, 44, 44 };
            Glyph(quad, letters[glyph - 103]);
            if (glyph >= 107 && count < 6144)
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
                Glyph(second, glyph == 107 || glyph == 109 ? 34 : 52);
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
        Icons = std::clamp(settings.Int("MAIN.GamepadIcons"), 0, 2);
        settings.OnChange("MAIN.GamepadIcons", []()
        {
            Icons = std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 2);
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
