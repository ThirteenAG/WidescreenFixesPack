module;

#include <stdafx.h>
#include <d3d9.h>

export module Frontend;

import ComVars;
import Resolution;
import Compat;

export namespace FEScale
{
    float fFEScale = 1.0f;
    float fCalcFEScale = 1.0f;
    float fFMVScale = 1.0f;
    float fCalcFMVScale = 1.0f;
    bool bEnabled = false;
    bool bAutoFitFE = true;
    bool bAutoFitFMV = true;

    injector::hook_back<void(__cdecl*)(D3DMATRIX*, uint32_t)> hbSetTransformHook;
    void __cdecl SetTransformHook(D3DMATRIX* mat, uint32_t EVIEW_ID)
    {
        D3DMATRIX cMat;
        memcpy(&cMat, mat, sizeof(D3DMATRIX));

        if (MoviePlayerInstance)
        {
            cMat._11 *= fCalcFMVScale;
            cMat._22 *= fCalcFMVScale;
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

        if (!XtendedInput::bFoundXInput)
            return;
        if (!XtendedInput::GetUseWin32Cursor())
            return;
        if (XtendedInput::SetFEScale)
            XtendedInput::SetFEScale(fCalcFEScale);
    }
}

namespace FE
{
    namespace Object
    {
        void  (__cdecl* SetCenter)(void* FEObject, float X, float Y) = nullptr;
        void  (__cdecl* GetCenter)(void* FEObject, float* X, float* Y) = nullptr;
        void* (__cdecl* FindObject)(const char* pkg_name, int obj_hash) = nullptr;
    }
}

namespace cFEng
{
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset = 120.0f;

    std::unordered_map<void*, std::pair<float, float>> originalCenterCache;

    void** mInstance = nullptr;

    static void* s_currentLeftGroup = nullptr;
    static void* s_currentRightGroup = nullptr;

    injector::hook_back<void(__fastcall*)(void*, void*, int, char const*, int)> hbQueuePackageMessage;

    void __fastcall QueuePackageMessage(void* instance, void* edx, int MessageHash, char const* packageName, int a4)
    {
        hbQueuePackageMessage.fun(instance, edx, MessageHash, packageName, a4);

        void* leftGroup = FE::Object::FindObject(packageName, 0x1603009E);
        void* rightGroup = FE::Object::FindObject(packageName, 0x5D0101F1);

        if (leftGroup && leftGroup != s_currentLeftGroup)
        {
            originalCenterCache.erase(s_currentLeftGroup);
            s_currentLeftGroup = leftGroup;
        }

        if (rightGroup && rightGroup != s_currentRightGroup)
        {
            originalCenterCache.erase(s_currentRightGroup);
            s_currentRightGroup = rightGroup;
        }

        if (leftGroup)
        {
            auto it = originalCenterCache.find(leftGroup);
            if (it == originalCenterCache.end())
            {
                float x = 0.0f, y = 0.0f;
                FE::Object::GetCenter(leftGroup, &x, &y);
                originalCenterCache[leftGroup] = { x, y };
                it = originalCenterCache.find(leftGroup);
            }

            const auto& [origX, origY] = it->second;
            FE::Object::SetCenter(leftGroup, origX - fWidescreenHudOffset, origY);
        }

        if (rightGroup)
        {
            auto it = originalCenterCache.find(rightGroup);
            if (it == originalCenterCache.end())
            {
                float x = 0.0f, y = 0.0f;
                FE::Object::GetCenter(rightGroup, &x, &y);
                originalCenterCache[rightGroup] = { x, y };
                it = originalCenterCache.find(rightGroup);
            }

            const auto& [origX, origY] = it->second;
            FE::Object::SetCenter(rightGroup, origX + fWidescreenHudOffset, origY);
        }
    }
}

namespace Minimap
{
    GameRef<float> MinimapPivotX;
    GameRef<float> MinimapDispX;

    struct OriginalPositions
    {
        float localX;
        float childX[5];
    };

    std::unordered_map<void*, OriginalPositions> originalCache;

    SafetyHookInline shAdjustForWidescreen = {};

