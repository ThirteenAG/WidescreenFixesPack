#include "stdafx.h"
#include "GTA\CFileMgr.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    int32_t Width43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScaleX;
    float fHudPosX;
} Screen;

union HudPos
{
    uint32_t dwPos;
    float fPos;
    HudPos(uint32_t pos) : dwPos(pos) {}
    HudPos(float pos) : fPos(pos) {}
};

class CDatEntry
{
public:
    int32_t dwPosX;
    int32_t dwPosY;
    float fOffsetX;
    float fOffsetY;

    CDatEntry(uint32_t posx, uint32_t posy, float fOffsetX, float fOffsetY)
        : dwPosX(posx), dwPosY(posy), fOffsetX(fOffsetX), fOffsetY(fOffsetY)
    {}
};

std::vector<CDatEntry> HudCoords;
void LoadDatFile()
{
    CIniReader iniReader("");
    char* DataFilePath = (char*)iniReader.GetIniPath().c_str();
    *strrchr(DataFilePath, '.') = '\0';
    strcat(DataFilePath, ".dat");

    if (FILE* hFile = CFileMgr::OpenFile(DataFilePath, "r"))
    {
        while (const char* pLine = CFileMgr::LoadLine(hFile))
        {
            if (pLine[0] && pLine[0] != '#')
            {
                int32_t PosX, PosY;
                float fOffsetX, fOffsetY;
                sscanf(pLine, "%*s %x %x %f %f %*f %*f", &PosX, &PosY, &fOffsetX, &fOffsetY);

                if (Screen.fAspectRatio < (16.0f / 9.0f))
                {
                    fOffsetX /= (((16.0f / 9.0f) / Screen.fAspectRatio) * 1.5f);
                }

                HudCoords.push_back(CDatEntry(PosX, PosY, fOffsetX, fOffsetY));
            }
        }
        CFileMgr::CloseFile(hFile);
    }
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("68 20 03 00 00 BE 58 02 00 00");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bXbox360Scaling = iniReader.ReadInteger("MAIN", "Xbox360Scaling", 1) != 0;
    bool bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) != 0;
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    bool bCustomUsrDir = false;
    if (strncmp(szCustomUserFilesDirectoryInGameDir, "0", 1) != 0)
        bCustomUsrDir = true;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

    //menu
    uint32_t* dword_408A25 = hook::pattern("68 20 03 00 00 BE 58 02 00 00").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_408A25, Screen.Width, true);
    uint32_t dword_408A2A = (uint32_t)dword_408A25 + 5;
    injector::WriteMemory(dword_408A2A, Screen.Height, true);

    uint32_t* dword_4494C5 = hook::pattern("68 20 03 00 00 BE 58 02 00 00").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_4494C5, Screen.Width, true);
    uint32_t dword_4494CA = (uint32_t)dword_4494C5 + 5;
    injector::WriteMemory(dword_4494CA, Screen.Height, true);

    //game
    uint32_t* dword_408783 = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 77 75").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_408783, Screen.Width, true);
    uint32_t dword_408788 = (uint32_t)dword_408783 + 5;
    injector::WriteMemory(dword_408788, Screen.Height, true);

    injector::MakeNOP(dword_408788 + 4, 9, true);

    uint32_t* dword_408796 = hook::pattern("B8 80 02 00 00 BE E0 01 00 00 50").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_408796, Screen.Width, true);
    uint32_t dword_40879B = (uint32_t)dword_408796 + 5;
    injector::WriteMemory(dword_40879B, Screen.Height, true);
    uint32_t* dword_4087AF = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 50").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_4087AF, Screen.Width, true);
    uint32_t dword_4087B4 = (uint32_t)dword_4087AF + 5;
    injector::WriteMemory(dword_4087B4, Screen.Height, true);
    uint32_t* dword_4087C8 = hook::pattern("B8 00 04 00 00 BE 00 03 00 00 50").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_4087C8, Screen.Width, true);
    uint32_t dword_4087CD = (uint32_t)dword_4087C8 + 5;
    injector::WriteMemory(dword_4087CD, Screen.Height, true);
    uint32_t* dword_4087E1 = hook::pattern("B8 00 05 00 00 BE C0 03 00 00 50").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_4087E1, Screen.Width, true);
    uint32_t dword_4087E6 = (uint32_t)dword_4087E1 + 5;
    injector::WriteMemory(dword_4087E6, Screen.Height, true);

    //uint32_t* dword_4087FA = hook::pattern("BE 00 04 00 00 B8 00 05 00 00 50").count(1).get(0).get<uint32_t>(1);
    //injector::WriteMemory(dword_4087FA, Screen.Width, true);
    //uint32_t dword_4087FF = (uint32_t)dword_4087E1 + 5;
    //injector::WriteMemory(dword_4087FF, ResY, true);

    //__mbclen to strlen, "---" bug
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 76 21 8D 43 60");
    if (pattern.size() > 0)
    {
        auto dword_4148EA = pattern.get_first(0);
        injector::MakeCALL(dword_4148EA, strlen, true);
    }


    //HUD
    if (bFixHUD)
    {
        uint32_t* dword_6CC914 = *hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8 81 00 00 00 D8 ? ? ? ? ?").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_6CC914, Screen.fHudScaleX, true);

        //fHudScaleY = *(float*)0x6CCBA4;
        //CPatch::SetFloat(0x79AC14, fHudScaleY);

        for (size_t i = 0; i < 4; i++)
        {
            auto pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
            if (pattern.size() > 0)
            {
                uint32_t* dword_4F20A3 = pattern.get(0).get<uint32_t>(7);
                injector::WriteMemory<float>(dword_4F20A3, Screen.fHudPosX, true);
            }
            else
            {
                pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
                if (pattern.size() > 0)
                {
                    uint32_t* dword_4F2287 = pattern.get(0).get<uint32_t>(4);
                    injector::WriteMemory<float>(dword_4F2287, Screen.fHudPosX, true);
                }
            }
        }

        uint32_t* dword_4F1E2D = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").count(1).get(0).get<uint32_t>(4);
        injector::WriteMemory<float>(dword_4F1E2D, Screen.fHudPosX, true);
        uint32_t* dword_4F26E5 = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").count(1).get(0).get<uint32_t>(4);
        injector::WriteMemory<float>(dword_4F26E5, Screen.fHudPosX, true);

        uint32_t* dword_6CC910 = *hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_6CC910, Screen.fHudPosX, true);

        auto pattern = hook::pattern("C7 44 24 ? 00 00 DC 43 C7 44 24 ? 00 00 70 41");
        if (pattern.size() > 0)
        {
            injector::WriteMemory<float>(pattern.get_first(20), (Screen.fHudPosX - 320.0f) + 200.0f, true);
            injector::WriteMemory<float>(pattern.get_first(42), (Screen.fHudPosX - 320.0f) + 200.0f, true);
            injector::WriteMemory<float>(pattern.get_first(4),  (Screen.fHudPosX - 320.0f) + 440.0f, true);
            injector::WriteMemory<float>(pattern.get_first(64), (Screen.fHudPosX - 320.0f) + 440.0f, true);
        }

        pattern = hook::pattern("8D 72 04 83 C0 60 E8 ? ? ? ? 5F 5E C3"); //menu (#179)
        if (pattern.size() > 0)
            injector::MakeNOP(pattern.get_first(), 11, true);
    }

    if (bFixFOV)
    {
        static float hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 0.75f;
        static float mirrorScale = 0.45f;
        static float f1234 = 1.25f;
        static float f06 = 0.6f;
        static float f1 = 1.0f; // horizontal for vehicle reflection
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        if (bXbox360Scaling)
        {
            hor3DScale /= 1.0511562719f;
        }

        uint32_t* dword_40DDD2 = hook::pattern("DB 40 18 C7 44 24").count(1).get(0).get<uint32_t>(0);
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x20) = 1.0f;
                regs.eax = *(uint32_t*)(regs.ebp + 4);

                if (regs.eax == 1 || regs.eax == 4) //Headlights stretching, reflections etc 
                {
                    flt1 = hor3DScale;
                    flt2 = f06;
                    flt3 = f1234;
                }
                else
                {
                    if (regs.eax > 9)
                    {
                        flt1 = f1;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                        _asm fld ds : f1
                        return;
                    }
                    else
                    {
                        flt1 = 1.0f;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                    }
                }
                
                if (regs.eax == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                else
                    _asm fld ds : ver3DScale
            }
        }; injector::MakeInline<FOVHook>(dword_40DDD2, (uint32_t)dword_40DDD2 + 14);

        uint32_t* dword_40DEBB = hook::pattern("D8 3D ? ? ? ? D9 1F E8 ? ? ? ? D9").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_40DEBB, &flt1, true);

        // FOV being different in menus and in-game fix
        uint32_t* dword_40DE43 = hook::pattern("D8 0D ? ? ? ? ? ? E8 ? ? ? ? DD D8").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_40DE43, &flt2, true);

        uint32_t* dword_40DEEC = hook::pattern("D8 3D ? ? ? ? D9 E0 D9 5E 54 D9 44 24 20").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_40DEEC, &flt3, true);

        //Fixes vehicle reflection so that they're no longer broken and look exactly as they do without the widescreen fix.
        static uint16_t dx = 16400;
        uint32_t* dword_40B2F4 = hook::pattern("66 8B 15 ? ? ? ? 66 89 90 C4 00 00 00").count(1).get(0).get<uint32_t>(3);
        injector::WriteMemory(dword_40B2F4, &dx, true);
    }

    if (bFMVWidescreenMode)
    {
        uint32_t* dword_4F1FC3 = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<float>(dword_4F1FC3 + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
        injector::WriteMemory<float>((uint32_t)dword_4F1FC3 + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
        injector::WriteMemory<float>((uint32_t)dword_4F1FC3 + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
        injector::WriteMemory<float>((uint32_t)dword_4F1FC3 + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
    }

    if (bHudWidescreenMode && (Screen.fAspectRatio > (4.0f / 3.0f)))
    {
        LoadDatFile();
        static float fMinimapPosX = 320.0f;
        static float fMinimapPosY = 240.0f;

        static auto WidescreenHud = [](HudPos& HudPosX, HudPos& HudPosY, bool bAddHudOffset = false)
        {
            auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
                [&cc = CDatEntry(HudPosX.dwPos, HudPosY.dwPos, 0.0f, 0.0f)]
            (const CDatEntry& cde) -> bool { return (cc.dwPosX == cde.dwPosX && cc.dwPosY == cde.dwPosY); });

            if (it != HudCoords.end())
            {
                HudPosX.fPos += it->fOffsetX;
                HudPosY.fPos += it->fOffsetY;

                if (bAddHudOffset)
                {
                    HudPosX.fPos -= fMinimapPosX;
                    HudPosX.fPos += Screen.fHudPosX;
                }
            }
        };

        auto pattern = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04");
        if (pattern.size() > 0)
        {
            uint32_t* dword_4F18CB = pattern.count(1).get(0).get<uint32_t>(0);
            struct HudHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)(regs.edi + 0x1C);
                    regs.ecx = *(uint32_t*)(regs.edi + 0x20);
                    HudPos HudPosX = regs.eax;
                    HudPos HudPosY = regs.ecx;
                    WidescreenHud(HudPosX, HudPosY);
                    regs.eax = HudPosX.dwPos;
                    regs.ecx = HudPosY.dwPos;
                }
            }; injector::MakeInline<HudHook>((uint32_t)dword_4F18CB, (uint32_t)dword_4F18CB + 6);

            uint32_t* dword_58E5BD = hook::pattern("D8 05 ? ? ? ? D9 5B 68 D8 05 ? ? ? ? D9 5B 6C D9 40 3C").count(1).get(0).get<uint32_t>(2);
            injector::WriteMemory(dword_58E5BD, &fMinimapPosX, true);
            uint32_t dword_58E5C6 = (uint32_t)dword_58E5BD + 9;
            injector::WriteMemory(dword_58E5C6, &fMinimapPosY, true);


            uint32_t* dword_590DDC = hook::pattern("8B 4D 08 D9 41 68 53 57 8B 5D 0C 8B F8").count(1).get(0).get<uint32_t>(0);
            struct MinimapHook1
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(uint32_t*)(regs.ebp + 0x08);

                    HudPos HudPosX = *(uint32_t*)(regs.ecx + 0x68);
                    HudPos HudPosY = *(uint32_t*)(regs.ecx + 0x6C);
                    WidescreenHud(HudPosX, HudPosY, true);
                    *(uint32_t*)(regs.ecx + 0x68) = HudPosX.dwPos;
                    *(uint32_t*)(regs.ecx + 0x6C) = HudPosY.dwPos;
                     
                     _asm fld dword ptr [HudPosX.fPos]
                }
            }; injector::MakeInline<MinimapHook1>((uint32_t)dword_590DDC, (uint32_t)dword_590DDC + 6);

            uint32_t* dword_58FB1C = hook::pattern("D9 40 68 8B 4D 0C D8 60 70 53 56 8D 91 83 00 00 00 D9 54 24 34").count(1).get(0).get<uint32_t>(0);
            struct MinimapHook2
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(uint32_t*)(regs.ebp + 0x0C);

                    HudPos HudPosX = *(uint32_t*)(regs.eax + 0x68);
                    HudPos HudPosY = *(uint32_t*)(regs.eax + 0x6C);
                    WidescreenHud(HudPosX, HudPosY, true);
                    *(uint32_t*)(regs.eax + 0x68) = HudPosX.dwPos;
                    *(uint32_t*)(regs.eax + 0x6C) = HudPosY.dwPos;
                    _asm fld dword ptr[HudPosX.fPos]
                }
            }; injector::MakeInline<MinimapHook2>((uint32_t)dword_58FB1C, (uint32_t)dword_58FB1C + 6);

            uint32_t* dword_58E5B2 = hook::pattern("D9 40 1C 8B 4D 08 D9 40 20").count(1).get(0).get<uint32_t>(0);
            struct MinimapHook3
            {
                void operator()(injector::reg_pack& regs)
                {
                    HudPos HudPosX = *(uint32_t*)(regs.eax + 0x1C);
                    HudPos HudPosY = *(uint32_t*)(regs.eax + 0x20);
                    WidescreenHud(HudPosX, HudPosY, true);
                    *(uint32_t*)(regs.eax + 0x1C) = HudPosX.dwPos;
                    *(uint32_t*)(regs.eax + 0x20) = HudPosY.dwPos;
                    regs.ecx = *(uint32_t*)(regs.ebp + 0x8);
                    _asm fld dword ptr[HudPosX.fPos]
                    _asm fld dword ptr[HudPosY.fPos]
                }
            }; injector::MakeInline<MinimapHook3>((uint32_t)dword_58E5B2, (uint32_t)dword_58E5B2 + 9);

            uint32_t* dword_4F6DAB = hook::pattern("D8 43 1C 8B 54 24 18 8B 44 24 10 D9 1A 8B 4E 68 50 51 57").count(1).get(0).get<uint32_t>(0);
            struct LapsHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.edx = *(uint32_t*)(regs.esp + 0x18);

                    HudPos HudPosX = *(uint32_t*)(regs.ebx + 0x1C);
                    HudPos HudPosY = *(uint32_t*)(regs.ebx + 0x20);
                    WidescreenHud(HudPosX, HudPosY);
                    *(uint32_t*)(regs.ebx + 0x1C) = HudPosX.dwPos;
                    *(uint32_t*)(regs.ebx + 0x20) = HudPosY.dwPos;
                    _asm fadd dword ptr[HudPosX.fPos]

                }
            }; injector::MakeInline<LapsHook>((uint32_t)dword_4F6DAB, (uint32_t)dword_4F6DAB + 7);
        }
    }

    if (bCustomUsrDir)
    {
        char moduleName[MAX_PATH];
        GetModuleFileName(NULL, moduleName, MAX_PATH);
        *(strrchr(moduleName, '\\') + 1) = '\0';
        strcat(moduleName, szCustomUserFilesDirectoryInGameDir);
        strcpy(szCustomUserFilesDirectoryInGameDir, moduleName);

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
        {
            CreateDirectory(szCustomUserFilesDirectoryInGameDir, NULL);
            strcpy(pszPath, szCustomUserFilesDirectoryInGameDir);
            return S_OK;
        };

        auto dword_41C481 = *hook::pattern("68 ? ? ? ? 6A 00 6A 00 68 23 80 00 00 6A 00 FF 15").count(1).get(0).get<uint32_t*>(16 + 2);
        auto pattern = hook::pattern(pattern_str(0xFF, 0x15, to_bytes(dword_41C481)));
        for (size_t i = 0; i < pattern.size(); i++)
        {
            uint32_t* dword_6CBF17 = pattern.get(i).get<uint32_t>(0);
            injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
            injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
        }
    }

    if (nImproveGamepadSupport)
    {
        static char* GLOBALB = "scripts\\XBOXGLOBALB.BUN";
        static char* GLOBALB2 = "scripts\\PSGLOBALB.BUN";
        static char* FrontB = "scripts\\XBOXFrontB.BUN";
        static char* FrontB2 = "scripts\\PSFrontB.BUN";

        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B F0 83 C4 14 33 C9 33 C0"); //0x6B6E81 globalb
        if (nImproveGamepadSupport == 2)
            injector::WriteMemory(pattern.get_first(1), GLOBALB2, true);
        else
            injector::WriteMemory(pattern.get_first(1), GLOBALB, true);

        pattern = hook::pattern("68 ? ? ? ? C6 05 ? ? ? ? 01 C7 ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B F0 83 C4 14 33 C9"); //0x6B6E81 frontb
        if (nImproveGamepadSupport == 2)
            injector::WriteMemory(pattern.get_first(1), FrontB2, true);
        else
            injector::WriteMemory(pattern.get_first(1), FrontB, true);

        struct PadState
        {
            int32_t LSAxis1;
            int32_t LSAxis2;
            int32_t LTRT;
            int32_t RSAxis1;
            int32_t RSAxis2;
            uint8_t unk[28];
            int8_t A;
            int8_t B;
            int8_t X;
            int8_t Y;
            int8_t LB;
            int8_t RB;
            int8_t Select;
            int8_t Start;
            int8_t LSClick;
            int8_t RSClick;
        };

        static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate;
        pattern = hook::pattern("7C ? 5F 5D 5E 33 C0 5B C2 08 00"); //0x41989E
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8 + 5), 0x900008C2, true);
        //static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("").count(1).get(0).get<uint32_t*>(8); //0x
        static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").count(1).get(0).get<uint32_t*>(2); //0x77A920
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                PadState* PadKeyPresses = *(PadState**)(regs.esp + 0x4);
                //Keyboard 
                //006F9358 backspace
                //006F931C enter
                if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && *nGameState == 3)
                {
                    if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
                    {
                        if (!Qstate)
                        {
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Qstate = true;
                    }
                    else
                        Qstate = false;

                    if (PadKeyPresses->Y)
                    {
                        if (!Zstate)
                        {
                            keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Zstate = true;
                    }
                    else
                        Zstate = false;

                    if (PadKeyPresses->X)
                    {
                        if (!Pstate)
                        {
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Pstate = true;
                    }
                    else
                        Pstate = false;

                    if (PadKeyPresses->LSClick && !PadKeyPresses->RSClick)
                    {
                        if (!Tstate)
                        {
                            keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Tstate = true;
                    }
                    else
                        Tstate = false;

                    if (PadKeyPresses->RSClick && !PadKeyPresses->LSClick)
                    {
                        if (!Dstate)
                        {
                            keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Dstate = true;
                    }
                    else
                        Dstate = false;

                    if (PadKeyPresses->Select)
                    {
                        if (!Cstate)
                        {
                            keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Cstate = true;
                    }
                    else
                        Cstate = false;
                }
            }
        }; injector::MakeInline<CatchPad>(pattern.get_first(8));


        const wchar_t* ControlsTexts[] = { L" 0", L" 1", L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
        const wchar_t* ControlsTextsXBOX[] = { L"A", L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"LT / RT", L"D-pad" };
        const wchar_t* ControlsTextsPS[] = { L"Cross", L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

        static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
        static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
        static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

        pattern = hook::pattern("8D 43 60 8D 74 24 10 E8 ? ? ? ? 8B"); //0x4148F6
        struct Buttons
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = regs.ebx + 0x60;
                regs.esi = regs.esp + 0x10;

                auto pszStr = (wchar_t*)(regs.esp + 0x10);
                auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::wstring& str) { std::wstring s(pszStr); return s.find(str) != std::wstring::npos; });
                auto i = std::distance(Texts.begin(), it);

                if (it != Texts.end())
                {
                    if (nImproveGamepadSupport != 2)
                        wcscpy(pszStr, TextsXBOX[i].c_str());
                    else
                        wcscpy(pszStr, TextsPS[i].c_str());
                }
            }
        }; if (pattern.size() > 0) { injector::MakeInline<Buttons>(pattern.get_first(0), pattern.get_first(7)); }
    }

    if (fLeftStickDeadzone)
    {
        // [ -10000 | 10000 ] 
        static int32_t nLeftStickDeadzone = static_cast<int32_t>(fLeftStickDeadzone * 100.0f);
        pattern = hook::pattern("85 F6 7D 3D 8B 7C 24 18 57"); //0x4193B6
        static auto loc_4193F7 = (uint32_t)hook::get_pattern("83 7C 24 18 01 75 ? B9 04 00 00 00");
        static auto dword_71D8A8 = *hook::get_pattern<int32_t*>("8D 95 ? ? ? ? 8B F2 52 B9", 2);
        struct DeadzoneHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = regs.eax;
                if (*(int32_t*)&regs.esi >= 0)
                {
                    int32_t dStickStateX = *(int32_t*)(dword_71D8A8 + 0);
                    int32_t dStickStateY = *(int32_t*)(dword_71D8A8 + 1);

                    *(int32_t*)(dword_71D8A8 + 0) = (std::abs(dStickStateX) <= nLeftStickDeadzone) ? 0 : dStickStateX;
                    *(int32_t*)(dword_71D8A8 + 1) = (std::abs(dStickStateY) <= nLeftStickDeadzone) ? 0 : dStickStateY;
                    
                    *(uint32_t*)(regs.esp - 4) = loc_4193F7;
                }
            }
        }; injector::MakeInline<DeadzoneHook>(pattern.get_first(-2), pattern.get_first(4));
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, uint32_t reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}