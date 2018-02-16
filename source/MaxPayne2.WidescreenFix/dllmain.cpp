#include "stdafx.h"
#include "dxsdk\d3d8.h"
#include "dxsdk\d3dvtbl.h"

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
    float fHudOffsetReal;
    float fHudOffsetWide;
    float fWidescreenHudOffset;
    float fWidthScale;
    float fHalfWidthScale;
    float f1_fWidthScale;
    float fDouble1_fWidthScale;
    float fHalf1_fWidthScale;
    float fMirrorFactor;
    float fViewPortSizeX = 640.0f;
    float fViewPortSizeY = 480.0f;
    float fNovelsScale = 0.003125f;
    float fNovelsOffset = -1.0f;
    bool bIsX_QuadRenderer;
    bool bIsSniperZoomOn;
    bool bIsFading;
    bool bDrawBorders;
    bool bIs2D;
    bool bIsSkybox;
    bool bGraphicNovelMode;
    bool bIsInGraphicNovel;
    bool* bIsInCutscene;
} Screen;

struct TextCoords
{
    float a;
    float b;
    float c;
    float d;
};

typedef HRESULT(STDMETHODCALLTYPE* EndScene_t)(LPDIRECT3DDEVICE8);
EndScene_t RealEndScene = NULL;

void DrawRect(LPDIRECT3DDEVICE8 pDevice, int32_t x, int32_t y, int32_t w, int32_t h, D3DCOLOR color = D3DCOLOR_XRGB(0, 0, 0))
{
    D3DRECT BarRect = { x, y, x + w, y + h };
    pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 0, 0);
}

