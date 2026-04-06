module;

#include <stdafx.h>
#include <d3d9.h>

export module Frontend;

import ComVars;

export float* HudScaleX_8AF9A4 = (float*)0x8AF9A4;
export float* FE_Xpos_894B40 = (float*)0x894B40;

namespace XtendedInput
{
    HMODULE mhXtendedInput;
    float(__cdecl* SetFEScale)(float val);
    bool(__cdecl* GetUseWin32Cursor)();
    bool bLookedForXInput = false;
    bool bFoundXInput = false;

    void LookForXtendedInput()
    {
        if (!mhXtendedInput)
        {
            mhXtendedInput = GetModuleHandleA("NFS_XtendedInput.asi");
            if (mhXtendedInput)
            {
                SetFEScale = reinterpret_cast<float(__cdecl*)(float)>(GetProcAddress(mhXtendedInput, "SetFEScale"));
                GetUseWin32Cursor = reinterpret_cast<bool(__cdecl*)()>(GetProcAddress(mhXtendedInput, "GetUseWin32Cursor"));
                bFoundXInput = (SetFEScale != nullptr) && (GetUseWin32Cursor != nullptr);
            }
        }
    }
}

export namespace FEScale
{
    float fFEScale = 1.0f;
    float fCalcFEScale = 1.0f;
    float fFMVScale = 1.0f;
    float fCalcFMVScale = 1.0f;
    bool bEnabled = false;
    bool bAutoFitFE = true;
    bool bAutoFitFMV = true;

    uintptr_t SetTransformAddr = 0x6C8000;
    uintptr_t gMoviePlayerAddr = 0x91CB10;

    void __cdecl SetTransformHook(D3DMATRIX* mat, uint32_t EVIEW_ID)
    {
        D3DMATRIX cMat;
        memcpy(&cMat, mat, sizeof(D3DMATRIX));

        if (*(uintptr_t*)gMoviePlayerAddr)
        {
            cMat._11 *= fCalcFMVScale;
            cMat._22 *= fCalcFMVScale;
        }
        else
        {
            cMat._11 *= fCalcFEScale;
            cMat._22 *= fCalcFEScale;
        }

        return reinterpret_cast<void(__cdecl*)(D3DMATRIX*, uint32_t)>(SetTransformAddr)(&cMat, EVIEW_ID);
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

        if (!XtendedInput::bLookedForXInput)
            XtendedInput::LookForXtendedInput();
        if (!XtendedInput::bFoundXInput) return;
        if (!XtendedInput::GetUseWin32Cursor()) return;

        XtendedInput::SetFEScale(fCalcFEScale);
    }
}

uintptr_t PostStartFunc = 0x739600;
void InitPostStart()
{
    if (FEScale::bEnabled)
        FEScale::Update();
    return reinterpret_cast<void(*)()>(PostStartFunc)();
}

