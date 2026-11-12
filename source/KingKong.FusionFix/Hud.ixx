module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Hud;

import ComVars;
import RawInput;

// 2D layer: screen strings (prompts, subtitles, inventory, aiming visor) are laid out in 0..1 screen
// fractions, menu and object texts in pixels of the screen width. On screens wider than the box both are laid out
// for a screen of the box's width (glyph widths at the box ratio, object texts at the box width in pixels) and their
// orthographic projection puts that layout into the centred box. Menus keep their 16:9 design; the HUD box follows
// MAIN.HudAspectRatioConstraint, Auto using the whole screen. The 3D view, fades and full-screen layers are untouched.
namespace
{
    constexpr float MenuAspect = 16.0f / 9.0f;
    float Constraint = 0.0f;

    enum class Layer { None, Strings, Objects };
    Layer Drawing = Layer::None;

    float Number(std::string_view text)
    {
        while (!text.empty() && text.front() == ' ')
            text.remove_prefix(1);
        float value = 0.0f;
        std::from_chars(text.data(), text.data() + text.size(), value);
        return value;
    }

    float ParseAspect(std::string_view text)
    {
        const auto colon = text.find(':');
        float value = Number(colon == std::string_view::npos ? text : text.substr(0, colon));
        if (colon != std::string_view::npos)
        {
            const auto height = Number(text.substr(colon + 1));
            value = height > 0.0f ? value / height : 0.0f;
        }
        return std::isfinite(value) && value > 0.0f ? std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
    }

    bool InMenu()
    {
        // Every menu page updates the menu cursor each frame; while the game is paused in the background it keeps
        // its last state
        static bool menu = false;
        if (!GameInactive())
            menu = GetTickCount() - gMenuTick.load() < 250;
        return menu;
    }

    void UpdateBox()
    {
        const float screen = Screen.fAspectRatio;
        if (!std::isfinite(screen) || screen <= 0.0f)
            return;
        float box = InMenu() ? MenuAspect : (Constraint > 0.0f ? Constraint : screen);
        box = std::min(box, screen);
        Screen.fTextBox = box / screen;
        Screen.fTextScale = Screen.fHudScale / Screen.fTextBox;
    }

    SafetyHookInline StringsHook{};
    void __cdecl Strings()
    {
        UpdateBox();
        Drawing = Layer::Strings;
        StringsHook.unsafe_ccall();
        Drawing = Layer::None;
    }

    SafetyHookInline ObjectsHook{};
    int __cdecl Objects(void* object)
    {
        UpdateBox();
        Drawing = Layer::Objects;
        const auto result = ObjectsHook.unsafe_ccall<int>(object);
        Drawing = Layer::None;
        return result;
    }

    // Orthographic projection arguments (left, right, bottom, top, near, far) at esp + 4
    void Ortho(SafetyHookContext& regs)
    {
        const float k = Screen.fTextBox;
        if (Drawing == Layer::None || k >= 0.999f || k <= 0.0f)
            return;
        auto& left = *reinterpret_cast<float*>(regs.esp + 4);
        auto& right = *reinterpret_cast<float*>(regs.esp + 8);
        if (left != 0.0f || right <= 0.0f)
            return;
        if (Drawing == Layer::Strings && right == 1.0f)
        {
            left = 0.5f - 0.5f / k;
            right = 0.5f + 0.5f / k;
        }
        else if (Drawing == Layer::Objects)
        {
            // The texts span the box width from the left; move them to the centred box
            const float offset = right * (1.0f - k) * 0.5f;
            left -= offset;
            right -= offset;
        }
    }

    // Object texts: the screen width in pixels they are laid out for
    void ObjectWidth(int16_t& width)
    {
        if (Screen.fTextBox < 0.999f)
            width = static_cast<int16_t>(std::lround(width * Screen.fTextBox));
    }

    // Every match of a pattern; get_first() alone would end the search at the first one
    hook::pattern AllMatches(std::string_view bytes)
    {
        (void)hook::pattern(bytes).get_first();
        return hook::pattern(bytes);
    }
}

export bool KingKongInMenu()
{
    return InMenu();
}