HRESULT WINAPI EndScene(LPDIRECT3DDEVICE8 pDevice)
{
    if (Screen.bDrawBorders && !Screen.bIsInGraphicNovel)
    {
        auto x = Screen.fHudOffsetReal;
        auto y = (x - Screen.fHudOffsetReal);
        
        DrawRect(pDevice, 0, 0, static_cast<int32_t>(x), Screen.nHeight);
        DrawRect(pDevice, static_cast<int32_t>(Screen.fWidth - x), 0, static_cast<int32_t>(Screen.fHudOffsetReal + x), Screen.nHeight);
        
        DrawRect(pDevice, 0, 0, Screen.nWidth, static_cast<int32_t>(y));
        DrawRect(pDevice, 0, static_cast<int32_t>(Screen.fHeight - y), Screen.nWidth, static_cast<int32_t>(y));
        Screen.bDrawBorders = false;
    }

    return RealEndScene(pDevice);
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
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = (1.0f / (480.0f * Screen.fAspectRatio)) * 2.0f;
    Screen.fMirrorFactor = 1.0f * ((4.0f / 3.0f) / (Screen.fAspectRatio));
    Screen.fViewPortSizeX = 640.0f;
    Screen.fViewPortSizeY = 480.0f;
    Screen.fNovelsScale = Screen.fHudScale;
    Screen.fNovelsOffset = Screen.fHudOffset;

    CIniReader iniReader("");
    bool bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
    static bool bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
    Screen.fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 100.0f);
    Screen.bGraphicNovelMode = iniReader.ReadInteger("MAIN", "GraphicNovelMode", 1) != 0;
    if (!Screen.fWidescreenHudOffset) { Screen.fWidescreenHudOffset = 100.0f; }

    //fix aspect ratio
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
            *(float *)(PCamera + 0x20C) = v2;

            if (!Screen.bIs2D && !Screen.bIsInGraphicNovel)
            {
                if (fParam5 == 0.05f || Screen.bIsSkybox) /*|| stack[3] == (void*)0x004122bf*/
                    *(float *)(PCamera + 0x20C) = v2 * Screen.fDiffFactor;
            }

            *(float *)(PCamera + 0x210) = (fParam1 - fParam2) * v2 / (fParam3 - fParam4);
        }
    };

    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "E8 ? ? ? ? A0 ? ? ? ? 80 A6 EC 00 00 00 BF A8 01");
    injector::MakeCALL(pattern.get_first(), static_cast<void(__fastcall *)(uintptr_t, uintptr_t)>(PCameraValidate), true); //0x10017077

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

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 25 ? ? ? ? 89 44 24 44 83 EC 08");
        static auto pHudElementPosX = *pattern.get_first<float*>(2); //0x10061134
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 25 ? ? ? ? D9 5C 24 20");
        static auto pHudElementPosY = *pattern.get_first<float*>(2); //0x10061138
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? 8D 81 54");
        static auto pf10042294 = *pattern.get_first<float*>(2); //0x10042294

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? 8B 86 64 01 00 00");
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
                
                if (bWidescreenHud && !Screen.bIsSniperZoomOn)
                {
                    if (ElementPosX == 7.0f) // bullet time meter
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                    else
                    if (ElementPosX == 8.0f && ElementPosY != 8.0f) // bullet time overlay()
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                    else
                    if (ElementPosX == 12.0f) // painkillers
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                    else
                    if (ElementPosX == 22.5f) //health bar and overlay
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }
                    else
                    if (ElementPosX == 96.0f) // other weapons name
                    {
                        ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                    }
                    else
                    if (ElementPosX == 192.0f) //molotovs/grenades name pos
                    {
                        ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                    }
                }
                
                ElementNewPosX2 = ElementNewPosX1;

                if (Screen.bIsSniperZoomOn)
                {
                    Screen.bDrawBorders = true;
                }
                
                if (Screen.bIsX_QuadRenderer)
                {
                    ElementNewPosX1 = ElementPosX + Screen.fHudOffset2;
                    ElementNewPosX2 = ElementPosX - Screen.fHudOffset2;
                    Screen.bIsX_QuadRenderer = false;
                }

                if (Screen.bIsFading && !Screen.bIsSniperZoomOn && ElementPosX == 0.0f && ElementPosY == 0.0f)
                {
                    ElementNewPosX1 = ElementPosX + Screen.fHudOffset2;
                    ElementNewPosX2 = ElementPosX - Screen.fHudOffset2;
                    Screen.bIsFading = false;
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
                    if (!Screen.bIsSniperZoomOn)
                    {
                        if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f || pTextElementPosX->c == 57.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
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
                    }
                    //*(float*)(regs.esp + 0x2C) = TextPosX * *(float*)(regs.ebx + 0x198);
                    __asm fld dword ptr[TextNewPosX]
                }
            }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6)); //0x1000AACC
        }
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
        while (pattern.clear(GetModuleHandle("e2mfc")).count_hint(4).empty()) { Sleep(0); };

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

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "C6 45 FC 02 89 4B 44");
    struct P_SceneRenderHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ebp - 0x04) = 2;
            *(uint32_t*)(regs.ebx + 0x44) = regs.ecx;
            Screen.bIs2D = true;
        }
    }; injector::MakeInline<P_SceneRenderHook1>(pattern.get_first(0), pattern.get_first(7));

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "89 73 34 89 73 38");
    struct P_SceneRenderHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebx + 0x34) = 0;
            *(uint32_t*)(regs.ebx + 0x38) = 0;
            Screen.bIs2D = false;
        }
    }; injector::MakeInline<P_SceneRenderHook2>(pattern.get_first(0), pattern.get_first(6));

    //for graphic novels scaling
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 35 ? ? ? ? D8 4C 24 38 D9 5C 24 34");
    injector::WriteMemory(pattern.get_first(2), &Screen.fViewPortSizeX, true); //0x100176D4
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 35 ? ? ? ? D8 4C 24 38 D9 5C 24 30");
    injector::WriteMemory(pattern.get_first(2), &Screen.fViewPortSizeY, true); //0x10017719
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 35 ? ? ? ? 89 1D ? ? ? ? 89 1D");
    injector::WriteMemory(pattern.get_first(2), &Screen.fViewPortSizeY, true); //0x1001779A
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? 8B 44 24 1C 52 50 51"); //0x1000F05E
    injector::WriteMemory(pattern.get_first(2), &Screen.fNovelsOffset, true);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? D9 44 24 20 53"); //0x1000F011
    injector::WriteMemory(pattern.get_first(2), &Screen.fNovelsScale, true);

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

    pattern = hook::pattern("8B 4C 24 18 51 8B 4F 04 FF 15");
    struct X_CharacterSetSniperZoomOnHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.esp + 0x18);
            regs.ecx = *(uint32_t*)(regs.edi + 0x04);
            if (regs.edx == 1)
                Screen.bIsSniperZoomOn = true;
            else
                Screen.bIsSniperZoomOn = false;
        }
    }; injector::MakeInline<X_CharacterSetSniperZoomOnHook>(pattern.get_first(0), pattern.get_first(8)); //428FB2
    injector::WriteMemory<uint8_t>(pattern.get_first(6), 0x52i8, true); //push edx

    pattern = hook::pattern("8B 8B AD 00 00 00 DF E0 F6 C4 41");
    struct FadingHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.ebx + 0xAD);
            if (*(float*)(regs.esp + 0x34) == 0.0f)
                Screen.bIsFading = false;
            else
                Screen.bIsFading = true;
        }
    }; injector::MakeInline<FadingHook>(pattern.get_first(0), pattern.get_first(8)); //4872C1 

    pattern = hook::pattern("C7 44 24 ? ? ? ? ? 75 07 8A 46 41 84 C0");
    struct SkyboxHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x18) = 0;
            Screen.bIsSkybox = true;
        }
    }; injector::MakeInline<SkyboxHook1>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::pattern("C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8B 4C 24 10 5E 64 89 0D ? ? ? ? 83 C4 18 C2 08 00");
    struct SkyboxHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x18) = -1;
            Screen.bIsSkybox = false;
        }
    }; injector::MakeInline<SkyboxHook2>(pattern.get_first(0), pattern.get_first(8));

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
        while (pattern.clear(GetModuleHandle("X_GameObjectsMFC")).count_hint(4).empty()) { Sleep(0); };

    //mirrors fix
    injector::WriteMemory(pattern.get_first(2), &Screen.fMirrorFactor, true); //0x10101F39

    //doors graphics fix
    static float fVisibilityFactor1 = 0.5f;
    static float fVisibilityFactor2 = 1.5f;
    pattern = hook::module_pattern(GetModuleHandle("X_GameObjectsMFC"), "D9 05 ? ? ? ? 89 44 24 1C"); //1000AD9E
    injector::WriteMemory(pattern.get_first(2), &fVisibilityFactor1, true);
    pattern = hook::module_pattern(GetModuleHandle("X_GameObjectsMFC"), "D9 05 ? ? ? ? 89 4C 24 18"); //1000AD55
    injector::WriteMemory(pattern.get_first(2), &fVisibilityFactor2, true);

    //FOV
    CIniReader iniReader("");
    float fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (!fFOVFactor) { fFOVFactor = 1.0f; }
    Screen.fDynamicScreenFieldOfViewScale = fFOVFactor;
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

    //actually not a cutscene check, but X_Crosshair::sm_bCameraPathRunning
    pattern = hook::module_pattern(GetModuleHandle("X_GameObjectsMFC"), "A0 ? ? ? ? 84 C0 0F 85"); //byte_101A7AA0
    Screen.bIsInCutscene = *pattern.get_first<bool*>(1);
    
    return 0;
}