export void InitFrontend()
{
    CIniReader iniReader("");
    bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bHUDWidescreenMode = iniReader.ReadInteger("MAIN", "HUDWidescreenMode", 1) == 1;
    int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    FEScale::fFEScale = iniReader.ReadFloat("MAIN", "FEScale", 1.0f);
    FEScale::fCalcFEScale = FEScale::fFEScale;
    FEScale::fFMVScale = iniReader.ReadFloat("MAIN", "FMVScale", 1.0f);
    FEScale::fCalcFMVScale = FEScale::fFMVScale;
    FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 1) != 0;
    FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;

    // Post-start init function hook
    uintptr_t loc_66616E = reinterpret_cast<uintptr_t>(hook::pattern("68 89 88 88 3C E8 ? ? ? ? E8 ? ? ? ? 6A 00").get_first(0)) + 0x6F;
    PostStartFunc = static_cast<uintptr_t>(injector::GetBranchDestination(loc_66616E));
    injector::MakeCALL(loc_66616E, InitPostStart);

    if (bFixHUD)
    {
        DWORD oldprotect = 0;
        HudScaleX_8AF9A4 = *hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04").count(1).get(0).get<float*>(2);
        injector::UnprotectMemory(HudScaleX_8AF9A4, sizeof(float), oldprotect);
        *HudScaleX_8AF9A4 = Screen.fHudScaleX;

        //fHudScaleY = *(float*)0x8AF9A0;
        //injector::WriteMemory<float>(0x8AF9A0, fHudScaleY, true);

        FE_Xpos_894B40 = *hook::pattern("D8 25 ? ? ? ? D9 5C 24 14 DB 05 ? ? ? ? D8 25 ? ? ? ? D9 5C 24 1C 74 20").count(1).get(0).get<float*>(2);
        injector::UnprotectMemory(FE_Xpos_894B40, sizeof(float), oldprotect);
        *FE_Xpos_894B40 = Screen.fHudPosX;

        // make code read the FE X position from the variable
        auto pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x56FED4
        struct HudPosXHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x19C) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook1>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x584EEF
        struct HudPosXHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x90) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook2>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x599E79
        struct HudPosXHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x84) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook3>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A120
        struct HudPosXHook4
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0xC4) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook4>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A5AB
        struct HudPosXHook5
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x94) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook5>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A83E
        injector::MakeInline<HudPosXHook5>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43"); // 0x005A44C8
        struct HudPosXHook6
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x64) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook6>(pattern.get_first(0), pattern.get_first(8));

        //mirror position fix
        pattern = hook::pattern("C7 44 24 70 00 00 E1 43 C7 44 24 74 00 00 98 41 C7 84 24 80 00 00 00 00 00 3E 43"); // 0x6E70C0
        struct MirrorPosXHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x70) = (*(float*)(FE_Xpos_894B40)-320.0f) + 450.0f;
                *(float*)(regs.esp + 0xA0) = (*(float*)(FE_Xpos_894B40)-320.0f) + 450.0f;
                *(float*)(regs.esp + 0x80) = (*(float*)(FE_Xpos_894B40)-320.0f) + 190.0f;
                *(float*)(regs.esp + 0x90) = (*(float*)(FE_Xpos_894B40)-320.0f) + 190.0f;
                // others
                *(float*)(regs.esp + 0x74) = 19.0;
                *(float*)(regs.esp + 0x84) = 19.0;
                *(float*)(regs.esp + 0x94) = 89.0;
                *(float*)(regs.esp + 0xA4) = 89.0;
            }
        }; injector::MakeInline<MirrorPosXHook>(pattern.get_first(0), pattern.get_first(82));
    }

    uint32_t* dword_57CB82 = hook::pattern("3A 55 34 0F 85 0B 02 00 00 A1").count(1).get(0).get<uint32_t>(0); // HUD
    uint32_t* dword_5696CB = hook::pattern("8A 41 34 38 86 30 03 00 00 74 52 84 C0").count(1).get(0).get<uint32_t>(0); // HUD
    uint32_t* dword_58D883 = hook::pattern("8A 40 34 5F 5E 5D 3B CB 5B 75 12").count(1).get(0).get<uint32_t>(0); // EA Trax
    uint32_t* dword_56885A = hook::pattern("38 48 34 74 31 8B 4E 38 68 7E 78 8E 90").count(1).get(0).get<uint32_t>(0); // Wrong Way Sign
    if (bHUDWidescreenMode)
    {
        injector::WriteMemory<uint32_t>(dword_57CB82, 0x0F01FA80, true);
        injector::WriteMemory<uint32_t>(dword_5696CB, 0x389001B0, true);
        injector::WriteMemory<uint32_t>(dword_58D883, 0x5F9001B0, true);
        injector::WriteMemory<uint32_t>(dword_56885A, 0x7401F980, true);

        //Widescreen Splash
        auto pattern = hook::pattern("8B 46 10 8B 3D ? ? ? ? 53 50");
        injector::MakeNOP(pattern.get_first(-2), 2, true);
        pattern = hook::pattern("E8 ? ? ? ? 84 C0 B8 ? ? ? ? 75 ? B8 ? ? ? ? C3");
        auto aWs_mw_ls_splas = *pattern.count(2).get(1).get<char*>(8);
        auto aMw_ls_splash_0 = *pattern.count(2).get(1).get<char*>(15);
        injector::WriteMemoryRaw(aMw_ls_splash_0, aWs_mw_ls_splas, strlen(aWs_mw_ls_splas), true);
        uint32_t* dword_5A3142 = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 ? 3B FB").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory(dword_5A3142, 0xC4DF3FF2, true); //"CLICK to continue" (PC)

        //issue #343 HD Compatible For Optimal Gaming Logo
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C E8 ? ? ? ? 85 C0 75 ? 8B 46");
        injector::MakeCALL(pattern.get_first(0), injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 46 10 68 6D 44 CF 13"), true), true);
    }
    else
    {
        injector::WriteMemory<uint32_t>(dword_57CB82, 0x0F00FA80, true);
        injector::WriteMemory<uint32_t>(dword_5696CB, 0x389000B0, true);
        injector::WriteMemory<uint32_t>(dword_58D883, 0x5F9000B0, true);
        injector::WriteMemory<uint32_t>(dword_56885A, 0x7400F980, true);
    }

    if (nFMVWidescreenMode)
    {
        uint32_t* dword_59A02A = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B 4C 24 18").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<float>(dword_59A02A + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Bottom)
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Top)
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)

        if (nFMVWidescreenMode > 1)
        {
            injector::WriteMemory<float>(dword_59A02A + 0, (0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Bottom)
            injector::WriteMemory<float>((uint32_t)dword_59A02A + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
            injector::WriteMemory<float>((uint32_t)dword_59A02A + 10, -(0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Top)
            injector::WriteMemory<float>((uint32_t)dword_59A02A + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)
        }
    }

    if ((FEScale::fFEScale != 1.0f) || (FEScale::fFMVScale != 1.0f) || (FEScale::bAutoFitFE) || (FEScale::bAutoFitFMV))
    {
        FEScale::bEnabled = true;

        uintptr_t loc_6E6FB7 = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 10 00 00 80 3F C7 44 24 24 00 00 80 3F").get_first(0)) + 0x66;
        uintptr_t loc_6E7011 = loc_6E6FB7 + 0x5A;
        uintptr_t loc_559789 = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 60 00 00 0A 00 88 5C 24 70").get_first(0)) + 0x2D;

        FEScale::SetTransformAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6E6FB7));
        FEScale::gMoviePlayerAddr = *reinterpret_cast<uintptr_t*>(loc_559789 + 2);

        injector::MakeCALL(loc_6E6FB7, FEScale::SetTransformHook);
        injector::MakeCALL(loc_6E7011, FEScale::SetTransformHook);
    }
}