class Hud
{
public:
    Hud()
    {
        WFP::onInitEvent() += []()
        {
            // The string renderer's glyph width, set to the HUD box below
            auto textScale = hook::pattern("83 F8 04 7D 09 D9 04 85 ? ? ? ? EB 06 D9 86 ? 00 00 00 D8 0D ? ? ? ? 6A 01").get_first<uint8_t>(5);

            auto pattern = hook::pattern("D9 04 8D ? ? ? ? D9 5C 24 18 EB 0A 8B 90 B8 00 00 00");
            if (!Screen.bSignatureEdition) // Gamer's Edition
            {
                auto dword_temp = *pattern.get_first<void*>(3);
                pattern = AllMatches(pattern_str(0xD9, 0x04, '?', to_bytes(dword_temp))); //0xA01C67
                for (size_t i = 0; i < pattern.size(); i++)
                {
                    injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 1, true);
                    injector::WriteMemory<uint16_t>(pattern.get(i).get<uint16_t>(1), 0x05D9, true);
                    injector::WriteMemory<float*>(pattern.get(i).get<float*>(3), &Screen.fHudScale, true);
                }
            }
            else
            {
                pattern = hook::pattern("8B 14 8D ? ? ? ? 89 54 24");
                auto dword_temp = *pattern.get_first<void*>(3);
                injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0x90, true);
                injector::WriteMemory<uint16_t>(pattern.get_first<uint16_t>(1), 0x158B, true);
                injector::WriteMemory<float*>(pattern.get_first<float*>(3), &Screen.fHudScale, true);

                pattern = AllMatches(pattern_str(0xD9, 0x04, '?', to_bytes(dword_temp)));
                for (size_t i = 0; i < pattern.size(); i++)
                {
                    injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 1, true);
                    injector::WriteMemory<uint16_t>(pattern.get(i).get<uint16_t>(1), 0x05D9, true);
                    injector::WriteMemory<float*>(pattern.get(i).get<float*>(3), &Screen.fHudScale, true);
                }

                pattern = hook::pattern("D8 0C 95 ? ? ? ? E8");
                injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0x90, true);
                injector::WriteMemory<uint16_t>(pattern.get_first<uint16_t>(1), 0x0DD8, true);
                injector::WriteMemory<float*>(pattern.get_first<float*>(3), &Screen.fHudScale, true);
            }
            injector::WriteMemory<float*>(reinterpret_cast<float**>(textScale + 3), &Screen.fTextScale, true);

            const auto refresh = []() { Constraint = ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint")); };
            refresh();
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", refresh);

            pattern = hook::pattern("81 EC D8 01 00 00 57 33 FF C7 84 24 DC 00 00 00 09 00 00 00");
            if (!Screen.bSignatureEdition) // Gamer's Edition
            {
                StringsHook = safetyhook::create_inline(pattern.get_first(), Strings);
                ObjectsHook = safetyhook::create_inline(hook::pattern("81 EC 2C 04 00 00 8B 15 ? ? ? ? 53 57 8B BC 24 38 04 00 00").get_first(), Objects);

                static auto ObjectWidthHook = safetyhook::create_mid(hook::pattern("D9 80 F4 00 00 00 E8 ? ? ? ? 66 89 43 18").get_first(15), [](SafetyHookContext& regs)
                {
                    ObjectWidth(*reinterpret_cast<int16_t*>(regs.ebx + 0x18));
                });

                // The projection builder itself
                static auto OrthoHook = safetyhook::create_mid(hook::pattern("83 EC 44 D9 44 24 4C 56 D8 64 24 4C 57").get_first(), Ortho);
            }
            else // Signature Edition
            {
                StringsHook = safetyhook::create_inline(hook::pattern("81 EC D8 01 00 00 A1 ? ? ? ? 57 33 FF 3B C7 C7 84 24 DC 00 00 00 09 00 00 00").get_first(), Strings);
                ObjectsHook = safetyhook::create_inline(hook::pattern("A1 ? ? ? ? 81 EC 2C 04 00 00 57 33 FF 3B C7 0F 85").get_first(), Objects);

                static auto ObjectWidthHook = safetyhook::create_mid(hook::pattern("D9 80 EC 00 00 00 E8 ? ? ? ? 66 89 45 18").get_first(15), [](SafetyHookContext& regs)
                {
                    ObjectWidth(*reinterpret_cast<int16_t*>(regs.ebp + 0x18));
                });

                // The 2D camera's call of the projection builder (normalized or pixel space)
                pattern = hook::pattern("68 00 00 80 BF 6A 00 68 00 00 80 3F 68 00 00 80 3F EB 16 DB 44 24 ? 8B 54 24 ? 6A 00 D9 5C 24 ? 8B 4C 24 ? 51 6A 00 52 6A 00 50 E8");
                static auto OrthoHook = safetyhook::create_mid(pattern.get_first(44), Ortho);
            }
        };
    }
} Hud;
