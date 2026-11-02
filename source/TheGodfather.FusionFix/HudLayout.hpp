#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <string_view>

namespace GodfatherHud
{
    enum class Anchor
    {
        Left = -1,
        Center = 0,
        Right = 1
    };

    inline std::optional<Anchor> NamedAnchor(std::string_view name)
    {
        if (name == "hudcontrol_playerhealth" || name == "hudcontrol_weapons" ||
            name == "respect_Text" || name == "cash_Text" || name == "Respect_LevelUp_Text")
            return Anchor::Left;
        if (name == "hudcontrol_minimap" || name == "hudcontrol_targethealth")
            return Anchor::Right;
        if (name == "gfhud_root" || name == "gfhud_pause_root" || name == "normal_HUD" ||
            name == "notebook" || name == "bank_heist_Container" || name == "subtitlesBox" ||
            name == "igc_subtitlesBox")
            return Anchor::Center;
        return std::nullopt;
    }

    inline float Offset(float width, float height, float constraint, Anchor anchor)
    {
        constexpr float original = 4.0f / 3.0f;
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
            return 0.0f;
        const auto aspect = width / height;
        const auto requested = std::isfinite(constraint) && constraint > 0.0f
            ? std::clamp(constraint, original, 32.0f / 9.0f) : aspect;
        return float(int(anchor)) * (std::min(aspect, requested) - original) * height * 0.5f;
    }
}
