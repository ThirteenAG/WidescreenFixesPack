module;

#include <stdafx.h>
#include <d3d9.h>
#include <usercall.hpp>

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

    injector::hook_back<void(__cdecl*)(uint32_t, D3DMATRIX*)> hbSetTransformHook;
    void __cdecl SetTransformHook(uint32_t EVIEW_ID, D3DMATRIX* mat)
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

        return usercall::Call<usercall::reg::edi, usercall::reg::stack>(hbSetTransformHook.fun, EVIEW_ID, &cMat);
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

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
            int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
            FEScale::fFEScale = iniReader.ReadFloat("MAIN", "FEScale", 1.0f);
            FEScale::fCalcFEScale = FEScale::fFEScale;
            FEScale::fFMVScale = iniReader.ReadFloat("MAIN", "FMVScale", 1.0f);
            FEScale::fCalcFMVScale = FEScale::fFMVScale;
            FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 0) != 0;
            FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;

            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? A1 ? ? ? ? 33 F6");
            if (!pattern.empty())
                eSetCurrentRenderTarget = (decltype(eSetCurrentRenderTarget))injector::GetBranchDestination(pattern.get_first()).as_int();
            else
            {
                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? 56 57 E8");
                eSetCurrentRenderTarget = (decltype(eSetCurrentRenderTarget))pattern.get_first();
            }

            //HUD
            if (bFixHUD)
            {
                static GameRef<float> fHudScaleX;
                fHudScaleX.SetAddress(*hook::pattern("D8 0D ? ? ? ? 57 0F B6 B8").count(1).get(0).get<float*>(2));
                injector::UnprotectMemory(fHudScaleX.get_ptr(), sizeof(float));

                static std::vector<GameRef<float>> fHudPosXArray;

                auto pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
                pattern.for_each_result([](auto match)
                {
                    GameRef<float> ref;
                    ref.SetAddress(match.get<float>(7));
                    injector::UnprotectMemory(ref.get_ptr(), sizeof(float));
                    fHudPosXArray.push_back(std::move(ref));
                });

                pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43");
                if (!pattern.empty())
                {
                    pattern.for_each_result([](auto match)
                    {
                        GameRef<float> ref;
                        ref.SetAddress(match.get<float>(4));
                        injector::UnprotectMemory(ref.get_ptr(), sizeof(float));
                        fHudPosXArray.push_back(std::move(ref));
                    });
                }

                pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43");
                if (!pattern.empty())
                {
                    pattern.for_each_result([](auto match)
                    {
                        GameRef<float> ref;
                        ref.SetAddress(match.get<float>(4));
                        injector::UnprotectMemory(ref.get_ptr(), sizeof(float));
                        fHudPosXArray.push_back(std::move(ref));
                    });
                }

                GameRef<float> temp_ref;
                temp_ref.SetAddress(*hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 0F D9 45 FC").count(1).get(0).get<float*>(2));
                injector::UnprotectMemory(temp_ref.get_ptr(), sizeof(float));
                fHudPosXArray.push_back(std::move(temp_ref));

                static GameRef<float> fRearviewMirrorLeftX1;
                static GameRef<float> fRearviewMirrorLeftX2;
                static GameRef<float> fRearviewMirrorRightX1;
                static GameRef<float> fRearviewMirrorRightX2;

                pattern = hook::pattern("C7 44 24 ? 00 00 DC 43 C7 44 24 ? 00 00 70 41");

                if (!pattern.empty())
                {
                    fRearviewMirrorLeftX1.SetAddress(pattern.get_first<float>(20));
                    injector::UnprotectMemory(fRearviewMirrorLeftX1.get_ptr(), sizeof(float));

                    fRearviewMirrorLeftX2.SetAddress(pattern.get_first<float>(42));
                    injector::UnprotectMemory(fRearviewMirrorLeftX2.get_ptr(), sizeof(float));

                    fRearviewMirrorRightX1.SetAddress(pattern.get_first<float>(4));
                    injector::UnprotectMemory(fRearviewMirrorRightX1.get_ptr(), sizeof(float));

                    fRearviewMirrorRightX2.SetAddress(pattern.get_first<float>(64));
                    injector::UnprotectMemory(fRearviewMirrorRightX2.get_ptr(), sizeof(float));
                }
                else
                {
                    pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? C7 84 24 ? ? ? ? ? ? ? ? FF 91");
                    fRearviewMirrorLeftX1.SetAddress(pattern.get_first<float>(26));
                    injector::UnprotectMemory(fRearviewMirrorLeftX1.get_ptr(), sizeof(float));

                    fRearviewMirrorLeftX2.SetAddress(pattern.get_first<float>(48));
                    injector::UnprotectMemory(fRearviewMirrorLeftX2.get_ptr(), sizeof(float));

                    fRearviewMirrorRightX1.SetAddress(pattern.get_first<float>(4));
                    injector::UnprotectMemory(fRearviewMirrorRightX1.get_ptr(), sizeof(float));

                    fRearviewMirrorRightX2.SetAddress(pattern.get_first<float>(70));
                    injector::UnprotectMemory(fRearviewMirrorRightX2.get_ptr(), sizeof(float));
                }

                //mouse cursor fix
                static std::vector<GameRef<int32_t>> fNegativeHudPosXArray;

                pattern = hook::pattern("81 C2 C0 FE FF FF");
                pattern.for_each_result([](auto match)
                {
                    GameRef<int32_t> ref;
                    ref.SetAddress(match.get<int32_t>(2));
                    injector::UnprotectMemory(ref.get_ptr(), sizeof(int32_t));
                    fNegativeHudPosXArray.push_back(std::move(ref));
                });

                pattern = hook::pattern("05 C0 FE FF FF");
                pattern.for_each_result([](auto match)
                {
                    GameRef<int32_t> ref;
                    ref.SetAddress(match.get<int32_t>(1));
                    injector::UnprotectMemory(ref.get_ptr(), sizeof(int32_t));
                    fNegativeHudPosXArray.push_back(std::move(ref));
                });

                GameRef<int32_t> temp_ref2;
                temp_ref2.SetAddress(hook::pattern("8D 88 C0 FE FF FF").get_first<int32_t>(2));
                injector::UnprotectMemory(temp_ref2.get_ptr(), sizeof(int32_t));
                fNegativeHudPosXArray.push_back(std::move(temp_ref2));

                pattern = hook::pattern("8B 06 8B 4C 24 08");
                struct ClientRectHook1
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = *(uint32_t*)(regs.esi + 0x00);
                        regs.ecx = static_cast<uint32_t>(fHudPosXArray.front() + fHudPosXArray.front());
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

                onResChange() += [](int Width, int Height)
                {
                    fHudScaleX = (1.0f / static_cast<float>(Width) * (static_cast<float>(Height) / 480.0f)) * 2.0f;

                    float fHudPosX = 640.0f / (640.0f * fHudScaleX);

                    for (auto& pos : fHudPosXArray)
                    {
                        pos = fHudPosX;
                    }

                    for (auto& pos : fNegativeHudPosXArray)
                    {
                        pos = static_cast<uint32_t>(-fHudPosX);
                    }

                    float fHudOffset = ((480.0f * (static_cast<float>(Width) / static_cast<float>(Height))) - 640.0f) / 2.0f;

                    fRearviewMirrorLeftX1 = (fHudPosX - 320.0f) + 200.0f;
                    fRearviewMirrorLeftX2 = (fHudPosX - 320.0f) + 200.0f;
                    fRearviewMirrorRightX1 = (fHudPosX - 320.0f) + 440.0f;
                    fRearviewMirrorRightX2 = (fHudPosX - 320.0f) + 440.0f;

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

                static std::vector<CDatEntry> HudCoords;

                CIniReader iniReader("");
                auto DataFilePath = iniReader.GetIniPath();
                DataFilePath.replace_extension(".dat");

                LoadDatFile(DataFilePath, [](std::string_view line)
                {
                    int32_t PosX, PosY;
                    float fOffsetX, fOffsetY;
                    sscanf_s(line.data(), "%*s %x %x %f %f %*f %*f", &PosX, &PosY, &fOffsetX, &fOffsetY);
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
                            constexpr auto fBaseOffset = (480.0f * (16.0f / 9.0f) - 640.0f) / 2.0f;

                            if (it->fOffsetX >= 0.0f)
                                HudPosX.fPos += fWidescreenHudOffset + (it->fOffsetX - fBaseOffset);
                            else
                                HudPosX.fPos -= fWidescreenHudOffset - (it->fOffsetX + fBaseOffset);

                            HudPosY.fPos += it->fOffsetY;

                            if (bAddHudOffset)
                            {
                                HudPosX.fPos -= fMinimapPosX;
                                HudPosX.fPos += fHudPosXArray.front();
                            }
                        }
                    }
                };

                pattern = hook::pattern("8B 47 1C 8B 4F 20 89 84 24 10 01 00 00 8B 47 24 89 84 24 18 01 00 00 39 5C 24 04");
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

            if ((FEScale::fFEScale != 1.0f) || (FEScale::fFMVScale != 1.0f) || (FEScale::bAutoFitFE) || (FEScale::bAutoFitFMV))
            {
                FEScale::bEnabled = true;

                auto pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 68");
                static auto SetMovieFlag = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    FEScale::SetMovieFlag();
                });

                pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 8B F1 E8 ? ? ? ? 56 E8 ? ? ? ? A1 ? ? ? ? 50 C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 ? 68");
                static auto UnsetMovieFlag1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    FEScale::UnsetMovieFlag();
                });

                pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 8B F1 E8 ? ? ? ? 56 E8 ? ? ? ? A1 ? ? ? ? 50 C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 ? C7 05");
                static auto UnsetMovieFlag2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    FEScale::UnsetMovieFlag();
                });

                pattern = find_pattern("E8 ? ? ? ? A1 ? ? ? ? ? ? 83 C4 ? 57 6A ? 50 FF 91 ? ? ? ? A1 ? ? ? ? ? ? 57 6A ? 50 FF 92 ? ? ? ? 85 DB", "E8 ? ? ? ? A1 ? ? ? ? ? ? 83 C4 ? 57 6A ? 50 FF 91 ? ? ? ? A1 ? ? ? ? ? ? 57 6A ? 50 FF 92 ? ? ? ? 8B 45");
                FEScale::hbSetTransformHook.fun = usercall::MakeUsercall(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto arg1 = (uint32_t)regs.edi;
                    auto arg2 = *(D3DMATRIX**)(regs.esp + 0x0);
                    FEScale::SetTransformHook(arg1, arg2);
                });

                WFP::onGameProcessEvent() += []()
                {
                    if (FEScale::bEnabled)
                        FEScale::Update();
                };

                pattern = hook::pattern("8B 45 ? 8B 78 ? 8B 55");
                static auto RenderMovieHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    Direct3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                });
            }
        };
    };
} Frontend;