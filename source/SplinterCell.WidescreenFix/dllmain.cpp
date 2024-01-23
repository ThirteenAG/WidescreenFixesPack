#include "stdafx.h"
#include <dxsdk/d3d8.h>

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float HUDScaleX;
    float fHudOffset;
    float fHudOffsetRight;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
    float fIniHudOffset;
    float fWidescreenHudOffset;
    bool bHudWidescreenMode;
} Screen;

union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
};

uint32_t nFMVWidescreenMode;
uintptr_t pDrawTile;

bool isIngameText;
void WidescreenHud(float& offsetX1, float& offsetX2, float& offsetY1, float& offsetY2, FColor& Color)
{
    uint32_t n_offsetX1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX1));
    uint32_t n_offsetX2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetX2));
    uint32_t n_offsetY1 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY1));
    uint32_t n_offsetY2 = static_cast<uint32_t>((480.0f * (Screen.fWidth / Screen.fHeight)) / (Screen.fWidth / offsetY2));

    DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{0:d} {1:d} {2:d} {3:d} {4:08x}", n_offsetX1, n_offsetX2, n_offsetY1, n_offsetY2, Color.RGBA); });

    if (
        (n_offsetX1 >= 598 && n_offsetX2 <= 616 && n_offsetY1 >= 38 && n_offsetY2 <= 233 /*&& (Color.RGBA == 4269834368 || Color.RGBA == 671088640)*/) || //health bar
        (n_offsetX1 >= 519 && n_offsetX2 <= 616 && n_offsetY1 == 304 && (n_offsetY2 == 331 || n_offsetY2 == 332) && (Color.RGBA == 4269834368)) || //hud icons
        (n_offsetX1 >= 493 && n_offsetX2 <= 616 && n_offsetY1 >= 333 && n_offsetY2 <= 410 && (Color.RGBA == 4264768307 || Color.RGBA == 1283489920 || Color.RGBA == 4269834368 || Color.RGBA == 4265623616 || Color.RGBA == 3237936894 || Color.RGBA == 1275068416 || (Color.RGBA == 2550136832 && n_offsetX1 != 573) || Color.RGBA == 2558558336 || (Color.RGBA == 4278124286 && (n_offsetX1 == 524 || n_offsetX1 == 525 || n_offsetX1 == 498 || n_offsetX1 == 499)))) || //stealth bar and weapon hud
        (n_offsetX1 >= 519 && n_offsetX2 <= 616 && n_offsetY1 >= 411 && n_offsetY2 <= 440 && Color.RGBA != 4264768307) //fire mode switch
        )
    {
        offsetX1 += Screen.fWidescreenHudOffset;
        offsetX2 += Screen.fWidescreenHudOffset;
    }
    //else
    //{
    //	if (
    //	(n_offsetX1 == 29 && n_offsetX2 == 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 1275068416 || Color.RGBA == 4265623616 || Color.RGBA == 4278124286)) || //top dialogue menu background
    //	((n_offsetX1 == 23 || n_offsetX1 == 29 || n_offsetX1 == 32 || n_offsetX1 == 354 || n_offsetX1 == 356) && (n_offsetX2 == 29 || n_offsetX2 == 30 || n_offsetX2 == 32 || n_offsetX2 == 354 || n_offsetX2 == 356 || n_offsetX2 == 362) && n_offsetY1 >= 39 && n_offsetY2 <= 160 /*&& Color.RGBA == 4266682200*/) || //top dialogue menu background border
    //	(n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 4265623616)) //|| //top dialogue menu icon
    //	//(n_offsetX1 >= 29 && n_offsetX2 <= 356 && (n_offsetY2 - n_offsetY1 == 15))
    //	)
    //	{
    //		if (n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA == 4265623616))
    //			isIngameText = true;
    //		else
    //			isIngameText = false;
    //
    //		offsetX1 -= Screen.fWidescreenHudOffset;
    //		offsetX2 -= Screen.fWidescreenHudOffset;
    //	}
    //}

    /*if (n_offsetX1 >= 29 && n_offsetX2 <= 356 && n_offsetY1 >= 40 && n_offsetY2 <= 160 && (Color.RGBA != 4265623616) && isIngameText)
    {
    offsetX1 -= Screen.fWidescreenHudOffset;
    offsetX2 -= Screen.fWidescreenHudOffset;
    }*/

    if ((n_offsetX1 >= 178 && n_offsetX1 <= 184) && (n_offsetX2 >= 607 && n_offsetX2 <= 612)) //inventory fix
    {
        offsetX2 += Screen.fWidescreenHudOffset;
    }
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color)
{
    static auto DrawTile = (void(__fastcall *)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor)) pDrawTile; //0x10305F01;
    static FColor ColBlack; ColBlack.RGBA = 0xFF000000;

    /*if (Color.RGBA == 0xfe000000 && X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>(Screen.fWidth - Screen.fHudOffset - Screen.fHudOffset) && //cutscene borders
    ((Y == 0.0f && static_cast<uint32_t>(SizeY) <= static_cast<uint32_t>((Screen.fWidth * (3.0f / 4.0f)) / (480.0f / 60.0f))) ||
    (static_cast<uint32_t>(Y) <= static_cast<uint32_t>((Screen.fWidth * (3.0f / 4.0f)) / (480.0f / (421.0f))) && static_cast<uint32_t>(SizeY) == static_cast<uint32_t>(Screen.fHeight))))
    {
    return;
    }*/

    if (
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 188.0f))) || //zoom scope borders
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 43.0f))) || //sticky camera borders
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 30.0f))) || //optic cable
        (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 163.0f)))    //optic mic
        )
    {
        DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
        DrawTile(_this, EDX, Screen.fWidth - Screen.fHudOffset, Y, Screen.fWidth, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
    }

    if (X == 0.0f && static_cast<uint32_t>(SizeX) == static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f)))
    {
        if ((Color.R == 0x40 && Color.G == 0x00 && Color.B == 0x00) || (Color.R == 0x80 && Color.G == 0x80 && Color.B == 0x80)) //mission failed red screen
        {
            DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
            return;
        }
        else
        {
            DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
            DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
        }
    }

    if (Screen.bHudWidescreenMode)
        WidescreenHud(X, SizeX, Y, SizeY, Color);

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    Screen.bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    Screen.fIniHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 120.0f);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    char UserIni[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(NULL), UserIni, (sizeof(UserIni)));
    *strrchr(UserIni, '\\') = '\0';
    strcat(UserIni, "\\SplinterCellUser.ini");

    CIniReader iniWriter(UserIni);
    char szRes[50];
    sprintf(szRes, "%dx%d", Screen.Width, Screen.Height);
    iniWriter.WriteString("Engine.EPCGameOptions", "Resolution", szRes);

    *strrchr(UserIni, '\\') = '\0';
    strcat(UserIni, "\\SplinterCell.ini");
    iniWriter.SetIniPath(UserIni);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);
    iniWriter.WriteInteger("D3DDrv.D3DRenderDevice", "ForceShadowMode", iniReader.ReadInteger("MAIN", "ForceShadowBufferMode", 1));
    iniWriter.WriteString("D3DDrv.D3DRenderDevice", "FullScreenVideo", "True");
}

