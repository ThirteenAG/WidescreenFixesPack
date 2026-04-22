module;

#include <stdafx.h>
#include <d3d9.h>

export module Frontend;

import ComVars;
import Resolution;

namespace XtendedInput
{
    HMODULE mhXtendedInput = NULL;
    float(__cdecl* SetFEScale)(float val) = nullptr;
    bool(__cdecl* GetUseWin32Cursor)() = nullptr;
    bool bLookedForXInput = false;
    bool bFoundXInput = false;
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

    void ScaleMat(D3DMATRIX* cMat)
    {
        if (MoviePlayerInstance)
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

namespace FEngHud
{
    bool bUpdateFEngHud = false;

    struct MinimapOriginal
    {
        float originalX;
    };

    static std::unordered_map<void*, MinimapOriginal> originalMinimapCache;
    static void* s_lastMinimapPtr = nullptr;

    SafetyHookInline shSetWideScreenMode = {};
    void __fastcall SetWideScreenMode(void* _this, void* edx)
    {
        if (bUpdateFEngHud)
        {
            uint8_t* pThis = reinterpret_cast<uint8_t*>(_this);

            uint8_t* pCachedState = pThis + 833;

            bool isWidescreen = *pCachedState;

            constexpr int kEnableHash = 0x62ED04EC;
            constexpr int kDisableHash = 0x53EC068C;

            int messageHash = isWidescreen ? kEnableHash : kDisableHash;
            cFEng::QueuePackageMessage(*cFEng::mInstance, 0, messageHash, *reinterpret_cast<char**>(pThis + 44), 0);

            void* minimap = reinterpret_cast<void*>(*reinterpret_cast<uint32_t*>(pThis + 820));
            if (!minimap)
                return;

            if (minimap != s_lastMinimapPtr)
            {
                originalMinimapCache.clear();
                s_lastMinimapPtr = minimap;
            }

            auto it = originalMinimapCache.find(minimap);
            if (it == originalMinimapCache.end())
            {
                MinimapOriginal orig;
                orig.originalX = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(minimap) + 180);
                originalMinimapCache.emplace(minimap, orig);
                it = originalMinimapCache.find(minimap);
            }

            const float origX = it->second.originalX;
            const float offset = isWidescreen ? -cFEng::fWidescreenHudOffset : 0.0f;
            const float newX = origX + offset;

            *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(minimap) + 180) = newX;

            {
                uint32_t child144 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(minimap) + 144);
                if (child144)
                {
                    uintptr_t dataPtr = *reinterpret_cast<uintptr_t*>(child144 + 44);
                    *reinterpret_cast<float*>(dataPtr + 28) = newX;
                }
            }

            {
                uint32_t child140 = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(minimap) + 140);
                if (child140)
                {
                    uintptr_t dataPtr = *reinterpret_cast<uintptr_t*>(child140 + 44);
                    *reinterpret_cast<uint32_t*>(dataPtr + 28) = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(minimap) + 180);
                }
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

            CallbackHandler::RegisterCallback(L"NFS_XtendedInput.asi", []()
            {
                XtendedInput::mhXtendedInput = GetModuleHandleA("NFS_XtendedInput.asi");
                if (XtendedInput::mhXtendedInput)
                {
                    XtendedInput::SetFEScale = reinterpret_cast<decltype(XtendedInput::SetFEScale)>(GetProcAddress(XtendedInput::mhXtendedInput, "SetFEScale"));
                    XtendedInput::GetUseWin32Cursor = reinterpret_cast<decltype(XtendedInput::GetUseWin32Cursor)>(GetProcAddress(XtendedInput::mhXtendedInput, "GetUseWin32Cursor"));
                    XtendedInput::bFoundXInput = (XtendedInput::SetFEScale != nullptr) && (XtendedInput::GetUseWin32Cursor != nullptr);
                }
            });

