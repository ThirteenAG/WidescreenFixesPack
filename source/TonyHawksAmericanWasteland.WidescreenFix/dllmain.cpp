#include "stdafx.h"
#include "FixSettings.hpp"
#include "NativeMenu.hpp"
#include "Hud.hpp"
#include "../Shared/TonyHawk/Display.hpp"
#include "../Shared/TonyHawk/Camera.hpp"
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
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

namespace THAWDisplay
{
    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        WindowedModeWrapper::EnableDpiAwareness();
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            auto pattern = hook::pattern("A0 ? ? ? ? 8B 3D ? ? ? ? 33 C9 84 C0 0F 94 C1 B8 03 00 00 00 2B C7");
            if (pattern.size() == 1)
            {
                static uint8_t* NativeWindowed = *pattern.get_first<uint8_t*>(1);
                static auto windowedHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext&)
                {
                    *NativeWindowed = 0;
                });
                TonyDisplay::Window(mode, width, height);
            }
            else
                OutputDebugStringA("THAW: native window mode pattern unavailable.\n");
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
    THAWMenu::Init();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    bool bFixHUD = settings.Bool("MAIN.FixHUD");
    bool bFixFOV = settings.Bool("MAIN.FixFOV");
    bool bRandomSongOrderFix = settings.Bool("MAIN.RandomSongOrderFix");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (FixSettings().Int("MAIN.WindowMode") == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THAWDisplay::Init(Screen.Width, Screen.Height);

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
        //The native writer starts four bytes before ECX and emits three DWORDs per entry.
        static std::vector<uint32_t> resolutionTable;
        pattern = hook::pattern("B9 ? ? ? ? 89 74 24 14 8B FF 8B C3 99 F7 FE"); //0x50D195
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        struct ResolutionTableHook
        {
            void operator()(injector::reg_pack& regs)
            {
                resolutionTable.resize(static_cast<size_t>(regs.esi) * 3 + 1);
                regs.ecx = reinterpret_cast<uint32_t>(resolutionTable.data() + 1);
            }
        }; injector::MakeInline<ResolutionTableHook>(pattern.get_first(), pattern.get_first(5));
    }();

    [&]()
    {
        hook::pattern pattern("");
        //Resolution
        pattern = hook::pattern("89 0D ? ? ? ? 89 0D ? ? ? ? 8A 0D ? ? ? ? 84 C9");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        static int32_t* dword_6D2224 = *pattern.get_first<int32_t*>(2);
        auto required1 = hook::pattern("89 3D ? ? ? ? 89 3D ? ? ? ? 89 15 ? ? ? ? 89 15 ? ? ? ? 74");
        if (required1.empty())
        {
            OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: required patch pattern unavailable.\n");
            return;
        }
        static int32_t* dword_6D2228 = *required1.get_first<int32_t*>(2);
        static int32_t* dword_7F3A2C = *pattern.get_first<int32_t*>(8);
        auto required2 = hook::pattern("89 3D ? ? ? ? 89 3D ? ? ? ? 89 15 ? ? ? ? 89 15 ? ? ? ? 74");
        if (required2.empty())
        {
            OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: required patch pattern unavailable.\n");
            return;
        }
        static int32_t* dword_7F3A30 = *required2.get_first<int32_t*>(8);
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_6D2224 = Screen.Width;
                *dword_6D2228 = Screen.Height;
                *dword_7F3A2C = Screen.Width;
                *dword_7F3A30 = Screen.Height;
            }
        };
        pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_6D2224)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D2224)));
        if (!pattern.count_hint(1).empty())
            injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D2228)));
        if (pattern.size() < 2)
        {
            OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeInline<SetResHook>(pattern.count_hint(2).get(0).get<void*>(0), pattern.count_hint(2).get(0).get<void*>(6));
        injector::MakeInline<SetResHook>(pattern.count_hint(2).get(1).get<void*>(0), pattern.count_hint(2).get(1).get<void*>(6));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_7F3A2C)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_7F3A30)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));
    }();

    [&]()
    {
        hook::pattern pattern("");
        //Aspect Ratio
        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? D9");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        ProtectedGameRef<float>{pattern.get_first<float>(1)} = Screen.fAspectRatio;
        pattern = hook::pattern("E8 ? ? ? ? D8 0D ? ? ? ? D8 7C 24");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        ProtectedGameRef<float*>{reinterpret_cast<float**>(injector::GetBranchDestination(pattern.get_first()).as_int() + 2)} = &Screen.fAspectRatio;
    }();

    [&]()
    {
        hook::pattern pattern("");
        TonyCamera::Install(
            {
                "THAW",
                "F3 0F 11 24 24 8B 44 24 14 8B 48 18 E8 ? ? ? ? 8B 44 24 2C 83 C0 FB 83 F8 07 77 ? FF 24 85",
                12, 0x0,
                "C7 06 ? ? ? ? F3 0F 11 86 D4 00 00 00 E8 ? ? ? ? D9 96 C0 00 00 00",
                "8B F1 8B 86 84 01 00 00 85 C0 0F 84 ? ? ? ? 8B 86 8C 01 00 00 85 C0 74 ? 8B 40 2C",
                "E8 ? ? ? ? D9 44 24 04 8B CE D9 F2 DD D8 D9 54 24 04 D8 B6 D4 00 00 00 DE C9",
                0xc0, 0xd4, 0x184, 0xd8, 0x18c, 0x130
            }, &FixSettings().Float("MAIN.FOVFactor"), bFixFOV);
        pattern = hook::pattern("D9 9E C4 00 00 00 E8 ? ? ? ? D9 86 C4 00 00 00 5E 59 C3");
        if (pattern.size() == 1)
        {
            struct FovHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float fov = 0.0f;
                    _asm { fstp dword ptr[fov] }
                    *reinterpret_cast<float*>(regs.esi + 0xC4) = TonyCamera::Correct(regs.esi, fov, Screen.fAspectRatio);
                }
            };
            injector::MakeInline<FovHook>(pattern.get_first(), pattern.get_first(6));
        }
        else
            OutputDebugStringA("THAW: camera FOV pattern unavailable.\n");
    }();

    [&]()
    {
        hook::pattern pattern("");
        //HUD
        if (bFixHUD)
        {
            THAWHud::Install(Screen.fWidth, Screen.fHeight, &settings.String("MAIN.HudAspectRatioConstraint"));
            static int32_t nHudOffset = static_cast<int32_t>(Screen.fHudOffsetReal);
            static int32_t* dword_787D88;
            struct SetOffsetHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *dword_787D88 = nHudOffset;
                }
            };

            pattern = hook::pattern("D8 0D ? ? ? ? 8B 35 ? ? ? ? 85 F6 D9 1D");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fHUDScaleX;
            auto required1 = hook::pattern("89 1D ? ? ? ? C7 05 ? ? ? ? 10 00 00 00");
            if (required1.empty())
            {
                OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: required patch pattern unavailable.\n");
                return;
            }
            dword_787D88 = *required1.get_first<int32_t*>(2);

            pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_787D88)));
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            injector::MakeInline<SetOffsetHook>(pattern.get_first(0), pattern.get_first(6));

            pattern = hook::pattern(pattern_str(0xC7, '?', to_bytes(dword_787D88)));
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            injector::MakeInline<SetOffsetHook>(pattern.get_first(0), pattern.get_first(10));
        }
    }();

    [&]()
    {
        hook::pattern pattern("");
        if (bRandomSongOrderFix)
        {
            pattern = hook::pattern("BD D0 07 00 00");
            if (pattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(0), (uintptr_t)pattern.get_first(110), "75 ? A1");
            if (rpattern.size() < 1)
            {
                OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
                return;
            }
            static auto dword_69B718 = *rpattern.get_first<int32_t*>(3);
            auto required1 = hook::range_pattern((uintptr_t)pattern.get_first(0), (uintptr_t)pattern.get_first(60), "8B 15 ? ? ? ?");
            if (required1.empty())
            {
                OutputDebugStringA("TonyHawksAmericanWasteland.WidescreenFix: required range pattern unavailable.\n");
                return;
            }
            static auto dword_681D14 = *required1.get_first<int32_t*>(2);
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
            }; injector::MakeInline<RandomHook>(pattern.get_first(0), rpattern.get_first(2));
        }
    }();


}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("89 0D ? ? ? ? 89 0D ? ? ? ? 8A 0D ? ? ? ? 84 C9"));
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
