module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <charconv>

export module Hud;


// The HUD is laid out in an interface rectangle (left, top, right, bottom as parts of the screen) that the game fits
// to a 16:9 design box. It covers the whole screen instead (MAIN.HudAspectRatioConstraint Auto), or a centred box
// of the chosen aspect.
export namespace CondemnedHud
{
    inline std::atomic<float> constraint{ 0.0f };
    inline SafetyHookInline rectangleHook;

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

    inline float* __stdcall Rectangle(float* rectangle, float width, float height)
    {
        rectangle[0] = rectangle[1] = 0.0f;
        rectangle[2] = rectangle[3] = 1.0f;
        const float box = constraint;
        if (!(width > 0.0f) || !(height > 0.0f) || box <= 0.0f)
            return rectangle;
        const float screen = width / height;
        if (box < screen)
        {
            rectangle[0] = (1.0f - box / screen) * 0.5f;
            rectangle[2] = 1.0f - rectangle[0];
        }
        else
        {
            rectangle[1] = (1.0f - screen / box) * 0.5f;
            rectangle[3] = 1.0f - rectangle[1];
        }
        return rectangle;
    }

    inline void Init(HMODULE gameClient)
    {
        auto pattern = hook::pattern(gameClient, "83 EC 08 8B 0D ? ? ? ? 85 C9 C7 44 24 04 00 00 10 3F C7 04 24 39 8E E3 3F");
        const auto refresh = []() { constraint = ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint")); };
        refresh();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", refresh);
        rectangleHook = safetyhook::create_inline(pattern.get_first(), Rectangle); //0x1006D5C0
    }
}
