module;

#include <stdafx.h>
#include <queue>
#include <functional>

export module Engine;

import ComVars;
import WidescreenHUD;

namespace UInput
{
    uint8_t(__fastcall* GetKey)(void* uInput, void* edx, const wchar_t* a2, int a3) = nullptr;

    void* gUInput = nullptr;
    SafetyHookInline shInit = {};
    void __fastcall Init(void* uInput, void* edx, void* uViewport)
    {
        gUInput = uInput;
        return shInit.unsafe_fastcall(uInput, edx, uViewport);
    }
}

namespace UEngine
{
    SafetyHookInline shInputEvent = {};
    int __fastcall InputEvent(void* _this, void* edx, int a2, int inputID, int a4, float value)
    {
        if (inputID == 202 && !bIsEnhanced) // A on gamepad
        {
            auto EchelonMainHUDState = UObject::GetState(L"EchelonMainHUD");
            if (EchelonMainHUDState == L"MainHUD" || EchelonMainHUDState == L"s_Slavery")
            {
                auto EPlayerControllerState = UObject::GetState(L"EPlayerController");

                if (EPlayerControllerState == L"s_FirstPersonTargeting" || EPlayerControllerState == L"s_RappellingTargeting" ||
                    EPlayerControllerState == L"s_PlayerBTWTargeting" || EPlayerControllerState == L"s_PlayerSniping" || EPlayerControllerState == L"s_HOHFUTargeting")
                {
                    auto EGameInteractionState = UObject::GetState(L"EGameInteraction");
                    if (EGameInteractionState != L"s_GameInteractionMenu")
                    {
                        shInputEvent.unsafe_fastcall<int>(_this, edx, a2, inputID, a4, value);
                        return shInputEvent.unsafe_fastcall<int>(_this, edx, a2, UInput::GetKey(UInput::gUInput, edx, L"ReloadGun", 0), a4, value);
                    }
                }
            }
        }
        return shInputEvent.unsafe_fastcall<int>(_this, edx, a2, inputID, a4, value);
    }
}

namespace FCanvasUtil
{
    void(__fastcall* DrawTile)(void* _this, uint32_t EDX, float, float, float, float, float, float, float, float, float, void*, FColor);
}

void __fastcall FCanvasUtilDrawTileHook(void* _this, uint32_t EDX, float X, float Y, float SizeX, float SizeY, float U, float V, float SizeU, float SizeV, float unk1, void* Texture, FColor Color)
{
    bool bIsMainMenu = UObject::GetState(L"EPCConsole") == L"UWindow";

    static FColor ColBlack; ColBlack.RGBA = 0xFF000000;

    if (!bIsMainMenu)
    {
        const float fZoomScopeBorders = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 188.0f);
        const float fStickyCamera = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 43.0f);
        const float fOpticCable = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 30.0f);
        const float fOpticMic = (Screen.fHeight * (4.0f / 3.0f)) / (640.0f / 163.0f);

        if (X == 0.0f && (FloatEqual(SizeX, fZoomScopeBorders) || FloatEqual(SizeX, fStickyCamera) ||
            FloatEqual(SizeX, fOpticCable) || FloatEqual(SizeX, fOpticMic)))
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
            FCanvasUtil::DrawTile(_this, EDX, Screen.fWidth - Screen.fHudOffset - 1.0f, Y, Screen.fWidth, SizeY, U, V, SizeU, SizeV, unk1, nullptr, ColBlack);
        }
    }

    const float fExpectedHeight = Screen.fHeight * (4.0f / 3.0f);

    if (X == 0.0f && FloatEqual(SizeX, fExpectedHeight))
    {
        if ((Color.R == 0x40 && Color.G == 0x00 && Color.B == 0x00) || (Color.R == 0x80 && Color.G == 0x80 && Color.B == 0x80)) //mission failed red screen
        {
            if (!bIsMainMenu)
            {
                FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
                return;
            }
        }
        else if ((Color.R == 0x00 && Color.G == 0x40 && Color.B == 0x00)) //mission passed green screen
        {
            if (!bIsMainMenu)
            {
                FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
                return;
            }
        }
        else
        {
            FCanvasUtil::DrawTile(_this, EDX, 0.0f, Y, Screen.fHudOffset, SizeY, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
            FCanvasUtil::DrawTile(_this, EDX, SizeX + Screen.fHudOffset, Y, SizeX + Screen.fHudOffset + Screen.fHudOffset + 1.0f, SizeY + 1.0f, U, V, SizeU, SizeV, unk1, Texture, ColBlack);
        }
    }

    if (!bIsMainMenu && Screen.nHudWidescreenMode == 1)
    {
        wchar_t buffer[256];
        curDrawTileManagerTextureName = UObject::GetFullName(Texture, 0, buffer);
        WidescreenHud(X, SizeX, Y, SizeY, Color);
    }

    X += Screen.fHudOffset;
    SizeX += Screen.fHudOffset;

    return FCanvasUtil::DrawTile(_this, EDX, X, Y, SizeX, SizeY, U, V, SizeU, SizeV, unk1, Texture, Color);
}

