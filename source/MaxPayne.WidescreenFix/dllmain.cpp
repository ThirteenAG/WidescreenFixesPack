#include "stdafx.h"
#include "dxsdk\d3d8.h"
#include "dxsdk\d3dvtbl.h"
#include <shlobj.h>

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFOVFactor;
    float fFieldOfView;
    float fAspectRatio;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fHudOffsetWide;
    float fWidescreenHudOffset;
    float fBorderOffset;
    float fWidthScale;
    float fHalfWidthScale;
    float f1_fWidthScale;
    float fDouble1_fWidthScale;
    float fHalf1_fWidthScale;
    bool bDrawBorders;
    bool bDrawBordersToFillGap;
    bool bDrawBordersForCameraOverlay;
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
    if (Screen.bDrawBordersToFillGap)
    {
        //hiding top/left 1px gap
        DrawRect(pDevice, 0, 0, Screen.nWidth, 1);
        DrawRect(pDevice, 0, 0, 1, Screen.nHeight);
        Screen.bDrawBordersToFillGap = false;
    }

    if ((Screen.bDrawBorders || Screen.bDrawBordersForCameraOverlay) && !Screen.bIsInGraphicNovel)
    {
        float fBadCamPosOffset = Screen.bDrawBordersForCameraOverlay ? 10.0f : 0.0f; // for motel camera gap https://i.imgur.com/JGNdm6y.jpg

        DrawRect(pDevice, 0, 0, static_cast<int32_t>(Screen.fHudOffsetReal), Screen.nHeight);
        DrawRect(pDevice, static_cast<int32_t>(Screen.fWidth - Screen.fHudOffsetReal - fBadCamPosOffset), 0, static_cast<int32_t>(Screen.fHudOffsetReal + Screen.fHudOffsetReal), Screen.nHeight);
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
    Screen.fWidthScale = 640.0f / Screen.fAspectRatio;
    Screen.fHalfWidthScale = Screen.fWidthScale / 2.0f;
    Screen.f1_fWidthScale = 1.0f / Screen.fWidthScale;
    Screen.fDouble1_fWidthScale = Screen.f1_fWidthScale * 2.0f;
    Screen.fHalf1_fWidthScale = Screen.f1_fWidthScale / 2.0f;
    Screen.fHudOffset = ((-1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
    Screen.fHudScale = (1.0f / (480.0f * Screen.fAspectRatio)) * 2.0f;
    Screen.fBorderOffset = (1.0f / Screen.fHudScale) - (640.0f / 2.0f);
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    static CIniReader iniReader("");
    static bool bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
    Screen.fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 100.0f);
    Screen.bGraphicNovelMode = iniReader.ReadInteger("MAIN", "GraphicNovelMode", 1) != 0;
    static int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);
    if (!Screen.fWidescreenHudOffset) { Screen.fWidescreenHudOffset = 100.0f; }
    Screen.fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (!Screen.fFOVFactor) { Screen.fFOVFactor = 1.0f; }

    //fix aspect ratio
    //injector::WriteMemory((DWORD)e2mfc + 0x148ED + 0x2, &f1_480, true); //doors fix ???
    //CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DE4, height_multipl); //D3DERR_INVALIDCALL
    static uintptr_t e2mfc_14775, e2mfc_1566C, e2mfc_146FA;
    static uintptr_t e2mfc_49DEC, e2mfc_49DFC;
    static uintptr_t e2mfc_1565E, e2mfc_49E00;

    uintptr_t dword_40B3B2 = (uintptr_t)hook::get_pattern("C7 86 F5 00 00 00 00 00 00 00 5E");
    struct DelayedHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esi + 0xF5) = 0;

            injector::WriteMemory(e2mfc_146FA, &Screen.f1_fWidthScale, true); // D3DERR_INVALIDCALL 5
            injector::WriteMemory(e2mfc_14775, &Screen.f1_fWidthScale, true); // D3DERR_INVALIDCALL 6
            injector::WriteMemory(e2mfc_1566C, &Screen.f1_fWidthScale, true); // D3DERR_INVALIDCALL 9

            injector::WriteMemory<float>(e2mfc_49DEC, Screen.fDouble1_fWidthScale, true);
            injector::WriteMemory<float>(e2mfc_49DFC, Screen.fHalf1_fWidthScale, true);
        }
    }; injector::MakeInline<DelayedHook>(dword_40B3B2, dword_40B3B2 + 10);

    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "BB 00 0C 00 00 25 00 0C 00 00 8B 4D F8");
    auto flt_10049DE4 = *pattern.count(10).get(5).get<uintptr_t>(-15);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), pattern_str(0xD8, 0x0D, to_bytes(flt_10049DE4)));
    while (pattern.clear(GetModuleHandle("e2mfc")).count_hint(9).empty()) { Sleep(0); };
    for (size_t i = 0; i < pattern.size(); i++)
    {
        if (i != 4 && i != 5 && i != 8)
            injector::WriteMemory(pattern.get(i).get<uintptr_t>(2), &Screen.f1_fWidthScale, true);
    }
    e2mfc_146FA = (uintptr_t)pattern.get(4).get<uintptr_t>(2);
    e2mfc_14775 = (uintptr_t)pattern.get(5).get<uintptr_t>(2);
    e2mfc_1566C = (uintptr_t)pattern.get(8).get<uintptr_t>(2);

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 1D ? ? ? ? 8A 86 E0");
    e2mfc_49DEC = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 5D E0 E8");
    e2mfc_49DFC = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 5D E4 D9 45 EC");
    e2mfc_49E00 = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? D8 0D");
    e2mfc_1565E = (uintptr_t)pattern.get_first(6);

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), pattern_str(to_bytes(480.0f))); //0x5ECD00
    while (pattern.clear(GetModuleHandle("e2mfc")).count_hint(6).empty()) { Sleep(0); };
    for (size_t i = 0; i < pattern.size(); i++)
        injector::WriteMemory<float>(pattern.get(i).get<float*>(0), Screen.fWidthScale, true);

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 3D ? ? ? ? D8 6D 0C D9 5D 0C");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fWidthScale, true); //0x10049DDC 480.0f

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 25 ? ? ? ? D9 86 DC 01 00 00");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHalfWidthScale, true); //0x10049DF0 240.0

    //corrupted graphic in tunnel (DisableSubViewport)
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "55 8B EC 83 EC 24 56 8B F1 D9 86 DC 01");
    injector::MakeRET(pattern.get_first(), 0x10, true); //10013FB0 ret 10h

    // Hud
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? 8B 4D F4");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHudOffset, true); //100495C8
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 5D D8 B9 ? ? ? ? 8B 45 D8 8B 55 D4");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHudScale, true); //100495D0

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? D9 E0 D9 45 FC D8 25");
    static float* pHudElementPosX = *pattern.count(2).get(1).get<float*>(2); //0x10065190
    static float* pHudElementPosY = *pattern.count(2).get(1).get<float*>(22); //0x10065194
    struct P_HudPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float ElementPosX = *pHudElementPosX;
            float ElementPosY = *pHudElementPosY;
            float ElementNewPosX1 = ElementPosX;
            float ElementNewPosY1 = ElementPosY;
            float ElementNewPosX2 = ElementPosX;
            float ElementNewPosY2 = ElementPosY;

            if (bWidescreenHud)
            {
                if (ElementPosX == 7.0f) // bullet time meter
                {
                    ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                }

                if (ElementPosX == 8.0f && regs.eax != 8) // bullet time overlay()
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

                if (ElementPosX == 95.0f) // other weapons name
                {
                    ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                }

                if (ElementPosX == 190.0f) //molotovs/grenades name pos
                {
                    ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                }
            }

            ElementNewPosX2 = ElementNewPosX1;

            if (ElementPosX == 0.0f && ElementPosY == 0.0f && regs.eax == 2 && *(float*)&regs.edx == 640.0f) // fading
            {
                ElementNewPosX1 = ElementPosX + 640.0f;
                ElementNewPosX2 = ElementPosX - 640.0f;
                //ElementNewPosY1 = ElementPosY + 48.0f;
                //ElementNewPosY2 = ElementPosY - 48.0f;
            }
            else if (Screen.bIsInGraphicNovel)
            {
                if (ElementPosX == 0.0f && ElementPosY == 100.0f /*&& regs.eax == 2*/ /*&& *(float*)&regs.edx == 80.0f*/) // graphic novels controls and background
                {
                    if (Screen.bGraphicNovelMode)
                    {
                        ElementNewPosY1 = ElementPosY - 90.0f;
                        ElementNewPosY2 = ElementPosY - 90.0f;
                    }
                    else
                    {
                        ElementNewPosY1 = ElementPosY - 160.0f;
                        ElementNewPosY2 = ElementPosY - 160.0f;
                    }
                }

                if (ElementPosX == 0.0f && ElementPosY == 0.0f && regs.eax == 2 && (*(float*)&regs.edx == 80.0f || *(float*)&regs.edx == 60.0f)) // graphic novels controls and background
                {
                    if (Screen.bGraphicNovelMode)
                    {
                        ElementNewPosY1 = ElementPosY - 60.0f;
                        ElementNewPosY2 = ElementPosY - 60.0f;
                    }
                    else
                    {
                        ElementNewPosY1 = ElementPosY - 160.0f;
                        ElementNewPosY2 = ElementPosY - 160.0f;
                    }
                }
            }
            else if (ElementPosX == 100.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f)) //sniper scope borders left side
            {
                Screen.bDrawBordersToFillGap = true;
                ElementNewPosX1 += Screen.fBorderOffset;
            }
            else if (ElementPosX == 0.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f) && *(float*)&regs.edx == 100.0f) //sniper scope borders right side
            {
                Screen.bDrawBordersToFillGap = true;
                ElementNewPosX2 -= Screen.fBorderOffset;
            }

            *(float *)(regs.ebp - 4) -= ElementNewPosX2;
            *(float *)(regs.ebp - 8) -= ElementNewPosY2;

            _asm
            {
                fld     dword ptr[ElementNewPosX1]
                fchs
                fld     dword ptr[ElementNewPosY1]
                fchs
            }
        }
    }; injector::MakeInline<P_HudPosHook>(pattern.count(2).get(1).get<uintptr_t>(0), pattern.count(2).get(1).get<uintptr_t>(40)); //1000856C


    if (bWidescreenHud)
    {
        Screen.fHudOffsetWide = Screen.fWidescreenHudOffset;

        if (Screen.fAspectRatio < (16.0f / 9.0f))
        {
            Screen.fHudOffsetWide = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
        }

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? D8 8E 74 01 00 00");
        static auto pTextElementPosX = *pattern.get_first<TextCoords*>(2); //0x100647D0
        struct P_TextPosHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto TextPosX = pTextElementPosX->a;
                auto TextNewPosX = TextPosX;

                if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
                    TextNewPosX = TextPosX + Screen.fHudOffsetWide;

                _asm fld    dword ptr[TextNewPosX]
            }
        }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6));

        static float TextPosX1, TextPosX2, TextPosY1;
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "C7 45 D0 00 00 00 00 D9 5D"); //100045FC
        struct P_TextPosHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.ebp - 0x30) = 0.0f;
                TextPosX1 = *(float*)(regs.ebp - 0x28);
                TextPosY1 = *(float*)(regs.ebp - 0x2C);
            }
        }; injector::MakeInline<P_TextPosHook2>(pattern.get_first(0), pattern.get_first(7));

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "89 41 08 D9 45 E4 D8 0D"); //0x10004693
        struct P_TextPosHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                TextPosX2 = *(float*)(regs.ebp - 0x1C);

                if (TextPosX1 == (69.0f + Screen.fHudOffsetWide) && TextPosY1 == 457.0f) // painkillers amount number
                    *(float*)(regs.ebp - 0x1C) += (24.0f * Screen.fHudOffsetWide);

                *(uint32_t*)(regs.ecx + 8) = regs.eax;
                auto ebp1C = *(float*)(regs.ebp - 0x1C);
                _asm fld  dword ptr[ebp1C]
            }
        }; injector::MakeInline<P_TextPosHook3>(pattern.get_first(0), pattern.get_first(6));
    }

    //actually not a cutscene check, but X_Crosshair::sm_bCameraPathRunning
    Screen.bIsInCutscene = *hook::get_pattern<bool*>("A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 86 ? ? ? ? 85 C0 0F 84", 1);

    //Graphic Novels Handler
    static bool bPatched;
    static uint16_t oldState = 0;
    static uint16_t curState = 0;
    static uint32_t callAddr;

    auto p = hook::pattern("8B 5C 24 3C 8B 10 53 8B C8"); //60145C
    struct GraphicNovelXRefHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uint32_t*)(regs.esp + 0x3C);
            regs.edx = *(uint32_t*)(regs.eax);
            callAddr = *(uint32_t*)(regs.edx + 0x38);
        }
    }; injector::MakeInline<GraphicNovelXRefHook>(p.get_first(0), p.get_first(6));

    static auto sub_49B6D0 = (uint32_t)hook::get_pattern("55 8B EC 83 EC 48 8B 45 08 53 56 57 8B F1 50"); //MaxPayne_GraphicNovelMode::update
    auto GraphicNovelPageUpdate = hook::pattern("8B 06 8B CE 33 FF FF 50 10"); //60146E
    struct GraphicNovelPageUpdateHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi);
            regs.ecx = regs.esi;
            regs.edi = 0;

            if (!*Screen.bIsInCutscene)
                Screen.bDrawBordersForCameraOverlay = false;

            Screen.bIsInGraphicNovel = (callAddr == sub_49B6D0);
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
                        injector::WriteMemory<float>(e2mfc_49E00, ((1.0f / (480.0f * Screen.fAspectRatio)) / 2.0f), true);
                        injector::WriteMemory<float>(e2mfc_49DFC, (1.0f / (640.0f / (4.0f / 3.0f)) / 2.0f), true);
                        injector::WriteMemory<float>(e2mfc_1565E, 0.0f, true);
                        bPatched = true;
                    }
                }
                else
                {
                    if (!bPatched)
                    {
                        injector::WriteMemory<float>(e2mfc_49E00, ((1.0f / (480.0f * Screen.fAspectRatio)) / 2.0f) * 1.27f, true);
                        injector::WriteMemory<float>(e2mfc_49DFC, (1.0f / (640.0f / (4.0f / 3.0f)) / 2.0f) * 1.27f, true);
                        injector::WriteMemory<float>(e2mfc_1565E, -0.39f, true);
                        bPatched = true;
                    }
                }
            }
            else
            {
                if (bPatched)
                {
                    injector::WriteMemory<float>(e2mfc_49E00, ((1.0f / 640.0f) / 2.0f), true);
                    injector::WriteMemory<float>(e2mfc_49DFC, Screen.fHalf1_fWidthScale, true);
                    injector::WriteMemory<float>(e2mfc_1565E, 0.0f, true);
                    bPatched = false;
                }
            }
        }
    }; injector::MakeInline<GraphicNovelPageUpdateHook>(GraphicNovelPageUpdate.get_first(0), GraphicNovelPageUpdate.get_first(6));

    return 0;
}