DWORD WINAPI InitX_ModesMFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("X_ModesMFC"), "8B 5C 24 18 8B 01 53 FF 50 38");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitX_ModesMFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("X_ModesMFC")).count_hint(1).empty()) { Sleep(0); };

    static CIniReader iniReader("");
    static int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);

    //Graphic Novels Handler
    static bool bPatched;
    static uint16_t oldState = 0;
    static uint16_t curState = 0;
    static uint32_t callAddr;

    struct GraphicNovelXRefHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uint32_t*)(regs.esp + 0x18);
            regs.eax = *(uint32_t*)(regs.ecx);
            callAddr = *(uint32_t*)(regs.eax + 0x38);
        }
    }; injector::MakeInline<GraphicNovelXRefHook>(pattern.get_first(0), pattern.get_first(6)); //10001A6A

    static auto sub_484AE0 = (uint32_t)hook::get_pattern("8B 44 24 04 83 EC 34 53 55 56 57 50 8B F1"); //MaxPayne_GraphicNovelMode::update
    auto GraphicNovelPageUpdate = hook::module_pattern(GetModuleHandle("X_ModesMFC"), "8B 16 8B CE 33 FF FF 52 10"); //10001A7A 
    struct GraphicNovelPageUpdateHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.esi);
            regs.ecx = regs.esi;
            regs.edi = 0;

            if (!*Screen.bIsInCutscene)
                Screen.bDrawBorders = false;

            Screen.bIsInGraphicNovel = (callAddr == sub_484AE0);
            callAddr = 0;

            if (Screen.bIsInGraphicNovel)
            {
                curState = GetAsyncKeyState(nGraphicNovelModeKey);

                if (!curState && oldState)
                {
                    Screen.bGraphicNovelMode = !Screen.bGraphicNovelMode;
                    bPatched = !bPatched;
                    iniReader.WriteInteger("MAIN", "GraphicNovelMode", Screen.bGraphicNovelMode);
                }

                oldState = curState;

                if (Screen.bGraphicNovelMode)
                {
                    if (!bPatched)
                    {
                        Screen.fNovelsScale = 0.003125f;
                        Screen.fNovelsOffset = -1.0f;
                        Screen.fViewPortSizeX = 480.0f * Screen.fAspectRatio;
                        Screen.fViewPortSizeY = 480.0f;
                        bPatched = true;
                    }
                }
                else
                {
                    if (!bPatched)
                    {
                        Screen.fNovelsScale = 0.003125f;
                        Screen.fNovelsOffset = -1.0f;
                        Screen.fViewPortSizeX = (480.0f * Screen.fAspectRatio) / 1.17936117936f;
                        Screen.fViewPortSizeY = 480.0f / 1.17936117936f;
                        bPatched = true;
                    }
                }
            }
            else
            {
                if (bPatched)
                {
                    Screen.fViewPortSizeX = 640.0f;
                    Screen.fViewPortSizeY = 480.0f;
                    Screen.fNovelsScale = Screen.fHudScale;
                    Screen.fNovelsOffset = Screen.fHudOffset;
                    bPatched = false;
                }
            }
        }
    }; injector::MakeInline<GraphicNovelPageUpdateHook>(GraphicNovelPageUpdate.get_first(0), GraphicNovelPageUpdate.get_first(6));

    return 0;
}

