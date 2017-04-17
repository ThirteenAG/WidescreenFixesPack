#include "stdafx.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fDynamicScreenFieldOfViewScale;
    float fFieldOfView;
    float fAspectRatio;
    float fDiffFactor;
    float fHudScale;
    float fHudOffset;
    float fHudOffset2;
    float fHudOffsetWide;
    float fWidescreenHudOffset;
    float fWidthScale;
    float fHalfWidthScale;
    float f1_fWidthScale;
    float fDouble1_fWidthScale;
    float fHalf1_fWidthScale;
    float fMirrorFactor;
    float fPSriteHudOffset;
    float fPSriteHudScale;
} Screen;

struct TextCoords
{
    float a;
    float b;
    float c;
    float d;
};

uint8_t* bIsInComicsMode;
uint8_t* bIsInMenu;
bool bComicsMode;

DWORD WINAPI ComicsHandler(LPVOID)
{
    static bool bPatched;
    CIniReader iniReader("");
    int32_t nComicsModeKey = iniReader.ReadInteger("MAIN", "ComicsModeKey", VK_F2);

    static float fViewPortSizeX = 640.0f;
    static float fViewPortSizeY = 480.0f;
    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 35 ? ? ? ? D8 4C 24 38 D9 5C 24 34");
    injector::WriteMemory(pattern.get_first(2), &fViewPortSizeX, true); //0x100176D4
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 35 ? ? ? ? D8 4C 24 38 D9 5C 24 30");
    injector::WriteMemory(pattern.get_first(2), &fViewPortSizeY, true); //0x10017719
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 35 ? ? ? ? 89 1D ? ? ? ? 89 1D");
    injector::WriteMemory(pattern.get_first(2), &fViewPortSizeY, true); //0x1001779A

    while (true)
    {
        Sleep(0);

        if (bIsInComicsMode && bIsInMenu)
        {
            if ((GetAsyncKeyState(nComicsModeKey) & 1) && (*bIsInComicsMode == 255))
            {
                bComicsMode = !bComicsMode;
                bPatched = !bPatched;
                iniReader.WriteInteger("MAIN", "ComicsMode", bComicsMode);
                while ((GetAsyncKeyState(nComicsModeKey) & 0x8000) > 0) { Sleep(0); }
            }

            if (bComicsMode)
            {
                if (*bIsInMenu && *bIsInComicsMode == 255) //+main menu check
                {
                    if (!bPatched)
                    {
                        fViewPortSizeX = 480.0f * Screen.fAspectRatio;
                        fViewPortSizeY = 480.0f;
                        bPatched = true;
                    }
                }
                else
                {
                    if (bPatched)
                    {
                        fViewPortSizeX = 640.0f;
                        fViewPortSizeY = 480.0f;
                        bPatched = false;
                    }
                }
            }
            else
            {
                if (*bIsInMenu && *bIsInComicsMode == 255) //+main menu check
                {
                    if (!bPatched)
                    {
                        fViewPortSizeX = (480.0f * Screen.fAspectRatio) / 1.17936117936f;
                        fViewPortSizeY = 480.0f / 1.17936117936f;
                        bPatched = true;
                    }
                }
                else
                {
                    if (bPatched)
                    {
                        fViewPortSizeX = 640.0f;
                        fViewPortSizeY = 480.0f;
                        bPatched = false;
                    }
                }
            }
        }
    }
    return 0;
}

DWORD WINAPI InitX_GameObjectsMFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("X_GameObjectsMFC"), "D8 3D ? ? ? ? D9 5C 24 0C D9");

    if (pattern.count_hint(4).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitX_GameObjectsMFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(4).empty()) { Sleep(0); };

    //mirrors fix
    injector::WriteMemory(pattern.get_first(2), &Screen.fMirrorFactor, true); //0x10101F39

    //doors graphics fix
    static float fVisibilityFactor1 = 0.5f;
    static float fVisibilityFactor2 = 1.5f;
    pattern = hook::module_pattern(GetModuleHandle("X_GameObjectsMFC"), "D9 05 ? ? ? ? 89 44 24 1C"); //1000AD9E
    injector::WriteMemory(pattern.get_first(2), &fVisibilityFactor1, true);
    pattern = hook::module_pattern(GetModuleHandle("X_GameObjectsMFC"), "D9 05 ? ? ? ? 89 4C 24 18"); //1000AD55
    injector::WriteMemory(pattern.get_first(2), &fVisibilityFactor2, true);

    return 0;
}

