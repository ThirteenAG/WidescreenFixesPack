module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/Console/Viewport.hpp"
#include "Shared/TonyHawk/PadPrompts.hpp"

export module Hud;

import ComVars;

export namespace THUG2Hud
{
    inline SafetyHookInline TextHook, SpriteHook;
    inline float Width = 640.0f, Height = 480.0f;
    inline const std::string* Constraint = nullptr;

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

    constexpr int Hidden = 3;

    struct Placement
    {
        int anchor = 0;     // -1 left edge, 1 right edge, 2 centre of a split-screen half, Hidden: cutscene bars
        uintptr_t panel = 0;
        int player = 0;
    };

    inline bool Split()
    {
        return HudLayout.get() == 1 || HudLayout.get() == 2;
    }

    // One view (and HORSE, one player at a time) keeps the native 640-wide frame; split screen places each
    // player's panel in its own half. The cutscene bar sprites are hidden (the frame clears outside the picture).
    inline Placement Classify(uintptr_t element)
    {
        if (const auto parent = *reinterpret_cast<uintptr_t*>(element + 76);
            parent && *reinterpret_cast<uint32_t*>(parent + 20) == Checksum("letterbox_anchor"))
            return { Hidden };
        if (!HudLayout.is_initialized() || (HudLayout.get() != 0 && HudLayout.get() != 1 && HudLayout.get() != 2 &&
            HudLayout.get() != 4 && HudLayout.get() != 5))
            return {};
        const bool split = Split();
        Placement result;
        for (int depth = 0; element && depth < 32; ++depth)
        {
            if (*reinterpret_cast<uint32_t*>(element + 8) & 0x80000u)
                return {};
            const auto id = *reinterpret_cast<uint32_t*>(element + 20);
            if (id == Checksum("player1_panel_container") || id == Checksum("player2_panel_container"))
            {
                result.panel = element;
                result.player = id == Checksum("player2_panel_container") ? 1 : 0;
            }
            if (!result.anchor && split)
            {
                switch (id)
                {
                    case Checksum("the_score"):
                    case Checksum("the_score_sprite"):
                    case Checksum("the_score") + 1:
                    case Checksum("the_score_sprite") + 1:
                        result.anchor = -1;
                        break;
                    case Checksum("the_run_timer"):
                    case Checksum("the_run_timer") + 1:
                        result.anchor = 1;
                        break;
                    case Checksum("trick_text_container"):
                    case Checksum("trick_text_container") + 1:
                    case Checksum("the_balance_meter"):
                    case Checksum("the_balance_meter") + 1:
                        result.anchor = 2;
                        break;
                }
            }
            else if (!result.anchor)
            {
                switch (id)
                {
                    case Checksum("the_score"):
                    case Checksum("the_score_sprite"):
                    case Checksum("the_score") + 1:
                    case Checksum("the_score_sprite") + 1:
                    case Checksum("goal_points_display"):
                    case Checksum("pager_new_message_icon"):
                    case Checksum("console_message_vmenu"):
                    case Checksum("horse_score_menu"):
                        result.anchor = -1;
                        break;
                    case Checksum("current_goal"):
                    case Checksum("current_goal_key_combo_text"):
                    case Checksum("goal_points_text"):
                        result.anchor = 1;
                        break;
                }
            }
            element = *reinterpret_cast<uintptr_t*>(element + 76);
        }
        if (split && !result.panel)
            return {};
        return result;
    }

    inline float Offset(const Placement& placement)
    {
        if (!NativeScaleX.is_initialized() || !std::isfinite(NativeScaleX.get()) || NativeScaleX <= 0.0f)
            return 0.0f;
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        // The 640x480 frame: full height on wider screens, full width (letterboxed) on narrower ones
        const float frameWidth = std::min(Width, Height * (4.0f / 3.0f));
        if (Split())
        {
            const bool vertical = HudLayout.get() == 1;
            const float viewportWidth = vertical ? Width * 0.5f : Width;
            const float viewportHeight = vertical ? Height : Height * 0.5f;
            const float boxWidth = aspect > 0.0f ? std::min(viewportWidth, viewportHeight * aspect) : viewportWidth;
            const float left = vertical ? placement.player * viewportWidth : 0.0f;
            const float edge = placement.anchor == 1 ? 1.0f : placement.anchor == 2 ? 0.5f : 0.0f;
            const float target = left + (viewportWidth - boxWidth) * 0.5f + edge * boxWidth;
            const float panelX = *reinterpret_cast<float*>(placement.panel + 296);
            if (!std::isfinite(panelX))
                return 0.0f;
            const float native = (Width - frameWidth) * 0.5f + panelX * NativeScaleX.get() +
                edge * frameWidth * (vertical ? 0.5f : 1.0f);
            return (target - native) / NativeScaleX.get();
        }
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        return placement.anchor * std::max(0.0f, boxWidth - frameWidth) * 0.5f / NativeScaleX.get();
    }

