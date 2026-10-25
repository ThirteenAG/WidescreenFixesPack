#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include "NativeMenu.hpp"
#include "../Shared/D3D9Options.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

struct Screen
{
    int32_t Width = 640;
    int32_t Height = 480;
    float fWidth = 640.0f;
    float fHeight = 480.0f;
    float fAspectRatio = 4.0f / 3.0f;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fAspectRatioDiff = 1.0f;
} Screen;

bool isTTB;
uintptr_t* pMemory;
uint32_t off_288, off_290, off_2C0;
float* NativeFOV = nullptr;
void __cdecl sub_44A390(uintptr_t a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, uintptr_t a12, uintptr_t a13, uintptr_t a14)
{
    if (!a1 || !pMemory || !*pMemory ||
        (!a14 && (!std::isfinite(a10) || !std::isfinite(a11) || a10 == 0.0f || a11 == 0.0f)))
        return;
    uintptr_t Memory = *pMemory;

    if (a2 == 0.0f && a4 == Screen.fWidth)
    {
        if (a12)
        {
            if (*(uint32_t*)(a12 + 0x00) == 0xffffffff && *(uint32_t*)(a12 + 0x04) == 0xffffffff && *(uint32_t*)(a12 + 0x08) == 0xffffffff && *(uint32_t*)(a12 + 0x0C) == 0xffffffff)
            {
                a2 /= Screen.fAspectRatioDiff;
                a4 /= Screen.fAspectRatioDiff;
                a2 += Screen.fHudOffsetReal;
            }
        }
    }
    else
    {
        a2 /= Screen.fAspectRatioDiff;
        a4 /= Screen.fAspectRatioDiff;
        a2 += Screen.fHudOffsetReal;
    }

    float v14 = a6 + 0.5f;
    float v22 = a11 - a7 - 0.5f;
    float v15 = 1.0f / a10;
    float v23 = 1.0f / a11;

    if (a14)
    {
        *reinterpret_cast<float*>(a1 + 0x20) = a2 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_288) + a4;
        *reinterpret_cast<float*>(a1 + 0x38) = a8;
        *reinterpret_cast<float*>(a1 + 0x24) = a3 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_290);
        *reinterpret_cast<float*>(a1 + 0x3C) = a7;
        *reinterpret_cast<float*>(a1 + 0x18) = a6;
        *reinterpret_cast<float*>(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_288);
        *reinterpret_cast<float*>(a1 + 0x1C) = a9;
        *reinterpret_cast<float*>(a1 + 0x04) = a5 + *(float*)(a1 + 0x24);
    }
    else
    {
        *reinterpret_cast<float*>(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_288);
        *reinterpret_cast<float*>(a1 + 0x04) = a3 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_290);
        *reinterpret_cast<float*>(a1 + 0x18) = v15 * v14;
        *reinterpret_cast<float*>(a1 + 0x1C) = v23 * v22;
        *reinterpret_cast<float*>(a1 + 0x20) = a4 + *(float*)a1;
        *reinterpret_cast<float*>(a1 + 0x24) = a5 + *(float*)(a1 + 4);
        *reinterpret_cast<float*>(a1 + 0x38) = (v14 + a8) * v15;
        *reinterpret_cast<float*>(a1 + 0x3C) = (v22 - a9) * v23;
    }
    *reinterpret_cast<uint32_t*>(a1 + 0x40) = *(uint32_t*)(a1 + 0x20);
    *reinterpret_cast<uint32_t*>(a1 + 0x44) = *(uint32_t*)(a1 + 0x24);
    *reinterpret_cast<uint32_t*>(a1 + 0x48) = *(uint32_t*)(a1 + 0x28);
    *reinterpret_cast<uint32_t*>(a1 + 0x4C) = *(uint32_t*)(a1 + 0x2C);
    *reinterpret_cast<uint32_t*>(a1 + 0x50) = *(uint32_t*)(a1 + 0x30);
    *reinterpret_cast<uint32_t*>(a1 + 0x54) = *(uint32_t*)(a1 + 0x34);
    *reinterpret_cast<uint32_t*>(a1 + 0x58) = *(uint32_t*)(a1 + 0x38);
    *reinterpret_cast<uint32_t*>(a1 + 0x5C) = *(uint32_t*)(a1 + 0x3C);
    *reinterpret_cast<uint32_t*>(a1 + 0x60) = *(uint32_t*)(a1 + 0x00);
    *reinterpret_cast<uint32_t*>(a1 + 0x64) = *(uint32_t*)(a1 + 0x04);
    *reinterpret_cast<uint32_t*>(a1 + 0x68) = *(uint32_t*)(a1 + 0x08);
    *reinterpret_cast<uint32_t*>(a1 + 0x6C) = *(uint32_t*)(a1 + 0x0C);
    *reinterpret_cast<uint32_t*>(a1 + 0x70) = *(uint32_t*)(a1 + 0x10);
    *reinterpret_cast<uint32_t*>(a1 + 0x74) = *(uint32_t*)(a1 + 0x14);
    *reinterpret_cast<uint32_t*>(a1 + 0x78) = *(uint32_t*)(a1 + 0x18);
    *reinterpret_cast<uint32_t*>(a1 + 0x7C) = *(uint32_t*)(a1 + 0x1C);
    *reinterpret_cast<uint32_t*>(a1 + 0x3C) = *(uint32_t*)(a1 + 0x1C);
    *reinterpret_cast<uint32_t*>(a1 + 0x24) = *(uint32_t*)(a1 + 0x04);
    *reinterpret_cast<uint32_t*>(a1 + 0x7C) = *(uint32_t*)(a1 + 0x5C);
    *reinterpret_cast<uint32_t*>(a1 + 0x64) = *(uint32_t*)(a1 + 0x44);
    *reinterpret_cast<uint32_t*>(a1 + 0x08) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x0C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x10) = *(uint32_t*)(a12 + 0x00);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x10) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x14) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x14) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x28) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x2C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x30) = *(uint32_t*)(a12 + 0x04);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x30) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x34) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x34) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x48) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x4C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x50) = *(uint32_t*)(a12 + 0x08);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x50) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x54) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x54) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x68) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x6C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x70) = *(uint32_t*)(a12 + 0x0C);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x70) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x74) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x74) = 0xFF000000;
    if (a13)
        *reinterpret_cast<uint32_t*>(a13) = 0;
}

