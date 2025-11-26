module;

#include <stdafx.h>

export module e2mfc;

import ComVars;

void InitWF()
{
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
    static bool bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    Screen.bGraphicNovelMode = iniReader.ReadInteger("MAIN", "GraphicNovelMode", 1) != 0;

    //fix aspect ratio
    auto PCameraValidate = [](uintptr_t PCamera, uintptr_t edx)
    {
        float fParam0 = *(float*)(PCamera + 0x11C);
        float fParam1 = *(float*)(PCamera + 0x1DC);
        float fParam2 = *(float*)(PCamera + 0x1D8);
        float fParam3 = *(float*)(PCamera + 0x1D4);
        float fParam4 = *(float*)(PCamera + 0x1D0);

        float fParam5 = *(float*)(PCamera + 0x1E0);
        float fParam6 = *(float*)(PCamera + 0x220);
        //BYTE Param7 = *(BYTE *)(PCamera + 0x208);
        //BYTE Param8 = *(BYTE *)(PCamera + 0x480);

        if ((*(BYTE*)(PCamera + 0xEC) >> 6) & 1)
        {
            float v2 = (float)tan(fParam0 / 2.0);
            *(float*)(PCamera + 0x20C) = v2;

            if (!Screen.bIs2D && !Screen.bIsInGraphicNovel)
            {
                if (fParam5 == 0.05f || Screen.bIsSkybox) /*|| stack[3] == (void*)0x004122bf*/
                    *(float*)(PCamera + 0x20C) = v2 * Screen.fDiffFactor;
            }

            *(float*)(PCamera + 0x210) = (fParam1 - fParam2) * v2 / (fParam3 - fParam4);
        }
    };

    auto pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "E8 ? ? ? ? A0 ? ? ? ? 80 A6 EC 00 00 00 BF A8 01");
    injector::MakeCALL(pattern.get_first(), static_cast<void(__fastcall*)(uintptr_t, uintptr_t)>(PCameraValidate), true); //0x10017077

    //corrupted graphic in tunnel in mp1, not sure if needed in mp2 (DisableSubViewport)
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "83 EC 08 56 8B F1 8B 86 E8");
    injector::MakeRET(pattern.get_first(), 0x10, true); //100156A0 ret 10h

    //ForceEntireViewport 0x10014C80 mov ax, 1 ret
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "A0 ? ? ? ? C3");
    injector::WriteMemory<uint8_t>(pattern.count(3).get(2).get<void*>(0), 0xB0, true);
    injector::WriteMemory(pattern.count(3).get(2).get<void*>(1), 0x90909001, true);

    //object disappearance fix
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "C7 05 ? ? ? ? ? ? ? ? D8 35"); //0x10015B87
    injector::WriteMemory<float>(pattern.get_first(6), 0.0f, true);

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? 8B 44 24 24 52 50 51"); //0x10042B50
    injector::WriteMemory<float>(*pattern.get_first<void*>(2), Screen.fHudOffset, true);
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? D9 44 24 1C"); //0x10042B58
    injector::WriteMemory<float>(*pattern.get_first<void*>(2), Screen.fHudScale, true);

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 25 ? ? ? ? 89 44 24 44 83 EC 08");
    static auto pHudElementPosX = *pattern.get_first<float*>(2); //0x10061134
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 25 ? ? ? ? D9 5C 24 20");
    static auto pHudElementPosY = *pattern.get_first<float*>(2); //0x10061138
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D9 05 ? ? ? ? 8D 81 54");
    static auto pf10042294 = *pattern.get_first<float*>(2); //0x10042294

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D9 05 ? ? ? ? 8B 86 64 01 00 00");
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
                    ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                }
                else
                    if (ElementPosX == 8.0f && ElementPosY != 8.0f) // bullet time overlay()
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                    }
                    else
                        if (ElementPosX == 12.0f) // painkillers
                        {
                            ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                        }
                        else
                            if (ElementPosX == 22.5f) //health bar and overlay
                            {
                                ElementNewPosX1 = ElementPosX + Screen.fWidescreenHudOffset;
                            }
                            else
                                if (ElementPosX == 96.0f) // other weapons name
                                {
                                    ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;
                                }
                                else
                                    if (ElementPosX == 192.0f) //molotovs/grenades name pos
                                    {
                                        ElementNewPosX1 = ElementPosX - Screen.fWidescreenHudOffset;
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

        pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D9 05 ? ? ? ? D8 8B 98");
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
                            TextNewPosX = TextPosX - Screen.fWidescreenHudOffset;
                        }
                        else
                        {
                            TextNewPosX = TextPosX + Screen.fWidescreenHudOffset;
                        }
                    }
                }
                //*(float*)(regs.esp + 0x2C) = TextPosX * *(float*)(regs.ebx + 0x198);
                __asm fld dword ptr[TextNewPosX]
            }
        }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6)); //0x1000AACC
    }
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
    auto pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "E8 ? ? ? ? 85 C0 89 44 24 38 DB");
    injector::MakeCALL(pattern.count_hint(4).get(0).get<uintptr_t>(0), static_cast<int32_t(__fastcall*)(uintptr_t, uintptr_t)>(PDriverGetWidth), true);  //e2mfc + 0x15582
    injector::MakeCALL(pattern.count_hint(4).get(1).get<uintptr_t>(0), static_cast<int32_t(__fastcall*)(uintptr_t, uintptr_t)>(PDriverGetHeight), true); //e2mfc + 0x155AB

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "C6 45 FC 02 89 4B 44");
    struct P_SceneRenderHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ebp - 0x04) = 2;
            *(uint32_t*)(regs.ebx + 0x44) = regs.ecx;
            Screen.bIs2D = true;
        }
    }; injector::MakeInline<P_SceneRenderHook1>(pattern.get_first(0), pattern.get_first(7));

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "89 73 34 89 73 38");
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
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 35 ? ? ? ? D8 4C 24 38 D9 5C 24 34");
    injector::WriteMemory(pattern.get_first(2), &Screen.fViewPortSizeX, true); //0x100176D4
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 35 ? ? ? ? D8 4C 24 38 D9 5C 24 30");
    injector::WriteMemory(pattern.get_first(2), &Screen.fViewPortSizeY, true); //0x10017719
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 35 ? ? ? ? 89 1D ? ? ? ? 89 1D");
    injector::WriteMemory(pattern.get_first(2), &Screen.fViewPortSizeY, true); //0x1001779A
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? 8B 44 24 1C 52 50 51"); //0x1000F05E
    injector::WriteMemory(pattern.get_first(2), &Screen.fNovelsOffset, true);
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? D9 44 24 20 53"); //0x1000F011
    injector::WriteMemory(pattern.get_first(2), &Screen.fNovelsScale, true);

    //relocate dllmain code of e2_d3d8_driver_mfc
    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "8B 55 ? 40 C7 47 ? ? ? ? ? 89 47 ? 8B 7D ? 89 0A EB 15 FF 15 ? ? ? ? 50 68 ? ? ? ? 56 E8 ? ? ? ? 83 C4 0C 8D 85 ? ? ? ? 50 57 FF 15 ? ? ? ? 85 C0 75 ? 57 FF 15 ? ? ? ? 8D 8D");
    static auto LoadLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.ecx)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.ecx, DLL_PROCESS_ATTACH, nullptr);
        }
    });

    pattern = hook::module_pattern(GetModuleHandle(L"e2mfc"), "52 FF D7 8B 76");
    static auto FreeLibraryHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (GetModuleHandle(L"e2_d3d8_driver_mfc") == (HMODULE)regs.edx)
        {
            shDllMainHook.unsafe_stdcall<BOOL>((HMODULE)regs.edx, DLL_PROCESS_DETACH, nullptr);
        }
    });
}