DWORD WINAPI InitX_HelpersMFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("X_HelpersMFC"), "8B 41 08 8B 49 0C 50");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitX_HelpersMFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("X_HelpersMFC")).count_hint(1).empty()) { Sleep(0); };

    struct X_QuadRendererRenderHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.ecx + 0x08);
            regs.ecx = *(uint32_t*)(regs.ecx + 0x0C);
            Screen.bIsX_QuadRenderer = true;
        }
    }; injector::MakeInline<X_QuadRendererRenderHook>(pattern.get_first(0), pattern.get_first(6)); //10004820

    pattern = hook::module_pattern(GetModuleHandle("X_HelpersMFC"), "05 58 01 00 00 33 D2");
    struct X_ProgressBarUpdateProgressBarHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax += 0x158;
            Screen.bDrawBorders = true;
        }
    }; injector::MakeInline<X_ProgressBarUpdateProgressBarHook>(pattern.get_first(0)); //10002FF0

    return 0;
}

DWORD WINAPI InitE2_D3D8_DRIVER_MFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("e2_d3d8_driver_mfc"), "8B 45 0C 53 56 33 F6 2B C6 57");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitE2_D3D8_DRIVER_MFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("e2_d3d8_driver_mfc")).count_hint(1).empty()) { Sleep(0); };

    //D3D Hook for borders
    CIniReader iniReader("");
    bool bD3DHookBorders = iniReader.ReadInteger("MAIN", "D3DHookBorders", 1) != 0;
    if (bD3DHookBorders)
    {
        pattern = hook::module_pattern(GetModuleHandle("e2_d3d8_driver_mfc"), "8B 86 ? ? ? ? 8B 08 50 FF 91");
        struct EndSceneHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.esi + 0x100);

                if (!RealEndScene)
                {
                    auto addr = *(uint32_t*)(regs.eax) + (IDirect3DDevice8VTBL::EndScene * 4);
                    RealEndScene = *(EndScene_t*)addr;
                    injector::WriteMemory(addr, &EndScene, true);
                }
            }
        }; injector::MakeInline<EndSceneHook>(pattern.get_first(0), pattern.get_first(6)); //0x1002856D 
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
        InitE2MFC(NULL);
        InitX_GameObjectsMFC(NULL);
        InitX_ModesMFC(NULL);
        InitX_HelpersMFC(NULL);
        InitE2_D3D8_DRIVER_MFC(NULL);
    }
    return TRUE;
}