namespace UGameEngine
{
    SafetyHookInline shDisplaySplash = {};
    void __fastcall DisplaySplash(void* UGameEngine, void* edx, int a2)
    {
        bDisplayingSplash = true;
        shDisplaySplash.unsafe_fastcall(UGameEngine, edx, a2);
        bDisplayingSplash = false;
    }

    SafetyHookInline shDisplaySplashProgress = {};
    void __fastcall DisplaySplashProgress(void* UGameEngine, void* edx, float a2)
    {
        bDisplayingSplash = true;
        shDisplaySplashProgress.unsafe_fastcall(UGameEngine, edx, a2);
        bDisplayingSplash = false;
    }

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
            UWindowsViewport::deferredCauseInputEventForRawInput(228, 4, static_cast<float>(RawInputHandler<int32_t>::RawMouseDeltaX));
            UWindowsViewport::deferredCauseInputEventForRawInput(229, 4, static_cast<float>(RawInputHandler<int32_t>::RawMouseDeltaY));
            RawInputHandler<int32_t>::RawMouseDeltaX = 0;
            RawInputHandler<int32_t>::RawMouseDeltaY = 0;
            UWindowsViewport::deferredCauseInputEventForRawInput = nullptr;
        }

        return shTick.unsafe_fastcall(UGameEngine, edx, deltaTime);
    }
}

