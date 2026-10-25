#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include "NativeMenu.hpp"
#include <sstream>

import ComVars;
import Settings;
import XeTexturePacker;
import RawInput;
import FusionDxHook;
import Framelimit;
import GameSpeedFix;
import WinMM;
import Xidi;

SafetyHookInline shsub_A2A350 = {};
void __fastcall sub_A2A350(void* _this, void* edx, int a2, int a3, void* a4)
{
    if (Screen.pBarsPtr && *reinterpret_cast<uintptr_t*>(Screen.pBarsPtr) &&
        (*reinterpret_cast<uint32_t*>(*reinterpret_cast<uintptr_t*>(Screen.pBarsPtr) + 0xCC) == 3))
        return shsub_A2A350.unsafe_fastcall(_this, edx, a2, a3, a4);
}

void* retAddrFading = nullptr;
bool bSkipFOVHook = false;
SafetyHookInline shsub_9B1C80 = {};
void __cdecl sub_9B1C80(float* a1)
{
    auto raddr = _ReturnAddress();
    if (raddr == retAddrFading)
        bSkipFOVHook = true;

    shsub_9B1C80.unsafe_ccall(a1);
    bSkipFOVHook = false;
}

// Startup logo videos: opening them fails, which the game treats as a finished video.
SafetyHookInline shBinkOpen;
void* __stdcall BinkOpenHook(const char* name, uint32_t flags)
{
    constexpr uint32_t BINKFILEHANDLE = 0x00800000;
    if (name && !(flags & BINKFILEHANDLE))
    {
        std::string_view file{ name };
        if (auto slash = file.find_last_of("\\/"); slash != file.npos)
            file.remove_prefix(slash + 1);
        file = file.substr(0, file.find('.'));
        for (auto logo : { "mov1", "mov2", "mov3", "mov4" })
        {
            if (file.size() == std::strlen(logo) && std::equal(file.begin(), file.end(), logo, [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }))
                return nullptr;
        }
    }
    return shBinkOpen.stdcall<void*>(name, flags);
}

void InstallLogoSkip()
{
    if (shBinkOpen || !FixSettings().Bool("MAIN.SkipIntro"))
        return;
    if (auto bink = GetModuleHandleW(L"binkw32.dll"))
    {
        if (auto open = GetProcAddress(bink, "_BinkOpen@8"))
            shBinkOpen = safetyhook::create_inline(open, BinkOpenHook);
    }
}