SafetyHookInline CutsceneBordersHook;
const int32_t* pCutsceneBorders = nullptr;

int __fastcall DrawCutsceneBorders(int* self, void*)
{
    // 0 Off, 1 Letterbox (the original bars)
    if (pCutsceneBorders && *pCutsceneBorders == 0)
        return 0;
    return CutsceneBordersHook.thiscall<int>(self);
}

void Init()
{
    hook::pattern pattern;
    pCutsceneBorders = &FixSettings().Int("MAIN.CutsceneBorders");
    static const auto& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");

    // The game is made for 60 frames per second
    D3D9Options::frameLimit = FixSettings().Int("MAIN.FPSLimit");
    D3D9Options::HookCreation();

    //unlocking resolutions
    if (wfp::PatchPreflight("The Suffering display modes: signatures unavailable; feature disabled.\n",
    {
        { "75 4B D9 45 ? D8 5D ? DF E0 F6 C4 41", 1 },
    }))
    {
        pattern = hook::pattern("75 4B D9 45 ? D8 5D ? DF E0 F6 C4 41");
        injector::MakeNOP(pattern.get_first(0), 2, true);
        injector::MakeNOP(pattern.get_first(13), 2, true);

    }

    //unlocking windowed resolutions
    if (wfp::PatchPreflight("The Suffering windowed display modes: signatures unavailable; feature disabled.\n",
    {
        { "8B 15 ? ? ? ? 83 E2 01", 1 },
    }))
    {
        pattern = hook::pattern("8B 15 ? ? ? ? 83 E2 01"); //0x4B35E2
        ProtectedGameRef<uint16_t>{pattern.get_first<uint16_t>(9)} = 0xE990;

    }

    //The Game Play disk could not be found error
    if (wfp::PatchPreflight("The Suffering disc compatibility: signatures unavailable; feature disabled.\n",
    {
        { "85 C0 ? ? 8B 4C 24 0C 51", 1 },
        { "75 ? 8D 44 24 08 50 8B 44 24 10", 1 },
    }))
    {
        pattern = hook::pattern("85 C0 ? ? 8B 4C 24 0C 51"); //0x43B989 in ttb
        ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(2)} = 0xEB; //0x43B959
        pattern = hook::pattern("75 ? 8D 44 24 08 50 8B 44 24 10");
        injector::MakeNOP(pattern.get_first(0), 2, true);

    }

    //FOV
    if (wfp::PatchPreflight("The Suffering FOV: signatures unavailable; feature disabled.\n",
    {
        { "D9 05 ? ? ? ? D8 B1 ? ? ? ? D9 E8 D9 F3", 2 },
        { "D8 0D ? ? ? ? D8 B6 ? ? ? ? 5E D9 E8 D9 F3", 1 },
        { "D8 0D ? ? ? ? D9 96", 1 },
        { "D8 3D ? ? ? ? C2 04 00", 1 },
        { "D8 3D ? ? ? ? D9 99 ? ? ? ? FF", 1 },
        { "D8 3D ? ? ? ? D9 99 ? ? ? ? C2 04 00", 1 },
    }))
    {
        pattern = hook::pattern("D9 05 ? ? ? ? D8 B1 ? ? ? ? D9 E8 D9 F3");
        ProtectedGameRef<float*>{pattern.count_hint(2).get(0).get<float*>(2)} = &Screen.fFieldOfView; //fld
        ProtectedGameRef<float*>{pattern.count_hint(2).get(1).get<float*>(2)} = &Screen.fFieldOfView; //fld
        pattern = hook::pattern("D8 0D ? ? ? ? D8 B6 ? ? ? ? 5E D9 E8 D9 F3");
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fFieldOfView; //fmul
        pattern = hook::pattern("D8 0D ? ? ? ? D9 96");
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fFieldOfView; //fmul
        pattern = hook::pattern("D8 3D ? ? ? ? C2 04 00");
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fFieldOfView; //fdiv
        pattern = hook::pattern("D8 3D ? ? ? ? D9 99 ? ? ? ? FF");
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fFieldOfView; //fdiv
        pattern = hook::pattern("D8 3D ? ? ? ? D9 99 ? ? ? ? C2 04 00");
        NativeFOV = *pattern.get_first<float*>(2);
        Screen.fFieldOfView = *NativeFOV;
        FixSettings().OnChange("MAIN.FOVFactor", []()
        {
            const float factor = std::isfinite(fFOVFactor) ? std::clamp(fFOVFactor, 0.5f, 2.0f) : 1.0f;
            Screen.fFieldOfView = *NativeFOV / (Screen.fAspectRatio / (4.0f / 3.0f)) / factor;
        });
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fFieldOfView; //fdiv

    }

    //HUD
    if (wfp::PatchPreflight("The Suffering HUD: signatures unavailable; feature disabled.\n",
    {
        { "8B 0D ? ? ? ? DD D8 8B 51 4C DD D8", 1 },
        { "D8 84 0A ? ? ? ? D8 44 24 14 D9 58 ? 8B 0D", 1 },
        { "8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", 1 },
        { "E8 ? ? ? ? A1 ? ? ? ? 83 C4 38 50", 1 },
        { "E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 38 51", 1 },
    }))
    {
        pMemory = *(uintptr_t**)hook::get_pattern("8B 0D ? ? ? ? DD D8 8B 51 4C DD D8", 2); //0x5C5D7C
        off_288 = *hook::get_pattern<uint32_t>("D8 84 0A ? ? ? ? D8 44 24 14 D9 58 ? 8B 0D", 3);
        off_290 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -4);
        off_2C0 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -11);
        pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 38 50");
        injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44A6C0
        pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 38 51");
        injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44AB58

    }

    //resolution
    if (wfp::PatchPreflight("The Suffering resolution tracking: signatures unavailable; feature disabled.\n",
    {
        { "68 ? ? ? ? 56 8D 4C 24", 1 },
        { "8B 52 08 89 50 08 C2 08 00", 1 },
    }))
    {
        static auto unk_5C6CEC = *(uintptr_t**)hook::get_pattern("68 ? ? ? ? 56 8D 4C 24", 1);
        pattern = hook::pattern("8B 52 08 89 50 08 C2 08 00"); //0x4460E7
        struct GetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edx = *(uint32_t*)(regs.edx + 0x08);
                *reinterpret_cast<uint32_t*>(regs.eax + 0x08) = regs.edx;

                auto width = static_cast<int32_t>(unk_5C6CEC[0]);
                auto height = static_cast<int32_t>(unk_5C6CEC[1]);
                if (width <= 0 || height <= 0 || width > 32767 || height > 32767)
                    return;
                Screen.Width = width;
                Screen.Height = height;
                Screen.fWidth = static_cast<float>(Screen.Width);
                Screen.fHeight = static_cast<float>(Screen.Height);
                Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
                Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
                Screen.fAspectRatioDiff = (Screen.fAspectRatio / (4.0f / 3.0f));
                Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
                Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
                if (NativeFOV)
                    Screen.fFieldOfView = (*NativeFOV / (Screen.fAspectRatio / (4.0f / 3.0f))) / (std::isfinite(fFOVFactor) ? std::clamp(fFOVFactor, 0.5f, 2.0f) : 1.0f);
            }
        }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(6));

    }

    pattern = hook::pattern("51 53 55 56 8B F1 8B 46 34 85 C0"); //0x4B0B30
    if (pattern.size() == 1)
        CutsceneBordersHook = safetyhook::create_inline(pattern.get_first(), DrawCutsceneBorders);
    else
        OutputDebugStringA("The Suffering: cutscene border setting unavailable.\n");
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            // Real pixels on scaled desktops (desktop resolution, full screen picture not scaled up from a corner)
            WindowedModeWrapper::EnableDpiAwareness();
            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallback(L"suffering.rfl", SufferingMenu::Init);
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("75 4B D9 45 ? D8 5D ? DF E0 F6 C4 41"));
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
