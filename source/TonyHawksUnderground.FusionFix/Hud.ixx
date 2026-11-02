module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/Console/Viewport.hpp"

export module Hud;

import ComVars;

export namespace THUGHud
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

    inline int Classify(uintptr_t element)
    {
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
                    case Checksum("minigame_timer"):
                    case Checksum("console_message_vmenu"):
                        anchor = -1;
                        break;
                    case Checksum("current_goal"):
                    case Checksum("current_goal_key_combo_text"):
                    case Checksum("goal_points_text"):
                    case Checksum("cash_goal_sprite"):
                    case Checksum("cash_text"):
                    case Checksum("cig_burn"):
                    case Checksum("the_run_timer"):
                        anchor = 1;
                        break;
                }
            }
            element = *reinterpret_cast<uintptr_t*>(element + 64);
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
        return anchor * (boxWidth - frameWidth) * 0.5f / NativeScaleX.get();
    }

    inline int Draw(uintptr_t element, bool sprite)
    {
        auto& original = sprite ? SpriteHook : TextHook;
        const int anchor = Classify(element);
        if (!anchor)
            return original.thiscall<int>(element);
        auto* x = reinterpret_cast<float*>(element + (sprite ? 276 : 288));
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
        auto text = hook::module_pattern(module, "55 8B EC 83 E4 F8 83 EC 2C 53 56 8B F1 83 BE 5C 01 00 00 00 57 0F 84");
        auto sprite = hook::module_pattern(module, "83 EC 10 53 56 8B F1 D9 86 24 01 00 00 32 DB DC 1D ? ? ? ? DF E0 F6 C4 05");
        auto layout = hook::module_pattern(module, "8B 44 24 08 56 8B 70 14 A1 ? ? ? ? 83 F8 05 77 ? FF 24 85");
        auto scale = hook::module_pattern(module, "83 EC 38 57 8B F9 D9 47 1C 8B 47 14 D8 0D ? ? ? ? 0F BF 88 30 01 00 00");
        NativeScaleX.SetAddress(*scale.get_first<float*>(14));
        HudLayout.SetAddress(*layout.get_first<int*>(9));
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
        SpriteHook = safetyhook::create_inline(sprite.get_first(), DrawSprite);
    }
}

export void InstallHud()
{
    auto& settings = FixSettings();
    hook::pattern pattern("");
    //HUD
    THUGHud::Install(Screen.fWidth, Screen.fHeight, &settings.String("MAIN.HudAspectRatioConstraint"));
    static int32_t nHudOffset = static_cast<int32_t>(Screen.fHudOffsetReal);
    struct SetOffsetHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_72DFC4 = nHudOffset;
        }
    };

    Screen.fHUDScaleX = 1.0f / Screen.fHUDScaleX;
    pattern = hook::pattern("DC 0D ? ? ? ? DE FB D9");
    injector::WriteMemory<double>(*pattern.get_first<double*>(2), Screen.fHUDScaleX, true);
    dword_72DFC4.SetAddress(*hook::get_pattern<int32_t*>("DA 05 ? ? ? ? 89 4C 24 20 89 54 24 18", 2));

    auto firstOffset = hook::pattern("89 3D ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? 00").get_first<uint8_t>();
    auto secondOffset = hook::pattern("89 3D ? ? ? ? D9 1D ? ? ? ?").get_first<uint8_t>();
    injector::MakeInline<SetOffsetHook>(firstOffset, firstOffset + 6);
    injector::MakeInline<SetOffsetHook>(secondOffset, secondOffset + 6);
}
