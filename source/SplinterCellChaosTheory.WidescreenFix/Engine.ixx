module;

#include <stdafx.h>

export module Engine;

import ComVars;
import WidescreenHUD;

export void InitEngine()
{
    //HUD
    static uint8_t* bIsInMenu = nullptr;
    auto pattern = hook::pattern("88 81 39 4D 00 00 C2 08 00");
    struct MenuCheckHook
    {
        void operator()(injector::reg_pack& regs)
        {
            bIsInMenu = reinterpret_cast<uint8_t*>(regs.ecx + 0x4D39);
            *bIsInMenu = 0;
        }
    }; injector::MakeInline<MenuCheckHook>(pattern.get_first(0), pattern.get_first(6)); //0x10C7FC4C

    Screen.fHUDScaleXDyn = Screen.fHUDScaleX;
    Screen.fHudOffsetDyn = Screen.fHudOffset;

    pattern = hook::pattern("D8 25 ? ? ? ? D9 05 ? ? ? ? D8 88");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffsetDyn, true); //0x10ADADBE + 0x2

    pattern = hook::pattern(pattern_str(0xD9, 0x05, to_bytes(dword_1120B6BC))); //fld
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::WriteMemory(pattern.get(i).get<uint32_t>(2), &Screen.fHUDScaleXDyn, true);

    pattern = hook::pattern(pattern_str(0xA1, to_bytes(dword_1120B6BC))); //mov eax
    injector::WriteMemory(pattern.get_first(1), &Screen.fHUDScaleXDyn, true);

    pattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(dword_1120B6BC))); //fmul
    injector::WriteMemory(pattern.get_first(2), &Screen.fHUDScaleXDyn, true);

    pattern = hook::pattern("33 CA 89 48 68 D9 44 24 48 DA E9 DF E0");
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx ^= regs.edx;
            *reinterpret_cast<uint32_t*>(regs.eax + 0x68) = regs.ecx;

            Screen.fHUDScaleXDyn = Screen.fHUDScaleX;
            Screen.fHudOffsetDyn = Screen.fHudOffset;

            int16_t fLeft = *reinterpret_cast<int16_t*>(regs.esp + 0x40);    // 0
            int16_t fRight = *reinterpret_cast<int16_t*>(regs.esp + 0x42);   // 640
            int16_t fTop = *reinterpret_cast<int16_t*>(regs.esp + 0x44);     // 0
            int16_t fBottom = *reinterpret_cast<int16_t*>(regs.esp + 0x46);  // 480
            FColor Color{ *reinterpret_cast<uint32_t*>(regs.esp + 0x3C) };

            if ((fLeft == 0 && fRight == 640 /*&& fTop == 0 && fBottom == 480*/) || (fLeft == -2 && fRight == 639 && fTop == -2 && fBottom == 479)
                || (fLeft == -1 && fRight == 640 && fTop == -2 && fBottom == 479) || (fTop == 0 && fBottom == 512)) //fullscreen images, 0 512 - camera feed overlay
            {
                Screen.fHUDScaleXDyn = Screen.fHUDScaleXOriginal;
                Screen.fHudOffsetDyn = Screen.fHudOffsetOriginal;
                return;
            }

            if (bIsInMenu && *bIsInMenu == 0)
            {
                if ((fLeft == -1 && fRight == 256) || (fLeft == -2 && fRight == 255) || (fLeft == -61 && fRight == 319) || (fLeft == -60 && fRight == 320)) //scopes image left
                {
                    *reinterpret_cast<int16_t*>(regs.esp + 0x40) -= Screen.nHudOffsetReal;
                    *reinterpret_cast<int16_t*>(regs.esp + 0x42) -= Screen.nHudOffsetReal;
                    return;
                }
                else if (((fLeft == 382 || fLeft == 383 || fLeft == 384) && (fRight == 639 || fRight == 640 || fRight == 641)) || ((fLeft == 319 && fRight == 699) || (fLeft == 320 && fRight == 700))) //scopes image right
                {
                    *reinterpret_cast<int16_t*>(regs.esp + 0x40) += Screen.nHudOffsetReal;
                    *reinterpret_cast<int16_t*>(regs.esp + 0x42) += Screen.nHudOffsetReal;
                    return;
                }

                if (fTop == 0 && fBottom == 480 && Color.A == 0x1E) // camera feed white overlay
                {
                    if (fLeft == 0 && fRight == 65)
                        *reinterpret_cast<int16_t*>(regs.esp + 0x40) -= Screen.nHudOffsetReal;
                    else if (fLeft == 575 && fRight == 640)
                        *reinterpret_cast<int16_t*>(regs.esp + 0x42) += Screen.nHudOffsetReal;
                }

                if ((fLeft == -1 || fLeft == 579) && (fRight == 62 || fRight == 649) && fTop == -3 && fBottom == 483 && Color.RGBA == 0x1bffffff)
                {
                    if (fLeft == -1)
                        *reinterpret_cast<int16_t*>(regs.esp + 0x40) -= Screen.nHudOffsetReal; //weapon scope
                    else
                        *reinterpret_cast<int16_t*>(regs.esp + 0x42) += Screen.nHudOffsetReal;
                }

                if (!bHudWidescreenMode)
                    return;

                if (
                    (
                    (Color.RGBA == 0x99ffffff || Color.RGBA == 0xc8ffffff || Color.RGBA == 0x59ffffff || Color.RGBA == 0x80ffffff || Color.RGBA == 0x32ffffff || Color.RGBA == 0x96ffffff) && //top right menus colors
                    ((fLeft >= 421 && fLeft <= 435) && (fRight >= 421 && fRight <= 435) && fTop >= 0 && fBottom <= 200) || //top right menu LEFT
                    ((fLeft >= 421 && fLeft <= 438) && (fRight <= 630) && fTop >= 0 && fBottom <= 200) || //top right menu MIDDLE
                    ((fLeft >= 600 && fLeft <= 635) && (fRight >= 600 && fRight <= 635) && fTop >= 0 && fBottom <= 200)    //top right menu RIGHT
                    )
                    ||
                    (
                    (Color.RGBA == 0x4bb8fac8 || Color.RGBA == 0x32ffffff || Color.RGBA == 0x40ffffff || Color.RGBA == 0x59ffffff || Color.RGBA == 0x80ffffff || Color.RGBA == 0x96ffffff || Color.RGBA == 0x99ffffff || Color.RGBA == 0xc8ffffff || Color.RGBA == 0xffffffff || (Color.R == 0xb8 && Color.G == 0xf7 && Color.B == 0xc8)) &&
                    ((fLeft >= 465 && fLeft <= 622) && (fRight >= 468 && fRight <= 625) && fTop >= 360 && fBottom <= 465) //bottom right panel
                    )
                    ||
                    (
                    (Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) && //objective text popup
                    ((fLeft >= 465 && fLeft <= 622) && (fRight >= 468 && fRight <= 625) && fTop >= 250 && fBottom <= 351)
                    )
                    )
                {
                    if ( //excludes
                        !(fLeft == 566 && fRight == 569 && fTop == 409 && fBottom == 425) && // camera screen bracket ]
                        !(fLeft == 562 && fRight == 566 && fTop == 409 && fBottom == 410) && // camera screen bracket ]
                        !(fLeft == 562 && fRight == 566 && fTop == 424 && fBottom == 425) && // camera screen bracket ]
                        !(fLeft == 592 && fRight == 602 && fTop == 270 && fBottom == 271) && // camera screen bracket ]
                        !(fLeft == 594 && fRight == 604 && fTop == 270 && fBottom == 271) && // camera screen bracket ]
                        !(fLeft == 602 && fRight == 607 && fBottom - fTop == 1) && // camera screen bracket ]
                        !(fLeft == 604 && fRight == 609 && fBottom - fTop == 1) && // camera screen bracket ]
                        !(fLeft == 559 && fRight == 560 && fTop == 333 && fBottom == 346) && // camera screen bracket ]
                        !(fLeft == 557 && fRight == 562 && fTop == 317 && fBottom == 318) && // camera screen bracket ]
                        !(fLeft == 555 && fRight == 556 && fTop == 342 && fBottom == 346) && // camera screen bracket ]
                        !((((fRight - fLeft) == 1) || ((fRight - fLeft) == 2) || ((fRight - fLeft) == 3) || ((fRight - fLeft) == 4)) && ((fBottom - fTop) == 1 || (fBottom - fTop) == 16 || (fBottom - fTop) == 21 || (fBottom - fTop) == 22) && (fTop >= 195 && fBottom <= 395)) && //other brackets of overlay menus
                        // Grouped alt-f4 excludes
                        !(fLeft >= 480 && fLeft <= 486 && fRight <= 486 && fTop >= 270 && fBottom <= 307 && Color.R == 0xff && (fRight - fLeft) <= 4 && (fBottom - fTop) <= 16) && // vertical lines around 270-307
                        !(fLeft >= 475 && fLeft <= 487 && fRight == fLeft + 32 && fTop >= 297 && fTop <= 317 && fBottom == fTop + 32 && Color.R == 0xff) && // large squares
                        !(fLeft == 489 && fRight == 490 && fTop == 265 && fBottom == 315 && Color.R == 0xff)    // single vertical line
                        )
                    {
                        DBGONLY(KEYPRESS(VK_F2)
                        {
                            spd::log()->info("{0:d} {1:d} {2:d} {3:d} {4:08x}", fLeft, fRight, fTop, fBottom, Color.RGBA);
                        });
                        *reinterpret_cast<int16_t*>(regs.esp + 0x40) += WidescreenHudOffset._int;
                        *reinterpret_cast<int16_t*>(regs.esp + 0x42) += WidescreenHudOffset._int;
                    }
                }
            }
        }
    }; injector::MakeInline<HudHook>(pattern.get_first(0)); //0x10ADABFA

    pattern = hook::pattern("8B 8E A4 01 00 00 E8 ? ? ? ? 8B 86 A4 01 00 00 8B 88 30 61 00 00 89 88 E0 56 00 00 8B 90 1C 76 00 00 89 90 E8 56 00 00 8B 06 6A 03");
    struct ScopeHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *reinterpret_cast<uint32_t*>(regs.esi + 0x1A4);
            *reinterpret_cast<int32_t*>(regs.esp + 0x68) = Screen.nScopeScale;
        }
    }; injector::MakeInline<ScopeHook>(pattern.get_first(0), pattern.get_first(6)); //0x10C9A646

    //TEXT
    pattern = hook::pattern("D8 3D ? ? ? ? D9 5C 24 68 DB");
    injector::WriteMemory(pattern.get_first(2), &Screen.fTextScaleX, true); //0x10B149CE + 0x2
    pattern = hook::pattern("D8 25 ? ? ? ? D9 44 24 24 D8 4C 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffset, true); //0x10B14BAD + 0x2

    // TODO: Add more specific offsets for another languages
    switch (eGameLang)
    {
    case GameLang::Italian:
        sTextOffset.bottomCorner.v1v1 -= 7;
        sTextOffset.objPopup.v2v2 += 5;
        sTextOffset.topCorner.v2v2 += 4;
        break;
    case GameLang::Polish:
        sTextOffset.bottomCorner.v1v1 -= 10;
        break;
    case GameLang::Russian:
        sTextOffset.bottomCorner.v1v1 -= 9;
        sTextOffset.objPopup.v2v2 += 4;
        sTextOffset.topCorner.v2v2 += 2;
        break;
    default:
        break;
    }

    if (bHudWidescreenMode)
    {
        pattern = hook::pattern("A1 ? ? ? ? 83 C4 04 85 C0 D8 3D");
        struct WSText
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)dword_11223A7C;

                int32_t offset1 = *reinterpret_cast<int32_t*>(regs.esp + 0x4);
                int32_t offset2 = *reinterpret_cast<int32_t*>(regs.esp + 0xC);
                int32_t offset3 = static_cast<int32_t>(*reinterpret_cast<float*>(regs.esp + 0x1C));
                FColor Color{ *reinterpret_cast<uint32_t*>(regs.esp + 0x160) };

                // Aliases for compact view
                auto tc = sTextOffset.topCorner;
                auto bc = sTextOffset.bottomCorner;
                auto ico = sTextOffset.icons;
                auto obj = sTextOffset.objPopup;

                //DBGONLY(KEYPRESS(VK_F2) { spd::log()->info("{:d} {:d} {:d} {:08x}", offset1, offset2, offset3, Color.RGBA); });

                if (bIsInMenu && *bIsInMenu == 0)
                {
                    // Color checks
                    auto color1 = Color.R == 0xff && Color.G == 0xff && Color.B == 0xff;
                    auto color2 = Color.R == 0xb8 && Color.G == 0xfa && Color.B == 0xc8;
                    auto color3 = Color.R == 0x66 && Color.G == 0x66 && Color.B == 0x66;
                    auto color4 = Color.R == 0xb8 && Color.G == 0xf7 && Color.B == 0xc8;
                    // Top corner checks
                    auto tcO1 = offset1 == tc.v1v1 || offset1 == tc.v1v2 || offset1 == tc.v1v3;
                    auto tcO2 = offset2 >= tc.v2v1 && offset2 <= tc.v2v2;
                    auto tcO3 = offset3 == tc.v3v1 || offset3 == tc.v3v2 || offset3 == tc.v3v3 || offset3 == tc.v3v4 || offset3 == tc.v3v5 || offset3 == tc.v3v6 || offset3 == tc.v3v7;
                    // Bottom corner checks
                    auto bcO1 = offset1 >= bc.v1v1 && offset1 <= bc.v1v2;
                    auto bcO2 = offset2 >= bc.v2v1 && offset2 <= bc.v2v2;
                    auto bcO3 = offset3 == bc.v3v1 || offset3 == bc.v3v2 || offset3 == bc.v3v3;
                    // Icons text checks
                    auto icoO = offset1 == ico.v1 && offset2 == ico.v2 && offset3 == ico.v3;
                    // Objective popup text checks
                    auto objO1 = offset1 >= obj.v1v1 && offset1 <= obj.v1v2;
                    auto objO2 = offset2 >= obj.v2v1 && offset2 <= obj.v2v2;
                    auto objO3 = offset3 >= obj.v3v1 && offset3 <= obj.v3v2;
                    if (
                        (tcO1 && tcO2 && tcO3 && (color1 || color2 || color3)) || // top corner
                        (bcO1 && bcO2 && bcO3 && (color1 || color2)) || // bottom corner
                        (icoO && color4) || //icons text
                        ((objO1 && objO2 && objO3 && color4) || (offset1 == 473 && color4)) // objective popup text
                        )
                    {
                        *reinterpret_cast<float*>(regs.esp + 0x14) += WidescreenHudOffset._float;
                    }
                }
            }
        }; injector::MakeInline<WSText>(pattern.get_first(0), pattern.get_first(5)); //0x10B149C4, 0x10B149C4 + 5
    }

    //FOV
    pattern = hook::pattern("8B 91 BC 02 00 00 52 8B 54 24 24");
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<float*>(&regs.edx) = AdjustFOV(*reinterpret_cast<float*>(regs.ecx + 0x2BC), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(0), pattern.get_first(6)); //0x10A3E67F

    pattern = hook::pattern("8B 88 BC 02 00 00 8B 44 24 20 51");
    struct UGameEngine_Draw_Hook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<float*>(&regs.ecx) = AdjustFOV(*reinterpret_cast<float*>(regs.eax + 0x2BC), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook2>(pattern.get_first(0), pattern.get_first(6)); //0x10A3E8A0

    //windowed alt-tab fix
    if (!bDisableAltTabFix)
    {
        pattern = hook::pattern("8B 84 24 ? ? ? ? 50 53 57 52 74 50 FF 15 ? ? ? ? 5F 5E 5D 5B 81 C4 ? ? ? ? C2 0C 00");
        struct WndProcHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *reinterpret_cast<uint32_t*>(regs.esp + 0x3A8);

                if (regs.edi == WM_WINDOWPOSCHANGED)
                {
                    auto lpwp = reinterpret_cast<LPWINDOWPOS>(regs.eax);
                    auto hwnd = reinterpret_cast<HWND>(regs.edx);
                    if (lpwp->x < 0 && lpwp->y < 0 && IsIconic(hwnd))
                    {
                        ShowWindow(hwnd, SW_RESTORE);
                        SetForegroundWindow(hwnd);
                    }
                }
            }
        }; injector::MakeInline<WndProcHook>(pattern.get_first(0), pattern.get_first(7)); //0x10CC4EEA
    }
}