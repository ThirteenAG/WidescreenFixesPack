module;

#include <stdafx.h>
#include "Shared/Console/Viewport.hpp"

export module Hud;

import ComVars;

export namespace THAWHud
{
    inline SafetyHookInline TextHook, SpriteHook, FontDrawHook;
    inline float Width = 640.0f, Height = 480.0f;
    inline const std::string* Constraint = nullptr;
    inline const int32_t* CinemaMode = nullptr;
    inline uintptr_t (*SetTextContext)(uintptr_t) = nullptr;

    constexpr uint32_t Checksum(const char* name)
    {
        uint32_t value = 0xFFFFFFFF;
        for (; *name; ++name)
        {
            value ^= static_cast<uint8_t>(*name);
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ ((value & 1) ? 0xEDB88320 : 0);
        }
        return value;
    }

    struct Placement
    {
        int anchor = 0;
        uintptr_t panel = 0;
        int player = 0;
        bool letterbox = false;
    };

    inline Placement Classify(uintptr_t element)
    {
        if (!HudLayout.is_initialized() || (HudLayout.get() != 0 && HudLayout.get() != 1 && HudLayout.get() != 2 && HudLayout.get() != 4 && HudLayout.get() != 5))
            return {};
        Placement result;
        for (int depth = 0; element && depth < 32; ++depth)
        {
            if (*reinterpret_cast<uint32_t*>(element + 8) & 0x80000u)
                return {};
            const auto id = *reinterpret_cast<uint32_t*>(element + 24);
            if (id == Checksum("letterbox_anchor"))
                result.letterbox = true;
            if (id == Checksum("player1_panel_container") || id == Checksum("player2_panel_container"))
            {
                result.panel = element;
                result.player = id == Checksum("player2_panel_container") ? 1 : 0;
            }
            if (!result.anchor)
            {
                switch (id)
                {
                    case Checksum("the_score"):
                    case Checksum("the_score_sprite"):
                    case Checksum("the_score") + 1:
                    case Checksum("the_score_sprite") + 1:
                        result.anchor = -1;
                        break;
                    case Checksum("current_goal"):
                    case Checksum("goal_points_text"):
                        if (HudLayout.get() != 1 && HudLayout.get() != 2)
                            result.anchor = 1;
                        break;
                    case Checksum("the_run_timer"):
                    case Checksum("the_run_timer") + 1:
                    case Checksum("the_run_timer_hand"):
                    case Checksum("the_run_timer_hand") + 1:
                        if (HudLayout.get() == 1 || HudLayout.get() == 2)
                            result.anchor = 1;
                        break;
                    case Checksum("trick_text_container"):
                    case Checksum("trick_text_container") + 1:
                    case Checksum("the_balance_meter"):
                    case Checksum("the_balance_meter") + 1:
                        if (HudLayout.get() == 1 || HudLayout.get() == 2)
                            result.anchor = 2;
                        break;
                }
            }
            element = *reinterpret_cast<uintptr_t*>(element + 96);
        }
        if ((HudLayout.get() == 1 || HudLayout.get() == 2) && !result.panel)
            return {};
        return result;
    }

    inline float Offset(const Placement& placement)
    {
        if (!HudNativeScaleX.is_initialized() || !std::isfinite(HudNativeScaleX.get()) || HudNativeScaleX.get() <= 0.0f)
            return 0.0f;
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        // The native 2D frame: 4:3, pillarboxed on wider screens, letterboxed on narrower ones
        const float frameWidth = std::min(Width, Height * (4.0f / 3.0f));
        if (HudLayout.get() == 1 || HudLayout.get() == 2)
        {
            const float viewportWidth = HudLayout.get() == 1 ? Width * 0.5f : Width;
            const float viewportHeight = HudLayout.get() == 2 ? Height * 0.5f : Height;
            const float boxWidth = aspect > 0.0f ? std::min(viewportWidth, viewportHeight * aspect) : viewportWidth;
            const float left = HudLayout.get() == 1 ? placement.player * viewportWidth : 0.0f;
            const float edge = placement.anchor == 1 ? 1.0f : placement.anchor == 2 ? 0.5f : 0.0f;
            const float target = left + (viewportWidth - boxWidth) * 0.5f +
                edge * boxWidth;
            const float panelX = *reinterpret_cast<float*>(placement.panel + 308);
            const float native = (Width - frameWidth) * 0.5f +
                panelX * HudNativeScaleX.get() + edge * frameWidth * (HudLayout.get() == 1 ? 0.5f : 1.0f);
            return (target - native) / HudNativeScaleX.get();
        }
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        return placement.anchor * (boxWidth - frameWidth) * 0.5f / HudNativeScaleX.get();
    }

