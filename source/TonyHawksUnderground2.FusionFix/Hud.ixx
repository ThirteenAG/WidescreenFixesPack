module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/Console/Viewport.hpp"

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

    constexpr int Hidden = 2;

    // -1 left edge, 1 right edge, 0 centred, Hidden for the cutscene bar sprites (the frame clears outside the picture)
    inline int Classify(uintptr_t element)
    {
        if (const auto parent = *reinterpret_cast<uintptr_t*>(element + 76);
            parent && *reinterpret_cast<uint32_t*>(parent + 20) == Checksum("letterbox_anchor"))
            return Hidden;
        if (!HudLayout.is_initialized() || HudLayout != 0)
            return 0;
        int anchor = 0;
        for (int depth = 0; element && depth < 32; ++depth)
        {
            if (*reinterpret_cast<uint32_t*>(element + 8) & 0x80000u)
                return 0;
            if (!anchor)
            {
                switch (*reinterpret_cast<uint32_t*>(element + 20))
                {
                    case Checksum("the_score"):
                    case Checksum("the_score_sprite"):
                    case Checksum("goal_points_display"):
                    case Checksum("pager_new_message_icon"):
                    case Checksum("console_message_vmenu"):
                    case Checksum("horse_score_menu"):
                        anchor = -1;
                        break;
                    case Checksum("current_goal"):
                    case Checksum("current_goal_key_combo_text"):
                    case Checksum("goal_points_text"):
                        anchor = 1;
                        break;
                }
            }
            element = *reinterpret_cast<uintptr_t*>(element + 76);
        }
        return anchor;
    }

    inline float Offset(int anchor)
    {
        if (!NativeScaleX.is_initialized() || !std::isfinite(NativeScaleX.get()) || NativeScaleX <= 0.0f)
            return 0.0f;
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        // The 640x480 frame: full height on wider screens, full width (letterboxed) on narrower ones
        const float frameWidth = std::min(Width, Height * (4.0f / 3.0f));
        return anchor * std::max(0.0f, boxWidth - frameWidth) * 0.5f / NativeScaleX.get();
    }

    inline int Draw(uintptr_t element, bool sprite)
    {
        auto& original = sprite ? SpriteHook : TextHook;
        const int anchor = Classify(element);
        if (!anchor)
            return original.thiscall<int>(element);
        // The native draw hides an element whose frame x is far outside the 640 frame. Parked edge elements (the goal
        // points panel slides out to the left) were outside the 4:3 screen, so they stay hidden beside a narrower box.
        auto* cull = reinterpret_cast<float*>(element + 312);
        const float savedCull = *cull;
        if (anchor == Hidden || (anchor < 0 && savedCull < 0.0f) || (anchor > 0 && savedCull > 640.0f))
        {
            *cull = -1000.0f;
            const auto result = original.thiscall<int>(element);
            *cull = savedCull;
            return result;
        }
        auto* x = reinterpret_cast<float*>(element + (sprite ? 296 : 308));
        const float savedX = *x;
        *x = savedX + Offset(anchor);
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
        if (!TextHook || !SpriteHook)
            std::terminate();
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
