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
            (n_offsetY1) == 304 && (n_offsetY2 == 331) || (n_offsetY2 == 332) &&
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
            (Color.RGBA == 0xFEFEFEFE && ((n_offsetX1 == 524) || (n_offsetX1 == 525) ||
            (n_offsetX1 == 498) || (n_offsetX1 == 499))));
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

    inline bool IsTopDialogue(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        if (FloatInRange(n_offsetX1, 0, 370) && FloatInRange(n_offsetY2, 0, 140))
        {
            constexpr const wchar_t* kTextures[] = { // starts_with
                L"Texture HUD.HUD.ETGAME",
                L"FinalBlend Transient.FinalBlend",
            };

            constexpr int kIndices[] = { 23, 26, 27, 33, 40, 47, 62, 68, 69, 70, 72 };

            constexpr uint32_t kColors[] = {
                0xFEFEFEFE,
                0xFE808080,
                0x4C000000,
                0xFE404040,
                0x96000000,
                0xFE333333,
            };

            const bool textureMatch = std::any_of(std::begin(kTextures), std::end(kTextures), [&](const wchar_t* name)
            {
                return curDrawTileManagerTextureName.starts_with(name);
            });

            if (textureMatch)
            {
                const bool indexMatch = std::any_of(std::begin(kIndices), std::end(kIndices), [&](int idx)
                {
                    return curDrawTileManagerTextureIndex == idx;
                });

                const bool colorMatch = std::any_of(std::begin(kColors), std::end(kColors), [&](uint32_t c)
                {
                    return Color.RGBA == c;
                });

                if (indexMatch && colorMatch)
                {
                    if (curDrawTileManagerTextureName.starts_with(L"FinalBlend Transient.FinalBlend") &&
                        FloatInRange(n_offsetY1, 130, 138) &&
                        FloatInRange(n_offsetY2, 136, 138) &&
                        FloatInRange(n_offsetX1, 280, 376) &&
                        FloatInRange(n_offsetX2, 280, 376))
                    {
                        return false;
                    }

                    return true;
                }
            }
        }

        return false;
    }

    inline bool IsTimer(uint32_t n_offsetX1, uint32_t n_offsetX2, uint32_t n_offsetY1, uint32_t n_offsetY2, FColor Color)
    {
        return FloatInRange(n_offsetX1, 40, 150) && FloatInRange(n_offsetX2, 40, 150) &&
            FloatInRange(n_offsetY1, 382, 412) && FloatInRange(n_offsetY2, 382, 412) &&
            (Color.RGBA == 0xFE808080 || Color.RGBA == 0xFE000000 ||
            Color.RGBA == 0xFE404040 || Color.RGBA == 0xFE400000);
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
        return IsTopDialogue(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color) ||
            IsTimer(n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color);
    }
}

export void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
    uint32_t n_offsetX1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX1));
    uint32_t n_offsetX2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX2));
    uint32_t n_offsetY1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY1));
    uint32_t n_offsetY2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY2));

    DBGONLY(KEYPRESS(VK_F1)
    {
        spd::log()->info("{} {} {} {} {:08x} {} {}", n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color.RGBA, curDrawTileManagerTextureIndex, std::string(curDrawTileManagerTextureName.begin(), curDrawTileManagerTextureName.end()).c_str());
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