#include "stdafx.h"
#include "../Shared/CheckedPattern.hpp"
#include "FixSettings.hpp"
#include "Menu.hpp"
#include "Video.hpp"
#include "Window.hpp"
#include "Hud.hpp"
#include "Camera.hpp"
#include "DisplayModes.hpp"
#include "Input.hpp"
#include "Focus.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fHudFullWidth;
    float fHudVerticalOffset;
    float fHudVerticalOffsetWide;
} Screen;


void Init()
{
    WindowedModeWrapper::EnableDpiAwareness();
    static auto& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");
    asi_settings::LiveLink::Start();
    BullyMenu::Init();
    DisplayModes::Init();
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", []()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    });
    BullyWindow::Install(FixSettings().Int("MAIN.WindowMode"));
    BullyFocus::Install(FixSettings().Int("MAIN.PauseOnFocusLoss"));
    BullyVideo::Install(FixSettings().Int("MAIN.FMVWidescreenMode"));
    BullyInput::Install(FixSettings().Int("MAIN.AutoGamepad"), FixSettings().Float("MAIN.GamepadCameraSpeed"), FixSettings().Float("MAIN.GamepadDeadzone"));
    if (FixSettings().Bool("MAIN.SkipIntro"))
    {
        auto logos = hook::pattern("8B 0D ? ? ? ? 8B 10 8B 52 04 6A 02 51 8B C8 FF D2 C6 05 ? ? ? ? 00");
        if (logos.size() == 1)
        {
            // Jump over the logo movie call and its two arguments (push 2; push ecx; mov ecx, eax; call edx)
            ProtectedGameRef<uint16_t>(logos.get_first<uint16_t>(11)) = 0x05EB;
        }
        else
            OutputDebugStringA("Bully: startup logo pattern unavailable.\n");
    }


    //Unlocking a bit more resolutions
    auto pattern = hook::pattern("81 FF ? ? ? ? 8B 54 24 14 7C 53 81 FA ? ? ? ? 7C 4B 8B 0D ? ? ? ? 85 C9");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    ProtectedGameRef<int32_t>(pattern.get_first<int32_t>(2)) = 640;
    ProtectedGameRef<int32_t>(pattern.get_first<int32_t>(14)) = 480;
    //Last resolution doesn't get reset
    pattern = hook::pattern("3D ? ? ? ? 0F 96 C0 C3");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    ProtectedGameRef<int32_t>(pattern.get_first<int32_t>(1)) = 16384;

    //Resolution
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 83 C4 28 C3");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    static auto nWidth = std::ref(**pattern.get_first<uint32_t*>(2));
    static auto nHeight = std::ref(**pattern.get_first<uint32_t*>(12));
    std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    ProtectedGameRef<int32_t>(pattern.get_first<int32_t>(6)) = Screen.nWidth;
    ProtectedGameRef<int32_t>(pattern.get_first<int32_t>(16)) = Screen.nHeight;

    static auto GetRes = []()
    {
        Screen.nWidth = nWidth;
        Screen.nHeight = nHeight;
        Screen.fWidth = static_cast<float>(Screen.nWidth);
        Screen.fHeight = static_cast<float>(Screen.nHeight);
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
        Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
        Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
        Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
        Screen.fHudScale = (4.0f / 3.0f) / Screen.fAspectRatio;
        Screen.fHudFullWidth = 640.0f * (Screen.fAspectRatio / (4.0f / 3.0f));
        Screen.fHudVerticalOffset = ((480.0f * (Screen.fAspectRatio / (4.0f / 3.0f))) - 480.0f) / 2.0f;
        Screen.fHudVerticalOffsetWide = ((480.0f * (Screen.fAspectRatio / (16.0f / 9.0f))) - 480.0f) / 2.0f;
    };

    BullyWindow::OnResolutionChanged = GetRes;
    BullyHud::Install(Screen.nWidth, Screen.nHeight, FixSettings().String("MAIN.HudAspectRatioConstraint"));
    BullyCutscene::Install(FixSettings().Int("MAIN.CutsceneBorders"), FixSettings().Int("MAIN.AnimatedCutsceneBorders"), Screen.fAspectRatio);

    pattern = hook::pattern("C6 44 24 18 00 8B 4C 24 18 52");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    struct ResHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.esp + 0x18) = 0;
            GetRes();
        }
    }; injector::MakeInline<ResHook1>(pattern.get_first(0)); //0x4052F1

    pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 89 15 ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? E8 ? ? ? ? 84 C0");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    static auto dword_BCBC68 = *pattern.get_first<uint32_t*>(1);
    struct ResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_BCBC68 = regs.eax;
            GetRes();
        }
    }; injector::MakeInline<ResHook2>(pattern.get_first(0)); //0x406393

    pattern = hook::pattern("89 1D ? ? ? ? 89 2D ? ? ? ? 89 15 ? ? ? ? A3 ? ? ? ? 33 C9 33 D2 33 C0");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    struct ResHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_BCBC68 = regs.ebx;
            GetRes();
        }
    }; injector::MakeInline<ResHook3>(pattern.get_first(0)); //0x4063FA

    pattern = hook::pattern("D9 05 ? ? ? ? D9 14 24 D9 5C 24 04 EB 07 D9 14 24 D9 54 24 04");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    ProtectedGameRef<float*>(pattern.get_first<float*>(2)) = &Screen.fHudScale; //0x4548FE
    pattern = hook::pattern("D9 05 ? ? ? ? D9 54 24 08 D9 5C 24 0C");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    ProtectedGameRef<float*>(pattern.get_first<float*>(2)) = &Screen.fHudScale; //0x406127

    auto requiredLookup0 = hook::pattern("D9 05 ? ? ? ? EB 06 D9 05 ? ? ? ? 83 3D");
    if (requiredLookup0.empty())
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    auto flt_93F228 = *requiredLookup0.get_first<void*>(2);
    pattern = hook::pattern(pattern_str(0xD9, 0x05, to_bytes(flt_93F228))); //fld
    for (size_t i = 0; i < pattern.size(); ++i)
        ProtectedGameRef<float*>(pattern.get(i).get<float*>(2)) = &Screen.fHudFullWidth;

    pattern = hook::pattern("8B 15 ? ? ? ? D9 5C 24 04 A1 ? ? ? ? D9 05 ? ? ? ? 03 D2 D9 5C 24 08");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    static auto dword_20DCD44 = *pattern.get_first<uint32_t*>(2);
    struct BackgroundsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *dword_20DCD44;

            auto y = *(float*)(regs.esp + 0x14);
            auto h = *(float*)(regs.esp + 0x2C);
            if (y == 0.0f && h == 240.0f)
            {
                *(float*)(regs.esp + 0x14) -= Screen.fHudVerticalOffset;
                *(float*)(regs.esp + 0x20) -= Screen.fHudVerticalOffset;
            }
            else if (y == 240.0f && h == 480.0f)
            {
                *(float*)(regs.esp + 0x2C) += Screen.fHudVerticalOffset;
                *(float*)(regs.esp + 0x38) += Screen.fHudVerticalOffset;
            }
        }
    }; injector::MakeInline<BackgroundsHook>(pattern.get_first(0), pattern.get_first(6)); //0x73984B

    //In widescreen mode the game loads 16:9 loadscreens from TXD\lsw instead of 4:3 ones from TXD\ls
    pattern = hook::pattern("80 3D ? ? ? ? 00 57 75 0C 68");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    static auto bWidescreen = *pattern.get_first<bool*>(2); //0x43BFBC

    pattern = hook::pattern("8B 3D ? ? ? ? D9 5C 24 08 85 FF D9 05 ? ? ? ? D9 5C 24 04 D9");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    static auto dword_20DCCC8 = *pattern.get_first<uint32_t*>(2);
    struct LoadscreensHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edi = *dword_20DCCC8;

            auto x = *(float*)(regs.esp + 0x1C);
            auto y = *(float*)(regs.esp + 0x20);
            auto w = *(float*)(regs.esp + 0x50);
            auto h = *(float*)(regs.esp + 0x4C);
            auto fVerticalOffset = *bWidescreen ? Screen.fHudVerticalOffsetWide : Screen.fHudVerticalOffset;

            if (x == 0.0f && (static_cast<int>(w) == 639 || static_cast<int>(w) == 640) && y == 0.0f && static_cast<int>(h) == 480)
            {
                *(float*)(regs.esp + 0x20) -= fVerticalOffset;
                *(float*)(regs.esp + 0x4C) += fVerticalOffset;
            }
            else
            {
                h = *(float*)(regs.esp + 0x20);
                x = *(float*)(regs.esp + 0x24);
                w = *(float*)(regs.esp + 0x28);
                y = *(float*)(regs.esp + 0x2C);

                if (x == 0.0f && (static_cast<int>(w) == 639 || static_cast<int>(w) == 640) && y == 0.0f && static_cast<int>(h) == 480)
                {
                    *(float*)(regs.esp + 0x2C) -= fVerticalOffset;
                    *(float*)(regs.esp + 0x20) += fVerticalOffset;
                }
            }
        }
    }; injector::MakeInline<LoadscreensHook>(pattern.get_first(0), pattern.get_first(6)); //0x73984B

    //Aspect Ratio
    pattern = hook::pattern("80 3D ? ? ? ? ? A1 ? ? ? ? D9 40 74 D8 70 78 D9 1C 24 74 0C D9 40 7C");
    if (pattern.size() < 1)
    {
        wfp::patterns::Report("Bully.WidescreenFix: feature signature unavailable.\n");
        return;
    }
    struct ARHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)regs.esp = Screen.fAspectRatio;
        }
    }; injector::MakeInline<ARHook>(pattern.get_first(0), pattern.get_first(33)); //0x4F1360

    //FOV
    pattern = hook::pattern("D9 05 ? ? ? ? EB 06 D9 05 ? ? ? ? D9 1C 24 D9 04 24 DC 0D ? ? ? ? D9 1C 24 D9 04 24");
    if (pattern.size() != 2)
    {
        BullyCutscene::ResetHooks();
        OutputDebugStringA("Bully: camera projection patterns unavailable.\n");
        return;
    }
    static auto flt_C3CD04 = *pattern.get(0).get<float*>(2);
    GameplayCamera::Manager = reinterpret_cast<uint8_t*>(flt_C3CD04) - 0x9C;
    auto blend = hook::pattern("51 56 8B F1 83 BE A4 01 00 00 00 0F 84 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84");
    if (blend.size() == 1)
        GameplayCamera::Blending = *blend.get_first<int32_t*>(18) + 2;
    else
        OutputDebugStringA("Bully: gameplay camera blend pattern unavailable; FOV multiplier disabled.\n");

    static std::array<SafetyHookMid, 2> projectionHooks;
    static auto applyProjection = [](SafetyHookContext& regs)
    {
        auto value = reinterpret_cast<float*>(regs.esp);
        const bool overridden = *reinterpret_cast<int32_t*>(GameplayCamera::Manager + 0x16C) != 0;
        Screen.fFieldOfView = GameplayCamera::Adjust(*value, Screen.fAspectRatio, fFOVFactor, true, overridden);
        *value = Screen.fFieldOfView;
    };
    projectionHooks[0] = safetyhook::create_mid(pattern.get(0).get<void>(17), [](SafetyHookContext& regs)
    {
        // Fixed-aspect auxiliary renders keep their own projection.
        if (!*reinterpret_cast<uint8_t*>(regs.esp + 0x34))
            applyProjection(regs);
    });
    projectionHooks[1] = safetyhook::create_mid(pattern.get(1).get<void>(17), applyProjection);
    if (!projectionHooks[0] || !projectionHooks[1])
    {
        for (auto& projection : projectionHooks)
            projection.reset();
        BullyCutscene::ResetHooks();
        OutputDebugStringA("Bully: camera projection hooks unavailable.\n");
        return;
    }

    //Render
    pattern = hook::pattern("D9 86 ? ? ? ? DC 0D ? ? ? ? DC 0D ? ? ? ? D9 5C 24 04 D9 44 24 04 E8 ? ? ? ? D9 5C 24 08 D9 44 24 08");
    if (pattern.size() != 1)
    {
        for (auto& projection : projectionHooks)
            projection.reset();
        BullyCutscene::ResetHooks();
        OutputDebugStringA("Bully: camera culling pattern unavailable.\n");
        return;
    }
    struct CullingFOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            const float original = *reinterpret_cast<float*>(regs.esi + 0x9C);
            const bool primary = reinterpret_cast<uint8_t*>(regs.esi) == GameplayCamera::Manager;
            const bool overridden = primary && *reinterpret_cast<int32_t*>(GameplayCamera::Manager + 0x16C) != 0;
            const float value = overridden ? *reinterpret_cast<float*>(GameplayCamera::Manager + 0x168) : original;
            Screen.fFieldOfView = GameplayCamera::Adjust(value, Screen.fAspectRatio, fFOVFactor, primary, overridden);
            _asm fld[Screen.fFieldOfView]
        }
    };
    injector::MakeInline<CullingFOVHook>(pattern.get_first(0), pattern.get_first(6)); //0x4F1D16
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("81 EC 8C 05 00 00 53 55 33 ED 56 57"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        CursorConfinement::Shutdown();
    }
    return TRUE;
}
