module;

#include <stdafx.h>
#include <d3d9.h>

export module Frontend;

import ComVars;
import Resolution;

std::optional<float> fHudAspectRatioConstraint;
float fWidescreenHudOffset = 120.0f;
void (__cdecl* eSetCurrentRenderTarget)(int a1) = nullptr;

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

    injector::hook_back<void(__cdecl*)(D3DMATRIX*, uint32_t)> hbSetTransformHook;
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

        return hbSetTransformHook.fun(&cMat, EVIEW_ID);
    }

    void Update()
    {
        fCalcFEScale = fFEScale;
        fCalcFMVScale = fFMVScale;
        float fAspectRatio = GetAspectRatio();

        if (bAutoFitFE)
            fCalcFEScale *= fAspectRatio / (4.0f / 3.0f);
        if (bAutoFitFMV)
            fCalcFMVScale *= fAspectRatio / (16.0f / 9.0f);

        if (fCalcFEScale > fFEScale)
            fCalcFEScale = fFEScale;

        if (fCalcFMVScale > fFMVScale)
            fCalcFMVScale = fFMVScale;
    }

    void SetMovieFlag()
    {
        bMovieFlag = true;
        // execute a draw call for the FE to set the transform (similar to Pro Street)
        eSetCurrentRenderTarget(0);
    }

    void UnsetMovieFlag()
    {
        bMovieFlag = false;
        eSetCurrentRenderTarget(0);
    }
}

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
                HudCoords.push_back(CDatEntry(PosX, PosY, fOffsetX, fOffsetY));
            }
        }
        hFile.close();
    }
}

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
            static bool bDisableCutsceneBorders = iniReader.ReadInteger("MISC", "DisableCutsceneBorders", 1) != 0;

            int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
            FEScale::fFEScale = iniReader.ReadFloat("MAIN", "FEScale", 1.0f);
            FEScale::fCalcFEScale = FEScale::fFEScale;
            FEScale::fFMVScale = iniReader.ReadFloat("MAIN", "FMVScale", 1.0f);
            FEScale::fCalcFMVScale = FEScale::fFMVScale;
            FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 0) != 0;
            FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;

            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? 84 C0 75 ? BE");
            eSetCurrentRenderTarget = (decltype(eSetCurrentRenderTarget))injector::GetBranchDestination(pattern.get_first()).as_int();

            //HUD
            if (bFixHUD)
            {
                static GameRef<float> fHudScaleX;
                fHudScaleX.SetAddress(*hook::pattern("D9 05 ? ? ? ? 89 7C 24 28 C7 44 24 2C 00 00 00 3F D8 C9 C7 44 24 30 00 00 00 3F").count(1).get(0).get<float*>(2));
                injector::UnprotectMemory(fHudScaleX.get_ptr(), sizeof(float));

                static std::vector<GameRef<float>> fHudPosXArray;

                auto setHudPosRef = [&](hook::pattern pat, int offset)
                {
                    GameRef<float> ref;
                    ref.SetAddress(pat.count(1).get(0).get<float>(offset));
                    injector::UnprotectMemory(ref.get_ptr(), sizeof(float));
                    fHudPosXArray.push_back(std::move(ref));
                };

                setHudPosRef(hook::pattern("C7 84 24 A0 00 00 00 00 00 A0 43 C7 84 24 A4 00 00 00 00 00 70 43 C7 84 24 A8 00 00 00 00 00 00 00 0F B7 48 20"), 7);
                setHudPosRef(hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 4C 24 70"), 4);
                setHudPosRef(hook::pattern("C7 84 24 84 00 00 00 00 00 A0 43 C7 84 24 88 00 00 00 00 00 70 43 C7 84 24 8C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 80 00 00 00"), 7);
                setHudPosRef(hook::pattern("C7 84 24 94 00 00 00 00 00 A0 43 C7 84 24 98 00 00 00 00 00 70 43 C7 84 24 9C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 90 00 00 00"), 7);
                setHudPosRef(hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 54 24 70 52"), 4);
                setHudPosRef(hook::pattern("C7 05 ? ? ? ? 00 00 A0 43 C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 82"), 6);
                setHudPosRef(hook::pattern("C7 84 24 A0 01 00 00 00 00 A0 43 C7 84 24 A4 01 00 00 00 00 70 43 C7 84 24 A8 01 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 30 01 00 00 51 8B F0"), 7);

                GameRef<float> temp_ref;
                temp_ref.SetAddress(*hook::pattern("D8 25 ? ? ? ? 8B 4C 24 28 6A 00 8D 54 24 48 D9 5C 24 48 89 4C 24 50 D9 44 24 28 8B 0B").count(1).get(0).get<float*>(2));
                injector::UnprotectMemory(temp_ref.get_ptr(), sizeof(float));
                fHudPosXArray.push_back(std::move(temp_ref));

                static GameRef<float> fHudPosX_x2;
                fHudPosX_x2.SetAddress(*hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? D9 44 24 ? D8 74 24").count(1).get(0).get<float*>(2));
                injector::UnprotectMemory(fHudPosX_x2.get_ptr(), sizeof(float));

                auto pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 33 D2");

                static GameRef<float> fRearviewMirrorLeftX1;
                fRearviewMirrorLeftX1.SetAddress(pattern.get_first<float>(4));
                injector::UnprotectMemory(fRearviewMirrorLeftX1.get_ptr(), sizeof(float));

                static GameRef<float> fRearviewMirrorLeftX2;
                fRearviewMirrorLeftX2.SetAddress(pattern.get_first<float>(4 + 16));
                injector::UnprotectMemory(fRearviewMirrorLeftX2.get_ptr(), sizeof(float));

                static GameRef<float> fRearviewMirrorRightX1;
                fRearviewMirrorRightX1.SetAddress(pattern.get_first<float>(4 - 16));
                injector::UnprotectMemory(fRearviewMirrorRightX1.get_ptr(), sizeof(float));

                static GameRef<float> fRearviewMirrorRightX2;
                fRearviewMirrorRightX2.SetAddress(pattern.get_first<float>(4 + 32));
                injector::UnprotectMemory(fRearviewMirrorRightX2.get_ptr(), sizeof(float));

                pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 83 C4 ? 5F");

                static GameRef<float> fCutsceneBordersWidth1;
                fCutsceneBordersWidth1.SetAddress(pattern.get_first<float>(4));
                injector::UnprotectMemory(fCutsceneBordersWidth1.get_ptr(), sizeof(float));

                static GameRef<float> fCutsceneBordersWidth2;
                fCutsceneBordersWidth2.SetAddress(pattern.get_first<float>(4 + 16));
                injector::UnprotectMemory(fCutsceneBordersWidth2.get_ptr(), sizeof(float));

                pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? BA");

                static GameRef<float> fCutsceneBordersWidth3;
                fCutsceneBordersWidth3.SetAddress(pattern.get_first<float>(4));
                injector::UnprotectMemory(fCutsceneBordersWidth3.get_ptr(), sizeof(float));

                static GameRef<float> fCutsceneBordersWidth4;
                fCutsceneBordersWidth4.SetAddress(pattern.get_first<float>(4 + 24));
                injector::UnprotectMemory(fCutsceneBordersWidth4.get_ptr(), sizeof(float));

                pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 7D");

                static GameRef<float> fRadarMaskWidth1;
                fRadarMaskWidth1.SetAddress(pattern.get_first<float>(4));
                injector::UnprotectMemory(fRadarMaskWidth1.get_ptr(), sizeof(float));

                static GameRef<float> fRadarMaskWidth2;
                fRadarMaskWidth2.SetAddress(pattern.get_first<float>(4 + 16));
                injector::UnprotectMemory(fRadarMaskWidth2.get_ptr(), sizeof(float));

                static GameRef<float> fRadarMaskHeight1;
                fRadarMaskHeight1.SetAddress(pattern.get_first<float>(4 + 24));
                injector::UnprotectMemory(fRadarMaskHeight1.get_ptr(), sizeof(float));

                static GameRef<float> fRadarMaskHeight2;
                fRadarMaskHeight2.SetAddress(pattern.get_first<float>(4 + 40));
                injector::UnprotectMemory(fRadarMaskHeight2.get_ptr(), sizeof(float));

                //graph of the dyno result issue #366 / #462
                pattern = hook::pattern("6A ? 68 ? ? ? ? 68 ? ? ? ? 8B C8 E8 ? ? ? ? EB ? 33 C0 8B CF");

                static GameRef<std::array<float, 2>> fDynoResultOffset;
                fDynoResultOffset.SetAddress(pattern.get_first<std::array<float, 2>>(8));
                injector::UnprotectMemory(fDynoResultOffset.get_ptr(), sizeof(std::array<float, 2>));

                //controls screenshot aspect ratio issue #462
                static auto Width43 = 0;
                pattern = hook::pattern("DA 0D ? ? ? ? 55 56 8B F0 A1 ? ? ? ? 99"); //5C30AC
                injector::WriteMemory(pattern.get_first(2), &Width43, true);

                onResChange() += [](int Width, int Height)
                {
                    fHudScaleX = (1.0f / static_cast<float>(Width) * (static_cast<float>(Height) / 480.0f)) * 2.0f;

                    float fHudPosX = 640.0f / (640.0f * fHudScaleX);

                    for (auto& pos : fHudPosXArray)
                    {
                        pos = fHudPosX;
                    }

                    fHudPosX_x2 = fHudPosX * 2.0f;

                    float fHudOffset = ((480.0f * (static_cast<float>(Width) / static_cast<float>(Height))) - 640.0f) / 2.0f;

                    fRearviewMirrorLeftX1 = (fHudPosX - 320.0f) + 200.0f;
                    fRearviewMirrorLeftX2 = (fHudPosX - 320.0f) + 200.0f;
                    fRearviewMirrorRightX1 = (fHudPosX - 320.0f) + 440.0f;
                    fRearviewMirrorRightX2 = (fHudPosX - 320.0f) + 440.0f;

                    fCutsceneBordersWidth1 = bDisableCutsceneBorders ? 0.0f : static_cast<float>(Width);
                    fCutsceneBordersWidth2 = bDisableCutsceneBorders ? 0.0f : static_cast<float>(Width);
                    fCutsceneBordersWidth3 = bDisableCutsceneBorders ? 0.0f : static_cast<float>(Width);
                    fCutsceneBordersWidth4 = bDisableCutsceneBorders ? 0.0f : static_cast<float>(Width);

                    fRadarMaskWidth1 = static_cast<float>(Width) * 2.0f;
                    fRadarMaskWidth2 = static_cast<float>(Width) * 2.0f;
                    fRadarMaskHeight1 = static_cast<float>(Height) * 2.0f;
                    fRadarMaskHeight2 = static_cast<float>(Height) * 2.0f;

                    fDynoResultOffset = { 79.0f + fHudOffset, 60.0f };

                    Width43 = static_cast<uint32_t>(static_cast<float>(Height) * (4.0f / 3.0f));

                    fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Width), static_cast<float>(Height), 640.0f, 480.0f);
                    if (fHudAspectRatioConstraint.has_value())
                    {
                        float value = fHudAspectRatioConstraint.value();
                        if (value < 0.0f || value > (32.0f / 9.0f))
                            fWidescreenHudOffset = value;
                        else
                        {
                            value = ClampHudAspectRatio(value, static_cast<float>(Width) / static_cast<float>(Height));
                            fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Height) * value, static_cast<float>(Height), 640.0f, 480.0f);
                        }
                    }
                };

                LoadDatFile();

                static auto WidescreenHud = [](HudPos& HudPosX, HudPos& HudPosY)
                {
                    auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
                                           [cc = CDatEntry(std::floor(HudPosX.fPos), std::floor(HudPosY.fPos), 0.0f, 0.0f)]
                                           (const CDatEntry& cde) -> bool
                    {
                        return (int(cc.fPosX) == int(cde.fPosX) && int(cc.fPosY) == int(cde.fPosY));
                    });

                    if (it != HudCoords.end())
                    {
                        constexpr auto fBaseOffset = (480.0f * (16.0f / 9.0f) - 640.0f) / 2.0f;

                        if (it->fOffsetX >= 0.0f)
                            HudPosX.fPos += fWidescreenHudOffset + (it->fOffsetX - fBaseOffset);
                        else
                            HudPosX.fPos -= fWidescreenHudOffset - (it->fOffsetX + fBaseOffset);

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

            if ((FEScale::fFEScale != 1.0f) || (FEScale::fFMVScale != 1.0f) || (FEScale::bAutoFitFE) || (FEScale::bAutoFitFMV))
            {
                FEScale::bEnabled = true;

                auto pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8");
                static auto SetMovieFlag = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    FEScale::SetMovieFlag();
                });

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 85 C0 C7 05 ? ? ? ? ? ? ? ? 8B E8");
                static auto UnsetMovieFlag1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    FEScale::UnsetMovieFlag();
                });

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 85 C0 C7 05 ? ? ? ? ? ? ? ? 8B D8");
                static auto UnsetMovieFlag2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    FEScale::UnsetMovieFlag();
                });

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? ? ? 83 C4 ? 6A ? 6A ? 50 FF 91");
                FEScale::hbSetTransformHook.fun = injector::MakeCALL(pattern.get_first(), FEScale::SetTransformHook).get();

                pattern = hook::pattern("A1 ? ? ? ? 8A 48 ? 84 C9 DE F9");
                static auto loc_5369CC = pattern.get_first(14);
                static auto MovieStartHookCrashFix = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
                {
                    if (!regs.eax)
                        *(void**)(regs.esp - 4) = loc_5369CC;
                });

                WFP::onGameProcessEvent() += []()
                {
                    if (FEScale::bEnabled)
                        FEScale::Update();
                };
            }
        };
    }
} Frontend;