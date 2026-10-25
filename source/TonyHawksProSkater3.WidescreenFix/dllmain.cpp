#include "stdafx.h"
#include "FixSettings.hpp"
#include "Camera.hpp"
#include "Hud.hpp"
#include "Video.hpp"
#include "../Shared/TonyHawk/Display.hpp"
#include "NativeMenu.hpp"
#include <random>

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    int32_t FOV;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

namespace THPS3Display
{
    SafetyHookInline LogoHook;
    char __cdecl Logo(const char* filename, int mode)
    {
        if (TonyDisplay::IsLogo(filename))
            return 1;
        return LogoHook.ccall<char>(filename, mode);
    }

    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        WindowedModeWrapper::EnableDpiAwareness();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            auto pattern = hook::pattern("A0 ? ? ? ? 8B 4C 24 20 84 C3 89 1D ? ? ? ? 74 47 8B 15 ? ? ? ? A1");
            if (pattern.size() == 1)
            {
                static uint8_t* ModeFlags = *pattern.get_first<uint8_t*>(1);
                static auto windowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
                {
                    *ModeFlags = *ModeFlags & ~1;
                });
                TonyDisplay::Window(mode, width, height);
            }
            else
                OutputDebugStringA("THPS3: native window mode pattern unavailable.\n");
        }
        if (settings.Bool("MAIN.SkipIntro"))
        {
            auto caller = hook::pattern("56 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 84 C0 0F 85 ? ? ? ?");
            void* playback = nullptr;
            for (size_t i = 0; i < caller.size(); ++i)
            {
                auto entry = caller.get(i);
                if (std::strcmp(*entry.get<const char*>(2), "movies\\nslogo") == 0)
                    playback = injector::GetBranchDestination(entry.get<void*>(6)).get<void>();
            }
            if (playback)
                LogoHook = safetyhook::create_inline(playback, Logo);
            else
                OutputDebugStringA("THPS3: startup logo pattern unavailable.\n");
        }
    }
}

void Init()
{
    WindowCursor::Initialize(FixSettings());
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    bool bFixHUD = settings.Int("MAIN.FixHUD") != 0;
    bool bRandomSongOrderFix = settings.Int("MAIN.RandomSongOrderFix") != 0;

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (FixSettings().Int("MAIN.WindowMode") == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THPS3Display::Init(Screen.Width, Screen.Height);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    [&]()
    {
        hook::pattern pattern("");
        //Resolution
        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? B0 01 5F 5E"); //40B349
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksProSkater3.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        static int32_t* dword_851084 = *pattern.get_first<int32_t*>(2);
        static int32_t* dword_851088 = *pattern.get_first<int32_t*>(12);
        injector::MakeNOP(pattern.get_first(-12), 2, true);

        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_851084 = Screen.Width;
                *dword_851088 = Screen.Height;
            }
        };
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(20));
        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 2D"); //40B5B8
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksProSkater3.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(20));
        pattern = hook::pattern("A3 ? ? ? ? 8B 04 AE 85 C0"); //40B835 40B84B
        if (pattern.size() < 2)
        {
            OutputDebugStringA("TonyHawksProSkater3.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeInline<SetResHook>(pattern.count_hint(2).get(0).get<void*>(0));
        injector::MakeInline<SetResHook>(pattern.count_hint(2).get(1).get<void*>(0));
    }();

    THPS3Camera::Install(1.0f / Screen.fAspectRatioDiff, &settings.Float("MAIN.FOVFactor"));

    THPS3Video::Install(&settings.Int("MAIN.FMVWidescreenMode"));

    [&]()
    {
        hook::pattern pattern("");
        //HUD
        if (bFixHUD)
        {
            THPS3Hud::Install(Screen.fWidth, Screen.fHeight, &settings.String("MAIN.HudAspectRatioConstraint"));
            pattern = hook::pattern("D8 0D ? ? ? ? 8B CF 89 5C 24 50 D8"); //0x58DDC8
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksProSkater3.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            **pattern.get_first<float*>(2) = Screen.fHUDScaleX;

            pattern = hook::pattern("0B F2 89 70 64 A1 8C 67 90"); //0x4F65B1
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksProSkater3.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            struct HUDHook //sub_4F62A0
            {
                void operator()(injector::reg_pack& regs)
                {
                    if (*(float*)(regs.eax + 0x00) == 0.0f && *(float*)(regs.eax + 0x1C) == 0.0f
                        && (int32_t)(*(float*)(regs.eax + 0x38)) == Screen.Width43
                        && (int32_t)(*(float*)(regs.eax + 0x54)) == Screen.Width43)
                    {
                        //blood overlay, maybe more
                        //*(float*)(regs.eax + 0x00) += Screen.fHudOffsetReal;
                        //*(float*)(regs.eax + 0x1C) += Screen.fHudOffsetReal;
                        *(float*)(regs.eax + 0x38) += Screen.fHudOffsetReal * 2.0f;
                        *(float*)(regs.eax + 0x54) += Screen.fHudOffsetReal * 2.0f;
                    }
                    else
                    {
                        const float offset = Screen.fHudOffsetReal + THPS3Hud::Offset();
                        for (auto vertexOffset : { 0x00, 0x1C, 0x38, 0x54 })
                        {
                            *reinterpret_cast<float*>(regs.eax + vertexOffset) += offset;
                        }
                    }

                    regs.esi |= regs.edx;
                    *(uint32_t*)(regs.eax + 0x64) = regs.esi;
                }
            }; injector::MakeInline<HUDHook>(pattern.get_first(0));

            pattern = hook::pattern("8B 81 A4 00 00 00 89 46"); //0x4F66F0
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksProSkater3.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            struct HUDHook2 //sub_4F65E0
            {
                void operator()(injector::reg_pack& regs)
                {
                    *reinterpret_cast<float*>(regs.esi - 0x04) += Screen.fHudOffsetReal + THPS3Hud::Offset();
                    regs.eax = *(uint32_t*)(regs.ecx + 0xA4);
                }
            }; injector::MakeInline<HUDHook2>(pattern.get_first(0), pattern.get_first(6));
        }
    }();

    [&]()
    {
        hook::pattern pattern("");
        if (bRandomSongOrderFix)
        {
            pattern = hook::pattern("E8 ? ? ? ? 8B 96 E8");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksProSkater3.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            struct RandomHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    std::mt19937 r{ std::random_device{}() };
                    std::uniform_int_distribution<uint32_t> uid(0, regs.eax);
                    regs.eax = uid(r);
                }
            }; injector::MakeInline<RandomHook>(pattern.get_first(0));
        }
    }();

    THPS3Menu::Init();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("53 55 56 57 52 6A 00"));
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
        WindowCursor::Shutdown();
    }
    return TRUE;
}
