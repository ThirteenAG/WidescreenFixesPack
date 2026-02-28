module;

#include "stdafx.h"

export module Engine;

import ComVars;
import GUI;
import WidescreenHUD;

namespace UGameEngine
{
    SafetyHookInline shTick = {};
    void __fastcall Tick(void* UGameEngine, void* edx, float deltaTime)
    {
        if (Screen.bDeferredInput)
        {
            while (!UWindowsViewport::deferredCauseInputEvent.empty())
            {
                UWindowsViewport::deferredCauseInputEvent.front()();
                UWindowsViewport::deferredCauseInputEvent.pop();
            }
        }

        if (Screen.fRawInputMouse > 0.0f && UWindowsViewport::deferredCauseInputEventForRawInput)
        {
            UWindowsViewport::deferredCauseInputEventForRawInput(228, 4, static_cast<float>(RawInputHandler<>::RawMouseDeltaX));
            UWindowsViewport::deferredCauseInputEventForRawInput(229, 4, static_cast<float>(RawInputHandler<>::RawMouseDeltaY));
            RawInputHandler<>::RawMouseDeltaX = 0;
            RawInputHandler<>::RawMouseDeltaY = 0;
            UWindowsViewport::deferredCauseInputEventForRawInput = nullptr;
        }

        return shTick.unsafe_fastcall(UGameEngine, edx, deltaTime);
    }
}

#if _DEBUG
SafetyHookInline shFindAxisName = {};
float* __fastcall FindAxisName(void* UInput, void* edx, void* AActor, const wchar_t* a3)
{
    auto ret = shFindAxisName.unsafe_fastcall<float*>(UInput, edx, AActor, a3);

    if (std::wstring_view(a3) == L"aMouseX")
    {
        return ret;
    }
    else if (std::wstring_view(a3) == L"aMouseY")
    {
        return ret;
    }

    return ret;
}
#endif

namespace ALight
{
    SafetyHookInline shGetShadowTurnOffRatio = {};
    float __fastcall GetShadowTurnOffRatio(void* aLight, void* edx)
    {
        return 4.0f;
    }
}

namespace AActor
{
    SafetyHookInline shGetShadowTurnOffRatio = {};
    float __fastcall GetShadowTurnOffRatio(void* aActor, void* edx)
    {
        return 4.0f;
    }
}