    inline int Draw(uintptr_t element, bool sprite)
    {
        struct TextScope
        {
            uintptr_t previous = 0;
            bool active = false;
            TextScope(uintptr_t current, bool text) : active(text && SetTextContext)
            {
                if (active) previous = SetTextContext(current);
            }
            ~TextScope() { if (active) SetTextContext(previous); }
        } scope(element, !sprite);
        auto& original = sprite ? SpriteHook : TextHook;
        const auto placement = Classify(element);
        // The cutscene bars, without top and bottom borders; Cinema.hpp clears the screen outside the picture, and the
        // bars stay over the scene's own 2D elements
        if (sprite && placement.letterbox && CinemaMode && (std::clamp(*CinemaMode, 0, 3) & 1) == 0)
        {
            auto* y = reinterpret_cast<int16_t*>(element + 322);
            const int16_t savedY = *y;
            *y = INT16_MIN;
            const auto result = original.thiscall<int>(element);
            *y = savedY;
            return result;
        }
        if (!placement.anchor)
            return original.thiscall<int>(element);
        if (!sprite)
        {
            if (!HudTextQuantum.is_initialized() || !std::isfinite(HudTextQuantum.get()) || HudTextQuantum.get() <= 0.0f)
                return original.thiscall<int>(element);
            auto* x = reinterpret_cast<int16_t*>(element + 320);
            const int16_t savedX = *x;
            const double requested = std::round(savedX + static_cast<double>(Offset(placement)) / HudTextQuantum.get());
            *x = static_cast<int16_t>(std::clamp(requested, -32768.0, 32767.0));
            const auto result = original.thiscall<int>(element);
            *x = savedX;
            return result;
        }
        auto* x = reinterpret_cast<float*>(element + 308);
        const float savedX = *x;
        *x = savedX + Offset(placement);
        auto* flags = reinterpret_cast<uint32_t*>(element + 8);
        const uint32_t savedFlags = *flags;
        *flags = savedFlags | 0x800u;
        const auto result = original.thiscall<int>(element);
        *x = savedX;
        *flags = savedFlags;
        return result;
    }

    inline int __fastcall DrawText(uintptr_t element, void*)
    {
        return Draw(element, false);
    }

    inline int __fastcall DrawSprite(uintptr_t element, void*)
    {
        return Draw(element, true);
    }

    inline void __fastcall DrawFont(uintptr_t element, void*)
    {
        if (*reinterpret_cast<void**>(element + 0x1C))
            FontDrawHook.thiscall<void>(element);
    }

    inline void Install(float width, float height, const std::string* constraint, const int32_t* cinemaMode,
                        HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        Constraint = constraint;
        CinemaMode = cinemaMode;
        auto font = hook::module_pattern(module, "81 EC 80 00 00 00 53 55 56 57 8B F9 D9 47 40 F3 0F 10 1D ? ? ? ? D9 FF F3 0F 10 4F 28");
        FontDrawHook = safetyhook::create_inline(font.get_first(), DrawFont);
        auto text = hook::module_pattern(module, "83 EC 28 56 8B F1 8B 86 84 01 00 00 85 C0 0F 84 ? ? ? ? 0F BF 86 42 01 00 00");
        auto sprite = hook::module_pattern(module, "F3 0F 10 0D ? ? ? ? 83 EC 0C 53 56 8B F1 0F BF 86 42 01 00 00 F3 0F 2A C0 F3 0F 59 05");
        auto layout = hook::module_pattern(module, "8B 44 24 08 8B 48 1C 8B 15 ? ? ? ? 33 C0 83 FA 05 77 ? FF 24 95");
        auto scale = hook::module_pattern(module, "D8 0D ? ? ? ? 8B 35 ? ? ? ? 85 F6 D9 1D");
        HudNativeScaleX.SetAddress(*scale.get_first<float*>(16));
        HudTextQuantum.SetAddress(*sprite.get_first<float*>(30));
        HudLayout.SetAddress(*layout.get_first<int*>(9));
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
        SpriteHook = safetyhook::create_inline(sprite.get_first(), DrawSprite);
    }
}