            //HUD
            if (bFixHUD)
            {
                static ProtectedGameRef<float> fHudScaleX;
                fHudScaleX.SetAddress(*hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04 D8 0D ? ? ? ? D8 25 ? ? ? ? D9 E0").count(1).get(0).get<float*>(2));

                static ProtectedGameRef<float> fWorldMapHudPosX;
                fWorldMapHudPosX.SetAddress(*hook::pattern("D8 05 ? ? ? ? 89 44 24 18 D9 44 24 18").count(1).get(0).get<float*>(2));

                static ProtectedGameRef<float> fFEHudPosX;
                fFEHudPosX.SetAddress(*hook::pattern("D8 25 ? ? ? ? 8D 46 34 50 D9 5C 24 20").count(1).get(0).get<float*>(2));

                static ProtectedGameRef<float> fRenderRaceRouteHudPosX;
                fRenderRaceRouteHudPosX.SetAddress(*hook::pattern("D8 05 ? ? ? ? D9 5C 24 7C D9 86 B8 00 00 00 D8 05 ? ? ? ? D9 9C 24 80 00 00 00").count(1).get(0).get<float*>(2));

                static ProtectedGameRef<float> fPiPUCAPPosition;
                fPiPUCAPPosition.SetAddress(hook::pattern("68 ? ? ? ? 52 E8 ? ? ? ? A1 ? ? ? ? ? ? 83 C4").count(1).get(0).get<float>(1));

                static std::vector<ProtectedGameRef<float>> hudPosXRefs;

                for (size_t i = 0; i < 4; i++)
                {
                    float* ptr = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43").count(4).get(i).get<float>(7);

                    ProtectedGameRef<float> fTemp;
                    fTemp.SetAddress(ptr);
                    hudPosXRefs.push_back(std::move(fTemp));
                }

                for (size_t i = 0; i < 2; i++)
                {
                    float* ptr = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").count(2).get(i).get<float>(4);
                    ProtectedGameRef<float> fTemp;
                    fTemp.SetAddress(ptr);
                    hudPosXRefs.push_back(std::move(fTemp));
                }

                onResChange() += [](int Width, int Height)
                {
                    fHudScaleX = (1.0f / static_cast<float>(Width) * (static_cast<float>(Height) / 480.0f)) * 2.0f;

                    float fHudPosX = 640.0f / (640.0f * fHudScaleX);
                    fWorldMapHudPosX = fHudPosX;
                    fFEHudPosX = fHudPosX;
                    fRenderRaceRouteHudPosX = (fHudPosX - 320.0f) + 384.0f; //mini_map_route fix

                    for (auto& ref : hudPosXRefs)
                        ref = fHudPosX;

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

                    fPiPUCAPPosition = ((((320.0f + cFEng::fWidescreenHudOffset) / 320.0f) - 1.042f) / -2.0f);

                    FEngHud::bUpdateFEngHud = true;
                };

                // Widescreen HUD
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

                auto pattern = hook::pattern("D9 5E 4C 89 46 5C 5E 5B C3");
                struct MirrorFix
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        float fMirrorScaling = 1.0f * ((4.0f / 3.0f) / GetAspectRatio());
                        *(float*)(regs.esi + 0x00) = *(float*)(regs.esi + 0x00) * fMirrorScaling;
                        *(float*)(regs.esi + 0x18) = *(float*)(regs.esi + 0x18) * fMirrorScaling;
                        *(float*)(regs.esi + 0x30) = *(float*)(regs.esi + 0x30) * fMirrorScaling;
                        *(float*)(regs.esi + 0x48) = *(float*)(regs.esi + 0x48) * fMirrorScaling;
                    }
                }; injector::MakeInline<MirrorFix>(pattern.get_first(6)); //750C23
                injector::WriteMemory(pattern.get_first(6 + 5), 0x90C35B5E, true); //pop esi pop ebx ret

                pattern = hook::pattern("E8 ? ? ? ? 8B 46 ? 55 50 E8 ? ? ? ? 8A 44 24");
                FE::Object::SetCenter = (decltype(FE::Object::SetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? 8B 54 24 ? 8B 44 24 ? 52 50 56 E8 ? ? ? ? 83 C4 ? 5E C2 ? ? 8B 51");
                FE::Object::GetCenter = (decltype(FE::Object::GetCenter))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 50 57 68 ? ? ? ? 8B CD");
                FE::Object::FindObject = (decltype(FE::Object::FindObject))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? 8B B6 ? ? ? ? 85 F6 0F 84 ? ? ? ? ? ? ? ? ? ? 8B 86");
                cFEng::mInstance = *pattern.get_first<void**>(-9);
                cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(pattern.get_first(), cFEng::QueuePackageMessage, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 8B B6 ? ? ? ? 85 F6 74 ? ? ? ? ? ? ? 8B 86");
                cFEng::hbQueuePackageMessage.fun = injector::MakeCALL(pattern.get_first(), cFEng::QueuePackageMessage, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 8B 8E ? ? ? ? 41 0B FB");
                FEngHud::shSetWideScreenMode = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), FEngHud::SetWideScreenMode);

                pattern = hook::pattern("D8 25 ? ? ? ? D9 96 ? ? ? ? 74");
                injector::WriteMemory(pattern.get_first(2), &cFEng::fWidescreenHudOffset, true);

                pattern = hook::pattern("D8 05 ? ? ? ? D9 96 ? ? ? ? 74");
                injector::WriteMemory(pattern.get_first(2), &cFEng::fWidescreenHudOffset, true);

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
                pattern = hook::pattern("89 46 ? 89 4E ? 8B 0D ? ? ? ? 50");
                static auto ELoadingScreenOn = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bInLoadingScreen = true;
                });

