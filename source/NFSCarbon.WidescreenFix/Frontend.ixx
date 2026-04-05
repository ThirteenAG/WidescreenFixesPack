module;

#include <stdafx.h>
#include <d3d9.h>

export module Frontend;

import ComVars;

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

namespace FEScale
{
    float fFEScale = 1.0f;
    float fCalcFEScale = 1.0f;
    float fFMVScale = 1.0f;
    float fCalcFMVScale = 1.0f;
    bool bEnabled = false;
    bool bAutoFitFE = true;
    bool bAutoFitFMV = true;

    uintptr_t gMoviePlayerAddr = 0x00A97BB4;

    void ScaleMat(D3DMATRIX* cMat)
    {
        if (*(uintptr_t*)gMoviePlayerAddr)
        {
            cMat->_11 *= fCalcFMVScale;
            cMat->_22 *= fCalcFMVScale;
        }
        else
        {
            cMat->_11 *= fCalcFEScale;
            cMat->_22 *= fCalcFEScale;
        }
    }

    struct FEScaleHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            uintptr_t vTable = *reinterpret_cast<uintptr_t*>(regs.ecx);
            uintptr_t SetTransformAddr = *reinterpret_cast<uintptr_t*>(vTable + 8);

            D3DMATRIX* mat = reinterpret_cast<D3DMATRIX*>(regs.esp + 0x30);
            D3DMATRIX cMat;
            memcpy(&cMat, mat, sizeof(D3DMATRIX));

            ScaleMat(&cMat);

            reinterpret_cast<void(__thiscall*)(uintptr_t, D3DMATRIX*, uintptr_t, uint32_t)>(SetTransformAddr)(regs.ecx, &cMat, regs.ebx, 0);
        }
    };

    struct FEScaleHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            uintptr_t vTable = *reinterpret_cast<uintptr_t*>(regs.esi);
            uintptr_t SetTransformAddr = *reinterpret_cast<uintptr_t*>(vTable + 8);

            D3DMATRIX* mat = reinterpret_cast<D3DMATRIX*>(regs.esp + 0x30);
            D3DMATRIX cMat;
            memcpy(&cMat, mat, sizeof(D3DMATRIX));

            ScaleMat(&cMat);

            reinterpret_cast<void(__thiscall*)(uintptr_t, D3DMATRIX*, uintptr_t, uint32_t)>(SetTransformAddr)(regs.esi, &cMat, regs.ebx, 0);
        }
    };

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

#pragma runtime_checks( "", restore )

uintptr_t PostStartFunc = 0x7AFEF0;
void InitPostStart()
{
    if (FEScale::bEnabled)
        FEScale::Update();
    return reinterpret_cast<void(*)()>(PostStartFunc)();
}

