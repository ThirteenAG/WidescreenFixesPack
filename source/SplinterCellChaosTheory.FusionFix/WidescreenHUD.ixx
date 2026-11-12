module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module WidescreenHUD;

import ComVars;
import WidescreenHUD2;

export void RefreshHudConstraint()
{
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.fWidth <= 0.0f || Screen.fHeight <= 0.0f ||
        !std::isfinite(Screen.fWidth) || !std::isfinite(Screen.fHeight) || !std::isfinite(Screen.fAspectRatio))
        return;

    static std::string previous;
    static int previousWidth = 0, previousHeight = 0;
    const auto wanted = FixSettings().Wanted("MAIN.HudAspectRatioConstraint");
    if (wanted == previous && Screen.Width == previousWidth && Screen.Height == previousHeight)
        return;

    // Only the game thread publishes values consumed by the HUD draw hooks.
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(wanted);
    const float height = Screen.fHeight;
    float width = Screen.fWidth;
    const auto constraint = Screen.fHudAspectRatioConstraint;
    if (constraint && (*constraint < 0.0f || *constraint > 32.0f / 9.0f))
        Screen.fWidescreenHudOffset = *constraint;
    else
    {
        if (constraint)
            width = height * ClampHudAspectRatio(*constraint, Screen.fAspectRatio);
        Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(width, height, 640.0f, 480.0f);
    }
    RefreshModernHudPositions(Screen.fWidescreenHudOffset);
    previous = wanted;
    previousWidth = Screen.Width;
    previousHeight = Screen.Height;
}

namespace HudImageMatchers
{
    inline bool IsScopeImageLeft(int16_t left, int16_t right)
    {
        return (left == -1 && right == 256) ||
            (left == -2 && right == 255) ||
            (left == -61 && right == 319) ||
            (left == -60 && right == 320);
    }

    inline bool IsScopeImageRight(int16_t left, int16_t right)
    {
        return ((left == 382 || left == 383 || left == 384) && (right == 639 || right == 640 || right == 641)) ||
            ((left == 319 && right == 699) || (left == 320 && right == 700));
    }

    // Night/thermal vision brackets in third person, drawn at their 4:3 place (first person views move them with the goggles)
    inline bool IsGogglesBracketLeft(int16_t left, int16_t right, int16_t top, int16_t bottom)
    {
        return left >= 30 && right <= 47 && top >= 205 && bottom <= 275;
    }

    inline bool IsGogglesBracketRight(int16_t left, int16_t right, int16_t top, int16_t bottom)
    {
        return left >= 590 && right <= 608 && top >= 205 && bottom <= 275;
    }

    inline bool IsWeaponScopeLeft(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        return (left == -1 || left == 579) && (right == 62 || right == 649) && top == -3 && bottom == 483 && color.RGBA == 0x1bffffff && left == -1;
    }

    inline bool IsWeaponScopeRight(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        return (left == -1 || left == 579) && (right == 62 || right == 649) && top == -3 && bottom == 483 && color.RGBA == 0x1bffffff && left == 579;
    }
}

export void WidescreenHudImage(int16_t& left, int16_t& right, int16_t& top, int16_t& bottom, FColor color, std::wstring_view textureName)
{
    using namespace HudImageMatchers;

    if (textureName == L"Texture hud_PC.mgb.GOGGLES_BORDER")
    {
        if (IsScopeImageLeft(left, right))
        {
            left -= Screen.nHudOffsetReal;
            right -= Screen.nHudOffsetReal;
        }
        else if (IsScopeImageRight(left, right))
        {
            left += Screen.nHudOffsetReal + 1;
            right += Screen.nHudOffsetReal + 1;
        }
    }
    else if (textureName == L"Texture system_PC.mgb.Green")
    {
        if (IsWeaponScopeLeft(left, right, top, bottom, color))
        {
            left -= Screen.nHudOffsetReal;
        }
        else if (IsWeaponScopeRight(left, right, top, bottom, color))
        {
            right += Screen.nHudOffsetReal;
        }
        else if (Screen.fWidescreenHudOffset > 0.0f && IsGogglesBracketLeft(left, right, top, bottom))
        {
            const auto shift = static_cast<int16_t>(Screen.fWidescreenHudOffset + 15.0f);
            left -= shift;
            right -= shift;
        }
        else if (Screen.fWidescreenHudOffset > 0.0f && IsGogglesBracketRight(left, right, top, bottom))
        {
            const auto shift = static_cast<int16_t>(Screen.fWidescreenHudOffset + 20.0f);
            left += shift;
            right += shift;
        }
    }
}
