#include "stdafx.h"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHUDScaleX;
    float fHudOffset;
    const float fHUDScaleXOriginal = 0.003125f;
    const float fHudOffsetOriginal = 1.0f;
    float fHUDScaleXDyn;
    float fHudOffsetDyn;
    float fTextScaleX;
    int32_t nHudOffsetReal;
    int32_t nScopeScale;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
} Screen;

union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
};

struct FLTColor
{
    float R, G, B, A = 1.0f;
    inline FLTColor() {}
    inline FLTColor(uint32_t color) {
        R = ((color >> 16) & 0xFF) / 255.0f;
        G = ((color >> 8) & 0xFF) / 255.0f;
        B = ((color) & 0xFF) / 255.0f;
        A = 1.0f;
    }
    bool empty() {
        return (R == 0.0f && G == 0.0f && B == 0.0f);
    }
    FLTColor& operator=(uint32_t color) {
        *this = FLTColor(color);
        return *this;
    }
};

struct WidescreenHudOffset
{
    int32_t _int;
    float _float;
} WidescreenHudOffset;

bool bHudWidescreenMode;
int32_t nWidescreenHudOffset;
float fWidescreenHudOffset;

FLTColor gColor;
float* __cdecl FGetHSV(float* dest, uint8_t H, uint8_t S, uint8_t V)
{
    if ((H == 0x41 && S == 0xC8) || (H == 0x2C && S == 0xCC))
    {
        dest[0] = gColor.R;
        dest[1] = gColor.G;
        dest[2] = gColor.B;
        dest[3] = 1.0f;
        return dest;
    }

    float r, g, b, a = 1.0f;
    float v14 = (float)H * 6.0f * 0.00390625f;
    float v4 = floor(v14);
    float v5 = (float)(255 - S) * 0.0039215689f;
    float v6 = (float)V * 0.0039215689f;
    float v16 = (1.0f - v5) * v6;
    float v10 = (1.0f - (v5 * (v14 - v4))) * v6;
    float v7 = (1.0f - (v14 - v4)) * v5;
    float v15 = (float)V * 0.0039215689f;
    float v17 = (1.0f - v7) * v6;

    switch ((uint32_t)v4)
    {
    case 0:
        r = v15;
        g = v17;
        b = v16;
        break;
    case 1:
        r = v10;
        g = v15;
        b = v16;
        break;
    case 2:
        r = v16;
        g = v15;
        b = v17;
        break;
    case 3:
        r = v16;
        g = v10;
        b = v15;
        break;
    case 4:
        r = v17;
        g = v16;
        b = v15;
        break;
    case 5:
        r = v15;
        g = v16;
        b = v10;
        break;
    default:
        break;
    }

    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = a;
    return dest;
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    nWidescreenHudOffset = iniReader.ReadInteger("MAIN", "WidescreenHudOffset", 100);
    fWidescreenHudOffset = static_cast<float>(nWidescreenHudOffset);
    gColor = iniReader.ReadInteger("BONUS", "GogglesLightColor", 0);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    auto pattern = hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14"); //0x10CD09C5
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = (regs.esp + 0x434); // lea eax, [esp+434h]

            if (Screen.Width < 640 || Screen.Height < 480)
                return;

            char pszPath[MAX_PATH];
            wcstombs(pszPath, (const wchar_t*)regs.edi, wcslen((const wchar_t*)regs.edi));
            pszPath[wcslen((const wchar_t*)regs.edi)] = '\0';
            char ResX[20];
            char ResY[20];
            _snprintf(ResX, 20, "%d", Screen.Width);
            _snprintf(ResY, 20, "%d", Screen.Height);
            WritePrivateProfileStringA("WinDrv.WindowsClient", "WindowedViewportX", ResX, pszPath);
            WritePrivateProfileStringA("WinDrv.WindowsClient", "WindowedViewportY", ResY, pszPath);
            WritePrivateProfileStringA("WinDrv.WindowsClient", "FullscreenViewportX", ResX, pszPath);
            WritePrivateProfileStringA("WinDrv.WindowsClient", "FullscreenViewportY", ResY, pszPath);
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(7));


    pattern = hook::pattern("89 6B 58 89 7B 5C EB 0E");
    static auto dword_1120B6BC = *hook::pattern("D9 1D ? ? ? ? 0F BF").count(1).get(0).get<uint32_t*>(2);
    static auto dword_1120B6B0 = *hook::pattern("8B 0D ? ? ? ? 85 C9 74 ? 8B 54 24 04 8B 82").count(1).get(0).get<uint32_t*>(2);
    static auto dword_11223A7C = *hook::pattern("A1 ? ? ? ? 83 C4 04 85 C0 D8 3D ? ? ? ?").count(1).get(0).get<uint32_t*>(1);
    struct GetRes
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebx + 0x58) = regs.ebp;
            *(uint32_t*)(regs.ebx + 0x5C) = regs.edi;
            Screen.Width = regs.ebp;
            Screen.Height = regs.edi;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

            Screen.fHUDScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
            injector::WriteMemory(dword_1120B6BC, Screen.fHUDScaleX, true);
            Screen.fTextScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * 2.0f;
            Screen.fHudOffset = Screen.fTextScaleX / 2.0f;
            Screen.nHudOffsetReal = static_cast<int32_t>(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
            Screen.nScopeScale = static_cast<int32_t>(Screen.fWidth * (Screen.fAspectRatio / (4.0f / 3.0f)));

            if (Screen.fAspectRatio < (16.0f / 9.0f))
            {
                WidescreenHudOffset._float = fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
                WidescreenHudOffset._int = static_cast<int32_t>(WidescreenHudOffset._float);
            }
            else
            {
                WidescreenHudOffset._int = nWidescreenHudOffset;
                WidescreenHudOffset._float = fWidescreenHudOffset;
            }

            //FMV
            //Screen.fFMVoffsetStartX = ((Screen.fHeight * Screen.fAspectRatio) - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
            //injector::WriteMemory(0x10C863D6 + 0x4, Screen.fFMVoffsetStartX, true);
            //actually it scales perfectly as is.
        }
    }; injector::MakeInline<GetRes>(pattern.get_first(0), pattern.get_first(6)); //<0x10CC622C, 0x10CC6232>


    //HUD
    static uint8_t* bIsInMenu;
    pattern = hook::pattern("88 81 39 4D 00 00 C2 08 00");
    struct MenuCheckHook
    {
        void operator()(injector::reg_pack& regs)
        {
            bIsInMenu = (uint8_t*)(regs.ecx + 0x4D39);
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
            *(uint32_t*)(regs.eax + 0x68) = regs.ecx;

            Screen.fHUDScaleXDyn = Screen.fHUDScaleX;
            Screen.fHudOffsetDyn = Screen.fHudOffset;

            int32_t fLeft = *(int16_t*)(regs.esp + 0x40);    // 0
            int32_t fRight = *(int16_t*)(regs.esp + 0x42);   // 640
            int32_t fTop = *(int16_t*)(regs.esp + 0x44);     // 0
            int32_t fBottom = *(int16_t*)(regs.esp + 0x46);  // 480
            FColor Color; Color.RGBA = *(int32_t*)(regs.esp + 0x3C);

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
                    *(int16_t*)(regs.esp + 0x40) -= Screen.nHudOffsetReal;
                    *(int16_t*)(regs.esp + 0x42) -= Screen.nHudOffsetReal;
                    return;
                }
                else if (((fLeft == 382 || fLeft == 383 || fLeft == 384) && (fRight == 639 || fRight == 640 || fRight == 641)) || ((fLeft == 319 && fRight == 699) || (fLeft == 320 && fRight == 700))) //scopes image right
                {
                    *(int16_t*)(regs.esp + 0x40) += Screen.nHudOffsetReal;
                    *(int16_t*)(regs.esp + 0x42) += Screen.nHudOffsetReal;
                    return;
                }

                if (fTop == 0 && fBottom == 480 && Color.A == 0x1E) // camera feed white overlay
                {
                    if (fLeft == 0 && fRight == 65)
                        *(int16_t*)(regs.esp + 0x40) -= Screen.nHudOffsetReal;
                    else if (fLeft == 575 && fRight == 640)
                        *(int16_t*)(regs.esp + 0x42) += Screen.nHudOffsetReal;
                }

                if ((fLeft == -1 || fLeft == 579) && (fRight == 62 || fRight == 649) && fTop == -3 && fBottom == 483 && Color.RGBA == 0x1bffffff)
                {
                    if (fLeft == -1)
                        *(int16_t*)(regs.esp + 0x40) -= Screen.nHudOffsetReal; //weapon scope
                    else
                        *(int16_t*)(regs.esp + 0x42) += Screen.nHudOffsetReal;
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
                        !(fLeft == 594 && fRight == 604 && fTop == 270 && fBottom == 271) && // camera screen bracket ]
                        !((((fRight - fLeft) == 1) || ((fRight - fLeft) == 2) || ((fRight - fLeft) == 3) || ((fRight - fLeft) == 4)) && ((fBottom - fTop) == 1 || (fBottom - fTop) == 16 || (fBottom - fTop) == 21 || (fBottom - fTop) == 22) && (fTop >= 195 && fBottom <= 395)) //other brackets of overlay menus
                        )
                    {
                        DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{0:d} {1:d} {2:d} {3:d} {4:08x}", fLeft, fRight, fTop, fBottom, Color.RGBA); });
                        *(int16_t*)(regs.esp + 0x40) += WidescreenHudOffset._int;
                        *(int16_t*)(regs.esp + 0x42) += WidescreenHudOffset._int;
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
            regs.ecx = *(uint32_t*)(regs.esi + 0x1A4);
            *(int32_t*)(regs.esp + 0x68) = Screen.nScopeScale;
        }
    }; injector::MakeInline<ScopeHook>(pattern.get_first(0), pattern.get_first(6)); //0x10C9A646

    //TEXT
    pattern = hook::pattern("D8 3D ? ? ? ? D9 5C 24 68 DB");
    injector::WriteMemory(pattern.get_first(2), &Screen.fTextScaleX, true); //0x10B149CE + 0x2
    pattern = hook::pattern("D8 25 ? ? ? ? D9 44 24 24 D8 4C 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudOffset, true); //0x10B14BAD + 0x2

    if (bHudWidescreenMode)
    {
        pattern = hook::pattern("A1 ? ? ? ? 83 C4 04 85 C0 D8 3D");
        struct WSText
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)dword_11223A7C;

                int32_t offset1 = *(int32_t*)(regs.esp + 0x4);
                int32_t offset2 = *(int32_t*)(regs.esp + 0xC);
                int32_t offset3 = static_cast<int32_t>(*(float*)(regs.esp + 0x1C));
                FColor Color; Color.RGBA = *(int32_t*)(regs.esp + 0x160);

                DBGONLY(KEYPRESS(VK_F2) { spd::log()->info("{0:d} {1:d} {2:d} {4:08x}", offset1, offset2, offset3, Color.RGBA); });

                if (bIsInMenu && *bIsInMenu == 0)
                {
                    if (
                        ((offset1 == 435 || offset1 == 436 || offset1 == 437) && (offset2 >= 3 && offset2 <= 21) && (offset3 == 313 || offset3 == 329 || offset3 == 345 || offset3 == 361 || offset3 == 377 || offset3 == 393 || offset3 == 416) && ((Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || (Color.R == 0xb8 && Color.G == 0xfa && Color.B == 0xc8) || (Color.R == 0x66 && Color.G == 0x66 && Color.B == 0x66))) || // top corner
                        ((offset1 >= 489 && offset1 <= 598) && ((offset2 >= 1 && offset2 <= 20)) && (offset3 == 23 || offset3 == 39 || offset3 == 93) && ((Color.R == 0xff && Color.G == 0xff && Color.B == 0xff) || (Color.R == 0xb8 && Color.G == 0xfa && Color.B == 0xc8))) || // bottom corner
                        (offset1 == 598 && offset2 == 3 && offset3 == 93 && (Color.R == 0xb8 && Color.G == 0xf7 && Color.B == 0xc8)) || //icons text
                        ((offset1 >= 465 && offset1 <= 615) && (offset2 >= 3 && offset2 <= 75) && (offset3 >= 128 && offset3 <= 215) && (Color.R == 0xb8 && Color.G == 0xf7 && Color.B == 0xc8)) // objective popup text
                        )
                    {
                        *(float*)(regs.esp + 0x14) += WidescreenHudOffset._float;
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
            *(float*)&regs.edx = AdjustFOV(*(float*)(regs.ecx + 0x2BC), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(0), pattern.get_first(6)); //0x10A3E67F

    pattern = hook::pattern("8B 88 BC 02 00 00 8B 44 24 20 51");
    struct UGameEngine_Draw_Hook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.ecx = AdjustFOV(*(float*)(regs.eax + 0x2BC), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook2>(pattern.get_first(0), pattern.get_first(6)); //0x10A3E8A0

    //windowed alt-tab fix
    pattern = hook::pattern("8B 84 24 ? ? ? ? 50 53 57 52 74 50 FF 15 ? ? ? ? 5F 5E 5D 5B 81 C4 ? ? ? ? C2 0C 00");
    struct WndProcHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esp + 0x3A8);

            if (regs.edi == WM_WINDOWPOSCHANGED)
            {
                auto lpwp = (LPWINDOWPOS)regs.eax;
                if (lpwp->x < 0 && lpwp->y < 0)
                    ShowWindow((HWND)regs.edx, SW_RESTORE);
            }
        }
    }; injector::MakeInline<WndProcHook>(pattern.get_first(0), pattern.get_first(7)); //0x10CC4EEA 

    //Goggles Light Color
    if (!gColor.empty())
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B 8E ? ? ? ? 8B 11 83 C4 10 6A 01 50 6A 14 51");
        injector::MakeCALL(pattern.get_first(0), FGetHSV, true); //0x10CB4325
        pattern = hook::pattern("E8 ? ? ? ? 8B 45 48 83 C4 10 3B C7 C7 44 24");
        injector::MakeCALL(pattern.get_first(0), FGetHSV, true); //0x10CB4798
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14").count_hint(1).empty(), 0x1100);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}