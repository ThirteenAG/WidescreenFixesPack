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
        return n_offsetX1 >= 519 && n_offsetX2 <= 616 && n_offsetY1 == 304 && (n_offsetY2 == 331 || n_offsetY2 == 332) && Color.RGBA == 0xFE808080;
    }

    inline bool IsStealthBarWeaponHud(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 493 && n_offsetX2 <= 616 && n_offsetY1 >= 333 && n_offsetY2 <= 410 &&
            (Color.RGBA == 0xFE333333 || Color.RGBA == 0x4C808080 || Color.RGBA == 0xFE808080 ||
            Color.RGBA == 0xFE404040 || Color.RGBA == 0xC0FEFEFE || Color.RGBA == 0x4C000000 ||
            (Color.RGBA == 0x98000000 && n_offsetX1 != 573) || Color.RGBA == 0x98808080 ||
            (Color.RGBA == 0xFEFEFEFE && (n_offsetX1 == 524 || n_offsetX1 == 525 || n_offsetX1 == 498 ||
            n_offsetX1 == 499)));
    }

    inline bool IsFireModeSwitch(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 519 && n_offsetX2 <= 616 && n_offsetY1 >= 411 && n_offsetY2 <= 440 && Color.RGBA != 0xFE333333;
    }

    inline bool IsInventoryFix(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return (n_offsetX1 >= 178 && n_offsetX1 <= 184) && (n_offsetX2 >= 607 && n_offsetX2 <= 612);
    }

    inline bool IsTopDialogueMenuBackground(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 25 && n_offsetX1 <= 35 && n_offsetX2 >= 350 && n_offsetX2 <= 370 && n_offsetY1 >= 35 && n_offsetY2 <= 165 &&
            (Color.RGBA == 0x4C000000 || Color.RGBA == 0xFE404040 || Color.RGBA == 0xFEFEFEFE || Color.RGBA == 0xFEFEFEFE);
    }

    inline bool IsTopDialogueMenuBorder(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return ((n_offsetX1 >= 20 && n_offsetX1 <= 40) || (n_offsetX1 >= 345 && n_offsetX1 <= 365)) &&
            ((n_offsetX2 >= 25 && n_offsetX2 <= 45) || (n_offsetX2 >= 345 && n_offsetX2 <= 375)) &&
            n_offsetY1 >= 35 && n_offsetY2 <= 165;
    }

    inline bool IsTopDialogueMenuIcon(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && Color.RGBA == 0xFE404040;
    }

    inline bool IsTopDialogueMenuText(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return n_offsetX1 >= 25 && n_offsetX1 <= 710 && n_offsetY1 >= 40 && n_offsetY1 <= 270 && Color.RGBA == 0xfe333333;
    }

    inline bool ShouldOffsetRight(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return IsHealthBar(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsHudIcons(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsStealthBarWeaponHud(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsFireModeSwitch(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);// ||
        //IsInventoryFix(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);
    }

    inline bool ShouldOffsetLeft(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return IsTopDialogueMenuBackground(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuBorder(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuIcon(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuText(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);
    }
}

bool isIngameText;

export void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
    uint32_t n_offsetX1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX1));
    uint32_t n_offsetX2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX2));
    uint32_t n_offsetY1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY1));
    uint32_t n_offsetY2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY2));

    DBGONLY(KEYPRESS(VK_F1)
    {
        spd::log()->info("{0:d} {1:d} {2:d} {3:d} {4:08x}", n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color.RGBA);
    });

    if (HudMatchers::ShouldOffsetRight(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color))
    {
        offsetX1 += Screen.fWidescreenHudOffset;
        offsetX2 += Screen.fWidescreenHudOffset;
    }
    else if (HudMatchers::ShouldOffsetLeft(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color))
    {
        offsetX1 -= Screen.fWidescreenHudOffset;
        offsetX2 -= Screen.fWidescreenHudOffset;
    }
    //else
    //{
    //	if (
    //	(n_offsetX1 == 29 && n_offsetX2 == 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 0xFE404040)) || //top dialogue menu background
    //	((n_offsetX1 == 23 || n_offsetX1 == 29 || n_offsetX1 == 32 || n_offsetX1 == 354 || n_offsetX1 == 356) && (n_offsetX2 == 29 || n_offsetX2 == 30 || n_offsetX2 == 32 || n_offsetX2 == 354 || n_offsetX2 == 356 || n_offsetX2 == 362) && n_offsetY1 >= 39 && n_offsetY2 <= 160) || //top dialogue menu background border
    //	(n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 0xFE404040)) //|| //top dialogue menu icon
    //	//(n_offsetX1 >= 29 && n_offsetX2 <= 356 && (n_offsetY2 - n_offsetY1 == 15))
    //	)
    //	{
    //		if (n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 0xFE404040))
    //			isIngameText = true;
    //		else
    //			isIngameText = false;
    //
    //		offsetX1 -= Screen.fWidescreenHudOffset;
    //		offsetX2 -= Screen.fWidescreenHudOffset;
    //	}
    //}

    //if (n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA != 0xFE404040) && isIngameText)
    //{
    //	offsetX1 -= Screen.fWidescreenHudOffset;
    //	offsetX2 -= Screen.fWidescreenHudOffset;
    //}

    if (HudMatchers::IsInventoryFix(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color))
    {
        offsetX2 += Screen.fWidescreenHudOffset;
    }
}