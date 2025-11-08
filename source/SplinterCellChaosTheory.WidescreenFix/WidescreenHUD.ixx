module;

#include <stdafx.h>

export module WidescreenHUD;

import ComVars;
import GUI;
import Hashes;

namespace HudImageMatchers
{
    inline bool IsFullScreenImage(int16_t left, int16_t right, int16_t top, int16_t bottom)
    {
        return (left == 0 && right == 640) ||
            (left == -2 && right == 639 && top == -2 && bottom == 479) ||
            (left == -1 && right == 640 && top == -2 && bottom == 479) ||
            (top == 0 && bottom == 512);
    }

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

    inline bool IsCameraFeedOverlayLeft(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        return top == 0 && bottom == 480 && color.A == 0x1E && left == 0 && right == 65;
    }

    inline bool IsCameraFeedOverlayRight(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        return top == 0 && bottom == 480 && color.A == 0x1E && left == 575 && right == 640;
    }

    inline bool IsWeaponScopeLeft(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        return (left == -1 || left == 579) && (right == 62 || right == 649) && top == -3 && bottom == 483 && color.RGBA == 0x1bffffff && left == -1;
    }

    inline bool IsWeaponScopeRight(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        return (left == -1 || left == 579) && (right == 62 || right == 649) && top == -3 && bottom == 483 && color.RGBA == 0x1bffffff && left == 579;
    }

    inline bool IsTopRightMenu(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        uint32_t c = color.RGBA;
        if (c == 0x99ffffff || c == 0xc8ffffff || c == 0x59ffffff || c == 0x80ffffff || c == 0x32ffffff || c == 0x96ffffff)
        {
            bool leftPart = (left >= 421 && left <= 435) && (right >= 421 && right <= 435) && top >= 0 && bottom <= 200;
            bool middlePart = (left >= 421 && left <= 438) && (right <= 630) && top >= 0 && bottom <= 200;
            bool rightPart = (left >= 600 && left <= 635) && (right >= 600 && right <= 635) && top >= 0 && bottom <= 200;
            return leftPart || middlePart || rightPart;
        }
        return false;
    }

    inline bool IsBottomRightPanel(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        uint32_t c = color.RGBA;
        return (c == 0x4bb8fac8 || c == 0x32ffffff || c == 0x40ffffff || c == 0x59ffffff || c == 0x80ffffff ||
            c == 0x96ffffff || c == 0x99ffffff || c == 0xc8ffffff || c == 0xffffffff ||
            (color.R == 0xb8 && color.G == 0xf7 && color.B == 0xc8)) &&
            (left >= 465 && left <= 622) && (right >= 468 && right <= 625) && top >= 360 && bottom <= 465;
    }

    inline bool IsObjectivePopup(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        return color.R == 0xff && color.G == 0xff && color.B == 0xff &&
            (left >= 465 && left <= 622) && (right >= 468 && right <= 625) && top >= 250 && bottom <= 351;
    }

    inline bool IsExcludedBracket(int16_t left, int16_t right, int16_t top, int16_t bottom, FColor color)
    {
        int16_t width = right - left;
        int16_t height = bottom - top;

        return (left == 566 && right == 569 && top == 409 && bottom == 425) || // camera screen bracket ]
            (left == 562 && right == 566 && top == 409 && bottom == 410) || // camera screen bracket ]
            (left == 562 && right == 566 && top == 424 && bottom == 425) || // camera screen bracket ]
            (left == 592 && right == 602 && top == 270 && bottom == 271) || // camera screen bracket ]
            (left == 594 && right == 604 && top == 270 && bottom == 271) || // camera screen bracket ]
            (left == 602 && right == 607 && height == 1) || // camera screen bracket ]
            (left == 604 && right == 609 && height == 1) || // camera screen bracket ]
            (left == 540 && right == 560 && top == 393 && bottom == 394) || // camera screen bracket ]
            (left == 559 && right == 560 && top == 333 && bottom == 346) || // camera screen bracket ]
            (left == 557 && right == 562 && top == 317 && bottom == 318) || // camera screen bracket ]
            (left == 555 && right == 556 && top == 342 && bottom == 346) || // camera screen bracket ]
            ((width == 1 || width == 2 || width == 3 || width == 4) &&
            (height == 1 || height == 16 || height == 21 || height == 22) &&
            (top >= 195 && bottom <= 395)) || // other brackets of overlay menus
            (left >= 480 && left <= 486 && right <= 486 && top >= 270 && bottom <= 307 && color.R == 0xff && (right - left) <= 4 && (bottom - top) <= 16) || // vertical lines around 270-307
            (left >= 475 && left <= 487 && right == left + 32 && top >= 297 && top <= 317 && bottom == top + 32 && color.R == 0xff) || // large squares
            (left == 489 && right == 490 && top == 265 && bottom == 315 && color.R == 0xff); // single vertical line
    }
}

