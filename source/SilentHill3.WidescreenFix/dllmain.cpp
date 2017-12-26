#include "stdafx.h"

bool bConfigTool;

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudOffset;
    int32_t Width43;
} Screen;

LONG WINAPI RegQueryValueExAHook(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    if (strstr(lpValueName, "movie") || strstr(lpValueName, "sound") || strstr(lpValueName, "data") || strstr(lpValueName, "save") || strstr(lpValueName, "installdir"))
    {
        if (lpData == NULL)
        {
            char temp[MAX_PATH];
            GetModuleFileName(NULL, temp, MAX_PATH);
            *(strrchr(temp, '\\') + 1) = '\0';
            strstr(lpValueName, "save") ? strcat_s(temp, "savedata\\") : 0;
            *lpcbData = std::size(temp);
            *lpType = 1;
        }
        else
        {
            GetModuleFileName(NULL, (char*)lpData, MAX_PATH);
            *(strrchr((char*)lpData, '\\') + 1) = '\0';
            strstr(lpValueName, "save") ? strcat((char*)lpData, "savedata\\") : 0;
        }
        return ERROR_SUCCESS;
    }
    return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

LONG WINAPI RegOpenKeyExAHook(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM  samDesired, PHKEY phkResult)
{
    if (strstr(lpSubKey, "KONAMI"))
        return ERROR_SUCCESS;

    return RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

LONG WINAPI RegDeleteKeyAHook(HKEY hKey, LPCTSTR lpSubKey)
{
    return ERROR_SUCCESS;
}

LONG WINAPI RegEnumKeyAHook(HKEY hKey, DWORD dwIndex, LPTSTR lpName, DWORD cchName)
{
    return ERROR_SUCCESS;
}

LONG WINAPI RegOpenKeyAHook(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult)
{
    return ERROR_SUCCESS;
}

LONG WINAPI RegQueryValueAHook(HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpValue, PLONG lpcbValue)
{
    return ERROR_SUCCESS;
}

LONG WINAPI RegCreateKeyExAHook(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    return ERROR_SUCCESS;
}

LONG WINAPI RegSetValueExAHook(HKEY hKey, LPCTSTR lpValueName,DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData)
{
    return ERROR_SUCCESS;
}

LONG WINAPI RegCloseKeyHook(HKEY hKey)
{
    return ERROR_SUCCESS;
}

void PatchRegistryFuncs()
{
    auto RegIATpat = hook::pattern("50 53 53 FF 75 F4 C7 45 F0 10 00 00 00 FF 15");
    if (RegIATpat.size() > 0)
    {
        uintptr_t* RegIAT = *RegIATpat.get(0).get<uintptr_t*>(15); //0x4232E4 in config tool
        injector::WriteMemory(&RegIAT[0], RegQueryValueExAHook, true);
        injector::WriteMemory(&RegIAT[1], RegOpenKeyExAHook, true);
        injector::WriteMemory(&RegIAT[2], RegDeleteKeyAHook, true);
        injector::WriteMemory(&RegIAT[3], RegEnumKeyAHook, true);
        injector::WriteMemory(&RegIAT[4], RegOpenKeyAHook, true);
        injector::WriteMemory(&RegIAT[5], RegQueryValueAHook, true);
        injector::WriteMemory(&RegIAT[6], RegCreateKeyExAHook, true);
        injector::WriteMemory(&RegIAT[7], RegSetValueExAHook, true);
        injector::WriteMemory(&RegIAT[8], RegCloseKeyHook, true);
        bConfigTool = true;
    }
    else
    {
        RegIATpat = hook::pattern("68 02 00 00 80 FF 15");
        if (RegIATpat.size() > 0)
        {
            uintptr_t* RegIAT = *RegIATpat.get(0).get<uintptr_t*>(7); //0x5E9709 in game exe
            injector::WriteMemory(&RegIAT[0], RegOpenKeyExAHook, true);
            injector::WriteMemory(&RegIAT[1], RegQueryValueExAHook, true);
            injector::WriteMemory(&RegIAT[2], RegOpenKeyAHook, true);
            injector::WriteMemory(&RegIAT[3], RegCloseKeyHook, true);
        }
    }
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("89 65 E8 8B F4 89 3E 56");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    PatchRegistryFuncs();
    if (bConfigTool)
        return 0;

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    static int32_t RenderResX = iniReader.ReadInteger("MAIN", "RenderResX", -1);
    static int32_t RenderResY = iniReader.ReadInteger("MAIN", "RenderResY", -1);
    bool bFixMenu = iniReader.ReadInteger("MAIN", "FixMenu", 1) != 0;
    bool bFixFMV = iniReader.ReadInteger("MAIN", "FixFMV", 1) != 0;
    static bool bFMVWidescreenMode = iniReader.ReadInteger("MISC", "FMVWidescreenMode", 1) != 0;
    bool bDisableCutsceneBorders = iniReader.ReadInteger("MISC", "DisableCutsceneBorders", 1) != 0;
    bool bDisableSafeMode = iniReader.ReadInteger("MISC", "DisableSafeMode", 1) != 0;
    bool bPixelationFix = iniReader.ReadInteger("MISC", "PixelationFix", 1) != 0;
    uint32_t nStatusScreenRes = iniReader.ReadInteger("MISC", "StatusScreenRes", 512);
    uint32_t nShadowsRes = iniReader.ReadInteger("MISC", "ShadowsRes", 1024);
    uint32_t nDOFRes = iniReader.ReadInteger("MISC", "DOFRes", 1024);
    bool bFrameRateFluctuationFix = iniReader.ReadInteger("MISC", "FrameRateFluctuationFix", 1) != 0;
    bool bSingleCoreAffinity = iniReader.ReadInteger("MISC", "SingleCoreAffinity", 0) != 0;
    static bool bReduceCutsceneFOV = iniReader.ReadInteger("MISC", "ReduceCutsceneFOV", 1) != 0;
    bool bSH2Reference = iniReader.ReadInteger("MISC", "SH2Reference", 1) != 0;
    static float fFogComplexity = static_cast<float>(iniReader.ReadInteger("MISC", "FogComplexity", 25));

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    if (RenderResX >= 0 && RenderResY >= 0)
    {
        pattern = hook::pattern("51 A1 ? ? ? ? 53 8B 5C 24 0C"); //00402E00 
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0xC3, true);
        pattern = hook::pattern("8B 44 24 04 8B 4C 24 08 A3 ? ? ? ? 89 0D ? ? ? ? 33 C0 C3"); //00402B60
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0xC3, true);

        if (RenderResX == 0 && RenderResY == 0)
        {
            RenderResX = Screen.Width;
            RenderResY = Screen.Height;
        }
    }

    //Resolution
    static int32_t* ResXAddr1; 
    static int32_t* ResXAddr2;
    static int32_t* ResXAddr3;
    static int32_t* ResXAddr4;
    static int32_t* ResYAddr1;
    static int32_t* ResYAddr2;
    static int32_t* ResYAddr3;
    static int32_t* ResYAddr4;
    static float FMVOffset1;
    static float FMVOffset2;
    static float FMVOffset3;

    struct SetResHook
    {
        void operator()(injector::reg_pack&)
        {
            if (RenderResX > 0 && RenderResY > 0)
            {
                *ResXAddr1 = RenderResX;
                *ResYAddr1 = RenderResY;
            }
            *ResXAddr2 = Screen.Width;
            *ResYAddr2 = Screen.Height;
            *ResXAddr3 = Screen.Width;
            *ResYAddr3 = Screen.Height;
            *ResXAddr4 = Screen.Width;
            *ResYAddr4 = Screen.Height;

            if (!bFMVWidescreenMode)
                FMVOffset1 = (float)*ResXAddr1 / (Screen.fWidth / ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f));
            else
                FMVOffset1 = (float)*ResXAddr1 / (Screen.fWidth / ((Screen.fWidth - Screen.fHeight * (964.0f / 622.0f)) / 2.0f));
            FMVOffset2 = *ResXAddr1 - FMVOffset1;
            FMVOffset3 = (float)*ResYAddr1;
        }
    };
    
    ResXAddr1 = *hook::pattern("A1 ? ? ? ? 53 8B 5C 24 0C 3B C3 55 8B 6C 24 14").count(1).get(0).get<int32_t*>(1); //72BFD0 rendering res
    ResYAddr1 = ResXAddr1 + 1; //72BFD4
    ResXAddr2 = *hook::pattern("BF ? ? ? ? F3 AB 52").count(1).get(0).get<int32_t*>(1); //0072C664 
    ResYAddr2 = ResXAddr2 + 1; //0072C668
    ResXAddr3 = *hook::pattern("8B 0D ? ? ? ? 8B 44 24 04 3B C8 8B 4C 24 08 75 08 39 0D").count(1).get(0).get<int32_t*>(2); //0072C780 
    ResYAddr3 = ResXAddr3 + 1; //0072C784
    ResXAddr4 = ResYAddr3 + 1; //0072C788 
    ResYAddr4 = ResXAddr4 + 1; //0072C78C

    //pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? 33 C0 C3");
    //injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(11)); //402B68
    //pattern = hook::pattern("89 1D ? ? ? ? 89 2D ? ? ? ? E8 ? ? ? ? 8B");
    //injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12)); //402E3A
    //pattern = hook::pattern("89 35 ? ? ? ? 89 3D ? ? ? ? E8 ? ? ? ? 8B");
    //injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12)); //402E61
    //pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 83 C4 04");
    //injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(10)); //402E93


    pattern = hook::pattern("89 15 ? ? ? ? 8B 10 50 FF 52 3C 85 C0");
    injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //419622
    pattern = hook::pattern("89 0D ? ? ? ? 89 15 ? ? ? ? E8");
    injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12)); //4197DE
    pattern = hook::pattern("89 35 ? ? ? ? 5E 33 C0 5B 83 C4 10");
    injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //419804


    pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? C6 05 ? ? ? ? 01");
    injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(22)); //416AFA
    pattern = hook::pattern("89 0D ? ? ? ? 89 0D ? ? ? ? C6 05 ? ? ? ? 01 33 C0 C3");
    injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12)); //416B4D
    pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? A3 ? ? ? ? C7 05 ? ? ? ? 3C 00");
    injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5+11)); //418C57


    pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? 33 C0 C3");
    injector::MakeInline<SetResHook>(pattern.count(2).get(1).get<uint32_t>(0), pattern.count(2).get(1).get<uint32_t>(11)); //416B78

    //FOV
    Screen.fFieldOfView = (((4.0f / 3.0f) / (Screen.fAspectRatio)));
    static auto byte_81FAB5 = *hook::get_pattern<uint8_t*>("A2 ? ? ? ? A2 ? ? ? ? 74 4D 49", 1);
    pattern = hook::pattern("6A 02 E8 ? ? ? ? 83 C4 04 85 C0 ? ? D9 05 ? ? ? ? C3"); //43AB80 43ABC0
    struct FOVHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            float hor = Screen.fFieldOfView * 0.88f;

            if (bReduceCutsceneFOV && *byte_81FAB5)
                hor /= Screen.fFieldOfView;

            _asm fld dword ptr[hor]
        }
    }; 
    injector::MakeInline<FOVHook1>(pattern.count(2).get(0).get<void*>(0));
    injector::WriteMemory<uint8_t>(pattern.count(2).get(0).get<void*>(5), 0xC3i8, true); //ret

    struct FOVHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            float ver = 1.0f;

            if (bReduceCutsceneFOV && *byte_81FAB5)
                ver /= Screen.fFieldOfView;

            _asm fld dword ptr[ver]
        }
    };
    injector::MakeInline<FOVHook2>(pattern.count(2).get(1).get<void*>(0));
    injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<void*>(5), 0xC3i8, true); //ret

    //Removes the black bars visible on the side of the screen
    static float f0 = 0.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? DC C0 D9 54 24 0C D9 5C 24 04 E8"); //41BE64
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f0, true);
    pattern = hook::pattern("D8 0D ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 6A 00 D9 5C 24 14"); //41BB2D
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f0, true);

    if (bFixFMV)
    {
        SetResHook();
        pattern = hook::pattern("D9 05 ? ? ? ? D8 25 ? ? ? ? 83 C8 FF"); //4045B9
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &FMVOffset1, true);
        pattern = hook::pattern("D9 05 ? ? ? ? C7 44 24 0C 00 00 00 00 D8 25"); //004045C8 
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &FMVOffset2, true);

        if (bFMVWidescreenMode)
        {
            static float f0 = 0.0f;
            pattern = hook::pattern("D9 05 ? ? ? ? 89 44 24 14 D8 25 ? ? ? ? C7"); //4045E2
            injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f0, true);

            pattern = hook::pattern("D9 05 ? ? ? ? D8 25 ? ? ? ? D9 C3 D9 5C"); //404606
            injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &FMVOffset3, true);
        }
    }

    if (bFixMenu)
    {
        //Menu Width
        static float fMenuWidthFactor = (1.0f / (480.0f * (Screen.fAspectRatio / 10.0f))) / 2.0f / 2.0f / 2.0f;
        static float fMenuXPos = 256.0f * (Screen.fAspectRatio / (4.0f / 3.0f));
        pattern = hook::pattern("DE C9 D8 0D ? ? ? ? 59 C3"); //682BBE
        injector::WriteMemory(pattern.count(3).get(2).get<uint32_t>(4), &fMenuWidthFactor, true);
        injector::WriteMemory(pattern.count(3).get(2).get<uint32_t>(-4), &fMenuXPos, true);

        pattern = hook::pattern("51 E8 ? ? ? ? 89 44 24 00 DB 44 24 00 DB 44");
        uintptr_t dword_413510 = (uintptr_t)pattern.count(3).get(0).get<uintptr_t>(0);
        uintptr_t dword_682BA0 = (uintptr_t)pattern.count(3).get(2).get<uintptr_t>(0);
        injector::MakeCALL(dword_413510 + 1, hook::pattern("A1 ? ? ? ? 85 C0 74 06 A1 ? ? ? ? C3 E9").count(1).get(0).get<uintptr_t>(0)); //402BA0

        // http://pastebin.com/p03E9Vw1 xrefs of 682BA0, ((j >= 0 && j <= 7) || (j >= 21 && j <= 26) || j == 75 || j == 76)
        auto r_start = (uintptr_t)hook::get_pattern("8B 84 24 84 00 00 00 8B 08 51 81 CE 00 00 00 FF", 0);
        auto r_end = (uintptr_t)hook::get_pattern("D9 9C 24 88 00 00 00 8B 45 04 50", 0);
        auto rpattern = hook::range_pattern(r_start, r_end, "E8 ? ? ? ?");

        for (size_t i = 0; i < rpattern.size(); ++i) 
        {
            if (injector::GetBranchDestination(rpattern.get(i).get<uintptr_t>(0), true).as_int() == dword_682BA0)
            {
                //[0]0x00468B85 [1]0x00468BDA [2]0x00468C20 [3]0x00468C6C [4]0x00468E2C [5]0x00468E73 [6]0x00468EB2 [7]0x00468F07
                injector::MakeCALL(rpattern.get(i).get<uintptr_t>(0), dword_413510, true);
            }
        }

        r_start = (uintptr_t)hook::get_pattern("8B 8C 24 CC 00 00 00 51", 0);
        r_end = (uintptr_t)hook::get_pattern("8B 4E 04 8B D8 51 89 5C 24 24", 0);
        rpattern = hook::range_pattern(r_start, r_end, "E8 ? ? ? ?");

        for (size_t i = 0; i < rpattern.size(); ++i)
        {
            if (injector::GetBranchDestination(rpattern.get(i).get<uintptr_t>(0), true).as_int() == dword_682BA0)
            {
                //[21]0x0047A67F [22]0x0047A6D3 [23]0x0047A721 [24]0x0047A76C [25]0x0047A7B4 [26]0x0049FAEB
                injector::MakeCALL(rpattern.get(i).get<uintptr_t>(0), dword_413510, true);
            }
        }

        pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 6B C9 1C"); //[75]0x005F6D22
        injector::MakeCALL(pattern.get_first(0), dword_413510, true);

        pattern = hook::pattern("E8 ? ? ? ? D9 5E FC D9 07 D8 0D ? ? ? ? E8 ? ? ? ? 50"); //[76]0x005F80EF
        injector::MakeCALL(pattern.get_first(0), dword_413510, true);

        //Mouse cursor width restoration
        auto ret_088 = []() -> float
        {
            return 0.88f;
        };

        pattern = hook::pattern("E8 ? ? ? ? D9 9D 9C FE FF FF E8");
        injector::MakeCALL(pattern.get_first(0), static_cast<float(*)()>(ret_088), true);

        //Splash
        static float fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
        static float w2 = Screen.fWidth - fHudOffsetReal;
        pattern = hook::pattern("A1 ? ? ? ? 6A 05 C7 44 24 18"); //005D744A
        injector::WriteMemory<float>(pattern.get_first(11), fHudOffsetReal, true);
        pattern = hook::pattern("C7 44 24 50 ? ? ? ? C7 44 24 58 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 64"); //005D74B0 
        injector::WriteMemory<float>(pattern.get_first(4), fHudOffsetReal, true);
        pattern = hook::pattern("D8 25 ? ? ? ? 8D 54 24 0C 52 6A 02"); //005D7414 
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0x05D9i16, true); //fsub -> fld
        injector::WriteMemory(pattern.get_first(2), &w2, true);

        //Credits
        pattern = hook::pattern("6A 05 C7 44 24 1C"); //0059BE49 
        injector::WriteMemory<float>(pattern.get_first(6), fHudOffsetReal, true);
        pattern = hook::pattern("C7 44 24 54 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 60 ? ? ? ? C7 44 24 68"); //0059BEAF 
        injector::WriteMemory<float>(pattern.get_first(4), fHudOffsetReal, true);
        pattern = hook::pattern("D8 25 ? ? ? ? 8D 4C 24 10 51 6A 02 D9 54 24 34 "); //0059BE13 
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0x05D9i16, true); //fsub -> fld
        injector::WriteMemory(pattern.get_first(2), &w2, true);
    }

    //inventory background size
    pattern = hook::pattern("B8 00 01 00 00 BA 00 FF FF FF 8B E8"); //5DAFE5
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), 1024, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(6), -1024, true);

    //window style
    pattern = hook::pattern("68 00 03 00 00 FF 15 ? ? ? ? 5E"); //403042
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), 0, true);

    //options menu freeze
    pattern = hook::pattern("81 EC D8 00 00 00 55 8B AC"); //5E2300
    injector::MakeRET(pattern.count(1).get(0).get<uint32_t>(0));

    //Pick-Up Item Width Restoration
    pattern = hook::pattern("E8 ? ? ? ? D9 9D A0 FE FF FF"); //599C24
    struct FOVHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            float f = 0.88f;
            _asm fld dword ptr[f]
        }
    }; injector::MakeInline<FOVHook3>(pattern.get_first(0));

    if (bDisableCutsceneBorders)
    {
        pattern = hook::pattern("D9 05 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 D8 C9"); //41BB4B
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f0, true);
    }

    if (bDisableSafeMode)
    {
        static char* nullstr = "";
        pattern = hook::pattern("BF 01 00 00 00 68 ? ? ? ? 68 ? ? ? ? 6A 01"); //5F0A39
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), 0, true);
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(6), &nullstr, true);
    }

    if (bPixelationFix)
    {
        pattern = hook::pattern("89 15 ? ? ? ? A3 ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ?"); //426121
        static float* flt_6B1300 = *pattern.count(1).get(0).get<float*>(2);
        static float* flt_6B12D0 = *pattern.count(1).get(0).get<float*>(7);
        static float* flt_6B131C = *pattern.count(1).get(0).get<float*>(13);
        static float* flt_6B1304 = *pattern.count(1).get(0).get<float*>(19);
        struct PixelationFixHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                *flt_6B1300 = *ResXAddr1 - 0.5f;
                *flt_6B12D0 = *ResXAddr1 - 0.5f;
                *flt_6B131C = *ResYAddr1 - 0.5f;
                *flt_6B1304 = *ResYAddr1 - 0.5f;
                float temp = 0.0f;
                __asm fstp temp
            }
        }; injector::MakeInline<PixelationFixHook1>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(23));

        pattern = hook::pattern("D9 15 ? ? ? ? D9 1D ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ? DB"); //41C0D6
        static float* flt_6B0BC0 = *pattern.count(1).get(0).get<float*>(2);
        static float* flt_6B0B60 = *pattern.count(1).get(0).get<float*>(8);
        static float* flt_6B0BE0 = *pattern.count(1).get(0).get<float*>(14);
        static float* flt_6B0B90 = *pattern.count(1).get(0).get<float*>(20);
        struct PixelationFixHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *flt_6B0BC0 = *ResXAddr1 - 0.5f;
                *flt_6B0B60 = *ResXAddr1 - 0.5f;
                *flt_6B0BE0 = *ResXAddr1 - 0.5f;
                *flt_6B0B90 = *ResXAddr1 - 0.5f;
                float temp = 0.0f;
                __asm fstp temp
                __asm fstp temp
            }
        }; injector::MakeInline<PixelationFixHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(24));

        pattern = hook::pattern("D9 15 ? ? ? ? D9 1D ? ? ? ? D9 15 ? ? ? ? D9 1D ? ? ? ? 8B 10"); //41C0F4 
        static float* flt_6B0BE4 = *pattern.count(1).get(0).get<float*>(2);
        static float* flt_6B0B94 = *pattern.count(1).get(0).get<float*>(8);
        static float* flt_6B0BF4 = *pattern.count(1).get(0).get<float*>(14);
        static float* flt_6B0BAC = *pattern.count(1).get(0).get<float*>(20);
        struct PixelationFixHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                *flt_6B0BE4 = *ResYAddr1 - 0.5f;
                *flt_6B0B94 = *ResYAddr1 - 0.5f;
                *flt_6B0BF4 = *ResYAddr1 - 0.5f;
                *flt_6B0BAC = *ResYAddr1 - 0.5f;
                float temp = 0.0f;
                __asm fstp temp
                __asm fstp temp
            }
        }; injector::MakeInline<PixelationFixHook3>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(24));
    }

    if (nStatusScreenRes)
    {
        pattern = hook::pattern("8B 3C 85 ? ? ? ? 8B 04 85 ? ? ? ? 56 6A 00 6A 15 6A 01 6A 01"); //6B1444 //6B1434
        injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(3), nStatusScreenRes, true);
        injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(10), nStatusScreenRes, true);
        pattern = hook::pattern("BE ? ? ? ? 56 56 89 5C 24 24 89 5C 24 28 89 5C 24 20 8B 08 50"); //41DE57
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), nStatusScreenRes, true);
        pattern = hook::pattern("BE ? ? ? ? 8D A4 24 00 00 00 00 8B 2C 01 89 28 83 C0 04 4E"); //41E165
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), nStatusScreenRes, true);

        pattern = hook::pattern("68 ? ? ? ? 6A 02 6A 05 50 FF 91 20 01 00 00 A1 ? ? ? ? 8B 10 6A 01"); 
        uintptr_t dword_6B0D00 = *pattern.count(2).get(1).get<uintptr_t>(1);
        injector::WriteMemory(dword_6B0D00 + 0x00, static_cast<float>(nStatusScreenRes), true); //006B0D00
        injector::WriteMemory(dword_6B0D00 + 0x30, static_cast<float>(nStatusScreenRes), true); //006B0D30 
        injector::WriteMemory(dword_6B0D00 + 0x34, static_cast<float>(nStatusScreenRes), true); //006B0D34 
        injector::WriteMemory(dword_6B0D00 + 0x4C, static_cast<float>(nStatusScreenRes), true); //006B0D4C 
    }

    if (nShadowsRes)
    {
        pattern = hook::pattern("C7 44 24 ? 00 00 80 43");
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            injector::WriteMemory(pattern.get(i).get<uint32_t>(4), static_cast<float>(nShadowsRes), true);
        }

        pattern = hook::pattern("C7 05 ? ? ? ? 00 00 80 43");
        for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/geuM13M3
        {
            if (i != 0 && i <= 16)
            {
                injector::WriteMemory(pattern.get(i).get<uint32_t>(6), static_cast<float>(nShadowsRes), true);
            }
        }

        pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00");
        for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/PwbVt6uy
        {
            if (i == 3 || i == 4 || i == 5)
            {
                injector::WriteMemory(pattern.get(i).get<uint32_t>(1), nShadowsRes, true);
                injector::WriteMemory(pattern.get(i).get<uint32_t>(6), nShadowsRes, true);
            }
        }
    }

    if (nDOFRes)
    {
        pattern = hook::pattern("8B 3C 85 ? ? ? ? 8B 04 85 ? ? ? ? 56 6A 00 6A 15 6A 01 6A 01"); //6B1444 //6B1434
        auto dword_6B1444 = *pattern.count(1).get(0).get<uint32_t*>(3);
        injector::WriteMemory(dword_6B1444 - 1, nDOFRes, true); //otherworld corrosion resolution
        injector::WriteMemory(dword_6B1444 - 2, nDOFRes, true); //otherworld corrosion resolution
        injector::WriteMemory(dword_6B1444 + 2, nDOFRes, true);
        injector::WriteMemory(dword_6B1444 + 3, nDOFRes, true);
    }

    if (bFrameRateFluctuationFix)
    {
        pattern = hook::pattern("4A 03 C8 A3 ? ? ? ? 89 15"); //41B5D1
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x42, true);
    }

    if (bSingleCoreAffinity)
    {
        SetProcessAffinityMask(GetCurrentProcess(), 1);
    }

    if (bSH2Reference)
    {
        pattern = hook::pattern("A0 ? ? ? ? 84 C0 ? ? E8 ? ? ? ? A2 ? ? ? ? C6 05"); //476C83
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xC3i8, true);
    }

    if (fFogComplexity)
    {
        pattern = hook::pattern("D8 3D ? ? ? ? D9 1D ? ? ? ? 83 C4 08 C3"); //41B5D1
        injector::WriteMemory(pattern.get_first(2), &fFogComplexity, true);
    }

    return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}
