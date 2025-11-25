module;

#include <stdafx.h>

export module e2mfc;

import ComVars;

void InitWF()
{
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
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    Screen.bGraphicNovelMode = iniReader.ReadInteger("MAIN", "GraphicNovelMode", 1) != 0;
    static int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);
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

    auto pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "BB 00 0C 00 00 25 00 0C 00 00 8B 4D F8");
    auto flt_10049DE4 = *pattern.count(10).get(5).get<uintptr_t>(-15);
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), pattern_str(0xD8, 0x0D, to_bytes(flt_10049DE4)));
    while (pattern.clear(GetModuleHandle(L"e2mfc")).count_hint(9).empty()) { Sleep(0); };
    for (size_t i = 0; i < pattern.size(); i++)
    {
        if (i != 4 && i != 5 && i != 8)
            injector::WriteMemory(pattern.get(i).get<uintptr_t>(2), &Screen.f1_fWidthScale, true);
    }
    e2mfc_146FA = (uintptr_t)pattern.get(4).get<uintptr_t>(2);
    e2mfc_14775 = (uintptr_t)pattern.get(5).get<uintptr_t>(2);
    e2mfc_1566C = (uintptr_t)pattern.get(8).get<uintptr_t>(2);

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? D9 1D ? ? ? ? 8A 86 E0");
    e2mfc_49DEC = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? D9 5D E0 E8");
    e2mfc_49DFC = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? D9 5D E4 D9 45 EC");
    e2mfc_49E00 = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? D8 0D");
    e2mfc_1565E = (uintptr_t)pattern.get_first(6);

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), pattern_str(to_bytes(480.0f))); //0x5ECD00
    while (pattern.clear(GetModuleHandle(L"e2mfc")).count_hint(6).empty()) { Sleep(0); };
    for (size_t i = 0; i < pattern.size(); i++)
        injector::WriteMemory<float>(pattern.get(i).get<float*>(0), Screen.fWidthScale, true);

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 3D ? ? ? ? D8 6D 0C D9 5D 0C");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fWidthScale, true); //0x10049DDC 480.0f

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 25 ? ? ? ? D9 86 DC 01 00 00");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHalfWidthScale, true); //0x10049DF0 240.0

    //corrupted graphic in tunnel (DisableSubViewport)
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "55 8B EC 83 EC 24 56 8B F1 D9 86 DC 01");
    injector::MakeRET(pattern.get_first(), 0x10, true); //10013FB0 ret 10h

    // Hud
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? 8B 4D F4");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHudOffset, true); //100495C8
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? D9 5D D8 B9 ? ? ? ? 8B 45 D8 8B 55 D4");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHudScale, true); //100495D0

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D9 05 ? ? ? ? D9 E0 D9 45 FC D8 25");
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
                    ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                }

                if (ElementPosX == 8.0f && regs.eax != 8) // bullet time overlay()
                {
                    ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                }

                if (ElementPosX == 12.0f) // painkillers
                {
                    ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                }

                if (ElementPosX == 22.5f) //health bar and overlay
                {
                    ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                }

                if (ElementPosX == 95.0f) // other weapons name
                {
                    ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;
                }

                if (ElementPosX == 190.0f) //molotovs/grenades name pos
                {
                    ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;
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

            *(float*)(regs.ebp - 4) -= ElementNewPosX2;
            *(float*)(regs.ebp - 8) -= ElementNewPosY2;

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
        Screen.fWidescreenHudOffset = std::abs(CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f));
        if (Screen.fHudAspectRatioConstraint.has_value())
        {
            float value = Screen.fHudAspectRatioConstraint.value();
            if (value < 0.0f || value > (32.0f / 9.0f))
                Screen.fWidescreenHudOffset = value;
            else
            {
                value = ClampHudAspectRatio(value, Screen.fAspectRatio);
                Screen.fWidescreenHudOffset = std::abs(CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f));
            }
        }

        pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D9 05 ? ? ? ? D8 8E 74 01 00 00");
        static auto pTextElementPosX = *pattern.get_first<TextCoords*>(2); //0x100647D0
        struct P_TextPosHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto TextPosX = pTextElementPosX->a;
                auto TextNewPosX = TextPosX;

                if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
                    TextNewPosX = TextPosX + Screen.fWidescreenHudOffset;

                _asm fld    dword ptr[TextNewPosX]
            }
        }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6));

        static float TextPosX1, TextPosX2, TextPosY1;
        pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "C7 45 D0 00 00 00 00 D9 5D"); //100045FC
        struct P_TextPosHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.ebp - 0x30) = 0.0f;
                TextPosX1 = *(float*)(regs.ebp - 0x28);
                TextPosY1 = *(float*)(regs.ebp - 0x2C);
            }
        }; injector::MakeInline<P_TextPosHook2>(pattern.get_first(0), pattern.get_first(7));

        pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "89 41 08 D9 45 E4 D8 0D"); //0x10004693
        struct P_TextPosHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                TextPosX2 = *(float*)(regs.ebp - 0x1C);

                if (TextPosX1 == (69.0f + Screen.fWidescreenHudOffset) && TextPosY1 == 457.0f) // painkillers amount number
                    *(float*)(regs.ebp - 0x1C) += (24.0f * Screen.fWidescreenHudOffset);

                *(uint32_t*)(regs.ecx + 8) = regs.eax;
                auto ebp1C = *(float*)(regs.ebp - 0x1C);
                _asm fld  dword ptr[ebp1C]
            }
        }; injector::MakeInline<P_TextPosHook3>(pattern.get_first(0), pattern.get_first(6));
    }

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

            if (!X_Crosshair::sm_bCameraPathRunning)
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
}

export void InitE2MFC()
{
    static std::once_flag wf;
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
            Screen.nHeight = *(int32_t*)(P_Driver__m_initializedDriver + 56);
        else
            Screen.nHeight = *(int32_t*)(P_Driver__m_initializedDriver + 8);

        if (Screen.nWidth && Screen.nHeight)
        {
            std::call_once(wf, []() { InitWF(); });
        }
        return Screen.nHeight;
    };

    //get resolution
    auto pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "E8 ? ? ? ? 8B 0D ? ? ? ? 89 45 DC 89 5D E0");
    injector::MakeCALL(pattern.count_hint(2).get(0).get<uintptr_t>(0), static_cast<int32_t(__fastcall*)(uintptr_t, uintptr_t)>(PDriverGetWidth), true);   //e2mfc + 0x15582
    injector::MakeCALL(pattern.count_hint(2).get(0).get<uintptr_t>(41), static_cast<int32_t(__fastcall*)(uintptr_t, uintptr_t)>(PDriverGetHeight), true); //e2mfc + 0x155AB
    //CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitWF, NULL, 0, NULL);

    //relocate dllmain code of e2_d3d8_driver_mfc
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "C7 46 ? ? ? ? ? 8B 75 ? 89 07 EB 15 FF 15 ? ? ? ? 50 68 ? ? ? ? 53 E8 ? ? ? ? 83 C4 0C 8D 8D ? ? ? ? 51 56 FF 15 ? ? ? ? 85 C0 75 ? 56 FF 15 ? ? ? ? 8D 95");
    static auto LoadLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.eax)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.eax, DLL_PROCESS_ATTACH, nullptr);
        }
    });

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "51 FF D3 8B 76");
    static auto FreeLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.ecx)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.ecx, DLL_PROCESS_DETACH, nullptr);
        }
    });
}