DWORD WINAPI InitE2MFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "E8 ? ? ? ? 8B 0D ? ? ? ? 89 45 DC 89 5D E0");

    if (pattern.count_hint(2).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitE2MFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("e2mfc")).count_hint(2).empty()) { Sleep(0); };

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
    injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), static_cast<int32_t(__fastcall *)(uintptr_t, uintptr_t)>(PDriverGetWidth), true);   //e2mfc + 0x15582
    injector::MakeCALL(pattern.get(0).get<uintptr_t>(41), static_cast<int32_t(__fastcall *)(uintptr_t, uintptr_t)>(PDriverGetHeight), true); //e2mfc + 0x155AB

    CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitWF, NULL, 0, NULL);
    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    static bool bUseGameFolderForSavegames = iniReader.ReadInteger("MISC", "UseGameFolderForSavegames", 0) != 0;
    if (bUseGameFolderForSavegames)
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x40FCAB

    bool bAltTab = iniReader.ReadInteger("MISC", "AllowAltTabbingWithoutPausing", 0) != 0;
    if (bAltTab)
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 5E C2 08 00");
        injector::MakeNOP(pattern.count(2).get(1).get<uintptr_t>(0), 5, true); //0x40D29B
    }

    static int32_t nCutsceneBorders = iniReader.ReadInteger("MAIN", "CutsceneBorders", 1);
    if (nCutsceneBorders)
    {
        auto f = [](uintptr_t _this, uintptr_t edx) -> float
        {
            if (nCutsceneBorders > 1)
                return *(float *)(_this + 12) * (1.0f / ((4.0f / 3.0f) / Screen.fAspectRatio));
            else
                return 1.0f;
        };
        pattern = hook::pattern("E8 ? ? ? ? EB ? D9 05 ? ? ? ? 8B CF");
        injector::MakeCALL(pattern.get_first(), static_cast<float(__fastcall *)(uintptr_t, uintptr_t)>(f), true); //0x4565B8
    }

    static int32_t nLoadSaveSlot = iniReader.ReadInteger("MISC", "LoadSaveSlot", -1);
    if (nLoadSaveSlot == -2 || nLoadSaveSlot == -3 || (nLoadSaveSlot >= 0 && nLoadSaveSlot <= 999))
    {
        static auto unk_8A6490 = *hook::get_pattern<void*>("BF ? ? ? ? F3 A5 8B C8 83 E1 03 F3 A4 5F 5E C3", 1);
        if (*(uint8_t*)unk_8A6490 == 0)
        {
            pattern = hook::pattern("E8 ? ? ? ? 8B 40 04 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8D 55 10 52");
            static auto AfxGetModuleState = injector::GetBranchDestination(pattern.get_first(0), true); //0x76F77E
            static auto GetProfileStringA = injector::GetBranchDestination(pattern.get_first(29), true); //0x76F7B4
            static auto aLastSavedGameF = *pattern.get_first<char*>(14); // 8680EC
            static auto aSaveGame = *pattern.get_first<char*>(19); // 866FDC
            static auto aSavegames = *hook::get_pattern<char*>("BF ? ? ? ? F2 AE F7 D1 2B F9 8B D1 83 C9 FF 8B F7 8B FB F2 AE 8B 45 08 8B CA C1 E9 02 4F", 1);
            static auto MaxPayne2Saveg = *hook::get_pattern<char*>("BF ? ? ? ? 75 05 BF ? ? ? ? 83 C9 FF 33 C0 F2 AE F7 D1 2B F9 8B D1", 1);
            pattern = hook::pattern("89 87 ? ? ? ? E8 ? ? ? ? A2 ? ? ? ? E8");
            struct SaveGameHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.edi + 0xCC) = regs.eax;

                    if (nLoadSaveSlot == -2)
                    {
                        void* pStr = nullptr;
                        auto _this = injector::stdcall<void*()>::call(AfxGetModuleState);
                        injector::thiscall<void(void* _this, void* out, char const* a1, char const* a2, char const* a3)>::call(GetProfileStringA, *(void**)((uint32_t)_this + 4), &pStr, (char const*)aSaveGame, (char const*)aLastSavedGameF, "");
                        std::string_view LastSavedGameFilename{ (char*)pStr };
                        if (!LastSavedGameFilename.empty())
                            injector::WriteMemoryRaw(unk_8A6490, (void*)LastSavedGameFilename.data(), LastSavedGameFilename.size(), true);
                    }
                    else
                    {
                        char buffer[MAX_PATH];
                        if (bUseGameFolderForSavegames)
                        {
                            GetModuleFileName(NULL, buffer, MAX_PATH);
                            *strrchr(buffer, '\\') = '\0';
                            strcat_s(buffer, "\\");
                            strcat_s(buffer, aSavegames);
                            strcat_s(buffer, "\\");
                        }
                        else
                        {
                            SHGetSpecialFolderPathA(0, buffer, 5, false);
                            strcat_s(buffer, "\\");
                            strcat_s(buffer, MaxPayne2Saveg);
                            strcat_s(buffer, "\\");
                        }

                        auto nSaveNum = -1;
                        std::string SFPath(buffer);

                        WIN32_FIND_DATA fd;
                        HANDLE File = FindFirstFile(std::string(SFPath + "*.mps").c_str(), &fd);
                        FILETIME LastWriteTime = fd.ftLastWriteTime;

                        if (File != INVALID_HANDLE_VALUE)
                        {
                            do
                            {
                                std::string str(fd.cFileName);
                                auto n = str.find_first_of("0123456789");

                                if (nLoadSaveSlot >= 0)
                                {
                                    if (n != std::string::npos)
                                    {
                                        nSaveNum = std::atoi(&str[n]);
                                        if (nSaveNum == nLoadSaveSlot)
                                        {
                                            SFPath += str;
                                            injector::WriteMemoryRaw(unk_8A6490, SFPath.data(), SFPath.size(), true);
                                            return;
                                        }
                                    }
                                }
                                else
                                {
                                    if (CompareFileTime(&fd.ftLastWriteTime, &LastWriteTime) >= 0)
                                    {
                                        LastWriteTime = fd.ftLastWriteTime;
                                        std::string str(fd.cFileName);
                                        if (n != std::string::npos)
                                        {
                                            nSaveNum = std::atoi(&str[n]);
                                        }
                                    }
                                }
                            } while (FindNextFile(File, &fd));
                            FindClose(File);
                        }

                        if (nSaveNum >= 0 && nLoadSaveSlot < 0)
                        {
                            char buffer[5]; sprintf(buffer, "%03d", nSaveNum);
                            SFPath += fd.cFileName;
                            auto offset = SFPath.end() - std::strlen("000.mps");
                            SFPath.replace(offset, offset + 3, buffer);
                            injector::WriteMemoryRaw(unk_8A6490, SFPath.data(), SFPath.size(), true);
                        }
                    }
                }
            }; injector::MakeInline<SaveGameHook>(pattern.get_first(0), pattern.get_first(6)); //0x415EBD
        }
    }

    //FOV
    static auto FOVHook = [](uintptr_t _this, uintptr_t edx) -> float
    {
        float f = AdjustFOV(*(float*)(_this + 0x58) * 57.295776f, Screen.fAspectRatio) * Screen.fFOVFactor;
        Screen.fFieldOfView = (((f > 179.0f) ? 179.0f : f) / 57.295776f);
        return Screen.fFieldOfView;
    };

    pattern = hook::pattern("E8 ? ? ? ? D8 8B 3C 12"); //0x50B9E0
    auto sub_50B9E0 = injector::GetBranchDestination(pattern.get_first(), true);
    pattern = hook::pattern("E8 ? ? ? ?");
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        auto addr = pattern.get(i).get<uint32_t>(0);
        auto dest = injector::GetBranchDestination(addr, true);
        if (dest == sub_50B9E0)
            injector::MakeCALL(addr, static_cast<float(__fastcall *)(uintptr_t, uintptr_t)>(FOVHook), true);
    }
    pattern = hook::pattern("E8 ? ? ? ? D9 5C 24 14 8B CF E8"); // 0x45650D
    injector::MakeCALL(pattern.get_first(0), sub_50B9E0, true); // restoring cutscene FOV

    bool bD3DHookBorders = iniReader.ReadInteger("MAIN", "D3DHookBorders", 1) != 0;
    if (bD3DHookBorders)
    {
        auto CutsceneFOVHook = [](uintptr_t _this, uintptr_t edx) -> float
        {
            return *(float*)(_this + 88) + ((((Screen.fHudOffsetReal / Screen.fWidth)) / *(float*)(_this + 88)) * 2.0f);
        };
        injector::MakeCALL(pattern.get_first(0), static_cast<float(__fastcall *)(uintptr_t, uintptr_t)>(CutsceneFOVHook), true);

        pattern = hook::pattern("C6 87 ? ? ? ? ? E8 ? ? ? ? 8B 4D F4");
        struct CameraOverlayHook
        {
            void operator()(injector::reg_pack& regs)
            {
                Screen.bDrawBordersForCameraOverlay = false;
                *(uint8_t*)(regs.edi + 0x14E) = 1;

                auto a1 = *(uint32_t*)(regs.esp + 0x10);
                auto a2 = *(uint32_t*)(regs.esp + 0x14);
                auto a3 = *(uint32_t*)(regs.esp + 0x18);
                auto a4 = *(uint32_t*)(regs.esp + 0x1C);

                //what happens here is check for some camera coordinates or angles
                if ((a1 == 0x3FE842CF && a4 == 0x3FE842CF) ||											//1.81 https://i.imgur.com/A7wRrgk.gifv
                    (a1 == 0x3FC00000 && a2 == 0x4096BEF4 && a3 == 0xC003936E && a4 == 0x3FC00000) ||   //1.5 https://i.imgur.com/ouRpysL.jpg
                    (a1 == 0xBFAAE30E && a2 == 0xBFC2B1AA && a3 == 0x3EC2E382 && a4 == 0xBFAAE30E) ||   //-1.33505 https://i.imgur.com/JGNdm6y.jpg
                    (a1 == 0x403F7470 && a2 == 0xC067ED50 && a3 == 0x40424DE0 && a4 == 0x403F7470)      // 2.99148  https://i.imgur.com/hj5FsXp.png
                    )
                {
                    Screen.bDrawBordersForCameraOverlay = true;
                }
            }
        }; injector::MakeInline<CameraOverlayHook>(pattern.get_first(0), pattern.get_first(7)); // 0x672EB1
    }

    pattern = hook::pattern("05 40 01 00 00 84 C9 89 50 24");
    struct X_ProgressBarUpdateProgressBarHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax += 0x140;
            Screen.bDrawBorders = true;
        }
    }; injector::MakeInline<X_ProgressBarUpdateProgressBarHook>(pattern.get_first(0)); //5829D1

    //screenshots aspect ratio
    static float fScreenShotHeight = 0.0f;
    pattern = hook::pattern("89 93 ? ? ? ? D9 05 ? ? ? ? D9 05 ? ? ? ? D9 9B");
    static auto off_6301B6 = pattern.get_first(32);
    struct SaveScrHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebx + 0x10E) = regs.edx;
            if (!fScreenShotHeight)
            {
                fScreenShotHeight = **(float**)off_6301B6 * ((4.0f / 3.0f) / Screen.fAspectRatio);
                injector::WriteMemory(off_6301B6, &fScreenShotHeight, true);
            }
        }
    }; injector::MakeInline<SaveScrHook>(pattern.get_first(0), pattern.get_first(6)); //630196

    //savegame date format
    static auto fmt = iniReader.ReadString("MISC", "SaveStringFormat", "%a, %b %d %Y, %H:%M");
    pattern = hook::pattern("68 ? ? ? ? 8D 54 24 24 68 ? ? ? ? 52"); //411091
    injector::WriteMemory(pattern.get_first(1), &fmt, true);

    return 0;
}

DWORD WINAPI InitE2_D3D8_DRIVER_MFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("e2_d3d8_driver_mfc"), "8B 45 0C 53 56 33 F6 2B C6 57");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitE2_D3D8_DRIVER_MFC, (LPVOID)true, 0, NULL);
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
                regs.eax = *(uint32_t*)(regs.esi + 0xD0);

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
        InitE2_D3D8_DRIVER_MFC(NULL);
    }
    return TRUE;
}