#include "stdafx.h"
#include "FixSettings.hpp"
#include "Hud.hpp"
#include "../Shared/TonyHawk/Display.hpp"
#include "../Shared/TonyHawk/Bink9.hpp"
#include "../Shared/TonyHawk/Camera.hpp"
#include "NativeMenu.hpp"

ASI_SETTINGS_EXPORT(FixSettings())
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

namespace THUG2Display
{
    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        WindowedModeWrapper::EnableDpiAwareness();
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            auto pattern = hook::pattern("A3 ? ? ? ? B8 01 00 00 00 89 1D ? ? ? ? A3 ? ? ? ? 89 74 24 10 76");
            if (pattern.size() == 1)
            {
                static int* NativeWindowed = *pattern.get_first<int*>(12);
                static auto windowedHook = safetyhook::create_mid(pattern.get_first(16), [](SafetyHookContext&)
                {
                    *NativeWindowed = 1;
                });
                TonyDisplay::Window(mode, width, height);
            }
            else
                OutputDebugStringA("THUG2: native window mode pattern unavailable.\n");
        }
        if (settings.Bool("MAIN.SkipIntro"))
            TonyDisplay::Intro("51 8B 4C 24 08 6A 01 8D 44 24 04 50 6A 00 E8 ? ? ? ? 84 C0 74 10 8B 0C 24 51 E8 ? ? ? ? 83 C4 04 B0 01 59 C3 32 C0 59 C3", 27, 32);
    }
}