                pattern = hook::pattern("89 46 ? 8B 0D ? ? ? ? 50");
                static auto ELoadingScreenOff = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bInLoadingScreen = false;
                });

                pattern = hook::pattern("88 5E ? 89 5E ? E8 ? ? ? ? 83 C4 ? E8");
                static auto SplashScreenCtor = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bInLoadingScreen = true;
                });

                pattern = hook::pattern("8B 46 ? 85 C0 C7 44 24 ? ? ? ? ? 74 ? 89 44 24");
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

                auto loc_730E4D = hook::pattern("6A ? 53 8D 54 24 ? 52 FF 50");
                injector::MakeInline<FEScale::FEScaleHook1>(loc_730E4D.get_first(), loc_730E4D.get_first(0xB));

                auto loc_730ECE = hook::pattern("57 53 8D 44 24 ? 50 8B CE FF 52");
                injector::MakeInline<FEScale::FEScaleHook2>(loc_730ECE.get_first(), loc_730ECE.get_first(0xC));
            }

            //Mouse cursor
            //uint32_t* dword_9E8F84 = *hook::pattern("D8 0D ? ? ? ? DA 74 24 10 DA 64 24 14 E8 ? ? ? ? 89 46 08").count(1).get(0).get<uint32_t*>(2);
            //injector::WriteMemory<float>(dword_9E8F84, (float)Screen.Height, true);

            //Autosave icon
            {
                uint32_t* dword_5CD4B0 = *hook::pattern("FF 24 85 ? ? ? ? 8B 0D ? ? ? ? 8B 11 6A 00 FF 92 98 00 00 00").count(1).get(0).get<uint32_t*>(3);
                injector::WriteMemory(dword_5CD4B0 + 2, hook::pattern("8B CE E8 ? ? ? ? 8B 0D ? ? ? ? 8B F1 E8").count(1).get(0).get<uint32_t*>(0), true);
            }

            //EA HD
            {
                uint32_t* dword_9D51D8 = *hook::pattern("6A 01 6A 14 68 ? ? ? ? 8B CE").count(1).get(0).get<uint32_t*>(5);
                uint32_t* dword_5BB83B = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 85 C0").count(5).get(4).get<uint32_t>(1);
                injector::WriteMemory(dword_5BB83B, dword_9D51D8, true);
                auto pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 5E 59 C3");
                uint32_t* dword_5BD4A3 = pattern.count(3).get(1).get<uint32_t>(1);
                injector::WriteMemory(dword_5BD4A3, dword_9D51D8, true);
                uint32_t* dword_5BD4B0 = pattern.count(3).get(2).get<uint32_t>(1);
                injector::WriteMemory(dword_5BD4B0, dword_9D51D8, true);
                injector::WriteMemory(hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B 4E 0C 50 51").count(3).get(2).get<uint32_t>(1), 0, true);
            }

            //World map cursor
            {
                uint32_t* dword_570DCD = hook::pattern("75 33 D9 44 24 14 D8 5C 24 08 DF E0 F6 C4 41").count(1).get(0).get<uint32_t>(0);
                injector::MakeNOP(dword_570DCD, 2, true);
                uint32_t* dword_570DDC = hook::pattern("7A 24 D9 44 24 18 D8 5C 24 04 DF E0").count(1).get(0).get<uint32_t>(0);
                injector::MakeNOP(dword_570DDC, 2, true);
            }
        };
    }
} Frontend;