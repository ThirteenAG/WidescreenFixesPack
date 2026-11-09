#include "stdafx.h"
#include "FixSettings.hpp"
#include <TlHelp32.h>
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import D3DDrv;
import Engine;
import Core;
import WinDrv;
import GUI;
import Echelon;
import Xidi;
import WidescreenHUD;
import WidescreenHUD2;
import Eax;
import WindowedMode;
import Focus;
import Gamepad;
import NativeMenu;
import Updates;

float FrameInterval = 0.0f;
HMONITOR FrameLimitMonitor = nullptr;
void UpdateFrameLimit()
{
    auto limit = FixSettings().Int("MAIN.FPSLimit");
    FrameLimitMonitor = MonitorFromWindow(hGameWindow, MONITOR_DEFAULTTOPRIMARY);
    if (limit == -1)
    {
        MONITORINFOEXW monitor{ sizeof(monitor) };
        DEVMODEW mode{ .dmSize = sizeof(mode) };
        limit = GetMonitorInfoW(FrameLimitMonitor, &monitor) && EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ? int(mode.dmDisplayFrequency) : 60;
    }
    FrameInterval = limit > 0 ? 1.0f / float(std::clamp(limit, 1, 1000)) : 0.0f;
}

const wchar_t* aJoysticks = nullptr;
SafetyHookInline shsub_10CC8580;
void* __fastcall sub_10CC8580(void* _this, void* edx)
{
    auto ret = shsub_10CC8580.unsafe_fastcall<void*>(_this, edx);

    if (aJoysticks)
    {
        std::filesystem::path aJoysticksDefaultPath = GetExeModulePath() / "JoysticksDefault.ini";
        std::filesystem::path aJoysticksPath = std::wstring(aJoysticks) + L".ini";
        for (auto it : { aJoysticksDefaultPath, aJoysticksPath })
        {
            // Read the existing joysticks INI file into a structure, leave it alone if that fails
            mINI::INIStructure joysticksIni;
            mINI::INIFile joysticksIniFile(it);
            if (!joysticksIniFile.read(joysticksIni) || joysticksIni.size() == 0)
                continue;

            auto& settings = FixSettings();
            for (const auto& setting : settings.GetDescription().settings)
            {
                if (setting.section.starts_with("Joystick"))
                    joysticksIni[setting.section].set(setting.key, settings.Get(setting.id));
            }

            joysticksIniFile.generate(joysticksIni);
        }
    }

    return ret;
}