void InitD3DDrv()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 50 8B CF FF 55 60"); //0x1000F72E
    static auto FMVPtrY = pattern.get_first(1);
    static auto FMVPtrX = pattern.get_first(6);

    static auto pPresentParams = *hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? 33 C0 8B D9 C1 E9 02 83 E3 03").get_first<D3DPRESENT_PARAMETERS*>(1);
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 5F 5E 5D 5B 81 C4 80 00 00 00 C2 10 00");
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 1;

            Screen.Width = *(uint32_t*)(regs.esp + 0x98); //pPresentParams->BackBufferWidth;
            Screen.Height = *(uint32_t*)(regs.esp + 0x9C); //pPresentParams->BackBufferHeight;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
            Screen.fFMVoffsetStartX = (Screen.fWidth - 640.0f) / 2.0f;
            Screen.fFMVoffsetStartY = (Screen.fHeight - 480.0f) / 2.0f;
            Screen.fWidescreenHudOffset = ((Screen.fHeight * (4.0f / 3.0f)) / (640.0f / Screen.fIniHudOffset));
            if (Screen.fAspectRatio < (16.0f / 9.0f))
                Screen.fWidescreenHudOffset = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);

            if (Screen.Width < 640 || Screen.Height < 480)
                return;

            injector::WriteMemory(FMVPtrY, Screen.Height, true);
            injector::WriteMemory(FMVPtrX, Screen.Width, true);

            CIniReader iniReader("");
            auto[DesktopResW, DesktopResH] = GetDesktopRes();
            if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
            {
                iniReader.WriteInteger("MAIN", "ResX", Screen.Width);
                iniReader.WriteInteger("MAIN", "ResY", Screen.Height);
            }
            else
            {
                iniReader.WriteInteger("MAIN", "ResX", 0);
                iniReader.WriteInteger("MAIN", "ResY", 0);
            }

            if (pPresentParams->Windowed)
            {
                tagRECT rect;
                rect.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
                rect.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
                rect.right = (LONG)Screen.Width;
                rect.bottom = (LONG)Screen.Height;
                SetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE, GetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(pPresentParams->hDeviceWindow, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
                SetForegroundWindow(pPresentParams->hDeviceWindow);
                SetCursor(NULL);
            }
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0));

    //FMV
    static auto SetFMVPos = [](injector::reg_pack& regs)
    {
        if (Screen.Width >= 1280 && Screen.Height >= 960)
        {
            static constexpr auto BINKCOPY2XWH = 0x40000000L; // copy the width and height zoomed by two
            regs.eax |= BINKCOPY2XWH;

            *(int32_t*)&regs.ebp = static_cast<int32_t>((Screen.fWidth - (640.0f * 2.0f)) / 2.0f);
            *(int32_t*)&regs.ebx = static_cast<int32_t>((Screen.fHeight - (480.0f * 2.0f)) / 2.0f);
        }
        else
        {
            *(int32_t*)&regs.ebp = static_cast<int32_t>((Screen.fWidth - 640.0f) / 2.0f);
            *(int32_t*)&regs.ebx = static_cast<int32_t>((Screen.fHeight - 480.0f) / 2.0f);
        }
    };

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 4C 24 3C 8B 54 24 28");
    struct BINKHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esp + 0x3C);
            regs.edx = *(uint32_t*)(regs.esp + 0x28);
            SetFMVPos(regs);
        }
    }; injector::MakeInline<BINKHook1>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 54 24 3C 8B 4C 24 2C");
    struct BINKHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.esp + 0x3C);
            regs.ecx = *(uint32_t*)(regs.esp + 0x2C);
            SetFMVPos(regs);
        }
    }; injector::MakeInline<BINKHook2>(pattern.get_first(0), pattern.get_first(8));
}

