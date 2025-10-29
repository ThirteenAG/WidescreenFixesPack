module;

#include <stdafx.h>

export module WidescreenHUD;

import ComVars;

namespace HudMatchers
{
    inline bool IsHealthBar(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 598, 616) && FloatInRange(n_offsetX2, 598, 616) &&
            FloatInRange(n_offsetY1, 38, 233) && FloatInRange(n_offsetY2, 38, 233);
    }
    
    inline bool IsHudIcons(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 519, 616) && FloatInRange(n_offsetX2, 519, 616) &&
            FloatEqual(n_offsetY1, 304) && (FloatEqual(n_offsetY2, 331) || FloatEqual(n_offsetY2, 332)) &&
            Color.RGBA == 0xFE808080;
    }

    inline bool IsInteractionBox(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        auto EPlayerControllerState = UObject::GetState(L"EPlayerController");
        if (EPlayerControllerState == L"s_Turret")
            return false;


        return FloatInRange(n_offsetX1, 373, 599) && FloatInRange(n_offsetX2, 373, 599) &&
            FloatInRange(n_offsetY1, 38, 138) && FloatInRange(n_offsetY2, 38, 138) &&
            (Color.RGBA == 0xFE808080 || Color.RGBA == 0xC8000000 ||
            Color.RGBA == 0xFE5C6D4C || Color.RGBA == 0xFE313828) &&
            !(Color.RGBA == 0xFE808080 && FloatInRange(n_offsetY1, 130, 137) && FloatInRange(n_offsetY2, 130, 137));
    }

    inline bool IsStealthBarWeaponHud(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 493, 616) && FloatInRange(n_offsetX2, 493, 616) &&
            FloatInRange(n_offsetY1, 333, 410) && FloatInRange(n_offsetY2, 333, 410) &&
            (Color.RGBA == 0xFE333333 || Color.RGBA == 0x4C808080 || Color.RGBA == 0xFE808080 ||
            Color.RGBA == 0xFE404040 || Color.RGBA == 0xC0FEFEFE || Color.RGBA == 0x4C000000 ||
            (Color.RGBA == 0x98000000 && n_offsetX1 != 573) || Color.RGBA == 0x98808080 ||
            (Color.RGBA == 0xFEFEFEFE && (FloatEqual(n_offsetX1, 524) || FloatEqual(n_offsetX1, 525) ||
            FloatEqual(n_offsetX1, 498) || FloatEqual(n_offsetX1, 499))));
    }

    inline bool IsFireModeSwitch(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 519, 616) && FloatInRange(n_offsetX2, 519, 616) &&
            FloatInRange(n_offsetY1, 411, 440) && FloatInRange(n_offsetY2, 411, 440) &&
            Color.RGBA != 0xFE333333;
    }

    inline bool IsCameraJammerView(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        auto EPlayerControllerState = UObject::GetState(L"EPlayerController");
        if (EPlayerControllerState == L"s_Turret")
            return false;

        return FloatInRange(n_offsetX1, 457, 599) && FloatInRange(n_offsetX2, 457, 599) &&
            FloatInRange(n_offsetY1, 39, 114) && FloatInRange(n_offsetY2, 39, 114);
    }

    inline bool IsInventoryFix(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 178, 184) && FloatInRange(n_offsetX2, 607, 612);
    }

    inline bool IsTopDialogueMenuBackground(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 25, 35) && FloatInRange(n_offsetX2, 350, 370) &&
            FloatInRange(n_offsetY1, 35, 165) && FloatInRange(n_offsetY2, 35, 165) &&
            (Color.RGBA == 0x4C000000 || Color.RGBA == 0xFE404040 || Color.RGBA == 0xFEFEFEFE);
    }

    inline bool IsTopDialogueMenuBorder(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return ((FloatInRange(n_offsetX1, 20, 40)) || (FloatInRange(n_offsetX1, 345, 365))) &&
            ((FloatInRange(n_offsetX2, 25, 45)) || (FloatInRange(n_offsetX2, 345, 375))) &&
            FloatInRange(n_offsetY1, 35, 165) && FloatInRange(n_offsetY2, 35, 165);
    }

    inline bool IsTopDialogueMenuIcon(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 29, 356) && FloatInRange(n_offsetX2, 29, 356) &&
            FloatInRange(n_offsetY1, 40, 160) && FloatInRange(n_offsetY2, 40, 160) &&
            Color.RGBA == 0xFE404040;
    }

    inline bool IsTopDialogueMenuText(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 25, 316) && FloatInRange(n_offsetY1, 40, 270) &&
            Color.RGBA == 0xfe333333;
    }

    inline bool ShouldOffsetRight(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return IsHealthBar(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsHudIcons(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsInteractionBox(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsStealthBarWeaponHud(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsFireModeSwitch(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsCameraJammerView(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);
    }

    inline bool ShouldOffsetLeft(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return IsTopDialogueMenuBackground(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuBorder(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuIcon(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTopDialogueMenuText(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTimer(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);
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

    if (HudMatchers::IsInventoryFix(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color))
    {
        offsetX2 += Screen.fWidescreenHudOffset;
    }
}