void Init()
{
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    InitWindowedMode(); // DPI awareness before the desktop resolution is read
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    Screen.bRawInputMouse = settings.Int("MAIN.RawInputMouse") != 0;
    settings.OnChange("MAIN.MouseSensitivity", []
    {
        RawInputHandler<>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity"));
    });
    Screen.bRawInputMouseRawData = settings.Int("MAIN.RawInputMouseRawData") != 0;
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(settings.Get("MAIN.HudAspectRatioConstraint"));
    nShadowMapResolution = settings.Int("GRAPHICS.ShadowMapResolution");
    bHardShadows = settings.Int("GRAPHICS.HardShadows") != 0;
    UpdateFrameLimit();
    settings.OnChange("MAIN.FPSLimit", UpdateFrameLimit);
    const auto gogglesColor = settings.Int("BONUS.GogglesLightColor");
    if (gogglesColor > 0 && gogglesColor <= static_cast<int32_t>(SuitLightColors.size()))
        gColor = SuitLightColors[gogglesColor - 1];
    gBlacklistIndicators = settings.Int("BONUS.BlacklistIndicators");
    bLightSyncRGB = settings.Int("MAIN.LightSyncRGB");
    auto bSkipIntro = settings.Int("MAIN.SkipIntro") != 0;
    auto bSkipPressStart = settings.Int("MAIN.SkipPressStart") != 0;

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    auto pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B 8C 24 20 02 00 00");
    aJoysticks = *pattern.get_first<const wchar_t*>(1);

    pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC 08 02 00 00");
    shsub_10CC8580 = safetyhook::create_inline(pattern.get_first(), sub_10CC8580);

    // Disable safe mode
    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 53 68");
    injector::MakeNOP(pattern.get_first(0), 5, true);

    // Unlock all resolutions
    pattern = hook::pattern("75 ? B8 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 8B 5C 24 3C");
    injector::MakeJMP(pattern.get_first(0), hook::get_pattern("8B 5C 24 18 8B 43 78 3B C5"), true);

    // Stop mouse sensitivity from affecting gamepad movement in certain states
    pattern = hook::pattern("E8 ? ? ? ? D8 4C 24 20 83 C4 04");
    injector::MakeCALL(pattern.get_first(0), static_cast<float(__cdecl*)(float)>([](float a1) -> float { return 1.0f; }), true);

    pattern = hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14"); //0x10CD09C5
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = (regs.esp + 0x434); // lea eax, [esp+434h]

            if (Screen.Width < 640 || Screen.Height < 480)
                return;

            auto pszPath = (const wchar_t*)regs.edi;
            auto ResX = std::to_wstring(Screen.Width);
            auto ResY = std::to_wstring(Screen.Height);
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"WindowedViewportX", ResX.c_str(), pszPath);
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"WindowedViewportY", ResY.c_str(), pszPath);

            // Fullscreen needs a display mode (a window resized by the player can have any size)
            bool bDisplayMode = false;
            DEVMODEW mode = { .dmSize = sizeof(DEVMODEW) };
            for (DWORD i = 0; !bDisplayMode && EnumDisplaySettingsW(NULL, i, &mode); ++i)
                bDisplayMode = int(mode.dmPelsWidth) == Screen.Width && int(mode.dmPelsHeight) == Screen.Height;
            if (!bDisplayMode)
            {
                auto [DesktopResW, DesktopResH] = GetDesktopRes();
                ResX = std::to_wstring(DesktopResW);
                ResY = std::to_wstring(DesktopResH);
            }
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"FullscreenViewportX", ResX.c_str(), pszPath);
            WritePrivateProfileStringW(L"WinDrv.WindowsClient", L"FullscreenViewportY", ResY.c_str(), pszPath);
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(7));

    Focus::Init();
    InitD3DDrv();
    InitEngine();
    InitCore();
    InitWinDrv();
    InitGUI();
    CTMenu::Init();
    Gamepad::Init(CTMenu::getLanguage);
    InitUpdateCheck();
    WFP::onGameProcessEvent() += []()
    {
        if (FixSettings().Int("MAIN.FPSLimit") == -1 && MonitorFromWindow(hGameWindow, MONITOR_DEFAULTTOPRIMARY) != FrameLimitMonitor)
            UpdateFrameLimit();
        RWGTA::UpdateCheck::Tick();
    };
    InitEchelon();
    InitWidescreenHUD2();

    #ifdef _DEBUG
    //pattern = hook::pattern("8B 88 80 00 00 00 6A 01");
    //static auto SkipMenuHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    //{
    //    static char bOnce = 0;
    //    if (bOnce < 2)
    //    {
    //        keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
    //        keybd_event(VK_RETURN, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    //        bOnce++;
    //    }
    //});
    #endif

    // The native pacing loop reads this minimum frame time every frame, including unlimited (zero).
    pattern = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 89 54 24 10 89 44 24 18");
    injector::WriteMemory<float*>(pattern.get_first(2), &FrameInterval, true);

    //Bink high res
    pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00 50 FF 52 ? EB 18");
    {
        injector::WriteMemory(pattern.get_first(1), 2160, true);
        injector::WriteMemory(pattern.get_first(6), 3840, true);
    }

    pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00 50 FF 51 ? EB 18 8B 10 6A 01 6A 15 6A 00 6A 01 68 E0 01 00 00 68 80 02 00 00 50 FF 52 ? 85 DB");
    {
        injector::WriteMemory(pattern.get_first(1), 2160, true);
        injector::WriteMemory(pattern.get_first(6), 3840, true);
    }

    pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00 50 FF 51 ? EB 18 8B 10 6A 01 6A 15 6A 00 6A 01 68 E0 01 00 00 68 80 02 00 00 50 FF 52 ? 8B AC 24 88 00 00 00");
    {
        injector::WriteMemory(pattern.get_first(1), 2160, true);
        injector::WriteMemory(pattern.get_first(6), 3840, true);
    }

    static float h = 1.0f / 2160.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? 8B 0E");
    injector::WriteMemory(pattern.get_first(2), &h, true);

    static float w = 1.0f / 3840.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? 8B 54 24 18 8B 44 24 24");
    injector::WriteMemory(pattern.get_first(2), &w, true);

    // Button names
    pattern = hook::pattern("56 E8 ? ? ? ? 83 C4 04 50 8D 8C 24 90 00 00 00");
    static auto ButtonNamesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (const auto label = Gamepad::Label(std::wstring_view(reinterpret_cast<const wchar_t*>(regs.esi))))
            regs.esi = reinterpret_cast<uintptr_t>(label);
    });

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("75 ? 68 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 08 85 C0 75 ? 68");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);
    }

    if (bSkipPressStart)
    {
        auto pattern = hook::pattern("6A 0A FF 50 ? 8B 0D");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);

        pattern = hook::pattern("6A 08 FF 52 ? 8B 0D ? ? ? ? 3B FE");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);
    }
}

DWORD FindProcessId(const std::wstring& processName)
{
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            if (processName == pe32.szExeFile)
            {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

void InitGameOverlayRenderer()
{
    auto steamPid = FindProcessId(L"steam.exe");
    if (steamPid)
    {
        auto steamPath = GetModulePath(GetModuleHandleW(L"GameOverlayRenderer.dll")).parent_path();
        auto exePath = steamPath / L"gameoverlayui64.exe";
        std::wstring args = L" -steampid " + std::to_wstring(steamPid) + L" -pid " + std::to_wstring(GetCurrentProcessId()) + L" -gameid " + L"13570" + L" -manuallyclearframes 0";

        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpFile = exePath.c_str();
        sei.lpParameters = args.c_str();
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteEx(&sei);
    }
}

ASI_SETTINGS_EXPORT(FixSettings())


CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8D 84 24 34 04 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 14"));
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
        CallbackHandler::RegisterCallback(L"GameOverlayRenderer.dll", InitGameOverlayRenderer);
        CallbackHandler::RegisterCallback(L"eax.dll", InitEAX);
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
