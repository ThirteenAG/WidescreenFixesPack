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
    float fHudOffset;
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
    float fPosX;
    float fPosY;
    float fOffsetX;
    float fOffsetY;

    CDatEntry(float posx, float posy, float fOffsetX, float fOffsetY)
        : fPosX(posx), fPosY(posy), fOffsetX(fOffsetX), fOffsetY(fOffsetY)
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
                float PosX, PosY;
                float fOffsetX, fOffsetY;
                sscanf(pLine, "%*s %f %f %f %f", &PosX, &PosY, &fOffsetX, &fOffsetY);

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

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bXbox360Scaling = iniReader.ReadInteger("MAIN", "Xbox360Scaling", 1) != 0;
    bool bHUDWidescreenMode = iniReader.ReadInteger("MAIN", "HUDWidescreenMode", 1) != 0;
    int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    bool bDisableCutsceneBorders = iniReader.ReadInteger("MISC", "DisableCutsceneBorders", 1) != 0;
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) != 0;
    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    static int nFPSLimit= iniReader.ReadInteger("MISC", "FPSLimit", 120);
    bool b60FPSCutscenes = iniReader.ReadInteger("MISC", "60FPSCutscenes", 1) != 0;
    bool bSingleCoreAffinity = iniReader.ReadInteger("MISC", "SingleCoreAffinity", 0) != 0;
    bool bNoOpticalDriveFix = iniReader.ReadInteger("MISC", "NoOpticalDriveFix", 1) != 0;
    static float fRainDropletsScale = iniReader.ReadFloat("MISC", "RainDropletsScale", 0.5f);
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;

    //game
    uint32_t* dword_5BF62F = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00").count(1).get(0).get<uint32_t>(2);
    //injector::MakeNOP(dword_5BF62F - 10 - 13, 13, true);
    injector::WriteMemory(dword_5BF62F, Screen.Width, true);
    uint32_t* dword_5BF646 = hook::pattern("C7 02 20 03 00 00 C7 00 58 02 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF646, Screen.Width, true);
    uint32_t* dword_5BF65D = hook::pattern("C7 01 00 04 00 00 C7 02 00 03 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF65D, Screen.Width, true);
    uint32_t* dword_5BF674 = hook::pattern("C7 00 00 05 00 00 C7 01 C0 03 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF674, Screen.Width, true);
    uint32_t* dword_5BF68B = hook::pattern("C7 02 00 05 00 00 C7 00 00 04 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF68B, Screen.Width, true);
    uint32_t* dword_5BF6A2 = hook::pattern("C7 01 40 06 00 00 C7 02 B0 04 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF6A2, Screen.Width, true);

    uint32_t dword_5BF635 = (uint32_t)dword_5BF62F + 6;
    injector::WriteMemory(dword_5BF635, Screen.Height, true);
    uint32_t dword_5BF64C = (uint32_t)dword_5BF646 + 6;
    injector::WriteMemory(dword_5BF64C, Screen.Height, true);
    uint32_t dword_5BF663 = (uint32_t)dword_5BF65D + 6;
    injector::WriteMemory(dword_5BF663, Screen.Height, true);
    uint32_t dword_5BF67A = (uint32_t)dword_5BF674 + 6;
    injector::WriteMemory(dword_5BF67A, Screen.Height, true);
    uint32_t dword_5BF691 = (uint32_t)dword_5BF68B + 6;
    injector::WriteMemory(dword_5BF691, Screen.Height, true);
    uint32_t dword_5BF6A8 = (uint32_t)dword_5BF6A2 + 6;
    injector::WriteMemory(dword_5BF6A8, Screen.Height, true);


    //Rain droplets
    static float fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));
    auto pattern = hook::pattern("D9 44 24 0C D8 44 24 10 8B 44 24 08 8B 54 24 10 8B C8");
    struct RainDropletsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp0C = *(float*)(regs.esp + 0x0C);
            float esp10 = *(float*)(regs.esp + 0x10);
            _asm {fld dword ptr[esp0C]}
            _asm {fmul dword ptr[fRainScaleX]}
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp10]}
        }
    }; injector::MakeInline<RainDropletsHook>(pattern.get_first(0), pattern.get_first(8)); //5C7C35

    struct RainDropletsYScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp08 = *(float*)(regs.esp + 0x08);
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp08]}
            *(uintptr_t*)(regs.esp + 0x38) = regs.eax;
        }
    }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(30), pattern.get_first(30 + 8)); //5C7C53


    //HUD
    if (bFixHUD)
    {
        uint32_t* dword_79AC10 = *hook::pattern("D9 05 ? ? ? ? 89 7C 24 28 C7 44 24 2C 00 00 00 3F D8 C9 C7 44 24 30 00 00 00 3F").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_79AC10, Screen.fHudScaleX, true);

        //fHudScaleY = *(float*)0x79AC14;
        //CPatch::SetFloat(0x79AC14, fHudScaleY);

        uint32_t* dword_51B3CB = hook::pattern("C7 84 24 A0 00 00 00 00 00 A0 43 C7 84 24 A4 00 00 00 00 00 70 43 C7 84 24 A8 00 00 00 00 00 00 00 0F B7 48 20").count(1).get(0).get<uint32_t>(7);
        injector::WriteMemory<float>(dword_51B3CB, Screen.fHudPosX, true);
        uint32_t* dword_5368C8 = hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 4C 24 70").count(1).get(0).get<uint32_t>(4);
        injector::WriteMemory<float>(dword_5368C8, Screen.fHudPosX, true);
        uint32_t* dword_536A99 = hook::pattern("C7 84 24 84 00 00 00 00 00 A0 43 C7 84 24 88 00 00 00 00 00 70 43 C7 84 24 8C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 80 00 00 00").count(1).get(0).get<uint32_t>(7);
        injector::WriteMemory<float>(dword_536A99, Screen.fHudPosX, true); //minimap
        uint32_t* dword_536CC9 = hook::pattern("C7 84 24 94 00 00 00 00 00 A0 43 C7 84 24 98 00 00 00 00 00 70 43 C7 84 24 9C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 90 00 00 00").count(1).get(0).get<uint32_t>(7);
        injector::WriteMemory<float>(dword_536CC9, Screen.fHudPosX, true);
        uint32_t* dword_537011 = hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 54 24 70 52").count(1).get(0).get<uint32_t>(4);
        injector::WriteMemory<float>(dword_537011, Screen.fHudPosX, true);
        uint32_t* dword_48B640 = hook::pattern("C7 05 ? ? ? ? 00 00 A0 43 C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 82").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<float>(dword_48B640, Screen.fHudPosX, true);
        uint32_t* dword_50B4F5 = hook::pattern("C7 84 24 A0 01 00 00 00 00 A0 43 C7 84 24 A4 01 00 00 00 00 70 43 C7 84 24 A8 01 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 30 01 00 00 51 8B F0").count(1).get(0).get<uint32_t>(7);
        injector::WriteMemory<float>(dword_50B4F5, Screen.fHudPosX, true);
        uint32_t* dword_797D50 = *hook::pattern("D8 25 ? ? ? ? 8B 4C 24 28 6A 00 8D 54 24 48 D9 5C 24 48 89 4C 24 50 D9 44 24 28 8B 0B").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_797D50, Screen.fHudPosX, true);
        injector::WriteMemory<float>(dword_797D50 + 2, Screen.fHudPosX * 2.0f, true);

        uint32_t* dword_5CC109 = hook::pattern("C7 44 24 40 00 00 48 43 C7 44 24 44 00 00 70 41 C7 44 24 50 00 00 48 43 C7 44 24 54 00 00 BE 42 C7 44 24 60 00 00 DC 43").count(1).get(0).get<uint32_t>(4); //rearview mirror
        uint32_t dword_5CC119 = (uint32_t)dword_5CC109 + 16;
        uint32_t dword_5CC0F9 = (uint32_t)dword_5CC109 - 16;
        uint32_t dword_5CC129 = (uint32_t)dword_5CC109 + 32;
        injector::WriteMemory<float>(dword_5CC109, (Screen.fHudPosX - 320.0f) + 200.0f, true);
        injector::WriteMemory<float>(dword_5CC119, (Screen.fHudPosX - 320.0f) + 200.0f, true);
        injector::WriteMemory<float>(dword_5CC0F9, (Screen.fHudPosX - 320.0f) + 440.0f, true);
        injector::WriteMemory<float>(dword_5CC129, (Screen.fHudPosX - 320.0f) + 440.0f, true);

        uint32_t* dword_5CBEF5 = hook::pattern("C7 44 24 38 00 00 20 44 C7 44 24 40 00 00 00 00 C7 44 24 48 00 00").count(1).get(0).get<uint32_t>(4);
        uint32_t  dword_5CBF05 = (uint32_t)dword_5CBEF5 + 16;
        uint32_t* dword_5CBE89 = hook::pattern("C7 44 24 2C 00 00 20 44 C7 44 24 30 00 00 00 00 C7 44 24 34 00 00 00 00 C7 44 24 3C 00 00 20 44 C7 44 24 44 00 00 00 00").count(1).get(0).get<uint32_t>(4);
        uint32_t  dword_5CBEA1 = (uint32_t)dword_5CBE89 + 24;
        injector::WriteMemory<float>(dword_5CBEF5, Screen.fWidth, true); // borders
        injector::WriteMemory<float>(dword_5CBF05, Screen.fWidth, true);
        injector::WriteMemory<float>(dword_5CBE89, Screen.fWidth, true);
        injector::WriteMemory<float>(dword_5CBEA1, Screen.fWidth, true);

        if (bDisableCutsceneBorders)
        {
            injector::WriteMemory<float>(dword_5CBEF5, 0.0f, true); // borders
            injector::WriteMemory<float>(dword_5CBF05, 0.0f, true);
            injector::WriteMemory<float>(dword_5CBE89, 0.0f, true);
            injector::WriteMemory<float>(dword_5CBEA1, 0.0f, true);
        }

        uint32_t* dword_5C726A = hook::pattern("C7 44 24 30 00 00 20 44 C7 44 24 34 00 00 00 00 C7 44 24 40 00 00 20 44 C7 44 24 44 00 00 F0 43 C7 44 24 50 00 00 00 00 C7 44 24 54 00 00 F0 43 7D 06").count(1).get(0).get<uint32_t>(4);
        injector::WriteMemory<float>(dword_5C726A, Screen.fWidth * 2.0f, true); // radar mask
        uint32_t  dword_5C727A = (uint32_t)dword_5C726A + 16;
        injector::WriteMemory<float>(dword_5C727A, Screen.fWidth * 2.0f, true);
        uint32_t  dword_5C7282 = (uint32_t)dword_5C726A + 24;
        injector::WriteMemory<float>(dword_5C7282, Screen.fHeight * 2.0f, true);
        uint32_t  dword_5C7292 = (uint32_t)dword_5C726A + 40;
        injector::WriteMemory<float>(dword_5C7292, Screen.fHeight * 2.0f, true);

        //graph of the dyno result issue #366 / #462
        auto flt_7FA3C0 = *hook::get_pattern<float*>("6A 04 68 ? ? ? ? 68 ? ? ? ? 8B C8", 8); //53D082
        injector::WriteMemory<float>(flt_7FA3C0, 79.0f + Screen.fHudOffset, true);
        injector::WriteMemory<float>(flt_7FA3C0 + 1, 60.0f, true);

        //controls screenshot aspect ratio issue #462
        auto pattern = hook::pattern("DA 0D ? ? ? ? 55 56 8B F0 A1 ? ? ? ? 99"); //5C30AC
        injector::WriteMemory(pattern.get_first(2), &Screen.Width43, true);
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

        uint32_t* dword_5C7F56 = hook::pattern("DB 40 18 C7 44 24 20 00 00 80 3F DA 70 14").count(1).get(0).get<uint32_t>(0);
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.ebp + 0x58);

                if (regs.ecx == 1 || regs.ecx == 4) //Headlights stretching, reflections etc 
                {
                    flt1 = hor3DScale;
                    flt2 = f06;
                    flt3 = f1234;
                }
                else
                {
                    if (regs.ecx > 10)
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

                if (regs.ecx == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                else
                    _asm fld ds : ver3DScale
            }
        }; injector::MakeInline<FOVHook>((uint32_t)dword_5C7F56 - 6, (uint32_t)dword_5C7F56 + 3);
        injector::WriteMemory((uint32_t)dword_5C7F56 - 1, 0x9001F983, true); //cmp     ecx, 1
        injector::MakeNOP((uint32_t)dword_5C7F56 + 3 + 8, 3, true); //nop fidiv

        uint32_t* dword_5C7FE8 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 1C DB 44 24 2C D8 4C 24 28 D8 0D ? ? ? ? E8").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_5C7FE8, &flt1, true);

        // FOV being different in menus and in-game fix
        uint32_t* dword_5C7FFA = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B D8 53").count(2).get(1).get<uint32_t>(2);
        injector::WriteMemory(dword_5C7FFA, &flt2, true);

        uint32_t* dword_5C801F = hook::pattern("D8 3D ? ? ? ? D9 5C 24 38 D9 44 24 24 D8 64 24 30 D8 7C 24 24 D9 5C 24 34").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_5C801F, &flt3, true);

        //Fixes vehicle reflection so that they're no longer broken and look exactly as they do without the widescreen fix.
        static uint16_t dx = 16400;
        uint32_t* dword_5C4FC9 = hook::pattern("66 A1 ? ? ? ? 66 89 86 C4 00 00 00").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_5C4FC9, &dx, true);
    }

    if (nFMVWidescreenMode)
    {
        uint32_t* dword_536A1F = hook::pattern("68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B 44 24 18 8B CB").count(1).get(0).get<uint32_t>(1);
        uint32_t  dword_536A24 = (uint32_t)dword_536A1F + 5;
        uint32_t  dword_536A29 = (uint32_t)dword_536A1F + 10;
        uint32_t  dword_536A2E = (uint32_t)dword_536A1F + 15;
        injector::WriteMemory<float>(dword_536A1F, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Bottom)
        injector::WriteMemory<float>(dword_536A24, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
        injector::WriteMemory<float>(dword_536A29, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Top)
        injector::WriteMemory<float>(dword_536A2E, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)

        if (nFMVWidescreenMode > 1)
        {
            injector::WriteMemory<float>(dword_536A1F, (0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Bottom)
            injector::WriteMemory<float>(dword_536A24, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
            injector::WriteMemory<float>(dword_536A29, -(0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Top)
            injector::WriteMemory<float>(dword_536A2E, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)
        }
    }

    if (bHUDWidescreenMode)
    {
        LoadDatFile();

        static auto WidescreenHud = [](HudPos& HudPosX, HudPos& HudPosY)
        {
            auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
                [&cc = CDatEntry(std::floor(HudPosX.fPos), std::floor(HudPosY.fPos), 0.0f, 0.0f)]
            (const CDatEntry& cde) -> bool { return (cc.fPosX == cde.fPosX && cc.fPosY == cde.fPosY); });

            if (it != HudCoords.end())
            {
                HudPosX.fPos += it->fOffsetX;
                HudPosY.fPos += it->fOffsetY;
            }
        };

        uint32_t* dword_51B190 = hook::pattern("89 4C 24 60 89 54 24 64 74 ? 8D 8C 24 F0 00 00 00 51").count(1).get(0).get<uint32_t>(0);
        struct HudHook
        {
            void operator()(injector::reg_pack& regs)
            {
                HudPos HudPosX = regs.ecx;
                HudPos HudPosY = regs.edx;
                WidescreenHud(HudPosX, HudPosY);
                *(uint32_t*)(regs.esp + 0x60) = HudPosX.dwPos;
                *(uint32_t*)(regs.esp + 0x64) = HudPosY.dwPos;
            }
        }; injector::MakeInline<HudHook>((uint32_t)dword_51B190, (uint32_t)dword_51B190 + 8);

        uint32_t* dword_51D64E = hook::pattern("8B 4B 1C 8B 54 24 18 89 0A 8B 43 20").count(1).get(0).get<uint32_t>(0);
        struct BlipsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                HudPos HudPosX = *(uint32_t*)(regs.ebx + 0x1C);
                HudPos HudPosY = *(uint32_t*)(regs.ebx + 0x20);
                WidescreenHud(HudPosX, HudPosY);
                *(uint32_t*)(regs.ebx + 0x1C) = HudPosX.dwPos;
                *(uint32_t*)(regs.ebx + 0x20) = HudPosY.dwPos;

                *(uint32_t*)(regs.edx) = *(uint32_t*)(regs.ebx + 0x1C);
                regs.eax = *(uint32_t*)(regs.ebx + 0x20);
            }
        }; injector::MakeInline<BlipsHook>((uint32_t)dword_51D64E + 7);

        uint32_t* dword_4C66B3 = hook::pattern("D9 56 48 8B 4E 1C D8 2D ? ? ? ? 89 4C 24 28 8B 56 20 89 54 24 2C 8B 46 24").count(1).get(0).get<uint32_t>(0); //addresses from 1.1 exe
        struct HudHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                float esi48 = 0.0f;
                _asm {fst dword ptr[esi48]}
                *(float*)(regs.esi + 0x48) = esi48;
                regs.ecx = *(uint32_t*)(regs.esi + 0x1C);
                regs.edx = *(uint32_t*)(regs.esi + 0x20);
                HudPos HudPosX = regs.ecx;
                HudPos HudPosY = regs.edx;
                WidescreenHud(HudPosX, HudPosY);
                regs.ecx = HudPosX.dwPos;
            }
        }; injector::MakeInline<HudHook2>((uint32_t)dword_4C66B3, (uint32_t)dword_4C66B3 + 6);

        uint32_t* dword_5050FB = hook::pattern("D8 02 D9 1E D9 41 04 D8 60 04 D8 4C 24 10 D8 42 04 D8 40 04 D9 5E 04 D9 41 08 D8 60 08").count(1).get(0).get<uint32_t>(0); //addresses from 1.1 exe
        struct StopSignHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float esi00 = 0.0f;
                float ecx4 = *(float*)(regs.ecx + 0x04);
                HudPos HudPosX = *(uint32_t*)(regs.edx + 0x00);
                HudPos HudPosY = *(uint32_t*)(regs.edx + 0x04);
                WidescreenHud(HudPosX, HudPosY);
                _asm {fadd dword ptr[HudPosX.fPos]}
                _asm {fstp dword ptr[esi00]}
                *(float*)(regs.esi + 0x00) = esi00;
                _asm fld dword ptr[ecx4]
            }
        }; injector::MakeInline<StopSignHook>((uint32_t)dword_5050FB, (uint32_t)dword_5050FB + 7);

        //uint32_t* dword_52C1B0 = hook::pattern("D9 07 8B 44 24 ? D8 01 D9 18 D9 41 ? D8 47 ? D9 58").count(1).get(0).get<uint32_t>(0);
        //struct HudHook3
        //{
        //    void operator()(injector::reg_pack& regs)
        //    {
        //        regs.eax = *(uint32_t*)(regs.esp + 0x34);
        //
        //        HudPos HudPosX = *(uint32_t*)(regs.edi + 0x00);
        //        HudPos HudPosY = *(uint32_t*)(regs.edi + 0x04);
        //        WidescreenHud(HudPosX, HudPosY);
        //        *(uint32_t*)(regs.edi + 0x00) = HudPosX.dwPos;
        //        *(uint32_t*)(regs.edi + 0x04) = HudPosY.dwPos;
        //        _asm fld  dword ptr[regs.edi]
        //    }
        //}; injector::MakeInline<HudHook3>((uint32_t)dword_52C1B0, (uint32_t)dword_52C1B0 + 6);
    }


    if (!szCustomUserFilesDirectoryInGameDir.empty())
    {
        szCustomUserFilesDirectoryInGameDir = GetExeModulePath<std::string>() + szCustomUserFilesDirectoryInGameDir;

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
        {
            CreateDirectoryA(szCustomUserFilesDirectoryInGameDir.c_str(), NULL);
            strcpy(pszPath, szCustomUserFilesDirectoryInGameDir.c_str());
            return S_OK;
        };

        auto pattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        uint32_t* dword_6CBF17 = pattern.count(1).get(0).get<uint32_t>(12);
        injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
        injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
    }

    if (nImproveGamepadSupport)
    {
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 89 44 24 00 85 C0 C7 44 24 ? ? ? ? ? 74 22 8B 4C 24 24");
        static auto CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x0057CEA0
        pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 86 ? ? ? ? 89 8E");
        static auto ResourceFileBeginLoading = (void(__thiscall *)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x0057BCA0
        static auto LoadResourceFile = [](const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
        {
            auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
            ResourceFileBeginLoading(r, nUnk4, nUnk5);
        };

        if (nImproveGamepadSupport < 4)
        {
            static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length()) + "test.tpk";

            static injector::hook_back<void(__cdecl*)()> hb_57FA60;
            auto LoadTPK = []()
            {
                LoadResourceFile(TPKPath.c_str(), 1);
                return hb_57FA60.fun();
            };

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 5F C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 5E"); //0x57ECEF
            hb_57FA60.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();
        }

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

        static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate, LBRACKETstate, RBRACKETstate;
        static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").count(1).get(0).get<uint32_t*>(2);
        pattern = hook::pattern("B9 ? ? ? ? BE ? ? ? ? F3 A5 5F 5E 5B C3");
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = 0x88;
                PadState* PadKeyPresses = (PadState*)(regs.edi);

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

                    /*if (PadKeyPresses->Y)
                    {
                        if (!Zstate)
                        {
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Zstate = true;
                    }
                    else
                        Zstate = false;
                        */
                }
            }
        }; injector::MakeInline<CatchPad>(pattern.get_first(0));

        if (nImproveGamepadSupport < 4)
        {
            const char* MenuTexts[] = { "Quit Game", "Continue", "Back", "Reset Keys To Default", "Activate GPS", "Deactivate GPS", "Install Package", "Install Part", "Install Paint", "Install Decal", "Reset To Default", "Delete Tuned Car", "Logoff", "Cancel Changes", "Customize", "Host LAN Server", "Read Message", "Delete", "Test N2O Purge", "Accept", "Reset to default", "Select", "Open/Close Doors", "Open/Close Hood" }; // "Tip", 
            const char* MenuTextsPC[] = { "[Q] Quit Game", "[Enter] Continue", "[Esc] Back", "[P] Reset Keys To Default", "[C] Activate GPS", "[C] Deactivate GPS", "[Enter] Install Package", "[Enter] Install Part", "[Enter] Install Paint", "[Enter] Install Decal", "[C] Reset To Default", "[C] Delete Tuned Car", "[Esc] Logoff", "[C] Cancel Changes", "[C] Customize", "[C] Host LAN Server", "[Enter] Read Message", "[C] Delete", "[C] Test N2O Purge", "[Enter] Accept", "[C] Reset to default", "[Enter] Select", "[C] Open/Close Doors", "[C] Open/Close Hood" };
            const char* MenuTextsXBOX[] = { "(LS+RS) Quit Game", "(A) Continue", "(B) Back", "(Y) Reset Keys To Default", "(X) Activate GPS", "(X) Deactivate GPS", "(A) Install Package", "(A) Install Part", "(A) Install Paint", "(A) Install Decal", "(X) Reset To Default", "(X) Delete Tuned Car", "(B) Logoff", "(X) Cancel Changes", "(X) Customize", "(X) Host LAN Server", "(A) Read Message", "(X) Delete", "(X) Test N2O Purge", "(A) Accept", "(X) Reset to default", "(A) Select", "(X) Open/Close Doors", "(X) Open/Close Hood" };
            const char* MenuTextsPS[] = { "(L3+R3) Quit Game", "(Cross) Continue", "(Circle) Back", "(Triangle) Reset Keys To Default", "(Square) Activate GPS", "(Square) Deactivate GPS", "(Cross) Install Package", "(Cross) Install Part", "(Cross) Install Paint", "(Cross) Install Decal", "(Square) Reset To Default", "(Square) Delete Tuned Car", "(Circle) Logoff", "(Square) Cancel Changes", "(Square) Customize", "(Square) Host LAN Server", "(Cross) Read Message", "(Square) Delete", "(Square) Test N2O Purge", "(Cross) Accept", "(Square) Reset to default", "(Cross) Select", "(Square) Open/Close Doors", "(Square) Open/Close Hood" };

            static std::vector<std::string> vMenuStrings(MenuTexts, std::end(MenuTexts));
            static std::vector<std::string> vMenuStringsPC(MenuTextsPC, std::end(MenuTextsPC));
            static std::vector<std::string> vMenuStringsXBOX(MenuTextsXBOX, std::end(MenuTextsXBOX));
            static std::vector<std::string> vMenuStringsPS(MenuTextsPS, std::end(MenuTextsPS));

            pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 50 74 12 8B 44"); //0x5124DD (v1.1)
            static auto dword_8383DC = *pattern.count(1).get(0).get<uint32_t>(2);
            struct MenuText
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = dword_8383DC;

                    auto pszStr = (char*)regs.eax;
                    auto it = std::find(vMenuStrings.begin(), vMenuStrings.end(), pszStr);
                    auto i = std::distance(vMenuStrings.begin(), it);

                    if (it != vMenuStrings.end())
                    {
                        if (nImproveGamepadSupport == 3)
                            regs.eax = (uint32_t)vMenuStringsPC[i].c_str();
                        else
                            if (nImproveGamepadSupport != 2)
                                regs.eax = (uint32_t)vMenuStringsXBOX[i].c_str();
                            else
                                regs.eax = (uint32_t)vMenuStringsPS[i].c_str();
                    }
                }
            }; injector::MakeInline<MenuText>(pattern.get_first(0), pattern.get_first(6));
        }

        // Start menu text
        uint32_t* dword_4A91E7 = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 50 55 E8").count(3).get(1).get<uint32_t>(1);
        if (nImproveGamepadSupport != 2)
            injector::WriteMemory(dword_4A91E7, 0xD18D4C4C, true); //"Please press START to begin" (Xbox)
        else
            injector::WriteMemory(dword_4A91E7, 0xDC64C04C, true); //"Press START Button" (PlayStation)

        // FrontEnd button remap (through game code, not key emulation)
        auto pattern = hook::pattern("89 4E ? 89 4E ? 8B E8"); // 005C1A07
        struct FrontEndRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uintptr_t*)(regs.esi + 0x08) = regs.ecx;
                *(uintptr_t*)(regs.esi + 0x0C) = regs.ecx;

                int Y_Button = (regs.eax + 0x5064);
                *(uintptr_t*)Y_Button = 0x2E; // FE Action "P"
                int Start = (regs.eax + 0x5074);
                *(uintptr_t*)Start = 0x24; // FE Action "Comma"
                int LB = (regs.eax + 0x5068); 
                *(uintptr_t*)LB = 0x26; // FE Action "Left Bracket"
                int RB = (regs.eax + 0x506C);
                *(uintptr_t*)RB = 0x27; // FE Action "Right Bracket"
            }
        };
        injector::MakeInline<FrontEndRemap>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
    }

    const wchar_t* ControlsTexts[] = { L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" 10", L" 1", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
    const wchar_t* ControlsTextsXBOX[] = { L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"A", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"LT / RT", L"D-pad" };
    const wchar_t* ControlsTextsPS[] = { L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"Cross", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

    static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
    static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
    static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

    pattern = hook::pattern("66 83 7C 24 08 00 5F 74 13 8D 4C 24 04 51"); //0x4ACF01
    injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x5F, true); //pop     edi
    static auto dword_4ACF1D = pattern.count(1).get(0).get<uint32_t>(28);
    struct Buttons
    {
        void operator()(injector::reg_pack& regs)
        {
            auto pszStr = (wchar_t*)(regs.esp + 0x4);
            if (wcslen(pszStr))
            {
                if (nImproveGamepadSupport)
                {
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
            }
            else
                *(uintptr_t*)(regs.esp - 4) = (uintptr_t)dword_4ACF1D;
        }
    }; injector::MakeInline<Buttons>(pattern.get_first(1), pattern.get_first(9));

    if (fLeftStickDeadzone)
    {
        // [ -10000 | 10000 ] 
        static int32_t nLeftStickDeadzone = static_cast<int32_t>(fLeftStickDeadzone * 100.0f);
        pattern = hook::pattern("85 F6 7D 38 53 68 ? ? ? ? E8"); //0x5C877D
        static auto loc_5C87B9 = (uint32_t)hook::get_pattern("83 FB 01 75 ? B9 04 00 00 00");
        static auto dword_86FFC0 = *hook::get_pattern<int32_t*>("8D 95 ? ? ? ? 8D 3C 28 8B", 2);
        struct DeadzoneHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = regs.eax;
                if (*(int32_t*)&regs.esi >= 0)
                {
                    int32_t dStickStateX = *(int32_t*)(dword_86FFC0 + 0);
                    int32_t dStickStateY = *(int32_t*)(dword_86FFC0 + 1);

                    *(int32_t*)(dword_86FFC0 + 0) = (std::abs(dStickStateX) <= nLeftStickDeadzone) ? 0 : dStickStateX;
                    *(int32_t*)(dword_86FFC0 + 1) = (std::abs(dStickStateY) <= nLeftStickDeadzone) ? 0 : dStickStateY;

                    *(uint32_t*)regs.esp = loc_5C87B9;
                }
            }
        }; injector::MakeInline<DeadzoneHook>(pattern.get_first(-2), pattern.get_first(4));
    }

    if (nFPSLimit)
    {
        static float FrameTime = 1.0f / nFPSLimit;
        uint32_t* dword_865558 = *hook::pattern("D9 05 ? ? ? ? B9 64 00 00 00 D8 64").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory(dword_865558, FrameTime, true);
        uint32_t* dword_7FB710 = *hook::pattern("D9 05 ? ? ? ? D8 74 ? ? D9 1D ? ? ? ? C3").count(1).get(0).get<uint32_t*>(33);
        injector::WriteMemory(dword_7FB710, FrameTime, true);
    }

    if (b60FPSCutscenes)
    {
        static float flt60 = 60.0f;
        uint32_t* dword_435FA4 = hook::pattern("7C ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 5F 5E").count(1).get(0).get<uint32_t>(3);
        injector::WriteMemory(dword_435FA4, flt60, true);
        uint32_t* dword_7A572C = *hook::pattern("8B 15 ? ? ? ? 52 A3 ? ? ? ? E8 ? ? ? ? 83 C4 14").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory(dword_7A572C, flt60, true);
    }

    if (bSingleCoreAffinity)
    {
        SetProcessAffinityMask(GetCurrentProcess(), 1);
    }

    if (bNoOpticalDriveFix)
    {
        auto pattern = hook::pattern("56 8B 35 ? ? ? ? A1 ? ? ? ? 85 C0 ? ? E8");
        uint32_t* dword_5C0D3F = pattern.count(1).get(0).get<uint32_t>(15);
        injector::WriteMemory<uint8_t>(dword_5C0D3F, 0x78, true);
        uint32_t* dword_5C0D54 = pattern.count(1).get(0).get<uint32_t>(36);
        injector::WriteMemory<uint8_t>(dword_5C0D54, 0x00, true);
    }

    if (bWriteSettingsToFile)
    {
        char szSettingsSavePath[MAX_PATH];
        auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.count(1).get(0).get<uintptr_t>(12), true).as_int();
        injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x801C, NULL, NULL, szSettingsSavePath);
        strcat(szSettingsSavePath, "\\NFS Underground 2");
        strcat(szSettingsSavePath, "\\Settings.ini");

        RegistryWrapper("Need for Speed", szSettingsSavePath);
        uintptr_t* RegIAT = *hook::pattern("FF 15 ? ? ? ? 8D 54 24 04 52").get(0).get<uintptr_t*>(2);
        injector::WriteMemory(&RegIAT[0], RegistryWrapper::RegCreateKeyA, true);
        injector::WriteMemory(&RegIAT[-1], RegistryWrapper::RegOpenKeyA, true);
        injector::WriteMemory(&RegIAT[-2], RegistryWrapper::RegOpenKeyExA, true);
        injector::WriteMemory(&RegIAT[1], RegistryWrapper::RegCloseKey, true);
        injector::WriteMemory(&RegIAT[-4], RegistryWrapper::RegSetValueExA, true);
        injector::WriteMemory(&RegIAT[-3], RegistryWrapper::RegQueryValueExA, true);
        RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
        RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
        RegistryWrapper::AddDefault("CD Drive", "D:\\");
        RegistryWrapper::AddDefault("CacheSize", "1879040000");
        RegistryWrapper::AddDefault("SwapSize", "0");
        RegistryWrapper::AddDefault("Language", "English US");
        RegistryWrapper::AddDefault("StreamingInstall", "0");
        RegistryWrapper::AddDefault("g_CarEnvironmentMapEnable", "3");
        RegistryWrapper::AddDefault("g_CarEnvironmentMapUpdateData", "1");
        RegistryWrapper::AddDefault("g_CarShadowEnable", "2");
        RegistryWrapper::AddDefault("g_CarHeadlightEnable", "1");
        RegistryWrapper::AddDefault("g_CarLightingEnable", "0");
        RegistryWrapper::AddDefault("g_CarDamageEnable", "0");
        RegistryWrapper::AddDefault("g_CrowdEnable", "1");
        RegistryWrapper::AddDefault("g_RoadReflectionEnable", "3");
        RegistryWrapper::AddDefault("g_FogEnable", "1");
        RegistryWrapper::AddDefault("g_MotionBlurEnable", "1");
        RegistryWrapper::AddDefault("g_LightStreaksEnable", "1");
        RegistryWrapper::AddDefault("g_LightGlowEnable", "1");
        RegistryWrapper::AddDefault("g_AnimatedTextureEnable", "1");
        RegistryWrapper::AddDefault("g_ParticleSystemEnable", "1");
        RegistryWrapper::AddDefault("g_DepthOfFieldEnable", "1");
        RegistryWrapper::AddDefault("g_WorldLodLevel", "3");
        RegistryWrapper::AddDefault("g_CarLodLevel", "1");
        RegistryWrapper::AddDefault("g_OverBrightEnable", "1");
        RegistryWrapper::AddDefault("g_BleachByPassEnable", "1");
        RegistryWrapper::AddDefault("g_TintingEnable", "1");
        RegistryWrapper::AddDefault("g_FSAALevel", "7");
        RegistryWrapper::AddDefault("g_HorizonFogEnable", "1");
        RegistryWrapper::AddDefault("g_RainEnable", "1");
        RegistryWrapper::AddDefault("g_TextureFiltering", "2");
        RegistryWrapper::AddDefault("g_RacingResolution", "4");
        RegistryWrapper::AddDefault("g_PerformanceLevel", "5");
        RegistryWrapper::AddDefault("g_VSyncOn", "0");
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00"));
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