    void __fastcall AdjustForWidescreen(void* _this, void* edx, char isWidescreen)
    {
        const float offset = isWidescreen ? -cFEng::fWidescreenHudOffset : cFEng::fWidescreenHudOffset;

        MinimapPivotX = isWidescreen ? -cFEng::fWidescreenHudOffset : 0.0f;
        MinimapDispX = isWidescreen ? -0.9375f : 0.9375f;

        if (!originalCache.empty() && originalCache.begin()->first != _this)
        {
            originalCache.clear();
        }

        auto it = originalCache.find(_this);
        if (it == originalCache.end())
        {
            OriginalPositions orig;

            orig.localX = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(_this) + 184);

            constexpr int kChildOffsets[5] = { 60, 64, 68, 72, 144 };
            for (int i = 0; i < 5; ++i)
            {
                uintptr_t childPtr = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_this) + kChildOffsets[i]);
                uintptr_t dataPtr = *reinterpret_cast<uint32_t*>(childPtr + 44);
                orig.childX[i] = *reinterpret_cast<float*>(dataPtr + 28);
            }

            it = originalCache.emplace(_this, orig).first;
        }

        const auto& orig = it->second;

        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(_this) + 184) = orig.localX + offset;

        constexpr int kChildOffsets[5] = { 60, 64, 68, 72, 144 };

        for (int i = 0; i < 5; ++i)
        {
            uintptr_t childPtr = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_this) + kChildOffsets[i]);
            uintptr_t dataPtr = *reinterpret_cast<uint32_t*>(childPtr + 44);

            float* pChildX = reinterpret_cast<float*>(dataPtr + 28);
            *pChildX = orig.childX[i] + offset;
        }

        {
            uintptr_t childPtr = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_this) + 144);
            uintptr_t dataPtr = *reinterpret_cast<uint32_t*>(childPtr + 44);

            *reinterpret_cast<uint32_t*>(dataPtr + 32) = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(_this) + 188);
        }
    }
}

namespace FEngHud
{
    bool bUpdateFEngHud = false;