DWORD WINAPI InitWF(LPVOID)
{
    while (!Screen.nWidth || !Screen.nHeight) { Sleep(0); };

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.fDiffFactor = 1.0f / ((4.0f / 3.0f) / (Screen.fAspectRatio));
    Screen.fWidthScale = 640.0f / Screen.fAspectRatio;
    Screen.fHalfWidthScale = Screen.fWidthScale / 2.0f;
    Screen.f1_fWidthScale = 1.0f / Screen.fWidthScale;
    Screen.fDouble1_fWidthScale = Screen.f1_fWidthScale * 2.0f;
    Screen.fHalf1_fWidthScale = Screen.f1_fWidthScale / 2.0f;
    Screen.fHudOffset = ((-1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
    Screen.fHudOffset2 = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudScale = (1.0f / (480.0f * Screen.fAspectRatio)) * 2.0f;
    Screen.fMirrorFactor = 1.0f * ((4.0f / 3.0f) / (Screen.fAspectRatio));

    if (Screen.fAspectRatio <= ((4.0f / 3.0f) + 0.03f))
        return 0;

    CIniReader iniReader("");
    bool bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
    static bool bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
    Screen.fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 100.0f);
    bComicsMode = iniReader.ReadInteger("MAIN", "ComicsMode", 1) != 0;
    if (!Screen.fWidescreenHudOffset) { Screen.fWidescreenHudOffset = 100.0f; }
    float fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (!fFOVFactor) { fFOVFactor = 1.0f; }
    bool bScopesFix = iniReader.ReadInteger("MAIN", "ScopesFix", 1) != 0;
    Screen.fDynamicScreenFieldOfViewScale = fFOVFactor;

    //fix aspect ratio
    InitX_GameObjectsMFC(NULL);

    auto PCameraValidate = [](uintptr_t PCamera, uintptr_t edx)
    {
        float fParam0 = *(float *)(PCamera + 0x11C);
        float fParam1 = *(float *)(PCamera + 0x1DC);
        float fParam2 = *(float *)(PCamera + 0x1D8);
        float fParam3 = *(float *)(PCamera + 0x1D4);
        float fParam4 = *(float *)(PCamera + 0x1D0);

        float fParam5 = *(float *)(PCamera + 0x1E0);
        float fParam6 = *(float *)(PCamera + 0x220);
        //BYTE Param7 = *(BYTE *)(PCamera + 0x208);
        //BYTE Param8 = *(BYTE *)(PCamera + 0x480);

        if ((*(BYTE *)(PCamera + 0xEC) >> 6) & 1)
        {
            float v2 = (float)tan(fParam0 / 2.0);

            if ((fParam5 == 0.05f && fParam6 != -1.0f) || (fParam5 == 1.0f && fParam6 >= 0.7966f && fParam6 <= 0.79669f) /*&& Param7 != 0x40*/ /*&& Param7 == 0x40 && Param8 == 0*/) //0.7966 is 2d background scaling, bugged on win7
            {
                *(float *)(PCamera + 0x20C) = v2 * Screen.fDiffFactor;
            }
            else
                *(float *)(PCamera + 0x20C) = v2;

            *(float *)(PCamera + 0x210) = (fParam1 - fParam2) * v2 / (fParam3 - fParam4);
        }
    };

    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "00 00 00 00 3F 3F 3F");
    injector::MakeCALL((DWORD)GetModuleHandle("e2mfc") + 0x17077, static_cast<void(__fastcall *)(uintptr_t, uintptr_t)>(PCameraValidate), true); //0x10017077

    //corrupted graphic in tunnel in mp1, not sure if needed in mp2 (DisableSubViewport)
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "83 EC 08 56 8B F1 8B 86 E8");
    injector::MakeRET(pattern.get_first(), 0x10, true); //100156A0 ret 10h

    //ForceEntireViewport 0x10014C80 mov ax, 1 ret
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "A0 ? ? ? ? C3");
    injector::WriteMemory<uint8_t>(pattern.count(3).get(2).get<void*>(0), 0xB0, true);
    injector::WriteMemory(pattern.count(3).get(2).get<void*>(1), 0x90909001, true);

    //object disappearance fix
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "C7 05 ? ? ? ? ? ? ? ? D8 35"); //0x10015B87
    injector::WriteMemory<float>(pattern.get_first(6), 0.0f, true);

    if (bFixHud)
    {
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? 8B 44 24 24 52 50 51"); //0x10042B50
        injector::WriteMemory<float>(*pattern.get_first<void*>(2), Screen.fHudOffset, true);
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? D9 44 24 1C"); //0x10042B58
        injector::WriteMemory<float>(*pattern.get_first<void*>(2), Screen.fHudScale, true);

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? 8B 44 24 1C 52 50 51"); //0x1000F05E
        injector::WriteMemory(pattern.get_first(2), &Screen.fPSriteHudOffset, true);
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? D9 44 24 20 53"); //0x1000F011
        injector::WriteMemory(pattern.get_first(2), &Screen.fPSriteHudScale, true);

        static bool bBulletTimeRender, bDynShadowRender, bScopesFix;
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "C7 06 ? ? ? ? C7 46 10 ? ? ? ? C7 07 ? ? ? ? 6A 01");
        auto off_10042DAC = *pattern.get_first<uintptr_t>(2);
        static auto PSpriteExecuteAlways = (uint8_t(__thiscall *)(uintptr_t)) *(uintptr_t*)(off_10042DAC + 0x4C);

        auto PSpriteExecuteAlwaysHook = [](uintptr_t PSprite, uintptr_t edx) -> int8_t
        {

            float fParam0 = *(float *)(PSprite + 0x15C);
            float fParam1 = *(float *)(PSprite + 0x180);
            BYTE Param3 = *(BYTE *)(PSprite + 0x182);

            if (fParam0 == 640.0f && fParam1 == 0.0f && Param3 == 0x00)
                bBulletTimeRender = true;

            if (fParam0 == 640.0f && (fParam1 == 0.0078125f || fParam1 == 0.015625) /*&& Param3 == 0x00*/) //for some reason it's 0.015625 on windows 7
                bDynShadowRender = true;

            if (fParam0 != 640.0f || bDynShadowRender || bBulletTimeRender)
            {
                Screen.fPSriteHudOffset = Screen.fHudOffset;
                Screen.fPSriteHudScale = Screen.fHudScale;

                return PSpriteExecuteAlways(PSprite);
            }

            Screen.fPSriteHudOffset = -1.0f;
            Screen.fPSriteHudScale = 0.003125f;
            return PSpriteExecuteAlways(PSprite);
           
        };
        injector::WriteMemory(off_10042DAC + 0x4C, static_cast<int8_t(__fastcall *)(uintptr_t, uintptr_t)>(PSpriteExecuteAlwaysHook), true); //0x10042DF8

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 25 ? ? ? ? 89 44 24 44 83 EC 08");
        static auto pHudElementPosX = *pattern.get_first<float*>(2); //0x10061134
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 25 ? ? ? ? D9 5C 24 20");
        static auto pHudElementPosY = *pattern.get_first<float*>(2); //0x10061138
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? 8D 81 54");
        static auto pf10042294 = *pattern.get_first<float*>(2); //0x10042294

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? 8B 86 64 01 00 00 ");
        struct P_HudPosHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto ElementPosX = *pHudElementPosX;
                auto ElementPosY = *pHudElementPosY;
                auto ElementNewPosX1 = ElementPosX;
                auto ElementNewPosY1 = ElementPosY;
                auto ElementNewPosX2 = ElementPosX;
                auto ElementNewPosY2 = ElementPosY;
                
                if (bWidescreenHud)
                {
                    if (ElementPosX == 7.0f) // bullet time meter
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                
                    if (ElementPosX == 8.0f) // bullet time overlay()
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                
                    if (ElementPosX == 12.0f) // painkillers
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                
                    if (ElementPosX == 22.5f) //health bar and overlay
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                
                    if (ElementPosX == 96.0f) // other weapons name
                    {
                        ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                    }
                
                    if (ElementPosX == 192.0f) //molotovs/grenades name pos
                    {
                        ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                    }
                }
                
                ElementNewPosX2 = ElementNewPosX1;
                
                if (bBulletTimeRender || (bDynShadowRender && ElementPosX == 0.0f))
                {
                    ElementNewPosX1 = ElementPosX + Screen.fHudOffset2;
                    ElementNewPosX2 = ElementPosX - Screen.fHudOffset2;
                    bBulletTimeRender = false;
                    bDynShadowRender = false;
                }
                else
                {
                    if (bScopesFix && ElementPosX == 0.0f && ElementPosY == 0.0f && regs.eax == 2 && regs.ecx == 640.0f && *(uint32_t*)(regs.esp + 0x20) != 0x40400000 && *(uint32_t*)(regs.esp + 0x30) != 0x43800000)
                    {
                        ElementNewPosY1 = ElementPosY + Screen.fHudOffset2; //to do, isn't scaled properly
                        ElementNewPosY2 = ElementPosY - Screen.fHudOffset2;
                    }
                }

                regs.eax = *(uint32_t*)(regs.esi + 0x164);
                *(uint32_t*)(regs.esp + 0x44) = regs.eax;
                regs.esp -= 8;
                
                *(float*)(regs.esp + 0x30) = 0.0f - ElementNewPosX1;
                *(float*)(regs.esp + 0x1C) -= ElementNewPosX2;
                *(float*)(regs.esp + 0x2C) = 0.0f - ElementNewPosY1;
                *(float*)(regs.esp + 0x20) -= ElementNewPosY2;

            }
        }; injector::MakeInline<P_HudPosHook>(pattern.get_first(0), pattern.get_first(73)); //0x1000F2B3
        injector::WriteMemory(pattern.get_first(10), 0x9008EC83, true); //sub     esp, 8

        if (bWidescreenHud)
        {
            Screen.fHudOffsetWide = Screen.fWidescreenHudOffset;

            if (Screen.fAspectRatio < (16.0f / 9.0f))
            {
                Screen.fHudOffsetWide = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
            }

            pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? D8 8B 98");
            static auto pTextElementPosX = *pattern.get_first<TextCoords*>(2); //0x10060374
            struct P_TextPosHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float TextUnkVal = *(float*)(*(uintptr_t*)(regs.esp + 0xC) + 0x5C);
                    float TextPosX = pTextElementPosX->a;
                    float TextNewPosX = TextPosX;

                    if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
                    {
                        if (TextUnkVal < 0.0f)
                        {
                            TextNewPosX = TextPosX - Screen.fHudOffsetWide;
                        }
                        else
                        {
                            TextNewPosX = TextPosX + Screen.fHudOffsetWide;
                        }
                    }
                    //*(float*)(regs.esp + 0x2C) = TextPosX * *(float*)(regs.ebx + 0x198);
                    __asm fld dword ptr[TextNewPosX]
                }
            }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6)); //0x1000AACC
        }
    }


    pattern = hook::module_pattern(GetModuleHandle("e2_d3d8_driver_mfc"), "C7 86 ? ? ? ? 00 00 00 00 81 FF 99 00 00 00");
    bIsInComicsMode = (uint8_t*)(*pattern.get_first<uintptr_t>(2) + 0x680); //0x1007D2E8

    pattern = hook::module_pattern(GetModuleHandle("X_ModesMFC"), "C6 41 71 01 C3"); //void __thiscall X_ModeBase::pauseTimeUpdate(X_ModeBase *__hidden this)
    struct pauseTimeUpdateHook
    {
        void operator()(injector::reg_pack& regs)
        {
            bIsInMenu = (uint8_t*)(regs.ecx + 0x71);
            *bIsInMenu = 1;
        }
    }; injector::MakeInline<pauseTimeUpdateHook>(pattern.get_first(0));
    injector::MakeRET(pattern.get_first(5));

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&ComicsHandler, NULL, 0, NULL);

    //FOV
    if (fFOVFactor && fFOVFactor != 1.0f)
    {
        pattern = hook::module_pattern(GetModuleHandle("X_GameObjectsMFC"), "8B 44 24 04 89 81 38 05 00 00"); //void __thiscall X_LevelRuntimeCamera::setFOV(X_LevelRuntimeCamera *this, float)
        struct pauseTimeUpdateHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.eax = *(float*)(regs.esp + 4);
                *(float*)(regs.ecx + 0x538) = *(float*)&regs.eax * Screen.fDynamicScreenFieldOfViewScale;
            }
        }; injector::MakeInline<pauseTimeUpdateHook>(pattern.get_first(0), pattern.get_first(10));
    }
    return 0;
}

