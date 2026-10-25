#include "stdafx.h"
#include "FixSettings.hpp"
#include "Hud.hpp"
#include "../Shared/TonyHawk/Display.hpp"
#include "../Shared/TonyHawk/Bink9.hpp"
#include "../Shared/TonyHawk/Camera.hpp"
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
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

namespace THUGDisplay
{
    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        WindowedModeWrapper::EnableDpiAwareness();
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            auto pattern = hook::pattern("89 3D ? ? ? ? 89 15 ? ? ? ? 76 05 BD 03 00 00 00 53 6A 04 B8 00 04 00 00");
            if (pattern.size() == 1)
            {
                static int* NativeWindowed = *pattern.get_first<int*>(2);
                static auto windowedHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext&)
                {
                    *NativeWindowed = 1;
                });
                TonyDisplay::Window(mode, width, height);
            }
            else
                OutputDebugStringA("THUG: native window mode pattern unavailable.\n");
        }
        if (settings.Bool("MAIN.SkipIntro"))
            TonyDisplay::Intro("51 8B 4C 24 08 6A 01 8D 44 24 04 50 6A 00 E8 ? ? ? ? 84 C0 74 10 8B 0C 24 51 E8 ? ? ? ? 83 C4 04 B0 01 59 C3 32 C0 59 C3", 27, 32);
    }
}

