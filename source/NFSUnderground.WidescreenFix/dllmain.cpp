#include "stdafx.h"
#include <d3d9.h>

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    int32_t nWidth43;
    float fWidth43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScaleX;
    float fHudPosX;
    float fHudOffsetReal;
} Screen;

bool bHUDWidescreenMode = true;

#pragma runtime_checks( "", off )
namespace FEScale
{
    float fFEScale = 1.0f;
    float fCalcFEScale = 1.0f;
    float fFMVScale = 1.0f;
    float fCalcFMVScale = 1.0f;
    bool bEnabled = false;
    bool bAutoFitFE = false;
    bool bAutoFitFMV = true;
    bool bMovieFlag = false;

    bool bWorkaroundReshade = false;

    uintptr_t SetTransformAddr = 0x00410400;
    uintptr_t MovieIsStartedAddr = 0x00733F74;
    uintptr_t InfectedTransform = 0;

    void __stdcall SetTransform(D3DMATRIX* mat, uint32_t EVIEW_ID)
    {
        _asm
        {
            mov edi, EVIEW_ID
            push mat
            call SetTransformAddr
            add esp, 4
        }
    }

    void __stdcall InfectedSetTransform(D3DMATRIX* mat, uint32_t EVIEW_ID)
    {
        _asm
        {
            mov edi, EVIEW_ID
            push mat
            call InfectedTransform
            add esp, 4
        }
    }

    void __cdecl SetTransformHook(D3DMATRIX* mat)
    {
        // hooking a fastcall, need to read a register directly...
        uint32_t EVIEW_ID;
        _asm mov EVIEW_ID, edi

        D3DMATRIX cMat;
        memcpy(&cMat, mat, sizeof(D3DMATRIX));

        if (bMovieFlag)
        {
            cMat._11 = fCalcFMVScale;
            cMat._22 = fCalcFMVScale;
        }
        else
        {
            cMat._11 *= fCalcFEScale;
            cMat._22 *= fCalcFEScale;
        }

        if (bWorkaroundReshade)
            InfectedSetTransform(mat, EVIEW_ID);

        return SetTransform(&cMat, EVIEW_ID);
    }

    void Update()
    {
        fCalcFEScale = fFEScale;
        fCalcFMVScale = fFMVScale;

        if (bHUDWidescreenMode)
        {
            if (bAutoFitFE)
                fCalcFEScale *= Screen.fAspectRatio / (16.0f / 9.0f);
            if (bAutoFitFMV)
                fCalcFMVScale *= Screen.fAspectRatio / (16.0f / 9.0f);
        }
        else
        {
            if (bAutoFitFE)
                fCalcFEScale *= Screen.fAspectRatio / (4.0f / 3.0f);
            if (bAutoFitFMV)
                fCalcFMVScale *= Screen.fAspectRatio / (4.0f / 3.0f);
        }

        if (fCalcFEScale > fFEScale)
            fCalcFEScale = fFEScale;

        if (fCalcFMVScale > fFMVScale)
            fCalcFMVScale = fFMVScale;
    }

    uintptr_t DrawFEAddr = 0x00409CD0;
    
    void SetMovieFlag()
    {
        bMovieFlag = true;
        // execute a draw call for the FE to set the transform (similar to Pro Street)
        reinterpret_cast<void(__cdecl*)(uint32_t)>(DrawFEAddr)(0);
    }
    
    void UnsetMovieFlag()
    {
        bMovieFlag = false;
        reinterpret_cast<void(__cdecl*)(uint32_t)>(DrawFEAddr)(0);
    }
    
    struct MovieStartHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)MovieIsStartedAddr = 1;
            SetMovieFlag();
        }
    };

    struct MovieStopHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)MovieIsStartedAddr = 0;
            UnsetMovieFlag();
        }
    };
}
#pragma runtime_checks( "", restore )

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

void LoadDatFile(std::filesystem::path strPath, std::function<void(std::string_view line)>&& cb)
{
    std::ifstream hFile;
    hFile.open(strPath);
    if (hFile.is_open())
    {
        std::string line;
        while (std::getline(hFile, line))
        {
            if (line[0] && line[0] != '#')
            {
                cb(line);
            }
        }
        hFile.close();
    }
}

void DrawRect(LPDIRECT3DDEVICE9 pDevice, int32_t x, int32_t y, int32_t w, int32_t h, D3DCOLOR color = D3DCOLOR_XRGB(0, 0, 0))
{
    D3DRECT BarRect = { x, y, x + w, y + h };
    pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 0, 0);
}