void InitEngine()
{
    //HUD
    auto flt_104E9F78 = *hook::pattern(GetModuleHandle(L"Engine"), "D8 C9 D8 0D").count(9).get(0).get<float*>(4);
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), pattern_str(0xD8, 0xC9, 0xD8, 0x0D, to_bytes(flt_104E9F78)));
    for (size_t i = 0; i < pattern.count_hint(4).size(); ++i)
    {
        injector::WriteMemory(pattern.get(i).get<void>(4), &Screen.HUDScaleX, true); //(DWORD)Engine + 0x1E9F78
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");
    pDrawTile = injector::GetBranchDestination(pattern.count(1).get(0).get<uintptr_t>(6), true).as_int();
    injector::MakeCALL(pattern.count(1).get(0).get<uintptr_t>(6), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC8ECE

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 52 E8");
    injector::MakeCALL(pattern.count(2).get(0).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9B7C
    injector::MakeCALL(pattern.count(2).get(1).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9DE1

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 46 34 8B 88 B0 02 00 00");
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.ecx = AdjustFOV(*(float*)(regs.eax + 0x2B0), Screen.fAspectRatio);
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.count(1).get(0).get<uint32_t>(3), pattern.count(1).get(0).get<uint32_t>(3 + 6)); //pfDraw + 0x104
}

void InitWinDrv()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8B 5D 10 8B 46 18 83 FB FF");
    struct OpenWindowHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto[DesktopResW, DesktopResH] = GetDesktopRes();
            tagRECT rect;
            rect.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)Screen.Width / 2.0f));
            rect.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)Screen.Height / 2.0f));
            rect.right = (LONG)Screen.Width;
            rect.bottom = (LONG)Screen.Height;

            *(int32_t*)(regs.esp + 0x474) = rect.right;
            *(int32_t*)(regs.esp + 0x478) = rect.bottom;
            *(int32_t*)(regs.esp + 0x47C) = rect.left;
            *(int32_t*)(regs.esp + 0x480) = rect.top;

            regs.ebx = *(uint32_t*)(regs.ebp + 0x10);
            regs.eax = *(uint32_t*)(regs.esi + 0x18);
        }
    }; injector::MakeInline<OpenWindowHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8D 4D B8 68 ? ? ? ? C7 45 ? ? ? ? ? 51");
    injector::WriteMemory(pattern.get_first(4), *pattern.get_first<LONG>(4) & ~WS_OVERLAPPEDWINDOW, true);
    injector::WriteMemory(pattern.get_first(11), *pattern.get_first<LONG>(11) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "6A 00 8D 55 B8 68 ? ? ? ? C7 45");
    injector::WriteMemory(pattern.get_first(6), *pattern.get_first<LONG>(6) & ~WS_OVERLAPPEDWINDOW, true);
    injector::WriteMemory(pattern.get_first(13), *pattern.get_first<LONG>(13) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "68 ? ? ? ? 6A F0 83 38 00 74 1D 8B 8E");
    injector::WriteMemory(pattern.get_first(1), *pattern.get_first<LONG>(1) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8B 8E ? ? ? ? 8B 51 04 52 FF 15 ? ? ? ? 8D A5");
    struct OpenWindowHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            auto[DesktopResW, DesktopResH] = GetDesktopRes();
            tagRECT rect;
            rect.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)Screen.Width / 2.0f));
            rect.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)Screen.Height / 2.0f));
            rect.right = (LONG)Screen.Width;
            rect.bottom = (LONG)Screen.Height;

            regs.ecx = *(uint32_t*)(regs.esi + 0x180);
            regs.edx = *(uint32_t*)(regs.ecx + 0x04);

            SetWindowLong((HWND)regs.edx, GWL_STYLE, GetWindowLong((HWND)regs.edx, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos((HWND)regs.edx, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
            SetForegroundWindow((HWND)regs.edx);
            SetCursor(NULL);
        }
    }; injector::MakeInline<OpenWindowHook2>(pattern.get_first(0), pattern.get_first(9));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init);
            CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
            CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
            CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}