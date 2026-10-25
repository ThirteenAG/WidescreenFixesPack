#pragma once
#include "../Shared/Console/Viewport.hpp"

namespace THAWHud
{
    inline SafetyHookInline TextHook, SpriteHook;
    inline float Width = 640.0f, Height = 480.0f;
    inline const float* NativeScaleX = nullptr;
    inline const float* TextQuantum = nullptr;
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
                switch (*reinterpret_cast<uint32_t*>(element + 24))
                {
                    case Checksum("the_score"):
                    case Checksum("the_score_sprite"):
                        anchor = -1;
                        break;
                    case Checksum("current_goal"):
                    case Checksum("goal_points_text"):
                        anchor = 1;
                        break;
                }
            }
            element = *reinterpret_cast<uintptr_t*>(element + 96);
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
        if (!sprite)
        {
            if (!TextQuantum || !std::isfinite(*TextQuantum) || *TextQuantum <= 0.0f)
                return original.thiscall<int>(element);
            auto* x = reinterpret_cast<int16_t*>(element + 320);
            const int16_t savedX = *x;
            const double requested = std::round(savedX + static_cast<double>(Offset(anchor)) / *TextQuantum);
            *x = static_cast<int16_t>(std::clamp(requested, -32768.0, 32767.0));
            const auto result = original.thiscall<int>(element);
            *x = savedX;
            return result;
        }
        auto* x = reinterpret_cast<float*>(element + 308);
        const float savedX = *x;
        *x = savedX + Offset(anchor);
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

    inline void Install(float width, float height, const std::string* constraint, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        Constraint = constraint;
        auto text = hook::module_pattern(module, "83 EC 28 56 8B F1 8B 86 84 01 00 00 85 C0 0F 84 ? ? ? ? 0F BF 86 42 01 00 00");
        auto sprite = hook::module_pattern(module, "F3 0F 10 0D ? ? ? ? 83 EC 0C 53 56 8B F1 0F BF 86 42 01 00 00 F3 0F 2A C0 F3 0F 59 05");
        auto layout = hook::module_pattern(module, "8B 44 24 08 8B 48 1C 8B 15 ? ? ? ? 33 C0 83 FA 05 77 ? FF 24 95");
        auto scale = hook::module_pattern(module, "D8 0D ? ? ? ? 8B 35 ? ? ? ? 85 F6 D9 1D");
        if (text.size() != 1 || sprite.size() != 1 || layout.size() != 1 || scale.size() != 1)
        {
            OutputDebugStringA("THAW: HUD anchoring patterns unavailable.\n");
            return;
        }
        NativeScaleX = *scale.get_first<const float*>(16);
        TextQuantum = *sprite.get_first<const float*>(30);
        Layout = *layout.get_first<const int*>(9);
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
        SpriteHook = safetyhook::create_inline(sprite.get_first(), DrawSprite);
        if (!TextHook || !SpriteHook)
        {
            TextHook.reset();
            SpriteHook.reset();
            OutputDebugStringA("THAW: HUD anchoring hooks unavailable.\n");
        }
    }
}
