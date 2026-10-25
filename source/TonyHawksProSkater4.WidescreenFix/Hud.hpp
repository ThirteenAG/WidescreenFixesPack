#pragma once
#include "../Shared/Console/Viewport.hpp"
#include <bit>

namespace THPS4Hud
{
    inline std::array<SafetyHookMid, 4> ScaleHooks;
    inline SafetyHookInline TextHook, SpriteHook;
    inline float Width = 640.0f, Height = 480.0f, Scale = 1.0f;
    inline const std::string* Constraint = nullptr;
    inline const int* Layout = nullptr;
    inline int Anchor = 0;

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
        for (int depth = 0; element && depth < 32; ++depth)
        {
            if (*reinterpret_cast<uint32_t*>(element + 8) & 0x80000u)
                return 0;
            switch (*reinterpret_cast<uint32_t*>(element + 20))
            {
                case Checksum("the_score"):
                case Checksum("the_score_sprite"):
                case Checksum("the_special_bar_special"):
                case Checksum("the_special_bar_sprite"):
                case Checksum("the_boardstance_sprite"):
                case Checksum("minigame_timer"):
                    return -1;
                case Checksum("current_goal"):
                case Checksum("cash_goal_sprite"):
                case Checksum("cash_text"):
                case Checksum("goal_points_text"):
                    return 1;
            }
            element = *reinterpret_cast<uintptr_t*>(element + 64);
        }
        return 0;
    }

    inline float Position(float x)
    {
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        // Native X conversion includes the original 32-pixel canvas margins.
        const float offset = Anchor * (boxWidth - Height * (4.0f / 3.0f)) * 0.5f;
        return x * Scale + offset * (704.0f / Width);
    }

    inline void __fastcall DrawText(uintptr_t element, void*)
    {
        const auto previous = std::exchange(Anchor, Classify(element));
        TextHook.thiscall<void>(element);
        Anchor = previous;
    }

    inline int __fastcall DrawSprite(uintptr_t element, void*)
    {
        const auto previous = std::exchange(Anchor, Classify(element));
        const auto result = SpriteHook.thiscall<int>(element);
        Anchor = previous;
        return result;
    }

    inline bool Install(float width, float height, const std::string* constraint, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        Scale = Height * (4.0f / 3.0f) / Width;
        Constraint = constraint;
        const char* signatures[] =
        {
            "8B 44 24 04 8B 54 24 08 89 41 68 8B 01 89 51 6C FF 50 0C C2 08 00",
            "8B 44 24 04 8B 54 24 08 89 41 70 8B 01 89 51 74 FF 50 0C C2 08 00",
            "8B 44 24 04 8B 54 24 08 89 41 08 8B 01 89 51 0C FF 50 0C C2 08 00",
            "8B 44 24 04 8B 54 24 08 89 41 10 8B 01 89 51 14 FF 50 0C C2 08 00"
        };
        std::array<void*, 4> sites{};
        for (size_t i = 0; i < sites.size(); ++i)
        {
            auto pattern = hook::module_pattern(module, signatures[i]);
            if (pattern.size() != 1)
            {
                OutputDebugStringA("THPS4: text/sprite scaling pattern unavailable.\n");
                return false;
            }
            sites[i] = pattern.get_first(8);
        }
        for (size_t i = 0; i < sites.size(); ++i)
        {
            if (i == 0 || i == 2)
                ScaleHooks[i] = safetyhook::create_mid(sites[i], [](SafetyHookContext& regs)
                {
                    regs.eax = std::bit_cast<uint32_t>(Position(std::bit_cast<float>(regs.eax)));
                });
            else
                ScaleHooks[i] = safetyhook::create_mid(sites[i], [](SafetyHookContext& regs)
                {
                    regs.eax = std::bit_cast<uint32_t>(std::bit_cast<float>(regs.eax) * Scale);
                });
            if (!ScaleHooks[i])
            {
                for (auto& item : ScaleHooks)
                    item.reset();
                OutputDebugStringA("THPS4: text/sprite scaling hooks unavailable.\n");
                return false;
            }
        }
        auto text = hook::module_pattern(module, "83 EC 30 53 55 56 8B F1 33 DB 57 39 9E 44 01 00 00");
        auto sprite = hook::module_pattern(module, "83 EC 0C 56 8B F1 C6 44 24 08 00 D9 86 14 01 00 00");
        auto layout = hook::module_pattern(module, "8B F9 E8 ? ? ? ? 48 6A 00 74 ? 48 74 ? 68 ? ? ? ? EB ? 68 ? ? ? ? EB ? 68 ? ? ? ? E8");
        if (text.size() == 1 && sprite.size() == 1 && layout.size() == 1)
        {
            auto getter = injector::GetBranchDestination(layout.get_first(2)).as_int();
            if (*reinterpret_cast<uint8_t*>(getter) == 0xA1 && *reinterpret_cast<uint8_t*>(getter + 5) == 0xC3)
            {
                Layout = *reinterpret_cast<const int**>(getter + 1);
                TextHook = safetyhook::create_inline(text.get_first(), DrawText);
                SpriteHook = safetyhook::create_inline(sprite.get_first(), DrawSprite);
            }
        }
        if (!TextHook || !SpriteHook)
        {
            TextHook.reset();
            SpriteHook.reset();
            OutputDebugStringA("THPS4: HUD anchoring hooks unavailable; centered scaling retained.\n");
        }
        return true;
    }
}
