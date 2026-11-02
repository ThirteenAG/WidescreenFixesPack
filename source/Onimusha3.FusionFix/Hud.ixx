module;
#include <stdafx.h>
#include <charconv>
#include "FixSettings.hpp"

export module Hud;
import ComVars;


export namespace OnimushaHud
{
    inline std::atomic<float> Constraint{ 0.0f };
    inline std::function<float()> Aspect;
    inline SafetyHookInline DrawHook, SubmitHook;
    inline thread_local bool Gameplay = false;
    inline thread_local float Offset = 0.0f;

    inline float Number(std::string_view text)
    {
        while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
        while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
        float value = 0.0f;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        return !text.empty() && parsed.ec == std::errc() && parsed.ptr == text.data() + text.size() &&
            std::isfinite(value) && value > 0.0f ? value : 0.0f;
    }

    inline float ParseAspect(std::string_view text)
    {
        const auto colon = text.find(':');
        float value = Number(colon == std::string_view::npos ? text : text.substr(0, colon));
        if (colon != std::string_view::npos)
        {
            const auto height = Number(text.substr(colon + 1));
            value = height > 0.0f ? value / height : 0.0f;
        }
        return value > 0.0f ? std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
    }

    inline float Margin(float aspect, float constraint)
    {
        const auto box = constraint > 0.0f ? std::min(aspect, constraint) : aspect;
        return 256.0f * (box / (4.0f / 3.0f) - 1.0f);
    }

    inline void Move(uint32_t& position, float offset)
    {
        const int x = int(position & 0xFFFF) + int(std::lround(offset * 16.0f));
        position = (position & 0xFFFF0000) | uint16_t(x);
    }

    inline void Place(uint32_t* command, float offset)
    {
        // Translate a complete native group, including fills and caps that extend across the screen centre.
        const auto format = command[1] >> 24;
        if (format == 0x20)
        {
            Move(command[8], offset);
        }
        else if (format == 0x14)
        {
            Move(command[4], offset);
            Move(command[6], offset);
        }
    }

    inline uint32_t __fastcall Submit(void* owner, void*, uint32_t* command, int list)
    {
        if (Gameplay)
            Place(command, -Offset);
        return SubmitHook.unsafe_thiscall<uint32_t>(owner, command, list);
    }

    inline uint32_t __fastcall RightSubmit(void* owner, void*, uint32_t* command, int list)
    {
        if (Gameplay)
            Place(command, Offset);
        return SubmitHook.unsafe_thiscall<uint32_t>(owner, command, list);
    }

    inline uint16_t __fastcall Draw(void* hud, void*, char mode)
    {
        const auto previous = Gameplay;
        const auto previousOffset = Offset;
        Gameplay = mode == 0;
        Offset = Margin(Aspect(), Constraint.load(std::memory_order_relaxed));
        const auto result = DrawHook.unsafe_thiscall<uint16_t>(hud, mode);
        Offset = previousOffset;
        Gameplay = previous;
        return result;
    }

    inline void Init(std::function<float()> aspect)
    {
        Aspect = std::move(aspect);
        const auto update = []()
        {
            Constraint = ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        };
        update();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", update);
        auto draw = hook::pattern("A1 ? ? ? ? A9 00 00 00 02 57 8B F9 0F 85 ? ? ? ? 53 33 DB A9 00 80 00 00");
        auto submit = hook::pattern("8B 41 08 8B 4C 24 08 8B 54 24 04 8D 0C 88 8B 01 0D 00 00 00 80 89 11");
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        SubmitHook = safetyhook::create_inline(submit.get_first(), Submit);
        // The companion gauge's fill, trailing fill and frame have their own native submissions.
        injector::MakeCALL(hook::pattern("8A 47 15 6A 05 56 B9 ? ? ? ? 88 46 0F E8 ? ? ? ? 8B 54 24 18 8B 4C 24 14").get_first(14), RightSubmit, true);
        injector::MakeCALL(hook::pattern("B9 ? ? ? ? 89 56 18 C7 46 1C FA FF 00 00 C7 46 04 46 20 1B 14 88 46 0F E8 ? ? ? ? 83 C6 20 F7 05").get_first(25), RightSubmit, true);
        injector::MakeCALL(hook::pattern("C7 46 04 56 21 2B 20 8A 47 15 56 B9 ? ? ? ? 88 46 0F E8 ? ? ? ? 8B 43 04 83 C3 04").get_first(19), RightSubmit, true);
    }
}