namespace UCanvas
{
    SafetyHookInline shSetClip = {};
    void __fastcall SetClip(void* uCanvas, void* edx, float a2, float a3)
    {
        if (Screen.nHudWidescreenMode > 1)
        {
            uint32_t n_offsetX1 = static_cast<uint32_t>(a2);
            uint32_t n_offsetX2 = static_cast<uint32_t>(a3);

            if (n_offsetX1 == 640 && n_offsetX2 == 480)
            {
                a2 = (480.0f * Screen.fAspectRatio);
            }
        }

        return shSetClip.unsafe_fastcall(uCanvas, edx, a2, a3);
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

export void InitEngine()
{
    //HUD
    InitWidescreenHUD();

    auto flt_104E9F78 = *hook::pattern(GetModuleHandle(L"Engine"), "D8 C9 D8 0D").count(9).get(0).get<float*>(4);
    auto pattern = hook::module_pattern(GetModuleHandle(L"Engine"), pattern_str(0xD8, 0xC9, 0xD8, 0x0D, to_bytes(flt_104E9F78)));
    for (size_t i = 0; i < pattern.count_hint(4).size(); ++i)
    {
        injector::WriteMemory(pattern.get(i).get<void>(4), &Screen.HUDScaleX, true); //(DWORD)Engine + 0x1E9F78
    }

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 E8");
    FCanvasUtil::DrawTile = (decltype(FCanvasUtil::DrawTile))injector::GetBranchDestination(pattern.count(1).get(0).get<uintptr_t>(6), true).as_int();

    injector::MakeCALL(pattern.count(1).get(0).get<uintptr_t>(6), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC8ECE

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B ? 94 00 00 00 52 E8");
    injector::MakeCALL(pattern.count(2).get(0).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9B7C
    injector::MakeCALL(pattern.count(2).get(1).get<uint32_t>(7), FCanvasUtilDrawTileHook, true); //(uint32_t)Engine + 0xC9DE1

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "8B 46 34 8B 88 B0 02 00 00");
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (bRestoreCutsceneFOV && UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
            {
                *(float*)&regs.ecx = *(float*)(regs.eax + 0x2B0);
            }
            else
            {
                *(float*)&regs.ecx = AdjustFOV(*(float*)(regs.eax + 0x2B0), Screen.fAspectRatio);
            }
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.count(1).get(0).get<uint32_t>(3), pattern.count(1).get(0).get<uint32_t>(3 + 6)); //pfDraw + 0x104

    // Resolution overrides
    static auto newSETRES = L"SETRES 000x000"; // Final video resolution switch to 640x480, also crashes with dgvoodoo on some resolutions
    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 8B 01 FF 10 68 00 00 80 3F");
    injector::WriteMemory(pattern.get_first(1), newSETRES, true);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 28 84 5E 58 74 23 8B 0D ? ? ? ? 8B 51 48 8B 42 30 8B 00 8D 48 30 A1 ? ? ? ? 8B 00 50 68 ? ? ? ? E9 1F 01 00 00");
    a640x480 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 09");
    a800x600 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 28 84 5E 58 74 23 8B 0D ? ? ? ? 8B 51 48 8B 42 30 8B 00 8D 48 30 A1 ? ? ? ? 8B 00 50 68 ? ? ? ? E9 AE 00 00 00");
    a1024x768 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 75 25");
    a1280x1024 = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::module_pattern(GetModuleHandle(L"Engine"), "68 ? ? ? ? 50 FF D5 83 C4 08 85 C0 8A 46 58");
    a1600x1200 = *pattern.get_first<const wchar_t*>(1);

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a640x480), wcslen(a640x480));
        wcscpy_s(const_cast<wchar_t*>(a640x480), ResList[0].second.size() + 1, ResList[0].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a800x600), wcslen(a800x600));
        wcscpy_s(const_cast<wchar_t*>(a800x600), ResList[1].second.size() + 1, ResList[1].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a1024x768), wcslen(a1024x768));
        wcscpy_s(const_cast<wchar_t*>(a1024x768), ResList[2].second.size() + 1, ResList[2].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a1280x1024), wcslen(a1280x1024));
        wcscpy_s(const_cast<wchar_t*>(a1280x1024), ResList[3].second.size() + 1, ResList[3].second.c_str());
    }

    {
        injector::scoped_unprotect(const_cast<wchar_t*>(a1600x1200), wcslen(a1600x1200));
        wcscpy_s(const_cast<wchar_t*>(a1600x1200), ResList[4].second.size() + 1, ResList[4].second.c_str());
    }

    // LOD
    if (!bIsEnhanced)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "0F 84 ? ? ? ? 8B 47 24 8D 8C 24 90 00 00 00");
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
    }

    if (bSkipIntro)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "75 ? A1 ? ? ? ? 85 C0 75 ? 68 00 00 80 3F");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp

        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "75 ? 39 3D ? ? ? ? 75");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
    }

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 0D ? ? ? ? 56 8B 74 24 10");
    static auto VideoPlaybackStartHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = true;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B C1 56 8B 74 24 0C");
    static auto VideoPlaybackEndHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPlayingVideo = false;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 55 00 6A 01 8B CD FF 52 ? 6A 00");
    static auto PressStartToContinueHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPressStartToContinue = true;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "A1 ? ? ? ? 83 38 00 74 ? C7 85 B4 01 00 00 01 00 00 00");
    static auto PressStartToContinueHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bPressStartToContinue = false;
    });

    if (bSkipPressStartToContinue)
    {
        pattern = find_module_pattern(GetModuleHandle(L"Engine"), "75 ? ? ? ? ? ? 8B CD FF 52 ? 6A 00");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jz -> jmp
    }

    UInput::GetKey = (decltype(UInput::GetKey))GetProcAddress(GetModuleHandle(L"Engine"), "?GetKey@UInput@@UAEEPBGH@Z");
    UEngine::shInputEvent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?InputEvent@UEngine@@UAEHPAVUViewport@@W4EInputKey@@W4EInputAction@@M@Z"), UEngine::InputEvent);
    UInput::shInit = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Init@UInput@@UAEXPAVUViewport@@@Z"), UInput::Init);

    UGameEngine::shDisplaySplash = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?DisplaySplash@UGameEngine@@UAEXH@Z"), UGameEngine::DisplaySplash);
    UGameEngine::shDisplaySplashProgress = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?DisplaySplashProgress@UGameEngine@@UAEXM@Z"), UGameEngine::DisplaySplashProgress);
    UGameEngine::shTick = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?Tick@UGameEngine@@UAEXM@Z"), UGameEngine::Tick);

    UCanvas::shSetClip = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Engine"), "?SetClip@UCanvas@@UAEXMM@Z"), UCanvas::SetClip);

    pattern = find_module_pattern(GetModuleHandle(L"Engine"), "8B 85 40 01 00 00 6A 00 8B 48 18");
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
}