void*(*CreateResourceFile)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int);
void(__fastcall *ResourceFileBeginLoading)(int a1, void* rsc, int a2);
void LoadResourceFile(const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
{
    auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
    _asm
    {
        mov     edx, r
        mov     ecx, nUnk5
        mov     eax, nUnk4
        call    ResourceFileBeginLoading
    }
};

uint32_t* dword_73645C;
uint8_t* byte_735768;
void* off_71AA4C;
void __declspec(naked) CmpShouldClearSurfaceDuring3DRender()
{
    _asm
    {
        mov eax, dword_73645C
        cmp dword ptr [eax], 1
        jne resume

        // byte_735768 is set if the magazine view screen is open.
        mov eax, byte_735768
        cmp byte ptr [eax], 0

        resume:
        ret
    }
}

void __declspec(naked) MaybeClearSurfaceDuringBackgroundRender()
{
    _asm
    {
        call CmpShouldClearSurfaceDuring3DRender
        jz resume

        // In case 3D rendering won't clear the render surface, it must
        // be done during the 2D background rendering.
        mov dword ptr [esp+4], 1

        resume:
        mov esi, off_71AA4C
        ret
    }
}

namespace DebugTags
{
    //by xan1242
    //https://github.com/ThirteenAG/WidescreenFixesPack/issues/643#issuecomment-1083564196
    unsigned int FEngSetInvisible_Addr = 0x000495F70;
    void __stdcall FEngSetInvisible(const char* pkgname, unsigned int hash)
    {
        _asm
        {
            mov edi, hash
            mov esi, pkgname
            call FEngSetInvisible_Addr
        }
    }

    unsigned int FEHashUpper_Addr = 0x004FD230;
    unsigned int __stdcall FEHashUpper(const char* instr)
    {
        unsigned int result = 0;
        _asm
        {
            mov edx, instr
            call FEHashUpper_Addr
            mov result, eax
        }
        return result;
    }

    unsigned int FEngFindObject_Addr = 0x004FFB70;
    unsigned int __stdcall FEngFindObject(const char* pkg, int hash)
    {
        unsigned int result = 0;
        _asm
        {
            mov ecx, hash
            mov edx, pkg
            call FEngFindObject_Addr
            mov result, eax
        }
        return result;
    }

    unsigned int FEPkgMgr_FindPackage_Addr = 0x004F65D0;
    unsigned int __stdcall FEPkgMgr_FindPackage(const char* pkgname)
    {
        unsigned int result = 0;
        _asm
        {
            mov eax, pkgname
            call FEPkgMgr_FindPackage_Addr
            mov result, eax
        }
        return result;
    }

    void __stdcall CorruptAllFEDevObjects(char* pkgname)
    {
        // so in order to hide the silly little side-tags used during FEng development, an attack at its FEObjects is necessary
        // all of them, and I mean each and every one of the tags' text object hashes and name is "ConduitMdITC_TT21i"
        // because of this, the game only references the first one in memory (which is usually next_tag), so FEngSetInvisible doesn't work
        // ...unless we invalidate/corrupt the NameHash on purpose and that's exactly what this function will do

        unsigned int TestObj = 0;

        // this may be a lot and potentially taxing but it's the best we can do without editing all FNGs themselves
        // hide ALL of those red blocks
        FEngSetInvisible(pkgname, 0x8BDC3C7D);	// QUIT_TAG
        FEngSetInvisible(pkgname, 0x3A8A5C3E);	// QUITTAG
        FEngSetInvisible(pkgname, 0xA1D233EB);	// BACK_TAG
        FEngSetInvisible(pkgname, 0x88C81DEC);	// BACKTAG
        FEngSetInvisible(pkgname, 0x1A705339);	// NEXT_TAG
        FEngSetInvisible(pkgname, 0x2F588B7A);	// NEXTTAG
        FEngSetInvisible(pkgname, 0xFF4BB93F);	// DISPLAYMENUBAR
        FEngSetInvisible(pkgname, 0xD0257F7D);	// DISPLAY_MENUBAR_
        FEngSetInvisible(pkgname, 0x3AE43C3E);	// DeleteProfile_tag
        FEngSetInvisible(pkgname, 0x28894A9D);	// CUSTOMIZE_TAG
        FEngSetInvisible(pkgname, 0x10BE265E);	// CUSTOMIZETAG
        FEngSetInvisible(pkgname, 0xA8ED017E);	// STOCK_TAG
        FEngSetInvisible(pkgname, 0x2426051F);	// STOCKTAG
        FEngSetInvisible(pkgname, 0x7BD9401F);	// changecolor_tag
        FEngSetInvisible(pkgname, 0x134473A0);	// changecolortag
        FEngSetInvisible(pkgname, 0x5B393372);	// DecalColor_tag
        FEngSetInvisible(pkgname, 0xDBF9C893);	// DecalColortag
        FEngSetInvisible(pkgname, 0x502AFE6C);	// performance_tag
        FEngSetInvisible(pkgname, 0x50011C4D);	// performancetag
        FEngSetInvisible(pkgname, 0x677392C4);  // defaultcontsettings_tag

        // hide all text
        // there are many duplicates... eww...
        FEngSetInvisible(pkgname, 0x8C4C21FA); // ConduitMdITC_TT21i(88)
        FEngSetInvisible(pkgname, 0x8C4C2533); // ConduitMdITC_TT21i(90)
        FEngSetInvisible(pkgname, 0x8C4C2554); // ConduitMdITC_TT21i(91)
        FEngSetInvisible(pkgname, 0x8C4C2575); // ConduitMdITC_TT21i(92)
        FEngSetInvisible(pkgname, 0x8C4C2596); // ConduitMdITC_TT21i(93)
        FEngSetInvisible(pkgname, 0x8C4C25B7); // ConduitMdITC_TT21i(94)
        FEngSetInvisible(pkgname, 0x15CC69FE); // ConduitMdITC_TT21i(103)
        FEngSetInvisible(pkgname, 0x15CC6A82); // ConduitMdITC_TT21i(107)
        FEngSetInvisible(pkgname, 0x15CC6AA3); // ConduitMdITC_TT21i(108)
        FEngSetInvisible(pkgname, 0x15CC6AC4); // ConduitMdITC_TT21i(109)
        FEngSetInvisible(pkgname, 0x15CC6DDC); // ConduitMdITC_TT21i(110)
        FEngSetInvisible(pkgname, 0x15CC6EC3); // ConduitMdITC_TT21i(117)
        FEngSetInvisible(pkgname, 0x15CC6EE4); // ConduitMdITC_TT21i(118)

        // there are exceptions we have to watch out for (we're just gonna use the game's hashing function to quickly compare strings)
        // anything that breaks goes here
        // hash can be calclulated simply by using bStringHash or this FEHashUpper ripped from the game
        if (FEHashUpper(pkgname) == 0xAB447B38) // PC_Loading.fng -- removes the loading text (if it would exist, e.g. the beta loading screen)
            return;
        if (FEHashUpper(pkgname) == 0x2729D8C3) // LS_Splash_PC.fng -- removes the copyright text
            return;
        if (FEHashUpper(pkgname) == 0x41613BD0) // MU_Online_NetworkLogin.fng -- removes some buttons
            return;
        if (FEHashUpper(pkgname) == 0x630135FF) // MU_QuickRaceOptions_PC.fng -- removes some buttons
            return;

        // we search for every single test object until it's done (FEngFindObject returns 0 if it can't find any)
        while (FEngFindObject((char*)FEPkgMgr_FindPackage(pkgname), FEHashUpper("ConduitMdITC_TT21i")))
        {
            // once found, catch it in a variable
            TestObj = FEngFindObject((char*)FEPkgMgr_FindPackage(pkgname), FEHashUpper("ConduitMdITC_TT21i"));
            // before corrupting, hide it
            FEngSetInvisible(pkgname, FEHashUpper("ConduitMdITC_TT21i"));
            // after hiding, we can corrupt the hash, this is simply done by changing the NameHash at 0x10 of the FEObject (as per its class)
            *(int*)(TestObj + 0x10) = 0xDEADBEEF;
            // rip and tear... until it's done
        }
    }

    // FEPkgMgr_SendMessageToPackage talks to ALL of FrontEnd, so it's an ideal place to globally affect it
    unsigned int FEPkgMgr_SendMessageToPackage_Cave_Exit = 0x004F7D18;
    unsigned int FEPkgMgr_SendMessageToPackage_EAX = 0;
    unsigned int FEPkgMgr_SendMessageToPackage_ECX = 0;
    unsigned int FEPkgMgr_SendMessageToPackage_ESI = 0;
    void __declspec(naked) FEPkgMgr_SendMessageToPackage_Cave()
    {
        _asm
        {
            mov eax, [esp + 0x50]
            mov ecx, [esp + 0x4C]
            mov FEPkgMgr_SendMessageToPackage_EAX, eax
            mov FEPkgMgr_SendMessageToPackage_ECX, ecx
            mov FEPkgMgr_SendMessageToPackage_ESI, esi
        }
        CorruptAllFEDevObjects(*(char**)(FEPkgMgr_SendMessageToPackage_ESI + 0xC));
        _asm
        {
            mov eax, FEPkgMgr_SendMessageToPackage_EAX
            mov ecx, FEPkgMgr_SendMessageToPackage_ECX
            mov esi, FEPkgMgr_SendMessageToPackage_ESI
            jmp FEPkgMgr_SendMessageToPackage_Cave_Exit
        }
    }
}

namespace IntroFMVScreen
{
#pragma runtime_checks( "", off )
    bool bOnceFlag = false;
    uintptr_t NotificationMessageFuncAddr = 0x004DE9F0;
    uintptr_t FEngPopPackageAddr = 0x004DE4A0;

    void __stdcall hkNotificationMessage(uint32_t msg, void* FEObject, uint32_t unk1, uint32_t unk2)
    {
        uintptr_t that;
        _asm mov that, ecx

        // pop the package once for the intro skip
        if (!bOnceFlag)
        {
            bOnceFlag = true;
            reinterpret_cast<void(__cdecl*)(char* pkgname)>(FEngPopPackageAddr)(*(char**)(that + 0xC));
            return;
        }

        return reinterpret_cast<void(__thiscall*)(uintptr_t, uint32_t, void*, uint32_t, uint32_t)>(NotificationMessageFuncAddr)(that, msg, FEObject, unk1, unk2);
    }
#pragma runtime_checks( "", restore )
}
HWND WINAPI CreateWindowExA_Hook_NFSU(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    HWND retval = WindowedModeWrapper::CreateWindowExA_Hook(dwExStyle, lpClassName, lpWindowName, 0, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    // icon fix
    SendMessage(retval, WM_SETICON, ICON_BIG, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)));
    SendMessage(retval, WM_SETICON, ICON_SMALL, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)));

    return retval;
}