namespace HudTextMatchers
{
    inline bool IsTopCornerText(int32_t offset1, int32_t offset2, int32_t offset3, FColor color)
    {
        bool color1 = color.R == 0xff && color.G == 0xff && color.B == 0xff;
        bool color2 = color.R == 0xb8 && color.G == 0xfa && color.B == 0xc8;
        bool color3 = color.R == 0x66 && color.G == 0x66 && color.B == 0x66;
        return (offset1 == 435 || offset1 == 436 || offset1 == 437) &&
            (offset2 >= 3 && offset2 <= 25) &&
            (offset3 == 313 || offset3 == 329 || offset3 == 345 || offset3 == 361 || offset3 == 377 || offset3 == 393 || offset3 == 416) &&
            (color1 || color2 || color3);
    }

    inline bool IsBottomCornerText(int32_t offset1, int32_t offset2, int32_t offset3, FColor color)
    {
        bool color1 = color.R == 0xff && color.G == 0xff && color.B == 0xff;
        bool color2 = color.R == 0xb8 && color.G == 0xfa && color.B == 0xc8;
        return (offset1 >= 479 && offset1 <= 598) &&
            (offset2 >= 1 && offset2 <= 20) &&
            (offset3 == 23 || offset3 == 39 || offset3 == 93) &&
            (color1 || color2);
    }

    inline bool IsIconsText(int32_t offset1, int32_t offset2, int32_t offset3, FColor color)
    {
        bool color4 = color.R == 0xb8 && color.G == 0xf7 && color.B == 0xc8;
        return offset1 == 598 && offset2 == 3 && offset3 == 93 && color4;
    }

    inline bool IsObjectivePopupText(int32_t offset1, int32_t offset2, int32_t offset3, FColor color)
    {
        bool color4 = color.R == 0xb8 && color.G == 0xf7 && color.B == 0xc8;
        return (((offset1 >= 465 && offset1 <= 615) &&
            (offset2 >= 3 && offset2 <= 80) &&
            (offset3 >= 128 && offset3 <= 215) && color4) ||
            (offset1 == 473 && color4));
    }
}

bool IsWidescreenHudNeeded()
{
    if (nHudWidescreenMode != 1)
        return false;

    if (UObject::GetState(L"EPlayerController") == L"s_KeyPadInteract" || bHackingGameplay)
        return false;

    if (UObject::GetState(L"EPlayerCam") == L"s_Fixed")
        return false;

    if (CMenusManager::IsOpsatDisplayed())
        return false;

    if (CMenusManager::IsMainMenuDisplayed())
        return false;

    if (CMenusManager::IsMenuDisplayed(KnownHashes::P_opticcable))
        return false;

    if (CMenusManager::IsMenuDisplayed(KnownHashes::P_KeyPad))
        return false;

    return true;
}

export void WidescreenHudImage(int16_t& left, int16_t& right, int16_t& top, int16_t& bottom, FColor color, std::wstring_view textureName)
{
    using namespace HudImageMatchers;

    if (IsFullScreenImage(left, right, top, bottom))
    {
        return;
    }

    if (IsScopeImageLeft(left, right))
    {
        left -= Screen.nHudOffsetReal;
        right -= Screen.nHudOffsetReal;
    }
    else if (IsScopeImageRight(left, right))
    {
        left += Screen.nHudOffsetReal;
        right += Screen.nHudOffsetReal;
    }

    if (IsCameraFeedOverlayLeft(left, right, top, bottom, color))
    {
        left -= Screen.nHudOffsetReal;
    }
    else if (IsCameraFeedOverlayRight(left, right, top, bottom, color))
    {
        right += Screen.nHudOffsetReal;
    }

    if (IsWeaponScopeLeft(left, right, top, bottom, color))
    {
        left -= Screen.nHudOffsetReal;
    }
    else if (IsWeaponScopeRight(left, right, top, bottom, color))
    {
        right += Screen.nHudOffsetReal;
    }

    if (textureName == L"Texture system_PC.mgb.Cursor" || textureName == L"Texture system_PC.mgb.CURSOR")
        return;

    if (!IsWidescreenHudNeeded())
        return;

    if (IsTopRightMenu(left, right, top, bottom, color) ||
        IsBottomRightPanel(left, right, top, bottom, color) ||
        IsObjectivePopup(left, right, top, bottom, color))
    {
        if (!IsExcludedBracket(left, right, top, bottom, color))
        {
            DBGONLY(KEYPRESS(VK_F2)
            {
                spd::log()->info("{0:d} {1:d} {2:d} {3:d} {4:08x}", left, right, top, bottom, color.RGBA);
            });
            left += WidescreenHudOffset._int;
            right += WidescreenHudOffset._int;
        }
    }
}

export void WidescreenHudText(float& textX, int32_t offset1, int32_t offset2, int32_t offset3, FColor color)
{
    using namespace HudTextMatchers;

    if (!IsWidescreenHudNeeded())
        return;

    if (IsTopCornerText(offset1, offset2, offset3, color) ||
        IsBottomCornerText(offset1, offset2, offset3, color) ||
        IsIconsText(offset1, offset2, offset3, color) ||
        IsObjectivePopupText(offset1, offset2, offset3, color))
    {
        DBGONLY(KEYPRESS(VK_F2)
        {
            spd::log()->info("{:d} {:d} {:d} {:08x}", offset1, offset2, offset3, color.RGBA);
        });

        textX += WidescreenHudOffset._float;
    }
}