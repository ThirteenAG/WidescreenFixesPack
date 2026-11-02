module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <charconv>

// The HUD is drawn in pixels, anchored to the screen edges inside the game's margins (left, top, right, bottom).
// MAIN.HudAspectRatioConstraint Auto keeps it at the screen edges; a ratio widens the left and right margins so it
// sits in a centred box of that shape. Hints and messages placed at a fixed small x (the left edge) move with the
// left side of the box.

export module Hud;

import ComVars;


export namespace SecondSightHud
{
    inline std::atomic<float> Constraint{ 0.0f };
    inline int32_t Applied = 0;
    inline SafetyHookMid Hooks[3];
    inline SafetyHookMid TextHooks[2];
    constexpr int32_t LeftText = 40;

    inline float Number(std::string_view text)
    {
        while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
        while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
        float value = 0.0f;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (text.empty() || parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || !std::isfinite(value) || value <= 0.0f)
            return 0.0f;
        return value;
    }

    // "Auto" (or anything unreadable) = 0, the whole screen; "W:H" or a decimal, clamped to 4:3 to 32:9
    inline float ParseAspect(std::string_view text)
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

    inline int32_t Offset()
    {
        const float ratio = Constraint.load(std::memory_order_relaxed);
        if (ratio <= 0.0f || !RenderWidth.is_initialized() || !RenderHeight.is_initialized())
            return 0;
        const float box = float(RenderHeight.get()) * ratio;
        return box < float(RenderWidth.get()) ? int32_t((float(RenderWidth.get()) - box) * 0.5f) : 0;
    }

    // The game has just written its margins: they get the box's offset at the sides
    inline void Written(SafetyHookContext&)
    {
        Applied = Offset();
        HudLeftMargin += Applied;
        HudRightMargin += Applied;
    }

    // Once per frame: follows a changed setting or resolution
    inline void Update()
    {
        if (!HudLeftMargin.is_initialized())
            return;
        const int32_t offset = Offset();
        if (offset == Applied)
            return;
        HudLeftMargin += offset - Applied;
        HudRightMargin += offset - Applied;
        Applied = offset;
    }

    // Text at a position passed by pointer (the pen, advanced by the call), or at a position passed by value
    inline void TextAtPointer(SafetyHookContext& context)
    {
        auto x = *reinterpret_cast<float**>(context.esp + 4);
        if (Applied && x && *x >= 0.0f && *x < float(LeftText))
            *x += float(Applied);
    }

    inline void TextAtValue(SafetyHookContext& context)
    {
        auto& x = *reinterpret_cast<float*>(context.esp + 4);
        if (Applied && x >= 0.0f && x < float(LeftText))
            x += float(Applied);
    }

    inline void Init()
    {
        // The three places that set the margins: a fixed 20 pixels, the same for some levels, and a computed layout
        auto fixed = hook::pattern("C7 05 ? ? ? ? 14 00 00 00 A3 ? ? ? ? C7 05 ? ? ? ? 14 00 00 00 A3 ? ? ? ? C3");
        auto level = hook::pattern("B8 14 00 00 00 A3 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? A3");
        auto computed = hook::pattern("83 C1 14 89 0D ? ? ? ? E8");
        HudLeftMargin.SetAddress(*fixed.get_first<int32_t*>(2));
        HudRightMargin.SetAddress(HudLeftMargin.get_ptr() + 2);
        const auto refresh = []() { Constraint = ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint")); };
        refresh();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", refresh);
        Hooks[0] = safetyhook::create_mid(fixed.get_first(25), &Written);
        Hooks[1] = safetyhook::create_mid(level.get_first(20), &Written);
        Hooks[2] = safetyhook::create_mid(computed.get_first(9), &Written);

        auto textPointer = hook::pattern("83 EC 20 53 56 57 6A 00 E8 ? ? ? ? 8B 54 24 34 8B 5C 24 38 68");
        auto textValue = hook::pattern("8B 44 24 14 8B 4C 24 10 8B 54 24 0C 50 8B 44 24 0C 51 8B 4C 24 0C 52 6A");
        TextHooks[0] = safetyhook::create_mid(textPointer.get_first(), &TextAtPointer);
        TextHooks[1] = safetyhook::create_mid(textValue.get_first(), &TextAtValue);
    }
}
