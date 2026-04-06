module;

#include <stdafx.h>
#include <d3d9.h>

export module Frontend;

import ComVars;

bool bHUDWidescreenMode;

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

    uintptr_t SetTransformAddr = 0x005BDDE0;
    uintptr_t MovieIsStartedAddr = 0x008383AC;

    void __cdecl SetTransformHook(D3DMATRIX* mat, uint32_t EVIEW_ID)
    {
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
    }

    uintptr_t DrawFEAddr = 0x005CBF40;
    uintptr_t eWaitUntilRenderingDone = 0x004022C0;

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

    void eWaitUntilRenderingDoneHook()
    {
        UnsetMovieFlag();
        reinterpret_cast<void(*)()>(eWaitUntilRenderingDone)();
    }
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
    float fPosX;
    float fPosY;
    float fOffsetX;
    float fOffsetY;

    CDatEntry(float posx, float posy, float fOffsetX, float fOffsetY)
        : fPosX(posx), fPosY(posy), fOffsetX(fOffsetX), fOffsetY(fOffsetY)
    {
    }
};

std::vector<CDatEntry> HudCoords;
void LoadDatFile()
{
    CIniReader iniReader("");
    auto DataFilePath = iniReader.GetIniPath();
    DataFilePath.replace_extension(".dat");

    std::ifstream hFile;
    hFile.open(DataFilePath);
    if (hFile.is_open())
    {
        std::string line;
        while (std::getline(hFile, line))
        {
            if (line[0] && line[0] != '#')
            {
                float PosX, PosY;
                float fOffsetX, fOffsetY;
                sscanf_s(line.data(), "%*s %f %f %f %f", &PosX, &PosY, &fOffsetX, &fOffsetY);

                if (Screen.fAspectRatio < (16.0f / 9.0f))
                    fOffsetX /= (((16.0f / 9.0f) / Screen.fAspectRatio) * 1.5f);

                HudCoords.push_back(CDatEntry(PosX, PosY, fOffsetX, fOffsetY));
            }
        }
        hFile.close();
    }
}

export void InitFrontend()
{
    CIniReader iniReader("");
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bDisableCutsceneBorders = iniReader.ReadInteger("MISC", "DisableCutsceneBorders", 1) != 0;

    bHUDWidescreenMode = iniReader.ReadInteger("MAIN", "HUDWidescreenMode", 1) != 0;
    int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    FEScale::fFEScale = iniReader.ReadFloat("MAIN", "FEScale", 1.0f);
    FEScale::fCalcFEScale = FEScale::fFEScale;
    FEScale::fFMVScale = iniReader.ReadFloat("MAIN", "FMVScale", 1.0f);
    FEScale::fCalcFMVScale = FEScale::fFMVScale;
    FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 0) != 0;
    FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;

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
                                   [cc = CDatEntry(std::floor(HudPosX.fPos), std::floor(HudPosY.fPos), 0.0f, 0.0f)]
                                   (const CDatEntry& cde) -> bool
            {
                return (cc.fPosX == cde.fPosX && cc.fPosY == cde.fPosY);
            });

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

    if ((FEScale::fFEScale != 1.0f) || (FEScale::fFMVScale != 1.0f) || (FEScale::bAutoFitFE) || (FEScale::bAutoFitFMV))
    {
        FEScale::bEnabled = true;

        uintptr_t loc_5CC08D = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 2C 00 00 80 3F 8B 10 50").get_first(0)) + 0xDE;
        uintptr_t loc_566042 = reinterpret_cast<uintptr_t>(hook::pattern("6A 00 68 B9 0E 96 C3 B9 ? ? ? ? E8 ? ? ? ? 59").get_first(0)) + 0x18;
        uintptr_t loc_5D2B18 = reinterpret_cast<uintptr_t>(hook::pattern("FF 91 A4 00 00 00 6A 01 E8").get_first(0)) + 8;
        uintptr_t loc_511BD9 = reinterpret_cast<uintptr_t>(hook::pattern("8A 48 34 84 C9 0F 85 ? ? ? ? 55").get_first(0)) + 0xC;
        uintptr_t loc_566197 = reinterpret_cast<uintptr_t>(hook::pattern("68 00 01 00 00 51 8D 54 24 14").get_first(0)) + 0x13;


        FEScale::SetTransformAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_5CC08D));
        FEScale::MovieIsStartedAddr = *reinterpret_cast<uintptr_t*>(loc_566042 + 2);
        FEScale::DrawFEAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_5D2B18));
        FEScale::eWaitUntilRenderingDone = static_cast<uintptr_t>(injector::GetBranchDestination(loc_511BD9));

        injector::MakeInline<FEScale::MovieStartHook>(loc_566042, loc_566042 + 0xA);
        injector::MakeCALL(loc_511BD9, FEScale::eWaitUntilRenderingDoneHook);
        injector::MakeCALL(loc_566197, FEScale::eWaitUntilRenderingDoneHook);
        injector::MakeCALL(loc_5CC08D, FEScale::SetTransformHook);

        auto pattern = hook::pattern("A1 ? ? ? ? 8A 48 ? 84 C9 DE F9");
        static auto loc_5369CC = pattern.get_first(14);
        static auto MovieStartHookCrashFix = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
        {
            if (!regs.eax)
                *(void**)(regs.esp - 4) = loc_5369CC;
        });

        FEScale::Update();
    }
}