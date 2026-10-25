#pragma once
#include "../Shared/Console/Viewport.hpp"

namespace THUGHud
{
    inline SafetyHookInline TextHook, SpriteHook;
    inline float Width = 640.0f, Height = 480.0f;
    inline const float* NativeScaleX = nullptr;
    inline const int* Layout = nullptr;
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
        if (!Layout || *Layout != 0)
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
                        anchor = -1;
                        break;
                    case Checksum("current_goal"):
                    case Checksum("current_goal_key_combo_text"):
                    case Checksum("goal_points_text"):
                    case Checksum("cash_goal_sprite"):
                    case Checksum("cash_text"):
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
        if (!NativeScaleX || !std::isfinite(*NativeScaleX) || *NativeScaleX <= 0.0f)
            return 0.0f;
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        return anchor * (boxWidth - Height * (4.0f / 3.0f)) * 0.5f / *NativeScaleX;
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
        if (text.size() != 1 || sprite.size() != 1 || layout.size() != 1 || scale.size() != 1)
        {
            OutputDebugStringA("THUG: HUD anchoring patterns unavailable.\n");
            return;
        }
        NativeScaleX = *scale.get_first<const float*>(14);
        Layout = *layout.get_first<const int*>(9);
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
        SpriteHook = safetyhook::create_inline(sprite.get_first(), DrawSprite);
        if (!TextHook || !SpriteHook)
        {
            TextHook.reset();
            SpriteHook.reset();
            OutputDebugStringA("THUG: HUD anchoring hooks unavailable.\n");
        }
    }
}