DWORD WINAPI InitE2MFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "E8 ? ? ? ? 85 C0 89 44 24 38 DB");

    if (pattern.count_hint(4).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitE2MFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(4).empty()) { Sleep(0); };

    auto PDriverGetWidth = [](uintptr_t P_Driver__m_initializedDriver, uintptr_t edx) -> int32_t
    {
        if (*(uintptr_t*)(P_Driver__m_initializedDriver + 48))
            Screen.nWidth = *(int32_t*)(P_Driver__m_initializedDriver + 52);
        else
            Screen.nWidth = *(int32_t*)(P_Driver__m_initializedDriver + 4);

        return Screen.nWidth;
    };

    auto PDriverGetHeight = [](uintptr_t P_Driver__m_initializedDriver, uintptr_t edx) -> int32_t
    {
        if (*(uintptr_t*)(P_Driver__m_initializedDriver + 48))
            Screen.nHeight = *(int32_t *)(P_Driver__m_initializedDriver + 56);
        else
            Screen.nHeight = *(int32_t *)(P_Driver__m_initializedDriver + 8);

        return Screen.nHeight;
    };

    //get resolution
    injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), static_cast<int32_t(__fastcall *)(uintptr_t, uintptr_t)>(PDriverGetWidth), true);  //e2mfc + 0x15582
    injector::MakeCALL(pattern.get(1).get<uintptr_t>(0), static_cast<int32_t(__fastcall *)(uintptr_t, uintptr_t)>(PDriverGetHeight), true); //e2mfc + 0x155AB

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitWF, NULL, 0, NULL);
    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    bool bUseGameFolderForSavegames = iniReader.ReadInteger("MAIN", "UseGameFolderForSavegames", 0) != 0;
    if (bUseGameFolderForSavegames)
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x41D14C

    bool bAltTab = iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0) != 0;
    if (bAltTab)
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B CF C6 87 82 00 00 00 00");
        injector::MakeNOP(pattern.get_first(0), 5, true); //0x404935
    }

    static int32_t nCutsceneBorders = iniReader.ReadInteger("MAIN", "CutsceneBorders", 1);
    if (nCutsceneBorders)
    {
        auto f = [](uintptr_t _this, uintptr_t edx) -> float
        {
            if (nCutsceneBorders > 1)
                return *(float *)(*(uintptr_t*)_this + 4692) * (1.0f / ((4.0f / 3.0f) / Screen.fAspectRatio));
            else
                return 1.0f;
        };
        pattern = hook::pattern("E8 ? ? ? ? D8 2D ? ? ? ? D8");
        injector::MakeCALL(pattern.get_first(), static_cast<float(__fastcall *)(uintptr_t, uintptr_t)>(f), true); //0x488C68
    }
    
    return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
        InitE2MFC(NULL);
    }
    return TRUE;
}