void FitFramedClient()
{
    static thread_local bool arranging = false;
    const auto window = WindowedModeWrapper::GameHWND;
    const int width = WindowedModeWrapper::desiredClientWidth;
    const int height = WindowedModeWrapper::desiredClientHeight;
    if (arranging || WindowedModeWrapper::bBorderlessWindowed || !IsWindow(window) || width <= 0 || height <= 0)
        return;
    RECT client{};
    if (!GetClientRect(window, &client) || (client.right == width && client.bottom == height))
        return;
    arranging = true;
    RECT frame{ 0, 0, width, height };
    const auto style = static_cast<DWORD>(GetWindowLongW(window, GWL_STYLE));
    const auto exStyle = static_cast<DWORD>(GetWindowLongW(window, GWL_EXSTYLE));
    WindowedModeWrapper::AdjustFrame(&frame, style, FALSE, exStyle);
    const auto monitor = WindowedModeWrapper::WindowMonitor();
    const int x = monitor.rcMonitor.left + (monitor.rcMonitor.right - monitor.rcMonitor.left - width) / 2;
    const int y = monitor.rcMonitor.top + (monitor.rcMonitor.bottom - monitor.rcMonitor.top - height) / 2;
    SetWindowPos(window, nullptr, x + frame.left, y + frame.top, frame.right - frame.left,
        frame.bottom - frame.top, SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
    arranging = false;
}

BOOL WINAPI SetGameWindowPos(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
{
    const auto result = WindowedModeWrapper::SetWindowPos_Hook(window, after, x, y, width, height, flags);
    if (result && window == WindowedModeWrapper::GameHWND)
        FitFramedClient();
    return result;
}

BOOL WINAPI MoveGameWindow(HWND window, int x, int y, int width, int height, BOOL repaint)
{
    const auto result = WindowedModeWrapper::MoveWindow_Hook(window, x, y, width, height, repaint);
    if (result && window == WindowedModeWrapper::GameHWND)
        FitFramedClient();
    return result;
}

void Init()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    InstallLogoSkip();
    static bool bCustomAR;
    auto szForceAspectRatio = settings.String("MAIN.ForceAspectRatio");
    static const auto& fmvMode = settings.Int("MAIN.FMVWidescreenMode");
    auto updateMouseSensitivity = []()
    {
        const float value = FixSettings().Float("MAIN.MouseSensitivity");
        Screen.fRawInputMouse = FixSettings().Bool("MAIN.RawInputMouse")
            ? (std::isfinite(value) ? std::clamp(value, 0.1f, 5.0f) : 1.0f) : 0.0f;
    };
    updateMouseSensitivity();
    settings.OnChange("MAIN.MouseSensitivity", updateMouseSensitivity);
    static const auto& fFOVFactor = settings.Float("MAIN.FOVFactor");
    static const int windowMode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
    static const bool bWindowedMode = windowMode != 0;
    WindowedModeWrapper::EnableDpiAwareness();
    WindowedModeWrapper::bBorderlessWindowed = windowMode == 2;
    WindowedModeWrapper::bStretchWindow = windowMode == 2;

    if (szForceAspectRatio != "auto")
    {
        std::istringstream ratio(szForceAspectRatio);
        float horizontal = 0.0f, vertical = 0.0f;
        char separator = 0;
        if ((ratio >> horizontal >> separator >> vertical) && separator == ':' &&
            (ratio >> std::ws).eof() && std::isfinite(horizontal) && std::isfinite(vertical) &&
            horizontal > 0.0f && vertical > 0.0f && std::isfinite(horizontal / vertical) && horizontal / vertical > 0.0f)
        {
            Screen.fCustomAspectRatioHor = horizontal;
            Screen.fCustomAspectRatioVer = vertical;
            bCustomAR = true;
        }
    }

    if (bWindowedMode)
    {
        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
            std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
            std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
            std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
            std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
            std::forward_as_tuple("SetWindowPos", SetGameWindowPos),
            std::forward_as_tuple("MoveWindow", MoveGameWindow)
        );
    }



    static auto WindowResolution = [](int width, int height)
    {
        if (windowMode == 2)
        {
            MONITORINFOEX info = { sizeof(info) };
            auto window = WindowedModeWrapper::GameHWND;
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &info);
            DEVMODE mode = { .dmSize = sizeof(mode) };
            if (EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &mode))
            {
                width = static_cast<int>(mode.dmPelsWidth);
                height = static_cast<int>(mode.dmPelsHeight);
            }
        }
        if (width <= 0 || height <= 0)
            std::tie(width, height) = GetDesktopRes();
        if (width <= 0 || height <= 0)
            std::tie(width, height) = std::make_tuple(640, 480);
        WindowedModeWrapper::desiredClientWidth = width;
        WindowedModeWrapper::desiredClientHeight = height;
        return std::pair{ width, height };
    };

    static auto SetScreenVars = [](int width, int height)
    {
        Screen.Width = width;
        Screen.Height = height;
        Screen.fWidth = static_cast<float>(Screen.Width);
        Screen.fHeight = static_cast<float>(Screen.Height);
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
        Screen.fWidth43 = static_cast<float>(Screen.Width43);
        Screen.fHudOffset = (1.0f / (Screen.fHeight * (4.0f / 3.0f))) * ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        Screen.fHudScale = Screen.fHeight / Screen.fWidth;
        Screen.fFieldOfView = 1.0f * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
        Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
        Screen.fFMVScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));

        if (bCustomAR)
        {
            Screen.fAspectRatio = Screen.fCustomAspectRatioHor / Screen.fCustomAspectRatioVer;
            Screen.fHudScale = Screen.fCustomAspectRatioVer / Screen.fCustomAspectRatioHor;
            Screen.fFieldOfView = 1.0f * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
        }

        const float factor = std::isfinite(fFOVFactor) ? std::clamp(fFOVFactor, 0.5f, 2.0f) : 1.0f;
        Screen.fFieldOfView /= factor;
        Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
    };

    settings.OnChange("MAIN.FOVFactor", []()
    {
        if (Screen.Width > 0 && Screen.Height > 0)
            SetScreenVars(Screen.Width, Screen.Height);
    });

    pattern = hook::pattern("89 86 BC 02 00 00 89 8E C0 02 00 00 89 96 C4 02 00 00"); //0x9F2161
    if (!pattern.empty()) // Gamer's Edition
    {
        struct ResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto [width, height] = WindowResolution(regs.eax, regs.ecx);
                regs.eax = width;
                regs.ecx = height;

                *(uint32_t*)(regs.esi + 0x2BC) = width;
                *(uint32_t*)(regs.esi + 0x2C0) = height;
                *(uint32_t*)(regs.esi + 0x2C4) = regs.edx;

                if (bWindowedMode)
                    *(uint32_t*)(regs.esi + 0x2C8) = 1;

                SetScreenVars(width, height);
            }
        }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(18));
    }
    else
    {
        pattern = hook::pattern("8B 46 64 8B 4E 68");
        if (!pattern.empty()) // 9d
        {
            struct ResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto [width, height] = WindowResolution(*reinterpret_cast<int*>(regs.esi + 0x64), *reinterpret_cast<int*>(regs.esi + 0x68));
                    regs.eax = width;
                    regs.ecx = height;
                    *reinterpret_cast<int*>(regs.esi + 0x64) = width;
                    *reinterpret_cast<int*>(regs.esi + 0x68) = height;
                    SetScreenVars(width, height);
                }
            }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));

            if (bWindowedMode)
            {
                if (wfp::PatchPreflight("King Kong Direct3D 9 window mode: signatures unavailable; feature disabled.\n",
                {
                    { "75 ? 8B 4E ? 8B 56 ? 8D 44 24", 1 },
                }))
                {
                    pattern = hook::pattern("75 ? 8B 4E ? 8B 56 ? 8D 44 24");
                    ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(0)} = 0xEB;
                }


            }
        }
        else
        {
            pattern = hook::pattern("8B 56 68 8B 44 24 44");
            if (!pattern.empty()) // 9
            {
                struct ResHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = *(uint32_t*)(regs.esp + 0x44);

                        auto [width, height] = WindowResolution(*reinterpret_cast<int*>(regs.esi + 0x64), *reinterpret_cast<int*>(regs.esi + 0x68));
                        regs.ecx = width;
                        regs.edx = height;
                        *reinterpret_cast<int*>(regs.esi + 0x64) = width;
                        *reinterpret_cast<int*>(regs.esi + 0x68) = height;
                        SetScreenVars(width, height);
                    }
                }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(10));

                if (wfp::PatchPreflight("King Kong window mode: signatures unavailable; feature disabled.\n",
                {
                    { "8B 46 ? 8D 4C 24 ? 51 8B 4E", 1 },
                }))
                {
                    pattern = hook::pattern("8B 46 ? 8D 4C 24 ? 51 8B 4E");
                    static auto WindowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                    {
                        if (bWindowedMode)
                        {
                            *(uint32_t*)(regs.esi + 0x10) = 1;
                            regs.eax = 1;
                        }
                    });
                }


            }
            else
            {
                pattern = hook::pattern("8B 4E ? 8B 56 ? 53");
                if (!pattern.empty()) // 8
                {
                    struct ResHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            auto [width, height] = WindowResolution(*reinterpret_cast<int*>(regs.esi + 0x64), *reinterpret_cast<int*>(regs.esi + 0x68));
                            regs.ecx = width;
                            regs.edx = height;
                            *reinterpret_cast<int*>(regs.esi + 0x64) = width;
                            *reinterpret_cast<int*>(regs.esi + 0x68) = height;
                            SetScreenVars(width, height);
                        }
                    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));
                }

                if (wfp::PatchPreflight("King Kong legacy window mode: signatures unavailable; feature disabled.\n",
                {
                    { "8B 46 ? 8D 54 24 ? 52 8B 16", 1 },
                }))
                {
                    pattern = hook::pattern("8B 46 ? 8D 54 24 ? 52 8B 16");
                    static auto WindowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                    {
                        if (bWindowedMode)
                        {
                            *(uint32_t*)(regs.esi + 0x10) = 1;
                            regs.eax = 1;
                        }
                    });
                }


            }
        }
    }

    pattern = hook::pattern("D9 04 8D ? ? ? ? D9 5C 24 18 EB 0A 8B 90 B8 00 00 00");
    if (!pattern.empty())
    {
        auto dword_temp = *pattern.get_first<void*>(3);
        pattern = hook::pattern(pattern_str(0xD9, 0x04, '?', to_bytes(dword_temp))); //0xA01C67
        for (size_t i = 0; i < pattern.size(); i++)
        {
            injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 1, true);
            ProtectedGameRef<uint16_t>{pattern.get(i).get<uint16_t>(1)} = 0x05D9;
            ProtectedGameRef<float*>{pattern.get(i).get<float*>(3)} = &Screen.fHudScale;
        }
    }
    else
    {
        if (wfp::PatchPreflight("King Kong HUD: signatures unavailable; feature disabled.\n",
        {
            { "8B 14 8D ? ? ? ? 89 54 24", 1 },
            { "D8 0C 95 ? ? ? ? E8", 1 },
        }))
        {
            pattern = hook::pattern("8B 14 8D ? ? ? ? 89 54 24");
            auto dword_temp = *pattern.get_first<void*>(3);
            ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(0)} = 0x90;
            ProtectedGameRef<uint16_t>{pattern.get_first<uint16_t>(1)} = 0x158B;
            ProtectedGameRef<float*>{pattern.get_first<float*>(3)} = &Screen.fHudScale;

            pattern = hook::pattern(pattern_str(0xD9, 0x04, '?', to_bytes(dword_temp)));
            for (size_t i = 0; i < pattern.size(); i++)
            {
                injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 1, true);
                ProtectedGameRef<uint16_t>{pattern.get(i).get<uint16_t>(1)} = 0x05D9;
                ProtectedGameRef<float*>{pattern.get(i).get<float*>(3)} = &Screen.fHudScale;
            }

            pattern = hook::pattern("D8 0C 95 ? ? ? ? E8");
            ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(0)} = 0x90;
            ProtectedGameRef<uint16_t>{pattern.get_first<uint16_t>(1)} = 0x0DD8;
            ProtectedGameRef<float*>{pattern.get_first<float*>(3)} = &Screen.fHudScale;
        }


    }

    // FOV
    pattern = find_pattern("83 EC 1C 53 55 56 8B 74 24 ? 8B 46", "A1 ? ? ? ? 81 EC ? ? ? ? 83 F8 03", "A1 ? ? ? ? 81 EC A4 00 00 00 53");
    if (!pattern.empty())
        shsub_9B1C80 = safetyhook::create_inline(pattern.get_first(), sub_9B1C80);
    else
        OutputDebugStringA("King Kong: camera scope hook unavailable.\n");

    pattern = find_pattern("E8 ? ? ? ? D9 44 24 ? A1 ? ? ? ? 8A 88");
    if (!pattern.empty())
    {
        static auto FOVHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
        {
            if (bSkipFOVHook)
                *(float*)(regs.esp + 0x34) = 1.0f / tan(*(float*)(regs.esp + 0x28 + 4) * 0.5f);
            else
                *(float*)(regs.esp + 0x34) = Screen.fFieldOfView / tan(*(float*)(regs.esp + 0x28 + 4) * 0.5f);
        });

        // Some interface elements
        //pattern = hook::pattern("83 C4 04 8B 15 ? ? ? ? 81 C2 D0 00 00 00 52 E8 ? ? ? ? 83 C4 04 A1 ? ? ? ? 05 9C 01 00 00 50 8B 0D ? ? ? ? 81 C1 B8 03 00 00");
        //retAddrMenuInterface = pattern.get_first();
    }
    else
    {
        if (wfp::PatchPreflight("King Kong legacy projection: signatures unavailable; feature disabled.\n",
        {
            { "E8 ? ? ? ? D9 44 24 ? 8B ? ? ? ? ? 8A", 1 },
            { "A1 ? ? ? ? 56 05 C8 00 00 00", 1 },
        }))
        {
            pattern = hook::pattern("E8 ? ? ? ? D9 44 24 ? 8B ? ? ? ? ? 8A");
            static auto offset = *pattern.get_first<int8_t>(-1);
            static auto FOVHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                if (bSkipFOVHook)
                    *(float*)(regs.esp + offset) = 1.0f / *(float*)(regs.esp + 0x18 + 4);
                else
                    *(float*)(regs.esp + offset) = Screen.fFieldOfView / *(float*)(regs.esp + 0x18 + 4);
            });

            // Some interface elements
            pattern = hook::pattern("A1 ? ? ? ? 56 05 C8 00 00 00");
            retAddrFading = pattern.get_first();

            //pattern = hook::pattern("83 C4 04 8B 15 ? ? ? ? 81 C2 C8 00 00 00 52 E8 ? ? ? ? 83 C4 04 A1 ? ? ? ? 05 94 01 00 00 50 8B 0D ? ? ? ? 81 C1 B0 03 00 00");
            //retAddrMenuInterface = pattern.get_first();
        }


    }

    //objects disappear at screen edges with hor+, fixing that @0098B8CF
    if (wfp::PatchPreflight("King Kong object culling: signatures unavailable; feature disabled.\n",
    {
        { "D8 0D ? ? ? ? D9 5C 24 10 8B 5C 24 10 53", 1 },
    }))
    {
        pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 10 8B 5C 24 10 53"); //0x98B8CF
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fCutOffArea;

    }

    //grass at screen edges
    static float f0 = 0.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? D9 C0 D9 FF D9 9C 24");
    if (!pattern.empty())
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &f0;
    else
    {
        if (wfp::PatchPreflight("King Kong grass culling: signatures unavailable; feature disabled.\n",
        {
            { "D8 0D ? ? ? ? D9 9D ? ? ? ? D9 85 ? ? ? ? 83 EC 08 DD 1C 24 E8 ? ? ? ? 83 C4 08 D9 9D ? ? ? ? D9 85 ? ? ? ? D8 3D ? ? ? ? D9 9D ? ? ? ? 8B 85 ? ? ? ? D9 05 ? ? ? ? D9 80 ? ? ? ? DA E9 DF E0 F6 C4 44 7A ? 8B 8D ? ? ? ? C7 81 ? ? ? ? ? ? ? ? 8B 15 ? ? ? ? 89 95 ? ? ? ? A1 ? ? ? ? 6B C0 18", 1 },
        }))
        {
            pattern = hook::pattern("D8 0D ? ? ? ? D9 9D ? ? ? ? D9 85 ? ? ? ? 83 EC 08 DD 1C 24 E8 ? ? ? ? 83 C4 08 D9 9D ? ? ? ? D9 85 ? ? ? ? D8 3D ? ? ? ? D9 9D ? ? ? ? 8B 85 ? ? ? ? D9 05 ? ? ? ? D9 80 ? ? ? ? DA E9 DF E0 F6 C4 44 7A ? 8B 8D ? ? ? ? C7 81 ? ? ? ? ? ? ? ? 8B 15 ? ? ? ? 89 95 ? ? ? ? A1 ? ? ? ? 6B C0 18");
            ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &f0;
        }


    }

    //FMVs
    pattern = hook::pattern("89 50 18 8B 06 8B CE FF 50 14 8B 4F 08"); //0xA25984
    if (!pattern.empty())
    {
        struct FMVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float scaleX = 1.0f, scaleY = 1.0f;
                const int mode = std::clamp(fmvMode, 0, 2);
                const float aspect = Screen.fFMVScale;
                if (mode != 2 && std::isfinite(aspect) && aspect > 0.0f)
                {
                    // The native video plane uses the game's 4:3 design space.
                    const float fit = mode == 0 ? std::min(1.0f, aspect) : std::max(1.0f, aspect);
                    scaleX = fit / aspect;
                    scaleY = fit;
                }
                for (int offset : { -0x54, -0x38, -0x1C, 0 })
                {
                    *(float*)(regs.eax + offset) *= scaleX;
                    *(float*)(regs.eax + offset + 4) *= scaleY;
                }

                *(uintptr_t*)(regs.eax + 0x18) = regs.edx;
                regs.eax = *(uintptr_t*)(regs.esi);
            }
        }; injector::MakeInline<FMVHook>(pattern.get_first(0));
    }

    auto bars = find_pattern("A1 ? ? ? ? 8B 88 00 08 00 00 81 C1 B4 68 06 00", "A1 ? ? ? ? 89 45 FC 8B 0D ? ? ? ? 89 0D");
    if (bars.empty())
        OutputDebugStringA("King Kong: aspect-label identity unavailable.\n");
    if (!bars.empty() && wfp::PatchPreflight("King Kong video and aspect labels: signatures unavailable; feature disabled.\n",
    {
        { "68 ? ? ? ? E8 ? ? ? ? 50 55 56 E8 ? ? ? ? 0F B7 4F 06 0F B7 57 04", 1 },
    }))
    {
        auto text_pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 50 55 56 E8 ? ? ? ? 0F B7 4F 06 0F B7 57 04"); //0x992C4C
        static auto unk_CC0E20 = *text_pattern.count_hint(1).get(0).get<char*>(1);
        struct TextHookGamersEdition
        {
            void operator()(injector::reg_pack& regs)
            {
                if (strncmp(unk_CC0E20, "16/9", 4) == 0)
                {
                    std::memcpy(unk_CC0E20, "Bars", 4);
                }
                else
                {
                    if (strncmp(unk_CC0E20, "4/3", 3) == 0)
                    {
                        std::memcpy(unk_CC0E20, "Std", 4);
                    }
                }

                regs.ecx = *(uint16_t*)(regs.edi + 6);
                regs.edx = *(uint16_t*)(regs.edi + 4);
            }
        };

        struct TextHookSignatureEdition
        {
            void operator()(injector::reg_pack& regs)
            {
                if (strncmp(unk_CC0E20, "16/9", 4) == 0)
                {
                    std::memcpy(unk_CC0E20, "Auto", 4);
                }
                else if (strncmp(unk_CC0E20, "4/3", 4) == 0)
                {
                    std::memcpy(unk_CC0E20, "Std", 4);
                }
                else if (strncmp(unk_CC0E20, "4/3 ", 4) == 0)
                {
                    std::memcpy(unk_CC0E20, "Black Bands", sizeof("Black Bands"));
                }

                regs.ecx = *(uint16_t*)(regs.edi + 6);
                regs.edx = *(uint16_t*)(regs.edi + 4);
            }
        };

        pattern = find_pattern("A1 ? ? ? ? 8B 88 00 08 00 00 81 C1 B4 68 06 00", "A1 ? ? ? ? 89 45 FC 8B 0D ? ? ? ? 89 0D");
        Screen.pBarsPtr = *pattern.get_first<uintptr_t>(1); //0xCBFBE0

        pattern = hook::pattern("8B 41 10 6A 00");
        if (!pattern.empty())
        {
            shsub_A2A350 = safetyhook::create_inline(pattern.get_first(), sub_A2A350);
            injector::MakeInline<TextHookGamersEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
        }
        else
        {
            Screen.bSignatureEdition = true;
            injector::MakeInline<TextHookSignatureEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
        }

    }

    static std::string defaultCmd("/B /lang:01  /spg:50 /GDBShaders KKMaps.bf");
    if (Screen.bSignatureEdition)
        defaultCmd = "/B /TX /lang:01 /spg:50 KingKongTheGame.bf";

    auto startup = find_pattern("C7 45 ? ? ? ? ? 68 04 01 00 00 8D 85 D8 FB FF FF 50", "C7 45 FC 00 00 00 00 E8 ? ? ? ? 8B 45 10"); //0x401A10
    if (!startup.empty())
    {
        struct StartupHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.ebp - 0x4) = 0;
                std::string_view lpCmdLine(*(char**)(regs.ebp + 0x10));
                if (lpCmdLine.empty())
                    *(uint32_t*)(regs.ebp + 0x10) = (uint32_t)defaultCmd.data();
            }
        }; injector::MakeInline<StartupHook>(startup.get_first(0), startup.get_first(7));

    }
    else
        OutputDebugStringA("King Kong: startup command hook unavailable.\n");

    //windowed mode text fix
    {
        auto [ResX, ResY] = GetDesktopRes();
        HKEY key = nullptr;
        const auto path = Screen.bSignatureEdition ?
            "Software\\Ubisoft\\KingKong\\{111E336D-30BF-4CD4-8D69-4541732AFB27}\\Basic video" :
            "Software\\Ubisoft\\KingKong\\{2C391F94-B8B9-4832-9C57-3AFC332CC037}\\Basic video";
        if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_READ, &key) == ERROR_SUCCESS)
        {
            auto readDimension = [key](const char* name, int32_t fallback)
            {
                DWORD value = 0, type = 0, size = sizeof(value);
                if (RegQueryValueExA(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS &&
                    type == REG_DWORD && size == sizeof(value) && value > 0 && value <= 16384)
                    return static_cast<int32_t>(value);
                return fallback;
            };
            ResX = readDimension("ResolutionWidth", ResX);
            ResY = readDimension("ResolutionHeight", ResY);
            RegCloseKey(key);
        }

        pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00");
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            ProtectedGameRef<int32_t>{pattern.get(i).get<int32_t>(6)} = ResX;
            ProtectedGameRef<int32_t>{pattern.get(i).get<int32_t>(1)} = ResY;
        }
    }

    // Disable debugger window
    pattern = find_pattern("74 ? FF D0 85 C0");
    if (!pattern.empty())
    {
        ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>()} = 0xEB;
    }

    // Camera Shake fix
    if (wfp::PatchPreflight("King Kong camera shake: signatures unavailable; feature disabled.\n",
    {
        { "D9 05 ? ? ? ? D8 49 ? 8B 55 ? D8 6A ? 8B 45 ? D9 58 ? 8B 4D ? D9 05", 1 },
    }))
    {
        pattern = hook::pattern("D9 05 ? ? ? ? D8 49 ? 8B 55 ? D8 6A ? 8B 45 ? D9 58 ? 8B 4D ? D9 05");
        fTimeStep = *pattern.get_first<float*>(2);

        static auto CameraShakeFix = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (std::isfinite(*fTimeStep) && *fTimeStep > 0.0f)
            {
                const float targetFPS = 1.0f / *fTimeStep;
                *(float*)(regs.ecx + 0x2C) *= targetFPS / 30.0f;
            }
        });

    }

    InitGameSpeedFix();
    InitXeTexturePacker();
    InitRawInput();
    InitFramelimit();
    KongMenu::Init();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        WindowedModeWrapper::EnableDpiAwareness();
        auto game = hook::pattern("33 DB 89 5D E0 53");
        auto config = hook::pattern("75 66 8D 4C 24 04 51");
        const bool isGame = !game.empty();
        if (isGame || !config.empty())
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(isGame ? Init : InitSettings, isGame ? game : config);
        else
            OutputDebugStringA("King Kong: unsupported executable.\n");
        CallbackHandler::RegisterModuleLoadCallback(L"winmm.dll", InitWinMM);
        CallbackHandler::RegisterModuleLoadCallback(L"Xidi.32.dll", InitXidi);
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