export void InitFrontend()
{
    CIniReader iniReader("");
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bHUDWidescreenMode = iniReader.ReadInteger("MAIN", "HUDWidescreenMode", 1) != 0;
    int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    FEScale::fFEScale = iniReader.ReadFloat("MAIN", "FEScale", 1.0f);
    FEScale::fCalcFEScale = FEScale::fFEScale;
    FEScale::fFMVScale = iniReader.ReadFloat("MAIN", "FMVScale", 1.0f);
    FEScale::fCalcFMVScale = FEScale::fFMVScale;
    FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 1) != 0;
    FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;

    // Post-start init function hook
    uintptr_t loc_6B783F = reinterpret_cast<uintptr_t>(hook::pattern("6A 01 C7 44 24 14 FF FF FF FF A3").get_first(0)) + 0x9D;
    PostStartFunc = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6B783F));
    injector::MakeCALL(loc_6B783F, InitPostStart);

    //HUD
    if (bFixHUD)
    {
        Screen.fHudScaleX = (1.0f / Screen.Width * (Screen.Height / 480.0f)) * 2.0f;
        Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

        uint32_t* dword_9E8F8C = *hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04 D8 0D ? ? ? ? D8 25 ? ? ? ? D9 E0").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_9E8F8C, Screen.fHudScaleX, true);

        //fHudScaleY = *(float*)0x9E8F88;
        //injector::WriteMemory<float>(0x9E8F88, fHudScaleY);

        for (size_t i = 0; i < 4; i++)
        {
            uint32_t* dword_598DC0 = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43").count(1).get(0).get<uint32_t>(7);
            injector::WriteMemory<float>(dword_598DC0, Screen.fHudPosX, true);
        }

        for (size_t i = 0; i < 2; i++)
        {
            uint32_t* dword_5A18BA = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").count(1).get(0).get<uint32_t>(4);
            injector::WriteMemory<float>(dword_5A18BA, Screen.fHudPosX, true);
        }

        uint32_t* dword_A604AC = *hook::pattern("D8 05 ? ? ? ? 89 44 24 18 D9 44 24 18").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_A604AC, Screen.fHudPosX, true);

        uint32_t* dword_9C778C = *hook::pattern("D8 25 ? ? ? ? 8D 46 34 50 D9 5C 24 20").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_9C778C, Screen.fHudPosX, true);

        //mini_map_route fix
        uint32_t* dword_9D5F3C = *hook::pattern("D8 05 ? ? ? ? D9 5C 24 7C D9 86 B8 00 00 00 D8 05 ? ? ? ? D9 9C 24 80 00 00 00").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_9D5F3C, (Screen.fHudPosX - 320.0f) + 384.0f, true);
    }

    if (bHUDWidescreenMode)
    {
        uint32_t* dword_5DC508 = hook::pattern("0F 95 C1 3A C1 75 2B 8B 0D ? ? ? ? 3B CE").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<uint8_t>(dword_5DC508, 0x94, true);
        uint32_t* dword_5D52B3 = hook::pattern("0F 95 C0 3A C8 0F 84 C7 00 00 00 84 C0 88 86 41 03 00 00").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<uint8_t>(dword_5D52B3, 0x94, true);
        uint32_t* dword_5B6BAE = hook::pattern("74 07 68 ? ? ? ? EB 05 68 ? ? ? ? E8 ? ? ? ? 8B ? ? ? ? ? 8B E8 8B 03 83 C4 04").count(1).get(0).get<uint32_t>(0);
        injector::WriteMemory<uint8_t>(dword_5B6BAE, 0x75, true);
        uint32_t* dword_5B6B5B = hook::pattern("74 07 68 ? ? ? ? EB 05 68 ? ? ? ? E8 ? ? ? ? 8B ? ? ? ? ? 89 44 24 14 8B 03 83 C4 04").count(1).get(0).get<uint32_t>(0);
        injector::WriteMemory<uint8_t>(dword_5B6B5B, 0x75, true);

        //PiP/rearview mirror
        uint32_t* dword_750DE7 = hook::pattern("68 CD CC 8C 3E 68 33 33 33 3F 8D 54 24 10 68 F4 FD D4 3C 52").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<float>(dword_750DE7, 0.2799999714f, true);
        uint32_t dword_750DF5 = (uint32_t)dword_750DE7 + 14;
        injector::WriteMemory<float>(dword_750DF5, -0.1649999917f, true);

        static float fMirrorScaling = 1.0f * ((4.0f / 3.0f) / ((float)Screen.Width / (float)Screen.Height));
        auto pattern = hook::pattern("D9 5E 4C 89 46 5C 5E 5B C3");
        struct MirrorFix
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esi + 0x00) = *(float*)(regs.esi + 0x00) * fMirrorScaling;
                *(float*)(regs.esi + 0x18) = *(float*)(regs.esi + 0x18) * fMirrorScaling;
                *(float*)(regs.esi + 0x30) = *(float*)(regs.esi + 0x30) * fMirrorScaling;
                *(float*)(regs.esi + 0x48) = *(float*)(regs.esi + 0x48) * fMirrorScaling;
            }
        }; injector::MakeInline<MirrorFix>(pattern.get_first(6)); //750C23
        injector::WriteMemory(pattern.get_first(6 + 5), 0x90C35B5E, true); //pop esi pop ebx ret
    }

    if (nFMVWidescreenMode)
    {
        // Widescreen FMV Text Placement
        uint32_t* dword_5AB6D7 = hook::pattern("74 3C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 68 D1 BC D5 FF").count(1).get(0).get<uint32_t>(0);
        injector::WriteMemory<uint8_t>(dword_5AB6D7, 0xEB, true);

        if (nFMVWidescreenMode > 1)
        {
            // HD FMV Support
            uint32_t* dword_598EB9 = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B CB C7").count(1).get(0).get<uint32_t>(6);
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 0, (0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Bottom)
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 10, -(0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Top)
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)
        }
        else
        {
            // Native Widescreen FMV Support
            uint32_t* dword_5BB818 = hook::pattern("74 6A 8B CE E8 ? ? ? ? 84 C0 75 5F 68 ? ? ? ? 68").count(1).get(0).get<uint32_t>(0);
            injector::MakeNOP(dword_5BB818, 2, true);
            uint32_t* dword_5BD4A1 = hook::pattern("74 0D 68 ? ? ? ? E8 ? ? ? ? 5E 59 C3").count(1).get(0).get<uint32_t>(0);
            injector::MakeNOP(dword_5BD4A1, 2, true);
        }
    }

    if ((FEScale::fFEScale != 1.0f) || (FEScale::fFMVScale != 1.0f) || (FEScale::bAutoFitFE) || (FEScale::bAutoFitFMV))
    {
        FEScale::bEnabled = true;

        uintptr_t loc_730E4D = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 60 00 00 80 3F C7 44 24 74 00 00 80 3F B9 10 00 00 00").get_first(0)) + 0x57;
        uintptr_t loc_730ECE = loc_730E4D + 0x81;
        uintptr_t loc_54B895 = reinterpret_cast<uintptr_t>(hook::pattern("3D 91 FA C3 01 74 ? 8B 0D").get_first(0)) + 7;

        FEScale::gMoviePlayerAddr = *reinterpret_cast<uintptr_t*>(loc_54B895 + 2);

        injector::MakeInline<FEScale::FEScaleHook1>(loc_730E4D, loc_730E4D + 0xB);
        injector::MakeInline<FEScale::FEScaleHook2>(loc_730ECE, loc_730ECE + 0xC);
    }
}