    inline int Draw(uintptr_t element, bool sprite)
    {
        // Text prompts look up their element for the controller icons
        if (!sprite)
            TonyPad::Prompts::SetTextContext(element);
        auto& original = sprite ? SpriteHook : TextHook;
        const auto placement = Classify(element);
        if (!placement.anchor)
            return original.thiscall<int>(element);
        // The native draw hides an element whose frame x is far outside the 640 frame. Parked edge elements (the goal
        // points panel slides out to the left) were outside the 4:3 screen, so they stay hidden beside a narrower box.
        auto* cull = reinterpret_cast<float*>(element + 312);
        const float savedCull = *cull;
        const bool parked = !Split() && ((placement.anchor < 0 && savedCull < 0.0f) || (placement.anchor == 1 && savedCull > 640.0f));
        if (placement.anchor == Hidden || parked)
        {
            *cull = -1000.0f;
            const auto result = original.thiscall<int>(element);
            *cull = savedCull;
            return result;
        }
        auto* x = reinterpret_cast<float*>(element + (sprite ? 296 : 308));
        const float savedX = *x;
        *x = savedX + Offset(placement);
        auto* flags = reinterpret_cast<uint32_t*>(element + 8);
        const uint32_t savedFlags = *flags;
        if (sprite)
            *flags = savedFlags | 0x800u;
        const auto result = original.thiscall<int>(element);
        *x = savedX;
        if (sprite)
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

    inline void Install(float width, float height, const std::string* constraint, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        Constraint = constraint;
        auto text = hook::module_pattern(module, "83 EC 28 56 8B F1 8B 86 78 01 00 00 85 C0 0F 84 ? ? ? ? D9 86 38 01 00 00");
        auto sprite = hook::module_pattern(module, "83 EC 0C 53 56 8B F1 D9 86 38 01 00 00 32 DB D8 1D ? ? ? ? DF E0 F6 C4 05");
        auto layout = hook::module_pattern(module, "8B 44 24 08 8B 48 14 8B 15 ? ? ? ? 33 C0 83 FA 05 77 ? FF 24 95");
        auto scale = hook::module_pattern(module, "DB 05 ? ? ? ? 57 D9 05 ? ? ? ? 8B F9");
        NativeScaleX.SetAddress(*scale.get_first<float*>(9));
        HudLayout.SetAddress(*layout.get_first<int*>(9));
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
        SpriteHook = safetyhook::create_inline(sprite.get_first(), DrawSprite);
    }
}

export void InstallHud()
{
    hook::pattern pattern("");
    //HUD
    THUG2Hud::Install(Screen.fWidth, Screen.fHeight, &FixSettings().String("MAIN.HudAspectRatioConstraint"));
    static int32_t nHudOffset = static_cast<int32_t>(Screen.fHudOffsetReal);
    struct SetOffsetHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_787D88 = nHudOffset;
        }
    };

    pattern = hook::pattern("D8 0D ? ? ? ? 89 0D ? ? ? ? 89 15 ? ? ? ? 0F B6 C8");
    injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHUDScaleX, true);
    dword_787D88.SetAddress(*hook::get_pattern<int32_t*>("DB 05 ? ? ? ? 57 D9 05 ? ? ? ? 8B F9", 2));

    auto firstOffset = hook::pattern("89 1D ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 1D ? ? ? ?").get_first<uint8_t>();
    auto secondOffset = hook::pattern("89 1D ? ? ? ? D9 1D ? ? ? ?").get_first<uint8_t>();
    injector::MakeInline<SetOffsetHook>(firstOffset, firstOffset + 6);
    injector::MakeInline<SetOffsetHook>(secondOffset, secondOffset + 6);

    // 3D models in menus (the pause sign) are placed from their 2D frame position, spread by the screen aspect,
    // which pushed them out of the 4:3 frame on wide screens; they keep the 4:3 spread
    pattern = hook::pattern("D9 5C 24 74 E8 ? ? ? ? D8 0D ? ? ? ? A1 ? ? ? ? 3D 80 02 00 00");
    injector::MakeCALL(pattern.get_first(4), static_cast<float(__cdecl*)()>([]() -> float { return 4.0f / 3.0f; }), true);
}
