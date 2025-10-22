module;

#include <stdafx.h>

export module WidescreenHUD;

import ComVars;

namespace HudMatchers
{
    inline bool IsStealthMeterLine(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX2 - offsetX1 == 17.0f) &&
            (offsetY1 == 337.0f || offsetY1 == 360.0f) &&
            (offsetY2 == 368.0f || offsetY2 == 345.0f);
    }

    inline bool IsStealthMeter(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX2 - offsetX1 == 109.0f) &&
            offsetY1 == 338.0f && offsetY2 == 368.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4271286934 || color.RGBA == 4283459416 || color.RGBA == 4288064150);
    }

    inline bool IsWeaponIconBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 491.0f && offsetX2 == 592.0f &&
            offsetY1 == 373.0f && offsetY2 == 441.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4283459416);
    }

    inline bool IsWeaponIconBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 487.0f || offsetX1 == 503.0f || offsetX1 == 580.0f) &&
            (offsetX2 == 503.0f || offsetX2 == 580.0f || offsetX2 == 596.0f) &&
            (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 429.0f) &&
            color.RGBA == 2152752984;
    }

    inline bool IsAlarmCounterText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 562.0f && offsetX2 <= 602.0f &&
            (offsetY1 == 316.0f || offsetY1 == 315.0f) &&
            offsetY2 == 330.0f &&
            (color.RGBA == 4265759816 || color.RGBA == 4283459416 || color.RGBA == 4282537289);
    }

    inline bool IsHealthBar(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 570.0f || offsetX1 == 586.0f) &&
            (offsetX2 == 586.0f || offsetX2 == 602.0f) &&
            color.RGBA == 3360712536;
    }

    inline bool IsEnvelopeIcon(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 492.0f && offsetX2 == 519.0f &&
            offsetY1 == 314.0f && offsetY2 == 333.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4283459416);
    }

    inline bool IsAlarmIcon(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 521.0f && offsetX2 == 562.0f &&
            offsetY1 == 306.0f && offsetY2 == 340.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 2152752984 || color.RGBA == 4283459416);
    }

    inline bool IsWeaponIcon(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 494.0f || offsetX1 == 515.0f) &&
            offsetY1 == 377.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4282537289 || color.RGBA == 4283459416);
    }

    inline bool IsWeaponNameText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 >= 491.0f || offsetX1 >= 490.0f) &&
            offsetY1 == 423.0f &&
            (offsetY2 == 437.0f || offsetY2 == 438.0f);
    }

    inline bool IsAmmoText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 571.0f &&
            offsetY1 == 373.0f &&
            (offsetY2 == 385.0f || offsetY2 == 388.0f) &&
            (color.RGBA == 4265759816 || color.RGBA == 4283459416 || color.RGBA == 4282537289);
    }

    inline bool IsBulletsImage(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 581.0f && (offsetX2 == 588.0f || offsetX2 == 592.0f)) ||
            (offsetX1 == 568.0f && offsetY1 == 406.0f);
    }

    inline bool IsRateOfFireImage(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 536.0f && offsetX2 <= 576.0f &&
            offsetY1 == 416.0f && offsetY2 == 421.0f;
    }

    inline bool IsWeaponAddonBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return ((offsetX1 == 441.0f && offsetX2 == 481.0f && offsetY1 == 373.0f && offsetY2 == 413.0f) ||
            (offsetX1 == 439.0f && offsetX2 == 483.0f && offsetY1 == 380.0f && offsetY2 == 412.0f)) &&
            (color.RGBA == 4266682200 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsWeaponAddonBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 437.0f || offsetX1 == 453.0f || offsetX1 == 469.0f) &&
            (offsetX2 == 453.0f || offsetX2 == 469.0f || offsetX2 == 485.0f) &&
            (offsetY1 == 369.0f || offsetY1 == 385.0f || offsetY1 == 401.0f) &&
            color.RGBA == 2152752984;
    }

    inline bool IsWeaponAddonAmmoText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 452.0f || offsetX1 == 456.0f || offsetX1 == 457.0f || offsetX1 == 460.0f) &&
            (offsetX2 == 460.0f || offsetX2 == 465.0f || offsetX2 == 464.0f || offsetX2 == 467.0f || offsetX2 == 469.0f) &&
            offsetY1 == 371.0f &&
            (offsetY2 == 383.0f || offsetY2 == 386.0f) &&
            (color.RGBA == 4265759816 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsInteractionMenuBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return ((offsetX1 == 362.0f) || (offsetX1 == 422.0f || offsetX1 == 443.0f)) &&
            offsetX2 == 562.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4269316740 || color.RGBA == 4283459416 || color.RGBA == 4282537289 || color.RGBA == 4286094213);
    }

    inline bool IsInteractionMenuBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return ((offsetX1 == 359.0f || offsetX1 == 375.0f || offsetX1 == 549.0f || offsetX1 == 554.0f ||
            offsetX1 == 440.0f || offsetX1 == 456.0f || offsetX1 == 537.0f ||
            offsetX1 == 419.0f || offsetX1 == 435.0f || offsetX1 == 439.0f) &&
            (offsetX2 == 375.0f || offsetX2 == 370.0f || offsetX2 == 549.0f || offsetX2 == 565.0f ||
            offsetX2 == 456.0f || offsetX2 == 451.0f || offsetX2 == 435.0f || offsetX2 == 430.0f) &&
            (color.RGBA == 2152752984 || color.RGBA == 4266682200 || color.RGBA == 4283459416));
    }

    inline bool IsInteractionMenuText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 365.0f &&
            offsetY1 >= 42.0f && offsetY2 <= 192.0f &&
            (color.RGBA == 4265759816 || color.RGBA == 4269316740 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsTurretInterface(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 371.0f || offsetX1 == 379.0f || offsetX1 == 387.0f ||
            offsetX1 == 558.0f || offsetX1 == 566.0f) &&
            offsetY1 >= 39.0f && offsetY2 <= 162.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 3036610302 ||
            color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213 || color.RGBA == 3036676095);
    }

    inline bool IsTopDialogueMenuBackground(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 == 48.0f && offsetX2 == 348.0f &&
            offsetY1 >= 42.0f &&
            (offsetY2 <= 87.0f || offsetY2 <= 90.0f) &&
            (color.RGBA == 4266682200 || color.RGBA == 3036610302 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsTopDialogueMenuBorder(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return (offsetX1 == 45.0f || offsetX1 == 61.0f || offsetX1 == 335.0f) &&
            (offsetX2 == 61.0f || offsetX2 == 335.0f || offsetX2 == 351.0f) &&
            (color.RGBA == 4266682200 || color.RGBA == 3036610302 || color.RGBA == 4282537289 || color.RGBA == 4283459416 || color.RGBA == 4286094213);
    }

    inline bool IsTopDialogueMenuText(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return offsetX1 >= 52.0f &&
            offsetY1 >= 43.0f && offsetY2 <= 185.0f &&
            (color.RGBA == 4265759816 || color.RGBA == 4283459416 || color.RGBA == 4282537289);
    }

    inline bool IsTimer(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        bool timerText = (offsetX1 >= 67.0f && offsetX2 <= 171.0f &&
            offsetY1 >= 385.0f && offsetY2 <= 409.0f &&
            (color.RGBA == 4286094213 || color.RGBA == 4282384384));

        bool timerTopBorder = (offsetX1 == 64.0f && (offsetX2 == 144.0f || offsetX2 == 174.0f) &&
            offsetY1 == 382.0f && offsetY2 == 383.0f &&
            color.RGBA == 4283459416);

        bool timerBottomBorder = (offsetX1 == 64.0f && (offsetX2 == 144.0f || offsetX2 == 174.0f) &&
            offsetY1 == 411.0f && offsetY2 == 412.0f &&
            color.RGBA == 4283459416);

        bool timerLeftBorder = (offsetX1 == 64.0f && offsetX2 == 65.0f &&
            offsetY1 == 382.0f && offsetY2 == 412.0f &&
            color.RGBA == 4283459416);

        bool timerRightBorder = ((offsetX1 == 143.0f || offsetX1 == 173.0f) &&
            (offsetX2 == 144.0f || offsetX2 == 174.0f) &&
            offsetY1 == 382.0f && offsetY2 == 412.0f &&
            color.RGBA == 4283459416);

        bool timerBackground = (offsetX1 >= 65.0f && (offsetX2 == 143.0f || offsetX2 <= 173.0f) &&
            (offsetX2 - offsetX1 >= 70.0f) &&
            offsetY1 >= 383.0f && offsetY2 <= 411.0f &&
            (color.RGBA == 4266682200 || color.RGBA == 4283459416));

        return timerText || timerTopBorder || timerBottomBorder || timerLeftBorder || timerRightBorder || timerBackground;
    }

    inline bool ShouldOffsetRight(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return IsStealthMeterLine(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsStealthMeter(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponIconBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponIconBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsAlarmCounterText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsHealthBar(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsEnvelopeIcon(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsAlarmIcon(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponIcon(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponNameText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsAmmoText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsBulletsImage(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsRateOfFireImage(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponAddonBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponAddonBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsWeaponAddonAmmoText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsInteractionMenuBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsInteractionMenuBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsInteractionMenuText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTurretInterface(offsetX1, offsetX2, offsetY1, offsetY2, color);
    }

    inline bool ShouldOffsetLeft(float offsetX1, float offsetX2, float offsetY1, float offsetY2, FColor color)
    {
        return IsTopDialogueMenuBackground(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTopDialogueMenuBorder(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTopDialogueMenuText(offsetX1, offsetX2, offsetY1, offsetY2, color) ||
            IsTimer(offsetX1, offsetX2, offsetY1, offsetY2, color);
    }
}

export void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
    DBGONLY(KEYPRESS(VK_F2)
    {
        spd::log()->info("{0:f} {1:f} {2:f} {3:f} {4:08x}", offsetX1, offsetX2, offsetY1, offsetY2, Color.RGBA);
    });

    if (HudMatchers::ShouldOffsetRight(offsetX1, offsetX2, offsetY1, offsetY2, Color))
    {
        offsetX1 += Screen.fWidescreenHudOffset;
        offsetX2 += Screen.fWidescreenHudOffset;
    }
    else if (HudMatchers::ShouldOffsetLeft(offsetX1, offsetX2, offsetY1, offsetY2, Color))
    {
        offsetX1 -= Screen.fWidescreenHudOffset;
        offsetX2 -= Screen.fWidescreenHudOffset;
    }
}