void Init()
{
    CIniReader iniReader("");
    Screen.nWidth = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.nHeight = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bScaling = iniReader.ReadInteger("MAIN", "Scaling", 0);
    bHUDWidescreenMode = iniReader.ReadInteger("MAIN", "HUDWidescreenMode", 1) != 0;
    int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    FEScale::fFEScale = iniReader.ReadFloat("MAIN", "FEScale", 1.0f);
    FEScale::fCalcFEScale = FEScale::fFEScale;
    FEScale::fFMVScale = iniReader.ReadFloat("MAIN", "FMVScale", 1.0f);
    FEScale::fCalcFMVScale = FEScale::fFMVScale;
    FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 0) != 0;
    FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;

    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
    bool bShowLangSelect = iniReader.ReadInteger("MISC", "ShowLangSelect", 0) != 0;
    static std::filesystem::path CustomUserDir;
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");

    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    static int nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", -1);
    int nHideDebugObjects = iniReader.ReadInteger("MISC", "HideDebugObjects", 0);
    bool bBlackMagazineFix = iniReader.ReadInteger("MISC", "BlackMagazineFix", 0) != 0;
    int nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    // clamp the size because below 32x32 the game crashes!
    if (Screen.nWidth < 32)
        Screen.nWidth = 32;

    if (Screen.nHeight < 32)
        Screen.nHeight = 32;

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //Game state
    static auto nGameState = std::ref(**hook::get_pattern<int32_t*>("83 3D ? ? ? ? 06 ? ? A1", 2)); //0x77A920

    //Resolution
    //menu
    auto pattern = hook::pattern("68 20 03 00 00 BE 58 02 00 00");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);

    pattern = hook::pattern("68 20 03 00 00 BE 58 02 00 00");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);

    //game
    pattern = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 77 75");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    injector::MakeNOP(pattern.get_first(10), 9, true);
    pattern = hook::pattern("B8 80 02 00 00 BE E0 01 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    pattern = hook::pattern("B8 20 03 00 00 BE 58 02 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    pattern = hook::pattern("B8 00 04 00 00 BE 00 03 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    pattern = hook::pattern("B8 00 05 00 00 BE C0 03 00 00 50");
    injector::WriteMemory(pattern.get_first(1), Screen.nWidth, true);
    injector::WriteMemory(pattern.get_first(6), Screen.nHeight, true);
    // initial ResX and ResY
    uint32_t* dword_701034 = *hook::pattern("83 EB 00 56 8B 35 ? ? ? ? 57 8B 3D ? ? ? ?").count(1).get(0).get<uint32_t*>(6); // ResX
    uint32_t* dword_701038 = *hook::pattern("83 EB 00 56 8B 35 ? ? ? ? 57 8B 3D ? ? ? ?").count(1).get(0).get<uint32_t*>(13); // ResY
    *dword_701034 = Screen.nWidth;
    *dword_701038 = Screen.nHeight;

    //HUD
    if (bFixHUD)
    {
        pattern = hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8 81 00 00 00 D8"); //6CC914
        injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHudScaleX, true);

        for (size_t i = 0; i < 4; i++)
        {
            auto pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
            if (!pattern.empty())
            {
                injector::WriteMemory<float>(pattern.get(0).get<void>(7), Screen.fHudPosX, true);
            }
            else
            {
                pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
                if (!pattern.empty())
                {
                    injector::WriteMemory<float>(pattern.get(0).get<void>(4), Screen.fHudPosX, true);
                }
            }
        }

        pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43");
        injector::WriteMemory<float>(pattern.get_first(4), Screen.fHudPosX, true);
        pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43");
        injector::WriteMemory<float>(pattern.get_first(4), Screen.fHudPosX, true);

        pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC");
        injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHudPosX, true);

        pattern = hook::pattern("C7 44 24 ? 00 00 DC 43 C7 44 24 ? 00 00 70 41");
        if (!pattern.empty())
        {
            injector::WriteMemory<float>(pattern.get_first(20), (Screen.fHudPosX - 320.0f) + 200.0f, true);
            injector::WriteMemory<float>(pattern.get_first(42), (Screen.fHudPosX - 320.0f) + 200.0f, true);
            injector::WriteMemory<float>(pattern.get_first(4), (Screen.fHudPosX - 320.0f) + 440.0f, true);
            injector::WriteMemory<float>(pattern.get_first(64), (Screen.fHudPosX - 320.0f) + 440.0f, true);
        }

        //pattern = hook::pattern("8D 72 04 83 C0 60 E8 ? ? ? ? 5F 5E C3"); //menu (#179)
        //if (pattern.size() > 0)
        //  injector::MakeNOP(pattern.get_first(), 11, true);

        //mouse cursor fix
        pattern = hook::pattern("81 C2 C0 FE FF FF");
        for (size_t i = 0; i < pattern.count_hint(26).size(); ++i)
        {
            injector::WriteMemory(pattern.get(i).get<void>(2), static_cast<int32_t>(-Screen.fHudPosX), true);
        }

        pattern = hook::pattern("05 C0 FE FF FF");
        for (size_t i = 0; i < pattern.count_hint(3).size(); ++i)
        {
            injector::WriteMemory(pattern.get(i).get<void>(1), static_cast<int32_t>(-Screen.fHudPosX), true);
        }

        pattern = hook::pattern("8D 88 C0 FE FF FF");
        injector::WriteMemory(pattern.get_first(2), static_cast<int32_t>(-Screen.fHudPosX), true);

        pattern = hook::pattern("8B 06 8B 4C 24 08");
        struct ClientRectHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.esi + 0x00);
                regs.ecx = static_cast<uint32_t>(Screen.fHudPosX + Screen.fHudPosX);
            }
        }; injector::MakeInline<ClientRectHook1>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("8B 07 8B 4C 24 0C");
        struct ClientRectHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.edi + 0x00);
                regs.ecx = 480;
            }
        }; injector::MakeInline<ClientRectHook2>(pattern.get_first(0), pattern.get_first(6));
    }

    if (bFixFOV)
    {
        static float hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 1.0f; // don't touch this
        static float mirrorScale = 0.45f;
        static float f129 = 1.28f;
        static float f04525 = 0.4525f;
        static float f1 = 1.0f; // horizontal for vehicle reflection
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        if (bScaling)
        {
            hor3DScale /= 1.0252904893f;
        }

        pattern = hook::pattern("DB 40 18 C7 44 24");
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x20) = 1.0f;
                regs.eax = *(uint32_t*)(regs.ebp + 4);

                if (regs.eax == 1 || regs.eax == 4) //Headlights stretching, reflections etc
                {
                    flt1 = hor3DScale;
                    flt2 = f04525;
                    flt3 = f129;
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
        }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(14));

        pattern = hook::pattern("D8 3D ? ? ? ? D9 1F E8 ? ? ? ? D9");
        injector::WriteMemory(pattern.get_first(2), &flt1, true);

        // FOV being different in menus and in-game fix
        pattern = hook::pattern("D8 0D ? ? ? ? ? ? E8 ? ? ? ? DD D8");
        injector::WriteMemory(pattern.get_first(2), &flt2, true);

        pattern = hook::pattern("D8 3D ? ? ? ? D9 E0 D9 5E 54 D9 44 24 20");
        injector::WriteMemory(pattern.get_first(2), &flt3, true);
    }

    if (nFMVWidescreenMode)
    {
        pattern = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF");
        injector::WriteMemory<float>(pattern.get_first(6 + 0), (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Bottom)
        injector::WriteMemory<float>(pattern.get_first(6 + 5), (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
        injector::WriteMemory<float>(pattern.get_first(6 + 10), -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Top)
        injector::WriteMemory<float>(pattern.get_first(6 + 15), -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)

        if (nFMVWidescreenMode > 1)
        {
            injector::WriteMemory<float>(pattern.get_first(6 + 0), (0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Bottom)
            injector::WriteMemory<float>(pattern.get_first(6 + 5), (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
            injector::WriteMemory<float>(pattern.get_first(6 + 10), -(0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Top)
            injector::WriteMemory<float>(pattern.get_first(6 + 15), -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)
        }
    }

    if (bHUDWidescreenMode && (Screen.fAspectRatio > (4.0f / 3.0f)))
    {
        static std::vector<CDatEntry> HudCoords;

        CIniReader iniReader("");
        auto DataFilePath = iniReader.GetIniPath();
        DataFilePath.replace_extension(".dat");

        LoadDatFile(DataFilePath, [](std::string_view line)
        {
            int32_t PosX, PosY;
            float fOffsetX, fOffsetY;
            sscanf_s(line.data(), "%*s %x %x %f %f %*f %*f", &PosX, &PosY, &fOffsetX, &fOffsetY);

            if (Screen.fAspectRatio < (16.0f / 9.0f))
                fOffsetX /= (((16.0f / 9.0f) / Screen.fAspectRatio) * 1.5f);

            HudCoords.emplace_back(PosX, PosY, fOffsetX, fOffsetY);
        });

        static float fMinimapPosX = 320.0f;
        static float fMinimapPosY = 240.0f;

        static auto WidescreenHud = [](HudPos& HudPosX, HudPos& HudPosY, bool bAddHudOffset = false)
        {
            if (nGameState != 3) // Disables WidescreenHUD in FrontEnd
            {
                auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
                                       [cc = CDatEntry(HudPosX.dwPos, HudPosY.dwPos, 0.0f, 0.0f)]
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
            }
        };

        pattern = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04");
        if (!pattern.empty())
        {
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
            }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("D8 05 ? ? ? ? D9 5B 68 D8 05 ? ? ? ? D9 5B 6C D9 40 3C");
            injector::WriteMemory(pattern.get_first(2), &fMinimapPosX, true);
            injector::WriteMemory(pattern.get_first(11), &fMinimapPosY, true);

            pattern = hook::pattern("8B 4D 08 D9 41 68 53 57 8B 5D 0C 8B F8");
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

                    _asm fld dword ptr[HudPosX.fPos]
                }
            }; injector::MakeInline<MinimapHook1>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("D9 40 68 8B 4D 0C D8 60 70 53 56 8D 91 83 00 00 00 D9 54 24 34");
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
            }; injector::MakeInline<MinimapHook2>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern("D9 40 1C 8B 4D 08 D9 40 20");
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
                    _asm {fld dword ptr[HudPosX.fPos]}
                    _asm {fld dword ptr[HudPosY.fPos]}
                }
            }; injector::MakeInline<MinimapHook3>(pattern.get_first(0), pattern.get_first(9));

            pattern = hook::pattern("D8 43 1C 8B 54 24 18 8B 44 24 10 D9 1A 8B 4E 68 50 51 57");
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
            }; injector::MakeInline<LapsHook>(pattern.get_first(0), pattern.get_first(7));
        }
    }

    if (bSkipIntro)
    {
        // patch bootflow

        // LS_IntroFMV.fng NEEDS to be there in order to hear the splash screen music!
        // This means that the intro video technically still plays, which means e3_title.mad still needs to be in the movies folder
        static const char* BootFlowScreens[] = { "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };
        static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd"};

        const char** outScreens = BootFlowScreens;
        const char** outLastScreen = &BootFlowScreens[_countof(BootFlowScreens) - 1];
        if (bShowLangSelect)
        {
            outScreens = BootFlowScreensLangSelect;
            outLastScreen = &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1];
        }

        uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("89 36 89 76 04 A1 ? ? ? ? 83 F8 05").get_first(0)) + 0x15;
        uintptr_t loc_4DE273 = loc_4DE21A + 0x59;
        uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 04 89 01 83 C7 04 81 FF ? ? ? ? 89 46 04 89 48 04 89 30 7C C8 8B 36").get_first(0)) + 8;

        // force US Retail bootflow
        injector::MakeJMP(loc_4DE21A, loc_4DE273);

        // list start
        injector::WriteMemory<const char**>(loc_4DE273 + 1, outScreens, true);

        // list end
        injector::WriteMemory<const char**>(loc_4DE2A0 + 2, outLastScreen, true);

        // IntroFMV NotificationMessage Hook -- for skipping the video for the first time it plays
        // dereference instruction at 0x004DE985 (IntroFMVScreen constructor) for the vtable
        uintptr_t loc_4DE985 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 85 C0 C7 44 24 10 00 00 00 00 C7 07 ? ? ? ? 74 49 68 ? ? ? ? B8").get_first(0)) + 0xF;
        uintptr_t IntroFMVScreenVTable = *reinterpret_cast<uintptr_t*>(loc_4DE985 + 2);
        IntroFMVScreen::NotificationMessageFuncAddr = *reinterpret_cast<uintptr_t*>(IntroFMVScreenVTable + 8);
        // dereference instruction at 0x004DEA5A
        uintptr_t loc_4DEA5A = reinterpret_cast<uintptr_t>(hook::pattern("5F 5E C2 10 00 8B 49 0C 51 E8 ? ? ? ? 83 C4 04 5E C2 10 00").get_first(0)) + 9;
        IntroFMVScreen::FEngPopPackageAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_4DEA5A));

        injector::WriteMemory<uintptr_t>(IntroFMVScreenVTable + 8, (uintptr_t)&IntroFMVScreen::hkNotificationMessage, true);
    }
    else if (bShowLangSelect)
    {
        static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_EAlogo.fng", "LS_PSAMovie.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };
        
        uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("89 36 89 76 04 A1 ? ? ? ? 83 F8 05").get_first(0)) + 0x15;
        uintptr_t loc_4DE273 = loc_4DE21A + 0x59;
        uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 04 89 01 83 C7 04 81 FF ? ? ? ? 89 46 04 89 48 04 89 30 7C C8 8B 36").get_first(0)) + 8;

        // force US Retail bootflow
        injector::MakeJMP(loc_4DE21A, loc_4DE273);

        // list start
        injector::WriteMemory<const char**>(loc_4DE273 + 1, BootFlowScreensLangSelect, true);

        // list end
        injector::WriteMemory<const char**>(loc_4DE2A0 + 2, &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1], true);
    }

    if (!szCustomUserFilesDirectoryInGameDir.empty())
    {
        CustomUserDir = GetExeModulePath<std::filesystem::path>();
        CustomUserDir.append(szCustomUserFilesDirectoryInGameDir);

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
        {
            CreateDirectoryW((LPCWSTR)(CustomUserDir.u16string().c_str()), NULL);
            memcpy(pszPath, CustomUserDir.u8string().data(), CustomUserDir.u8string().size() + 1);

            return S_OK;
        };

        auto dword_41C481 = *hook::pattern("68 ? ? ? ? 6A 00 6A 00 68 23 80 00 00 6A 00 FF 15").count(1).get(0).get<uint32_t*>(16 + 2);
        auto pattern = hook::pattern(pattern_str(0xFF, 0x15, to_bytes(dword_41C481)));
        for (size_t i = 0; i < pattern.size(); i++)
        {
            injector::MakeCALL(pattern.get(i).get<void*>(0), static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
            injector::MakeNOP(pattern.get(i).get<void*>(5), 1, true);
        }
    }

    if (nImproveGamepadSupport)
    {
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 A1 ? ? ? ? 57");
        CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x004482F0
        pattern = hook::pattern("56 8B F2 89 8E 98 00 00 00 8B 0D ? ? ? ? 89 86 94 00 00 00 8B 86 9C 00 00 00");
        ResourceFileBeginLoading = (void(__fastcall *)(int a1, void* rsc, int a2)) pattern.get_first(0); //0x00448110;

        if (nImproveGamepadSupport < 3)
        {
            static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length());

            if (nImproveGamepadSupport == 1)
                TPKPath += "buttons-xbox.tpk";
            else if (nImproveGamepadSupport == 2)
                TPKPath += "buttons-playstation.tpk";

            static injector::hook_back<void(__cdecl*)()> hb_448600;
            auto LoadTPK = []()
            {
                if (std::filesystem::exists(TPKPath))
                {
                    LoadResourceFile(TPKPath.c_str(), 1);
                }
                return hb_448600.fun();
            };

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 6A 00 6A 00 6A 00 68 F0 59 00 00"); //0x447280
            hb_448600.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();
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

        static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate;
        pattern = hook::pattern("7C ? 5F 5D 5E 33 C0 5B C2 08 00"); //0x41989E
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8 + 5), 0x900008C2, true);
        //static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("").count(1).get(0).get<uint32_t*>(8); //0x
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                PadState* PadKeyPresses = *(PadState**)(regs.esp + 0x4);
                //Keyboard
                //006F9358 backspace
                //006F931C enter
                if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && nGameState == 3)
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

        // FrontEnd button remap (through game code, not key emulation)
        auto pattern = hook::pattern("8B 86 ? ? ? ? 8B 0C ? ? ? ? ? 85 C9 0F"); // 004071C5
        struct FrontEndRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uintptr_t*)(regs.esi + 0x0130);

                int LB = (regs.esi + 0x0130) - (0x88);
                *(uintptr_t*)LB = 0x1A; // FE Action "Comma"
                int RB = (regs.esi + 0x0130) - (0x84);
                *(uintptr_t*)RB = 0x19; // FE Action "Period"
            }
        };
        injector::MakeInline<FrontEndRemap>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
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

    if (nHideDebugObjects)
    {
        if (nHideDebugObjects == 2)
        {
            //D3D9 EndScene Hook
            pattern = hook::pattern("A1 ? ? ? ? 8B 08 83 C4 04 50");
            static LPDIRECT3DDEVICE9* pDevice = *pattern.get_first<LPDIRECT3DDEVICE9*>(1);
            struct EndSceneHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)pDevice;
                    regs.ecx = *(uint32_t*)(regs.eax);

                    if (nGameState == 3)
                    {
                        DrawRect(*pDevice, 0, 0, static_cast<int32_t>(Screen.fHudOffsetReal), Screen.nHeight);
                        DrawRect(*pDevice, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal), 0, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal + Screen.fHudOffsetReal), Screen.nHeight);
                    }
                }
            }; injector::MakeInline<EndSceneHook>(pattern.get_first(0), pattern.get_first(7));
        }
        else
        {
            using namespace DebugTags;
            
            FEngSetInvisible_Addr = static_cast<decltype(FEngSetInvisible_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8A 43 61"))); //0x000495F70;
            FEHashUpper_Addr = static_cast<decltype(FEHashUpper_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 75 6C"))); //0x004FD230;
            FEngFindObject_Addr = static_cast<decltype(FEngFindObject_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 3B C6 75 16"))); //0x004FFB70;
            FEPkgMgr_FindPackage_Addr = static_cast<decltype(FEPkgMgr_FindPackage_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 50 1C"))); //0x004F65D0;

            auto pattern = hook::pattern("8B 4C 24 4C 8B 16");
            FEPkgMgr_SendMessageToPackage_Cave_Exit = reinterpret_cast<decltype(FEPkgMgr_SendMessageToPackage_Cave_Exit)>(pattern.get_first(4)); //0x004F7D18;
            injector::MakeJMP(pattern.get_first(-4), FEPkgMgr_SendMessageToPackage_Cave, true);
        }
    }

    if (bBlackMagazineFix)
    {
        pattern = hook::pattern("8B F8 A0 ? ? ? ? 84 C0 0F 85");
        byte_735768 = *pattern.get_first<uint8_t*>(3);

        pattern = hook::pattern("83 3D ? ? ? ? 01 8B 77 58");
        dword_73645C = *pattern.get_first<uint32_t*>(2);
        injector::MakeRangedNOP(pattern.get_first(0), pattern.get_first(7));
        injector::MakeCALL(pattern.get_first(0), CmpShouldClearSurfaceDuring3DRender); // 4098A2

        pattern = hook::pattern("6A 00 BE ? ? ? ? 74 ? BE");
        off_71AA4C = *pattern.get_first<void*>(10);
        injector::MakeCALL(pattern.get_first(9), MaybeClearSurfaceDuringBackgroundRender); // 409CF2
    }

    //__mbclen to strlen, "---" bug fix
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 76 21 8D 43 60");
    if (!pattern.empty())
    {
        injector::MakeCALL(pattern.get_first(0), strlen, true);
    }

    if (nFPSLimit)
    {
        if (nFPSLimit < 0)
        {
            if (nFPSLimit == -1)
                nFPSLimit = GetDesktopRefreshRate();
            else if (nFPSLimit == -2)
                nFPSLimit = GetDesktopRefreshRate() * 2;
            else
                nFPSLimit = 60;
        }

        // the game limits FPS 2x over the frametime (or just reports it that way to D3D) -- this is the real game framerate here!
        static float FrameTime = 1.0f / nFPSLimit;
        // Video mode frametime
        uint32_t* dword_6CC7B0 = *hook::pattern("83 EC 10 A1 ? ? ? ? 89 44 24 04").count(1).get(0).get<uint32_t*>(31);
        injector::WriteMemory(dword_6CC7B0, FrameTime, true);
        // RealTimestep frametime
        uint32_t* dword_6F0890 = *hook::pattern("99 D9 05 ? ? ? ? B9 64 00 00 00").count(1).get(0).get<uint32_t*>(3);
        injector::WriteMemory(dword_6F0890, FrameTime, true);
        uint32_t* dword_6CCDEC = *hook::pattern("99 D9 05 ? ? ? ? B9 64 00 00 00").count(1).get(0).get<uint32_t*>(63);
        injector::WriteMemory(dword_6CCDEC, FrameTime * 2.0f, true);
        // a function in eDisplayFrame (particle effects?) frametime
        uint32_t* dword_40A744 = hook::pattern("68 89 88 88 3C").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory(dword_40A744, FrameTime, true);
        // something related to framerate and/or seconds. This value has to be an integer multiple of 60, otherwise the game can freeze. This affects some gameplay features such as NOS and menus.
        uint32_t* dword_6CC8B0 = *hook::pattern("83 E1 01 0B F9 D9 44 24 28 D8 1D ? ? ? ?").count(1).get(0).get<uint32_t*>(11);
        static float FrameSeconds = nFPSLimit;

        if (nFPSLimit % 60)
            FrameSeconds = nFPSLimit - (nFPSLimit % 60);

        // needed to avoid crashes, anything above 120 seems to be problematic at the moment...
        if (FrameSeconds > 120.0f)
            FrameSeconds = 120.0f;

        if (FrameSeconds < 60.0)
            FrameSeconds = 60.0;
        injector::WriteMemory(dword_6CC8B0, FrameSeconds, true);
        // another frametime -- seems to affect some gameplay elements...
        uint32_t* dword_6B5C08 = *hook::pattern("DF E0 F6 C4 41 7A ? 8A 44 24 12 D9 05 ? ? ? ?").count(1).get(0).get<uint32_t*>(13);
        injector::WriteMemory(dword_6B5C08, FrameTime, true);

        // GAME BUGFIX: fix sticky steering (especially on high FPS)
        // kill the autocentering!
        uint32_t* dword_460A3D = hook::pattern("D9 54 24 14 D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05 7A 08").count(1).get(0).get<uint32_t>(0x11); //0x460A2C anchor
        injector::WriteMemory<uint8_t>(dword_460A3D, 0xEB, true);

        // explanation:
        // this is a native game bug which can sadly happen on a bone-stock game, it's just been exaggerated by the higher FPS and faster input
        // what happens is this: if the steering curve isn't finished with the steering (if the wheels aren't centered), it will outright ignore the other direction you're pressing
        // to exaggerate the issue even further: nop out the instruction at 00460DFA which will make the steering extremely slow, then try steering all the way to the left, then quickly go and hold right. Notice how it *won't* continue to go right.
        // so to summarize in a chain of events:
        // 1. game controller sets steering value (-1.0 to 1.0)
        // 2. World::DoTimestep updates the player info and somewhere
        // 3. Somewhere in that chain it calls PlayerSteering::DoUndergroundSteering with the steering value as argument
        // 4. Steering curves get processed at PlayerSteering::CalculateSteeringSpeed                                          <-- THIS IS WHERE THE BUG HAPPENS (or shortly thereafter)
        // 5. Steering value gets passed to the car
        // Skipping 4. is possible by enabling the bool at 00736514, but that is intended exclusively for wheel input and not gamepad/keyboard input.
    }

    if ((FEScale::fFEScale != 1.0f) || (FEScale::fFMVScale != 1.0f) || (FEScale::bAutoFitFE) || (FEScale::bAutoFitFMV))
    {
        FEScale::bEnabled = true;

        uintptr_t loc_409E3C = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 18 00 00 80 3F C7 44 24 1C 00 00 00 00").get_first(0)) + 0xBA;
        uintptr_t loc_5A4A62 = reinterpret_cast<uintptr_t>(hook::pattern("8B 48 14 85 C9 74 ? 8B 11 FF 92 A0 00 00 00 A0").get_first(0)) - 0x90;
        uintptr_t loc_5A4C02 = reinterpret_cast<uintptr_t>(hook::pattern("83 C4 08 68 B9 0E 96 C3").get_first(0)) - 0x2C;
        uintptr_t loc_5A4C92 = reinterpret_cast<uintptr_t>(hook::pattern("8A 41 30 84 C0 75 ? 56 C7 05 ? ? ? ? 00 00 00 00").get_first(0)) + 8;
        uintptr_t loc_40A669 = reinterpret_cast<uintptr_t>(hook::pattern("8B 10 50 FF 92 A4 00 00 00 6A 01 E8").get_first(0)) + 0xB;

        FEScale::SetTransformAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_409E3C));
        FEScale::MovieIsStartedAddr = *reinterpret_cast<uintptr_t*>(loc_5A4A62 + 2);
        FEScale::DrawFEAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_40A669));

        if (FEScale::SetTransformAddr & 0xFF000000)
        {
            // NFS reshade workaround -- TODO: remove this once NFS reshade is updated!
            // reshade in question: https://github.com/xan1242/reshade

            FEScale::bWorkaroundReshade = true;
            FEScale::InfectedTransform = FEScale::SetTransformAddr;

            uintptr_t loc_40432B = reinterpret_cast<uintptr_t>(hook::pattern("8B 54 24 18 52 E8 ? ? ? ? 83 C4 08").get_first(0)) - 5;
            FEScale::SetTransformAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_40432B));
        }

        injector::MakeInline<FEScale::MovieStartHook>(loc_5A4A62, loc_5A4A62 + 0xA);
        injector::MakeInline<FEScale::MovieStopHook>(loc_5A4C02, loc_5A4C02 + 0xA);
        injector::MakeInline<FEScale::MovieStopHook>(loc_5A4C92, loc_5A4C92 + 0xA);
        injector::MakeCALL(loc_409E3C, FEScale::SetTransformHook);

        FEScale::Update();
    }

    // windowed mode
    if (nWindowedMode)
    {
        uint32_t* dword_408AA1 = hook::pattern("8B 0D ? ? ? ? 68 00 00 C0 10").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_408AB5 = (uint32_t*)((uint32_t)dword_408AA1 + 0x14);
        
        uint32_t* dword_4089B1 = hook::pattern("68 00 00 08 90 68 ? ? ? ? 68 ? ? ? ? 53 FF 15").count(1).get(0).get<uint32_t>(16);
        uint32_t* dword_408975 = hook::pattern("68 00 00 08 90 8D 44 24 14 50 89 4C 24 20 89 54 24 24 FF 15").count(1).get(0).get<uint32_t>(18);
        uint32_t* WindowedMode_73637C = *hook::pattern("B9 0E 00 00 00 BF ? ? ? ? F3 AB A1 ? ? ? ?").count(1).get(0).get<uint32_t*>(13);

        // skip SetWindowLong because it messes things up
        injector::MakeJMP(dword_408AA1, dword_408AB5, true);
        // hook the offending functions
        injector::MakeNOP(dword_4089B1, 6, true);
        injector::MakeCALL(dword_4089B1, CreateWindowExA_Hook_NFSU, true);
        injector::MakeNOP(dword_408975, 6, true);
        injector::MakeCALL(dword_408975, WindowedModeWrapper::AdjustWindowRect_Hook, true);
        // enable windowed mode variable
        *WindowedMode_73637C = 1;

        switch (nWindowedMode)
        {
        case 5:
            WindowedModeWrapper::bStretchWindow = true;
            break;
        case 4:
            WindowedModeWrapper::bScaleWindow = true;
            break;
        case 3:  // TODO: implement dynamic resizing (like in MW)
            WindowedModeWrapper::bEnableWindowResize = true;
        case 2:
            WindowedModeWrapper::bBorderlessWindowed = false;
            break;
        default:
            break;
        }
    }
    else
    {
        //icon fix
        auto SetWindowLongHook = [](HWND hWndl, int nIndex, LONG dwNewLong)
        {
            SetWindowLongA(hWndl, nIndex, dwNewLong);
            SendMessage(hWndl, WM_SETICON, ICON_BIG, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)));
            SendMessage(hWndl, WM_SETICON, ICON_SMALL, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)));
        };

        pattern = hook::pattern("FF 15 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 33 D2 3B CB 0F 95 C2");
        injector::MakeNOP(pattern.get_first(0), 6, true);
        injector::MakeCALL(pattern.get_first(0), static_cast<void(WINAPI*)(HWND, int, LONG)>(SetWindowLongHook), true);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("68 20 03 00 00 BE 58 02 00 00").count_hint(1).empty(), 0x1100);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, uint32_t reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}