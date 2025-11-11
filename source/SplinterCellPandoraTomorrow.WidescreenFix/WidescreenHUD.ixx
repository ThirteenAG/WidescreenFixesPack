module;

#include <stdafx.h>

export module WidescreenHUD;

import ComVars;

namespace HudMatchers
{
    inline bool IsStealthMeterLine(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX2 - offsetX1 == 17) &&
            (offsetY1 == 337 || offsetY1 == 360) &&
            (offsetY2 == 368 || offsetY2 == 345);
    }

    inline bool IsStealthMeter(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX2 - offsetX1 == 109) &&
            offsetY1 == 338 && offsetY2 == 368 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFE96AA96 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF96AA96);
    }

    inline bool IsWeaponIconBackground(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 == 491 && offsetX2 == 592 &&
            offsetY1 == 373 && offsetY2 == 441 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758);
    }

    inline bool IsWeaponIconBorder(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 487 || offsetX1 == 503 || offsetX1 == 580) &&
            (offsetX2 == 503 || offsetX2 == 580 || offsetX2 == 596) &&
            (offsetY1 == 369 || offsetY1 == 385 || offsetY1 == 429) &&
            color.RGBA == 0x80506758;
    }

    inline bool IsAlarmCounterText(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 >= 562 && offsetX2 <= 602 &&
            (offsetY1 == 316 || offsetY1 == 315) &&
            offsetY2 == 330 &&
            (color.RGBA == 0xFE425448 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF425549);
    }

    inline bool IsHealthBar(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 570 || offsetX1 == 586) &&
            (offsetX2 == 586 || offsetX2 == 602) &&
            color.RGBA == 0xC8506758;
    }

    inline bool IsEnvelopeIcon(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 == 492 && offsetX2 == 519 &&
            offsetY1 == 314 && offsetY2 == 333 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758);
    }

    inline bool IsAlarmIcon(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 == 521 && offsetX2 == 562 &&
            offsetY1 == 306 && offsetY2 == 340 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0x80506758 || color.RGBA == 0xFF506758);
    }

    inline bool IsWeaponIcon(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 494 || offsetX1 == 515) &&
            offsetY1 == 377 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF425549 || color.RGBA == 0xFF506758);
    }

    inline bool IsWeaponNameText(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 >= 491 || offsetX1 >= 490) &&
            offsetY1 == 423 &&
            (offsetY2 == 437 || offsetY2 == 438);
    }

    inline bool IsAmmoText(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 >= 571 &&
            offsetY1 == 373 &&
            (offsetY2 == 385 || offsetY2 == 388) &&
            (color.RGBA == 0xFE425448 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF425549);
    }

    inline bool IsBulletsImage(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 581 && (offsetX2 == 588 || offsetX2 == 592)) ||
            (offsetX1 == 568 && offsetY1 == 406);
    }

    inline bool IsRateOfFireImage(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 >= 536 && offsetX2 <= 576 &&
            offsetY1 == 416 && offsetY2 == 421;
    }

    inline bool IsWeaponAddonBackground(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return ((offsetX1 == 441 && offsetX2 == 481 && offsetY1 == 373 && offsetY2 == 413) ||
            (offsetX1 == 439 && offsetX2 == 483 && offsetY1 == 380 && offsetY2 == 412)) &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF425549 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF789B85);
    }

    inline bool IsWeaponAddonBorder(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 437 || offsetX1 == 453 || offsetX1 == 469) &&
            (offsetX2 == 453 || offsetX2 == 469 || offsetX2 == 485) &&
            (offsetY1 == 369 || offsetY1 == 385 || offsetY1 == 401) &&
            color.RGBA == 0x80506758;
    }

    inline bool IsWeaponAddonAmmoText(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 452 || offsetX1 == 456 || offsetX1 == 457 || offsetX1 == 460) &&
            (offsetX2 == 460 || offsetX2 == 465 || offsetX2 == 464 || offsetX2 == 467 || offsetX2 == 469) &&
            offsetY1 == 371 &&
            (offsetY2 == 383 || offsetY2 == 386) &&
            (color.RGBA == 0xFE425448 || color.RGBA == 0xFF425549 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF789B85);
    }

    inline bool IsInteractionMenuBackground(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return ((offsetX1 == 362) || (offsetX1 == 422 || offsetX1 == 443)) &&
            offsetX2 == 562 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFE789A84 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF425549 || color.RGBA == 0xFF789B85);
    }

    inline bool IsInteractionMenuBorder(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return ((offsetX1 == 359 || offsetX1 == 375 || offsetX1 == 549 || offsetX1 == 554 ||
            offsetX1 == 440 || offsetX1 == 456 || offsetX1 == 537 ||
            offsetX1 == 419 || offsetX1 == 435 || offsetX1 == 439) &&
            (offsetX2 == 375 || offsetX2 == 370 || offsetX2 == 549 || offsetX2 == 565 ||
            offsetX2 == 456 || offsetX2 == 451 || offsetX2 == 435 || offsetX2 == 430) &&
            (color.RGBA == 0x80506758 || color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758));
    }

    inline bool IsInteractionMenuText(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 >= 365 &&
            offsetY1 >= 42 && offsetY2 <= 192 &&
            (color.RGBA == 0xFE425448 || color.RGBA == 0xFE789A84 || color.RGBA == 0xFF425549 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF789B85);
    }

    inline bool IsTurretInterface(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 371 || offsetX1 == 379 || offsetX1 == 387 ||
            offsetX1 == 558 || offsetX1 == 566) &&
            offsetY1 >= 39 && offsetY2 <= 162 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xB4FEFEFE ||
            color.RGBA == 0xFF425549 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF789B85 || color.RGBA == 0xB4FFFFFF);
    }

    inline bool IsTopDialogueMenuBackground(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 == 48 && offsetX2 == 348 &&
            offsetY1 >= 42 &&
            (offsetY2 <= 87 || offsetY2 <= 90) &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xB4FEFEFE || color.RGBA == 0xFF425549 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF789B85);
    }

    inline bool IsTopDialogueMenuBorder(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return (offsetX1 == 45 || offsetX1 == 61 || offsetX1 == 335) &&
            (offsetX2 == 61 || offsetX2 == 335 || offsetX2 == 351) &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xB4FEFEFE || color.RGBA == 0xFF425549 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF789B85);
    }

    inline bool IsTopDialogueMenuText(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        return offsetX1 >= 52 &&
            offsetY1 >= 43 && offsetY2 <= 185 &&
            (color.RGBA == 0xFE425448 || color.RGBA == 0xFF506758 || color.RGBA == 0xFF425549);
    }

    inline bool IsTimer(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
    {
        bool timerText = (offsetX1 >= 73 && offsetX2 <= 165) &&
            (offsetY1 == 388 && offsetY2 == 406) &&
            (color.RGBA == 0xFE789A84 || color.RGBA == 0xFF789B85 || color.RGBA == 0xFE400000 || color.RGBA == 0xFF400000);

        bool timerTopBorder = offsetX1 == 64 && (offsetX2 == 144 || offsetX2 == 174) &&
            offsetY1 == 382 && offsetY2 == 383 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758);

        bool timerBottomBorder = offsetX1 == 64 && (offsetX2 == 144 || offsetX2 == 174) &&
            offsetY1 == 411 && offsetY2 == 412 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758);

        bool timerLeftBorder = offsetX1 == 64 && offsetX2 == 65 &&
            offsetY1 == 382 && offsetY2 == 412 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758);

        bool timerRightBorder = (offsetX1 == 143 || offsetX1 == 173) &&
            (offsetX2 == 144 || offsetX2 == 174) &&
            offsetY1 == 382 && offsetY2 == 412 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758);

        bool timerBackground = offsetX1 == 67 && (offsetX2 == 141 || offsetX2 == 171) &&
            offsetY1 == 385 && offsetY2 == 409 &&
            (color.RGBA == 0xFE506758 || color.RGBA == 0xFF506758);

        return timerText || timerTopBorder || timerBottomBorder || timerLeftBorder || timerRightBorder || timerBackground;
    }

    inline bool ShouldOffsetRight(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
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

    inline bool ShouldOffsetLeft(uint32_t offsetX1, uint32_t offsetX2, uint32_t offsetY1, uint32_t offsetY2, FColor color)
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
        spd::log()->info("{:f} {:f} {:f} {:f} {:08x} {}", offsetX1, offsetX2, offsetY1, offsetY2, Color.RGBA, curDrawTileManagerTextureIndex);
    });

    uint32_t n_offsetX1 = static_cast<uint32_t>(offsetX1);
    uint32_t n_offsetX2 = static_cast<uint32_t>(offsetX2);
    uint32_t n_offsetY1 = static_cast<uint32_t>(offsetY1);
    uint32_t n_offsetY2 = static_cast<uint32_t>(offsetY2);

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
}

export void InitWidescreenHUD()
{
    if (Screen.nHudWidescreenMode > 1)
    {
        UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.HUD_OFFSET_X", +[]() -> int
        {
            return static_cast<int>(Screen.fWidescreenHudOffset);
        });
    }
}