    SafetyHookInline shSetWideScreenMode = {};
    void __fastcall SetWideScreenMode(void* _this, void* edx)
    {
        if (bUpdateFEngHud)
        {
            if (*(uint8_t*)((uintptr_t)_this + 816))
            {
                cFEng::QueuePackageMessage(*cFEng::mInstance, 0, 0x62ED04EC, *(char**)((uintptr_t)_this + 36), 0);
                if (*(void**)((uintptr_t)_this + 800))
                    Minimap::AdjustForWidescreen(*(void**)((uintptr_t)_this + 800), 0, 1);
            }
            else
            {
                cFEng::QueuePackageMessage(*cFEng::mInstance, 0, 0x53EC068C, *(char**)((uintptr_t)_this + 36), 0);
                if (*(void**)((uintptr_t)_this + 800))
                    Minimap::AdjustForWidescreen(*(void**)((uintptr_t)_this + 800), 0, 0);
            }
            bUpdateFEngHud = false;
        }

        return shSetWideScreenMode.unsafe_fastcall(_this, edx);
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
            FEScale::bAutoFitFE = iniReader.ReadInteger("MAIN", "AutoFitFE", 1) != 0;
            FEScale::bAutoFitFMV = iniReader.ReadInteger("MAIN", "AutoFitFMV", 1) != 0;
            cFEng::fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));

            WFP::onGameProcessEvent() += []()
            {
                if (FEScale::bEnabled)
                    FEScale::Update();
            };

            //HUD
            if (bFixHUD)
            {
                static GameRef<float> fHudScaleX;
                fHudScaleX.SetAddress(*hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04").count(1).get(0).get<float*>(2));
                injector::UnprotectMemory(fHudScaleX.get_ptr(), sizeof(float));

                static GameRef<float> fFEHudPosX;
                fFEHudPosX.SetAddress(*hook::pattern("D8 25 ? ? ? ? D9 5C 24 14 DB 05 ? ? ? ? D8 25 ? ? ? ? D9 5C 24 1C 74 20").count(1).get(0).get<float*>(2));
                injector::UnprotectMemory(fFEHudPosX.get_ptr(), sizeof(float));

                auto pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x56FED4
                struct HudPosXHook1
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0x19C) = fFEHudPosX;
                    }
                }; injector::MakeInline<HudPosXHook1>(pattern.get_first(0), pattern.get_first(11));

                pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x584EEF
                struct HudPosXHook2
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0x90) = fFEHudPosX;
                    }
                }; injector::MakeInline<HudPosXHook2>(pattern.get_first(0), pattern.get_first(11));

                pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x599E79
                struct HudPosXHook3
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0x84) = fFEHudPosX;
                    }
                }; injector::MakeInline<HudPosXHook3>(pattern.get_first(0), pattern.get_first(11));

                pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A120
                struct HudPosXHook4
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0xC4) = fFEHudPosX;
                    }
                }; injector::MakeInline<HudPosXHook4>(pattern.get_first(0), pattern.get_first(11));

                pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A5AB
                struct HudPosXHook5
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0x94) = fFEHudPosX;
                    }
                }; injector::MakeInline<HudPosXHook5>(pattern.get_first(0), pattern.get_first(11));

                pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A83E
                injector::MakeInline<HudPosXHook5>(pattern.get_first(0), pattern.get_first(11));

                pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43"); // 0x005A44C8
                struct HudPosXHook6
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0x64) = fFEHudPosX;
                    }
                }; injector::MakeInline<HudPosXHook6>(pattern.get_first(0), pattern.get_first(8));

                //mirror position fix
                pattern = hook::pattern("C7 44 24 70 00 00 E1 43 C7 44 24 74 00 00 98 41 C7 84 24 80 00 00 00 00 00 3E 43"); // 0x6E70C0
                struct MirrorPosXHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esp + 0x70) = (fFEHudPosX - 320.0f) + 450.0f;
                        *(float*)(regs.esp + 0xA0) = (fFEHudPosX - 320.0f) + 450.0f;
                        *(float*)(regs.esp + 0x80) = (fFEHudPosX - 320.0f) + 190.0f;
                        *(float*)(regs.esp + 0x90) = (fFEHudPosX - 320.0f) + 190.0f;
                        // others
                        *(float*)(regs.esp + 0x74) = 19.0;
                        *(float*)(regs.esp + 0x84) = 19.0;
                        *(float*)(regs.esp + 0x94) = 89.0;
                        *(float*)(regs.esp + 0xA4) = 89.0;
                    }
                }; injector::MakeInline<MirrorPosXHook>(pattern.get_first(0), pattern.get_first(82));

                uint32_t* dword_57CB82 = hook::pattern("3A 55 34 0F 85 0B 02 00 00 A1").count(1).get(0).get<uint32_t>(0); // HUD
                uint32_t* dword_5696CB = hook::pattern("8A 41 34 38 86 30 03 00 00 74 52 84 C0").count(1).get(0).get<uint32_t>(0); // HUD
                uint32_t* dword_58D883 = hook::pattern("8A 40 34 5F 5E 5D 3B CB 5B 75 12").count(1).get(0).get<uint32_t>(0); // EA Trax
                uint32_t* dword_56885A = hook::pattern("38 48 34 74 31 8B 4E 38 68 7E 78 8E 90").count(1).get(0).get<uint32_t>(0); // Wrong Way Sign

                injector::WriteMemory<uint32_t>(dword_57CB82, 0x0F01FA80, true);
                injector::WriteMemory<uint32_t>(dword_5696CB, 0x389001B0, true);
                injector::WriteMemory<uint32_t>(dword_58D883, 0x5F9001B0, true);
                injector::WriteMemory<uint32_t>(dword_56885A, 0x7401F980, true);

                //Widescreen Splash
                pattern = hook::pattern("8B 46 10 8B 3D ? ? ? ? 53 50");
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

                pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? EB ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 8B 41");
                Minimap::MinimapPivotX.SetAddress(*pattern.get_first<float*>(2));
                Minimap::MinimapDispX.SetAddress(*pattern.get_first<float*>(12));

                onResChange() += [](int Width, int Height)
                {
                    fHudScaleX = (1.0f / static_cast<float>(Width) * (static_cast<float>(Height) / 480.0f)) * 2.0f;
                    float fHudPosX = 640.0f / (640.0f * fHudScaleX);
                    fFEHudPosX = fHudPosX;

                    cFEng::fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Width), static_cast<float>(Height), 640.0f, 480.0f);
                    if (cFEng::fHudAspectRatioConstraint.has_value())
                    {
                        float value = cFEng::fHudAspectRatioConstraint.value();
                        if (value < 0.0f || value > (32.0f / 9.0f))
                            cFEng::fWidescreenHudOffset = value;
                        else
                        {
                            value = ClampHudAspectRatio(value, static_cast<float>(Width) / static_cast<float>(Height));
                            cFEng::fWidescreenHudOffset = -CalculateWidescreenOffset(static_cast<float>(Height) * value, static_cast<float>(Height), 640.0f, 480.0f);
                        }
                    }

                    FEngHud::bUpdateFEngHud = true;
                };

                pattern = hook::pattern("E8 ? ? ? ? 8B 56 ? 8B 46 ? 52 50 E8 ? ? ? ? 83 C4 ? 8B C6");
                FE::Object::SetCenter = (decltype(FE::Object::SetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? ? ? ? ? 8B 97 ? ? ? ? 8D 44 24 ? 50 8D 4C 24 ? ? ? ? ? 51 52 E8");
                FE::Object::GetCenter = (decltype(FE::Object::GetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 50 56 68 ? ? ? ? 8B CF E8 ? ? ? ? 5F 5E 5B C2");
                FE::Object::FindObject = (decltype(FE::Object::FindObject))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("8B 0D ? ? ? ? 83 C4 ? 50 E8 ? ? ? ? 8B 4C 24 ? 64 89 0D");
                cFEng::mInstance = *pattern.get_first<void**>(2);

                pattern = hook::pattern("E8 ? ? ? ? 0B FB 0F 84 ? ? ? ? 8B 8E ? ? ? ? 85 C9");
                FEngHud::shSetWideScreenMode = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), FEngHud::SetWideScreenMode);

                pattern = hook::pattern("E8 ? ? ? ? 5E C3 8B 46 ? 50");
                Minimap::shAdjustForWidescreen = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), Minimap::AdjustForWidescreen);

                pattern = hook::pattern("E8 ? ? ? ? 8B 8E ? ? ? ? 85 C9 74 ? 6A ? E8 ? ? ? ? 5E C3");
                cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(pattern.count(2).get(0).get<void*>(0), cFEng::QueuePackageMessage, true).get();
                cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(pattern.count(2).get(1).get<void*>(0), cFEng::QueuePackageMessage, true).get();

                static auto DrawPillarboxes = []()
                {
                    auto [Width, Height] = GetRes();
                    float target_aspect = 16.0f / 9.0f;
                    float video_width = static_cast<float>(Height) * target_aspect;
                    float pillar = (static_cast<float>(Width) - video_width) / 2.0f;
                    if (pillar > 0.0f)
                    {
                        D3DRECT rects[2];
                        rects[0].x1 = 0;
                        rects[0].y1 = 0;
                        rects[0].x2 = static_cast<LONG>(pillar);
                        rects[0].y2 = Height;
                        rects[1].x1 = Width - static_cast<LONG>(pillar);
                        rects[1].y1 = 0;
                        rects[1].x2 = Width;
                        rects[1].y2 = Height;
                        Direct3DDevice->Clear(2, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                    }
                };

                static bool bInLoadingScreen = false;
                pattern = hook::pattern("89 47 ? 89 44 24");
                static auto ELoadingScreenOn = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bInLoadingScreen = true;
                });

                pattern = hook::pattern("C7 46 ? ? ? ? ? ? ? ? ? ? ? 8B 3D");
                static auto ELoadingScreenOff = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bInLoadingScreen = false;
                });

                pattern = hook::pattern("88 5E ? 89 5E ? 89 9C 24");
                static auto SplashScreenCtor = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bInLoadingScreen = true;
                });

                pattern = hook::pattern("8B F8 8D 44 24 ? 50 E8 ? ? ? ? 8B 4C 24 ? 89 4C 24 ? 83 C4 ? 8D 4C 24 ? C7 44 24 ? ? ? ? ? 89 5C 24");
                static auto SplashScreenDtor = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bInLoadingScreen = false;
                });

                WFP::onEndScene() += []()
                {
                    if (MoviePlayerInstance || bInLoadingScreen)
                    {
                        DrawPillarboxes();
                    }
                };
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

                auto loc_6E6FB7 = hook::pattern("E8 ? ? ? ? ? ? 83 C4 ? 8B CB FF 50 ? 8B 43");
                FEScale::hbSetTransformHook.fun = injector::MakeCALL(loc_6E6FB7.get_first(), FEScale::SetTransformHook).get();

                auto loc_6E7011 = hook::pattern("E8 ? ? ? ? ? ? 83 C4 ? 8B CE FF 52 ? 8B 76");
                FEScale::hbSetTransformHook.fun = injector::MakeCALL(loc_6E7011.get_first(), FEScale::SetTransformHook).get();
            }
        };
    }
} Frontend;