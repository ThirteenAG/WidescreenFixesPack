module;

#include <stdafx.h>

export module WidescreenHUD;

import ComVars;

namespace HudMatchers
{
    inline bool IsHealthBar(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 598 && n_offsetX2 <= 616 && n_offsetY1 >= 38 && n_offsetY2 <= 233;
    }

    inline bool IsHudIcons(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 519 && n_offsetX2 <= 616 && n_offsetY1 == 304 && (n_offsetY2 == 331 || n_offsetY2 == 332) && Color.RGBA == 4269834368;
    }

    inline bool IsStealthBarWeaponHud(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 493 && n_offsetX2 <= 616 && n_offsetY1 >= 333 && n_offsetY2 <= 410 &&
            (Color.RGBA == 4264768307 || Color.RGBA == 1283489920 || Color.RGBA == 4269834368 || Color.RGBA == 4265623616 || Color.RGBA == 3237936894 || Color.RGBA == 1275068416 || (Color.RGBA == 2550136832 && n_offsetX1 != 573) || Color.RGBA == 2558558336 || (Color.RGBA == 4278124286 && (n_offsetX1 == 524 || n_offsetX1 == 525 || n_offsetX1 == 498 || n_offsetX1 == 499)));
    }

    inline bool IsFireModeSwitch(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 519 && n_offsetX2 <= 616 && n_offsetY1 >= 411 && n_offsetY2 <= 440 && Color.RGBA != 4264768307;
    }

    inline bool IsInventoryFix(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return (n_offsetX1 >= 178 && n_offsetX1 <= 184) && (n_offsetX2 >= 607 && n_offsetX2 <= 612);
    }

    inline bool IsTopDialogueMenuBackground(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 == 29 && n_offsetX2 == 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 1275068416 || Color.RGBA == 4265623616 || Color.RGBA == 4278124286);
    }

    inline bool IsTopDialogueMenuBorder(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return (n_offsetX1 == 23 || n_offsetX1 == 29 || n_offsetX1 == 32 || n_offsetX1 == 354 || n_offsetX1 == 356) &&
            (n_offsetX2 == 29 || n_offsetX2 == 30 || n_offsetX2 == 32 || n_offsetX2 == 354 || n_offsetX2 == 356 || n_offsetX2 == 362) &&
            n_offsetY1 >= 39 && n_offsetY2 <= 160;
    }

    inline bool IsTopDialogueMenuIcon(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && Color.RGBA == 4265623616;
    }

    inline bool ShouldOffsetRight(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return IsHealthBar(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsHudIcons(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsStealthBarWeaponHud(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsFireModeSwitch(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsInventoryFix(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);
    }

    inline bool ShouldOffsetLeft(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return IsTopDialogueMenuBackground(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuBorder(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuIcon(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);
    }
}

export void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
    uint32_t n_offsetX1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX1));
    uint32_t n_offsetX2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX2));
    uint32_t n_offsetY1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY1));
    uint32_t n_offsetY2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY2));

    DBGONLY(KEYPRESS(VK_F2)
    {
        spd::log()->info("{0:f} {1:f} {2:f} {3:f} {4:08x}", offsetX1, offsetX2, offsetY1, offsetY2, Color.RGBA);
    });

    if (HudMatchers::ShouldOffsetRight(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color))
    {
        offsetX1 += Screen.fWidescreenHudOffset;
        offsetX2 += Screen.fWidescreenHudOffset;
    }
    else if (HudMatchers::ShouldOffsetLeft(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color))
    {
        if (HudMatchers::IsTopDialogueMenuIcon(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color))
            isIngameText = true;
        else
            isIngameText = false;

        offsetX1 -= Screen.fWidescreenHudOffset;
        offsetX2 -= Screen.fWidescreenHudOffset;
    }

    if (n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA != 4265623616) && isIngameText)
    {
        offsetX1 -= Screen.fWidescreenHudOffset;
        offsetX2 -= Screen.fWidescreenHudOffset;
    }
}