void Init()
{
    WindowCursor::Initialize(FixSettings());
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    bool bFixHUD = settings.Bool("MAIN.FixHUD");
    bool bRandomSongOrderFix = settings.Bool("MAIN.RandomSongOrderFix");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (FixSettings().Int("MAIN.WindowMode") == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THUGDisplay::Init(Screen.Width, Screen.Height);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    TonyBink9::Install(&FixSettings().Int("MAIN.FMVWidescreenMode"),
        "81 EC 80 01 00 00 A1 ? ? ? ? 33 C4 89 84 24 7C 01 00 00 56 8B B4 24 88 01 00 00 85 F6",
        "D9 1D ? ? ? ? 8B D1 85 D2 89 54 24 ? DB 44 24 ? 7D ? D8 05 ? ? ? ? 8B 15 ? ? ? ? D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 00 8D 7C 24 ? 57 6A 01 D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 16 6A 00 6A 01 51 D9 1D",
        { 2, 34, 55, 74 });

    //addresses is not based on drm free exe

    [&]()
    {
        hook::pattern pattern("");
        //Fixes (sort of) crash when using high resolutions
        static uint8_t vec[100000];
        pattern = hook::pattern("B9 ? ? ? ? 89 ? 24 14 ? ? 24");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        ProtectedGameRef<uint8_t*>{pattern.get_first<uint8_t*>(1)} = &vec[0];
    }();

    [&]()
    {
        hook::pattern pattern("");
        //Resolution
        pattern = hook::pattern("6A 01 6A 01 ? 8B 15");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        static int32_t* dword_6D3608 = *pattern.get_first<int32_t*>(7);
        static int32_t* dword_6D360C = *pattern.get_first<int32_t*>(-4);
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_6D3608 = Screen.Width;
                *dword_6D360C = Screen.Height;
            }
        };
        pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_6D3608)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D360C)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D3608)));
        if (!pattern.count_hint(1).empty())
            injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_6D360C)));
        if (!pattern.count_hint(1).empty())
            injector::MakeInline<SetResHook>(pattern.get_first(0));
    }();

    [&]()
    {
        hook::pattern pattern("");
        //Aspect Ratio
        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? D9 5C 24 08");
        if (!pattern.count_hint(1).empty())
        {
            ProtectedGameRef<float>{pattern.get_first<float>(1)} = Screen.fAspectRatio;

            pattern = hook::pattern("E8 ? ? ? ? D9 04 24 D8 74 24 04 DE C9");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            ProtectedGameRef<float*>{reinterpret_cast<float**>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2)} = &Screen.fAspectRatio;
        }
        else
        {
            pattern = hook::pattern("D9 05 ? ? ? ? 51 D9 1C 24 E8 ? ? ? ? 6A 00");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            ProtectedGameRef<float>{*pattern.get_first<float*>(2)} = Screen.fAspectRatio;

            pattern = hook::pattern("E8 ? ? ? ? D8 0C 24 D8 74 24 04 D9 5C 24 04 D9 44 24 04 83 C4 08 C3");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            ProtectedGameRef<float*>{reinterpret_cast<float**>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2)} = &Screen.fAspectRatio;
        }
    }();

    [&]()
    {
        hook::pattern pattern("");
        TonyCamera::Install(
            {
                "THUG",
                "D9 1C 24 E8 ? ? ? ? 8B 44 24 30 83 C0 FB 83 F8 07 77 ? FF 24 85",
                3, 0x1c,
                "C7 06 ? ? ? ? E8 ? ? ? ? D9 9E A0 00 00 00 8B CE E8",
                "8B F9 83 BF 58 01 00 00 00 0F 84 ? ? ? ? 8B 87 60 01 00 00 85 C0 74 ? 8B 40 2C",
                "E8 ? ? ? ? D8 4C 24 04 D9 5C 24 04 D9 44 24 04 E8 ? ? ? ? D9 5C 24 04",
                0xa0, 0x0, 0x158, 0x2b8, 0x160, 0x114
            }, &FixSettings().Float("MAIN.FOVFactor"), true);
        //FOV
        pattern = hook::pattern("0D ? ? ? ? D9 9E ? ? ? ? 5E 59 C3");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        struct FovHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float fov = 0.0f;
                _asm {fstp dword ptr[fov]}
                *reinterpret_cast<float*>(regs.esi + 0xA4) = TonyCamera::Correct(regs.esi, fov, Screen.fAspectRatio);
            }
        };
        if (pattern.size() == 1)
            injector::MakeInline<FovHook>(pattern.get_first(5), pattern.get_first(5 + 6));
        else
            OutputDebugStringA("THUG: camera FOV pattern unavailable.\n");
    }();

    [&]()
    {
        hook::pattern pattern("");
        //HUD
        if (bFixHUD)
        {
            THUGHud::Install(Screen.fWidth, Screen.fHeight, &settings.String("MAIN.HudAspectRatioConstraint"));
            static int32_t nHudOffset = static_cast<int32_t>(Screen.fHudOffsetReal);
            static int32_t* dword_72DFC4;
            struct SetOffsetHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *dword_72DFC4 = nHudOffset;
                }
            };

            pattern = hook::pattern("D8 0D ? ? ? ? 03 C2 C1 F8 02");
            if (!pattern.count_hint(1).empty())
            {
                **pattern.get_first<float*>(2) = Screen.fHUDScaleX;
                auto required1 = hook::pattern("DA 05 ? ? ? ? 0F BF 88");
                if (required1.empty())
                {
                    OutputDebugStringA("TonyHawksUnderground.WidescreenFix: required patch pattern unavailable.\n");
                    return;
                }
                dword_72DFC4 = *required1.get_first<int32_t*>(2);
            }
            else
            {
                Screen.fHUDScaleX = 1.0f / Screen.fHUDScaleX;
                pattern = hook::pattern("DC 0D ? ? ? ? DE FB D9");
                if (pattern.size() < 1)
                {
                    OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                    return;
                }
                **pattern.get_first<double*>(2) = Screen.fHUDScaleX;

                auto required2 = hook::pattern("DA 05 ? ? ? ? 89 4C 24 20 89 54 24 18");
                if (required2.empty())
                {
                    OutputDebugStringA("TonyHawksUnderground.WidescreenFix: required patch pattern unavailable.\n");
                    return;
                }
                dword_72DFC4 = *required2.get_first<int32_t*>(2);
            }

            pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_72DFC4)));
            for (size_t i = 0; i < pattern.size(); ++i)
                injector::MakeInline<SetOffsetHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));
        }
    }();

    [&]()
    {
        hook::pattern pattern("");
        // Without the launcher's saved controls the keyboard has no bindings at all; use the launcher's defaults then
        pattern = hook::pattern("68 60 02 00 00 6A FF 68 ? ? ? ? A3");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        auto readControls = hook::pattern("E8 ? ? ? ? 8B 4C 24 2C 83 C4 28 6A 00");
        if (pattern.size() == 1 && readControls.size() == 1)
        {
            static int32_t* keyboardMap = *pattern.get_first<int32_t*>(8);
            static auto defaultKeysHook = safetyhook::create_mid(readControls.get_first(5), [](SafetyHookContext&)
            {
                static constexpr int32_t defaults[24] = { 17, 31, 30, 32, 44, 45, 46, 47, 80, 77, 75, 72, -1, 15, 79, 81, 71, 73, 82, 83, 199, 207, 211, 209 };
                if (std::all_of(keyboardMap, keyboardMap + 24, [](int32_t key) { return key == -1; }))
                    std::copy(std::begin(defaults), std::end(defaults), keyboardMap);
            });
        }
    }();

    [&]()
    {
        hook::pattern pattern("");
        if (bRandomSongOrderFix)
        {
            pattern = hook::pattern("83 C4 08 ? D0 07 00 00"); //0x424BDD or 423CDB
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(110), "75 ? A1");
            if (rpattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            static auto dword_69B718 = *rpattern.get_first<int32_t*>(3);
            auto required1 = hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(60), "A1 ? ? ? ?");
            if (required1.empty())
            {
                OutputDebugStringA("TonyHawksUnderground.WidescreenFix: required range pattern unavailable.\n");
                return;
            }
            static auto dword_681D14 = *required1.get_first<int32_t*>(1);
            struct RandomHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto num_songs = *dword_681D14;
                    int32_t* sp_xbox_randomized_songs = dword_69B718;

                    std::vector<int32_t> songs;
                    songs.assign(std::addressof(sp_xbox_randomized_songs[0]), std::addressof(sp_xbox_randomized_songs[num_songs]));
                    std::mt19937 r{ std::random_device{}() };
                    std::shuffle(std::begin(songs), std::end(songs), r);

                    std::copy(songs.begin(), songs.end(), sp_xbox_randomized_songs);
                }
            }; injector::MakeInline<RandomHook>(pattern.get_first(3), rpattern.get_first(2));
        }
    }();

    THUGMenu::Init();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("6A 01 6A 01 ? 8B 15"));
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
        THUGMenu::Shutdown(lpReserved != nullptr);
        WindowCursor::Shutdown();
    }
    return TRUE;
}
