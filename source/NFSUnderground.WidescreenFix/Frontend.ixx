module;

#include <stdafx.h>
#include <d3d9.h>

export module Frontend;

import ComVars;

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
    {
    }
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
    FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 0) != 0;
    FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;

    //HUD
    if (bFixHUD)
    {
        auto pattern = hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8 81 00 00 00 D8"); //6CC914
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

    if (nFMVWidescreenMode)
    {
        auto pattern = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF");
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
                                       (const CDatEntry& cde) -> bool
                {
                    return (cc.dwPosX == cde.dwPosX && cc.dwPosY == cde.dwPosY);
                });

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

        auto pattern = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04");
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
}