export void InitEngine()
{
    InitGUI();
    InitWidescreenHUD();

    CIniReader iniReader("");
    bool bSingleCoreAffinity = iniReader.ReadInteger("MAIN", "SingleCoreAffinity", 1) != 0;
    bDisablePreCache = iniReader.ReadInteger("MAIN", "DisablePreCache", 0) != 0;

    #if _DEBUG
    bDisablePreCache = true;
    #endif

    static bool bIsOPSAT = false;
    static bool bIsVideoPlaying = false;
    static auto GIsWideScreen = *hook::module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? F3 0F 59 D9 F3 0F 10 0D ? ? ? ? F3 0F 5C D8 F3 0F 58 DC").get_first<uint8_t*>(2);
    static auto GIsSameFrontBufferOnNormalTV = *hook::module_pattern(GetModuleHandle(L"Engine"), "8B 15 ? ? ? ? 83 3A 00 74 58 F3 0F 10 88").get_first<uint8_t*>(2);
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "83 39 00 75 63 8B 15 ? ? ? ? 83 3A 00"); //
    static auto dword_109E09F0 = *pattern.get_first<float*>(60);
    static auto dword_109E09F4 = dword_109E09F0 + 1;
    static auto dword_109E09F8 = dword_109E09F0 + 2;
    static auto dword_109E09FC = dword_109E09F0 + 3;
    struct ImageUnrealDrawHook
    {
        void operator()(injector::reg_pack& regs)
        {
            //auto x = *(float*)&dword_109E09F0[regs.eax / 4];
            //auto y = *(float*)&dword_109E09F4[regs.eax / 4];
            auto w = *(float*)&dword_109E09F8[regs.eax / 4];
            auto h = *(float*)&dword_109E09FC[regs.eax / 4];

            if (!*GIsWideScreen && *GIsSameFrontBufferOnNormalTV)
            {
                *(float*)&dword_109E09F0[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09F4[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09F8[regs.eax / 4] *= 0.75f;
                *(float*)&dword_109E09FC[regs.eax / 4] *= 0.75f;
            }
            else
            {
                if (bIsOPSAT && bIsVideoPlaying)
                {
                    *(float*)&dword_109E09F0[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09F4[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09F8[regs.eax / 4] *= (4.0f / 3.0f);
                    *(float*)&dword_109E09FC[regs.eax / 4] *= (4.0f / 3.0f);
                }
            }

            if ((w == 1275.0f && h == 637.0f) || (w == 800.0f)) // loadscreen and fullscreen images
            {
                *(float*)&dword_109E09F0[regs.eax / 4] *= Screen.fHudScaleX;
                *(float*)&dword_109E09F8[regs.eax / 4] *= Screen.fHudScaleX;

                if (w == 1275.0f && h == 637.0f)
                {
                    *(float*)&dword_109E09F4[regs.eax / 4] *= Screen.fHudScaleX;
                    *(float*)&dword_109E09FC[regs.eax / 4] *= Screen.fHudScaleX;
                }
            }
        }
    }; injector::MakeInline<ImageUnrealDrawHook>(pattern.get_first(0), pattern.get_first(104));

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "D9 05 ? ? ? ? 83 C4 08 D8 C9 D9 5C 24 24");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "D9 05 ? ? ? ? D8 4C 24 10 D9 5C 24 28");
    injector::WriteMemory(pattern.get_first(2), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 11 44 24 ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? F3 0F 11 4C 24 ? F3 0F 11 44 24");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 10 0D ? ? ? ? F3 0F 10 15 ? ? ? ? 2B D1 F3 0F 2A C2");
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudScaleX2, true);
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C D8 F3 0F 58 DC"); //0x1030503C
    struct HUDPosHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.xmm3.f32[0] -= Screen.fHudOffset;
            regs.xmm3.f32[0] += regs.xmm4.f32[0];
        }
    }; injector::MakeInline<HUDPosHook>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5E CA F3 0F 11 4C 24 14 83 C4 04"); //0x1030678D
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.xmm1.f32[0] /= Screen.fTextScaleX;
        }
    }; injector::MakeInline<TextHook>(pattern.get_first(-4), pattern.get_first(4));

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 83 39 00 89 54 24 20 75 3D 8B 15 ? ? ? ? 83 3A 00");
    struct TextHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            auto f3 = regs.xmm3.f32[0];
            auto f4 = *(float*)(regs.esp + 0x24);
            auto f5 = regs.xmm5.f32[0];
            auto f6 = regs.xmm6.f32[0];

            if (!*GIsWideScreen && *GIsSameFrontBufferOnNormalTV)
            {
                *(float*)(regs.ebp + 0x00) = f3 * 0.75f;
                *(float*)(regs.ebp + 0x04) = f4 * 0.75f;
                *(float*)(regs.ebp + 0x08) = f5 * 0.75f;
                *(float*)(regs.ebp + 0x0C) = f6 * 0.75f;
            }
            else
            {
                if (bIsOPSAT && bIsVideoPlaying)
                {
                    *(float*)(regs.ebp + 0x00) = f3 * (4.0f / 3.0f);
                    *(float*)(regs.ebp + 0x04) = f4 * (4.0f / 3.0f);
                    *(float*)(regs.ebp + 0x08) = f5 * (4.0f / 3.0f);
                    *(float*)(regs.ebp + 0x0C) = f6 * (4.0f / 3.0f);
                }
            }
        }
    }; injector::MakeInline<TextHook2>(pattern.get_first(0), pattern.get_first(76));

    //OPSAT and computer videos (scale text and hud like x360 version)
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 86 ? ? ? ? 83 C4 10 85 C0 74 49 83 78 64 00"); //0x10515086
    struct UCanvasOpenVideoHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static auto getVideoPath = [](const std::string_view& str, char beg, char end) -> std::string
            {
                std::size_t begPos;
                if ((begPos = str.find(beg)) != std::string_view::npos)
                {
                    std::size_t endPos;
                    if ((endPos = str.find(end, begPos)) != std::string_view::npos && endPos != begPos + 1)
                        return std::string(str).substr(begPos + 1, endPos - begPos - 1);
                }

                return std::string();
            };
            bIsVideoPlaying = true;
            regs.eax = *(uint32_t*)(regs.esi + 0x80);
            auto str = getVideoPath(std::string_view(*(char**)(regs.esp + 0x0C)), '\\', '.');
            if (iequals(str, "computer_hq_1") || iequals(str, "computer_hq_2") || iequals(str, "computer_hq_3") ||
                iequals(str, "computer_mission_1") || iequals(str, "computer_mission_2") || starts_with(str.c_str(), "opsat", false))
                bIsOPSAT = true;
            else
                bIsOPSAT = false;
        }
    }; injector::MakeInline<UCanvasOpenVideoHook>(pattern.get_first(0), pattern.get_first(6));

    //
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 86 ? ? ? ? 85 C0 74 12 83 78 64 00 74 0C"); //0x10515110
    struct UCanvasCloseVideoHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + 0x80);
            bIsVideoPlaying = false;
        }
    }; injector::MakeInline<UCanvasCloseVideoHook>(pattern.get_first(0), pattern.get_first(6));

    //FOV
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 10 05 ? ? ? ? F3 0F 11 83 ? ? ? ? E8");
    static auto FCameraSceneNodeCtorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (gCurrentCameraShotName == L"Take_01_EnterSWAT")
        {
            *(float*)(regs.ebx + 0x268) = AdjustFOV(*(float*)(regs.ebx + 0x268), Screen.fAspectRatio, 4.0f / 3.0f);
            return;
        }

        *(float*)(regs.ebx + 0x268) = AdjustFOV(*(float*)(regs.ebx + 0x268), Screen.fAspectRatio, Screen.fDefaultARforFOV);
    });

    if (bSingleCoreAffinity)
    {
        if (AffinityChanges::Init())
        {
            IATHook::Replace(GetModuleHandle(L"Engine"), "kernel32.dll",
                std::forward_as_tuple("CreateThread", AffinityChanges::CreateThread_GameThread)
            );
        }
    }

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "83 83 20 02 00 00 01");
    static auto UGameEngineLoadGameHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UObject::objectStates.clear();
        UIntOverrides::ClearCache();
        UFloatOverrides::ClearCache();
        UByteOverrides::ClearCache();
        UNameOverrides::ClearCache();
        UObjectOverrides::ClearCache();
        UArrayOverrides::ClearCache();
    });

    #if _DEBUG
    shFindAxisName = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?FindAxisName@UInput@@MBEPAMPAVAActor@@PBG@Z"), FindAxisName);
    #endif

    // Tick hook for deferred input and raw input helper
    UGameEngine::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@UGameEngine@@UAEXM@Z"), UGameEngine::Tick);

    // Shadows and lights draw distance
    ALight::shGetShadowTurnOffRatio = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?GetShadowTurnOffRatio@ALight@@UAEMXZ"), ALight::GetShadowTurnOffRatio);
    AActor::shGetShadowTurnOffRatio = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?GetShadowTurnOffRatio@AActor@@UAEMXZ"), AActor::GetShadowTurnOffRatio);

    // Helper for video skipping
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "FF D7 83 C4 08 68 ? ? ? ? 68 ? ? ? ? FF D7 83 C4 08");
    static auto UGameEngineLoadMapHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bVideoStartedFromLoadMap = true;
    });

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "FF 90 ? ? ? ? B0 01 5E");
    static auto UGameEnginePostLoadMapHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bVideoStartedFromLoadMap = false;
    });

    // AActor::MakeNoise patch - fixes AI hearing issue, should be safe as the xbox 360 code doesn't have that check
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "0F 87 ? ? ? ? E8");
    injector::MakeNOP(pattern.get_first(0), 6, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "C6 05 ? ? ? ? ? 8B 0D ? ? ? ? ? ? 33 F6");
    bLoadingScreenActive = *pattern.get_first<uint8_t*>(2);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 59 85 ? ? ? ? F3 0F 11 85 ? ? ? ? 85 DB");
    static auto UInputExecMouseWheelScrollHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto ECoopPlayerControllerState = UObject::GetState(L"ECoopPlayerController");

        if (ECoopPlayerControllerState == L"s_CrackSafe" || ECoopPlayerControllerState == L"s_Ledge")
            regs.xmm0.f32[0] = 1.0f;
    });

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "DC 0A DC 64 24");
    static auto LoadscreenAudioThreadHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        #ifndef _DEBUG
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000 || GetAsyncKeyState(VK_RETURN) & 0x8000)
            #endif
        {
            *(double*)(regs.esp + 0x38) = 0.0f;
        }
    });

    //Remove precache for faster loading
    if (bDisablePreCache)
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? C6 05");
        injector::WriteMemory<uint8_t>(pattern.get_first(6), 1, true);
        injector::WriteMemory<uint8_t>(pattern.get_first(13), 0, true);
        injector::WriteMemory<uint8_t>(pattern.get_first(20), 0, true);
        injector::WriteMemory<uint8_t>(pattern.get_first(27), 1, true);
        injector::WriteMemory<uint8_t>(pattern.get_first(34), 1, true);

        static auto UGameEngineLoadMapHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bLoadMapWasCalled = true;
        });
    }

    //ScopeLens
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "76 ? 8B 48 ? 8B 81");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "F3 0F 5E D0 F3 0F 59 E0");
    injector::MakeNOP(pattern.get_first(0), 8, true);
    static auto FCameraSceneNodeRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        float old_v28 = regs.xmm1.f32[0];
        float old_v29 = regs.xmm3.f32[0];

        float new_v29 = old_v29 / regs.xmm0.f32[0];
        float new_v28 = old_v28 + (old_v29 - new_v29) * 0.5f;

        regs.xmm1.f32[0] = new_v28;
        regs.xmm3.f32[0] = new_v29;
    });
}