void Init()
{
    WindowCursor::Initialize(FixSettings());
    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    bool bFixHUD = FixSettings().Bool("MAIN.FixHUD");
    bool bFixFOV = FixSettings().Bool("MAIN.FixFOV");
    bool bRandomSongOrderFix = FixSettings().Bool("MAIN.RandomSongOrderFix");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (FixSettings().Int("MAIN.WindowMode") == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THUG2Display::Init(Screen.Width, Screen.Height);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    TonyBink9::Install(&FixSettings().Int("MAIN.FMVWidescreenMode"),
        "81 EC 74 01 00 00 A1 ? ? ? ? 56 8B B4 24 7C 01 00 00 85 F6 89 84 24 74 01 00 00",
        "D9 1D ? ? ? ? 85 C9 DB 44 24 ? 7D ? D8 05 ? ? ? ? 8B 15 ? ? ? ? D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 00 8D 7C 24 ? 57 6A 01 D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 16 6A 00 6A 01 51 D9 1D",
        { 2, 28, 49, 68 });

    [&]()
    {
        hook::pattern pattern("");
        //Resolution
        pattern = hook::pattern("89 0D ? ? ? ? 89 15 ? ? ? ? 89 0D ? ? ? ? 89 15 ? ? ? ? 74");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        static int32_t* dword_67FBCC = *pattern.get_first<int32_t*>(2);
        static int32_t* dword_67FBD0 = *pattern.get_first<int32_t*>(8);
        static int32_t* dword_787A7C = *pattern.get_first<int32_t*>(12 + 2);
        static int32_t* dword_787A80 = *pattern.get_first<int32_t*>(12 + 8);
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_67FBCC = Screen.Width;
                *dword_67FBD0 = Screen.Height;
                *dword_787A7C = Screen.Width;
                *dword_787A80 = Screen.Height;
            }
        };

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_67FBCC)));
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));
        pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_67FBCC)));
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeInline<SetResHook>(pattern.get_first(0));
        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_67FBD0)));
        if (pattern.size() < 2)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeInline<SetResHook>(pattern.count_hint(2).get(0).get<void*>(0), pattern.count_hint(2).get(0).get<void*>(6));
        injector::MakeInline<SetResHook>(pattern.count_hint(2).get(1).get<void*>(0), pattern.count_hint(2).get(1).get<void*>(6));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_787A7C)));
        if (pattern.size() < 2)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeNOP(pattern.count_hint(2).get(0).get<void*>(0), 6, true); //injector::MakeInline<SetResHook>(pattern.count_hint(2).get(0).get<void*>(0), pattern.count_hint(2).get(0).get<void*>(6));
        injector::MakeNOP(pattern.count_hint(2).get(1).get<void*>(0), 6, true); //injector::MakeInline<SetResHook>(pattern.count_hint(2).get(1).get<void*>(0), pattern.count_hint(2).get(1).get<void*>(6));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_787A80)));
        if (pattern.size() < 2)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeNOP(pattern.count_hint(2).get(0).get<void*>(0), 6, true); //injector::MakeInline<SetResHook>(pattern.count_hint(2).get(0).get<void*>(0), pattern.count_hint(2).get(0).get<void*>(6));
        injector::MakeNOP(pattern.count_hint(2).get(1).get<void*>(0), 6, true); //injector::MakeInline<SetResHook>(pattern.count_hint(2).get(1).get<void*>(0), pattern.count_hint(2).get(1).get<void*>(6));
    }();

    [&]()
    {
        hook::pattern pattern("");
        //Aspect Ratio
        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? D9");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        ProtectedGameRef<float>(pattern.get_first<float>(1)) = Screen.fAspectRatio;
        pattern = hook::pattern("E8 ? ? ? ? D9 04 24 D8 74 24 04 DE C9");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        ProtectedGameRef<float*>(reinterpret_cast<float**>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2)) = &Screen.fAspectRatio;
    }();

    [&]()
    {
        hook::pattern pattern("");
        // The loading gradient writes three DWORDs per entry. Its starting ECX is four bytes past the first
        // destination; size the replacement for the actual loop count instead of overrunning a fixed buffer.
        pattern = hook::pattern("B9 ? ? ? ? 89 ? 24 14 ? ? 24");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        struct LoadingGradientBufferHook
        {
            void operator()(injector::reg_pack& regs)
            {
                static std::vector<uint32_t> gradient;
                gradient.resize(static_cast<size_t>(regs.esi) * 3 + 1);
                regs.ecx = reinterpret_cast<uint32_t>(gradient.data() + 1);
            }
        }; injector::MakeInline<LoadingGradientBufferHook>(pattern.get_first(), pattern.get_first(5));
    }();

    [&]()
    {
        hook::pattern pattern("");
        // FOV and its native projection/culling scale use the same corrected angle.
        TonyCamera::Install(
            {
                "THUG2",
                "8B 54 24 20 8B 4A 18 E8 ? ? ? ? 8B 44 24 38 83 C0 FB 83 F8 07 77 ? FF 24 85",
                7, 0x0,
                "C7 06 ? ? ? ? 89 BE C0 00 00 00 E8 ? ? ? ? D9 9E AC 00 00 00 8B CE E8",
                "8B F1 8B 86 70 01 00 00 33 ED 3B C5 0F 84 ? ? ? ? 8B 86 78 01 00 00 3B C5 74 ? 8B 40 2C",
                "E8 ? ? ? ? D9 44 24 04 D9 F2 DD D8 D9 54 24 04 D8 B6 C0 00 00 00 DE C9",
                0xac, 0xc0, 0x170, 0xbc, 0x178, 0x118
            }, &FixSettings().Float("MAIN.FOVFactor"), bFixFOV);
        pattern = hook::pattern("D9 96 B0 00 00 00 D8 0D ? ? ? ? D9 F2 DD D8");
        if (pattern.size() == 1)
        {
            struct FovHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float fov = 0.0f;
                    _asm { fstp dword ptr[fov] }
                    fov = TonyCamera::Correct(regs.esi, fov, Screen.fAspectRatio);
                    *reinterpret_cast<float*>(regs.esi + 0xB0) = fov;
                    _asm { fld dword ptr[fov] }
                }
            };
            injector::MakeInline<FovHook>(pattern.get_first(), pattern.get_first(6));
        }
        else
            OutputDebugStringA("THUG2: camera FOV pattern unavailable.\n");
    }();

    [&]()
    {
        hook::pattern pattern("");
        //HUD
        if (bFixHUD)
        {
            THUG2Hud::Install(Screen.fWidth, Screen.fHeight, &FixSettings().String("MAIN.HudAspectRatioConstraint"));
            static int32_t nHudOffset = static_cast<int32_t>(Screen.fHudOffsetReal);
            static int32_t* dword_787D88;
            struct SetOffsetHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *dword_787D88 = nHudOffset;
                }
            };

            pattern = hook::pattern("D8 0D ? ? ? ? 89 0D ? ? ? ? 89 15 ? ? ? ? 0F B6 C8");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            **pattern.get_first<float*>(2) = Screen.fHUDScaleX;
            auto required1 = hook::pattern("DB 05 ? ? ? ? 57 D9 05 ? ? ? ? 8B F9");
            if (required1.empty())
            {
                OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: required patch pattern unavailable.\n");
                return;
            }
            dword_787D88 = *required1.get_first<int32_t*>(2);

            pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_787D88)));
            for (size_t i = 0; i < pattern.size(); ++i)
                injector::MakeInline<SetOffsetHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));
        }
    }();

    [&]()
    {
        hook::pattern pattern("");
        // Without the launcher's saved controls the keyboard has no bindings at all; use the launcher's defaults then
        pattern = hook::pattern("83 C8 FF B9 98 00 00 00 BF ? ? ? ? F3 AB");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        auto readControls = hook::pattern("E8 ? ? ? ? 8B 44 24 0C 83 C4 04 6A 00");
        if (pattern.size() == 1 && readControls.size() == 1)
        {
            static int32_t* keyboardMap = *pattern.get_first<int32_t*>(9);
            static auto defaultKeysHook = safetyhook::create_mid(readControls.get_first(5), [](SafetyHookContext&)
            {
                static constexpr int32_t defaults[24] = { 17, 31, 30, 32, 44, 45, 46, 47, 80, 77, 75, 72, -1, 15, 79, 81, 71, 73, 82, -1, 199, 207, 211, 209 };
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
            pattern = hook::pattern("83 C4 08 ? D0 07 00 00");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(110), "75 ? A1");
            if (rpattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            static auto dword_69B718 = *rpattern.get_first<int32_t*>(3);
            auto required1 = hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(60), "A1 ? ? ? ?");
            if (required1.empty())
            {
                OutputDebugStringA("TonyHawksUnderground2.WidescreenFix: required range pattern unavailable.\n");
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

    THUG2Menu::Init();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("89 0D ? ? ? ? 89 15 ? ? ? ? 89 0D ? ? ? ? 89 15 ? ? ? ? 74"));
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
