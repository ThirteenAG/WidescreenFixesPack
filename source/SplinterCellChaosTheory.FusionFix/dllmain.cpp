#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/SplinterCell/FrameLimit.hpp"
#include "Shared/SplinterCell/ViewportMouse.hpp"
#include <TlHelp32.h>
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/WidescreenFixConflict.hpp"
#include "Shared/SplinterCell/SaveLocation.hpp"

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

void Init()
{
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    InitWindowedMode(); // DPI awareness before the desktop resolution is read
    Screen.Width = ReadFixInt("MAIN.ResX");
    Screen.Height = ReadFixInt("MAIN.ResY");
    Screen.bRawInputMouse = ReadFixInt("MAIN.RawInputMouse") != 0;
    settings.OnChange("MAIN.MouseSensitivity", ViewportMouse::ReadSensitivity);
    ViewportMouse::ReadSensitivity();
    Screen.bRawInputMouseRawData = ReadFixInt("MAIN.RawInputMouseRawData") != 0;
    ViewportMouse::rawData.store(Screen.bRawInputMouseRawData);
    nShadowMapResolution = ReadFixInt("GRAPHICS.ShadowMapResolution");
    bHardShadows = ReadFixInt("GRAPHICS.HardShadows") != 0;
    settings.OnChange("MAIN.FPSLimit", SplinterCellFrameLimit::ReadSetting);
    SplinterCellFrameLimit::ReadSetting();
    SplinterCellFrameLimit::Update(hGameWindow);
    const auto gogglesColor = ReadFixInt("BONUS.GogglesLightColor");
    if (gogglesColor > 0 && gogglesColor <= static_cast<int32_t>(SuitLightColors.size()))
        gColor = SuitLightColors[gogglesColor - 1];
    gBlacklistIndicators = ReadFixInt("BONUS.BlacklistIndicators");
    bLightSyncRGB = ReadFixInt("MAIN.LightSyncRGB");
    auto bSkipIntro = ReadFixInt("MAIN.SkipIntro") != 0;
    auto bSkipPressStart = ReadFixInt("MAIN.SkipPressStart") != 0;

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    InitControllerDefaults();

    // Disable safe mode
    auto pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 53 68");
    injector::MakeNOP(pattern.get_first(0), 5, true);

    // MAIN.SaveLocation: the profiles folder (saves inside) in Documents or the game folder instead of ProgramData
    static std::filesystem::path ProfilesFolder;
    switch (ReadFixInt("MAIN.SaveLocation"))
    {
        case 1:
            ProfilesFolder = SCSaveLocation::Documents(L"Splinter Cell Chaos Theory");
            break;
        case 2:
            ProfilesFolder = SCSaveLocation::GameFolder() / L"Save";
            break;
    }
    if (!ProfilesFolder.empty())
    {
        pattern = hook::pattern("6A 23 8D 8D ? ? ? ? 51 E8 ? ? ? ? 83 C4 08 68 ? ? ? ? 8D 95 ? ? ? ? 52 FF 15");
        static auto nPathOffset = *pattern.get_first<int32_t>(24);
        struct ProfilesFolderHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto pszPath = reinterpret_cast<wchar_t*>(regs.ebp + nPathOffset);
                SHCreateDirectoryExW(nullptr, ProfilesFolder.c_str(), nullptr);
                wcscpy_s(pszPath, MAX_PATH, ProfilesFolder.c_str());
            }
        }; injector::MakeInline<ProfilesFolderHook>(pattern.get_first(17), pattern.get_first(35));
    }

    // Unlock all resolutions
    pattern = hook::pattern("75 ? B8 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 8B 5C 24 3C");
    injector::MakeJMP(pattern.get_first(0), hook::get_pattern("8B 5C 24 18 8B 43 78 3B C5"), true);

    // Stop mouse sensitivity from affecting gamepad movement in certain states
    pattern = hook::pattern("E8 ? ? ? ? D8 4C 24 20 83 C4 04");
    injector::MakeCALL(pattern.get_first(0), static_cast<float(__cdecl*)(float)>([](float a1) -> float { return 1.0f; }), true);

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
        SplinterCellFrameLimit::Update(hGameWindow);
        RWGTA::UpdateCheck::Tick();
    };
    InitEchelon();
    InitWidescreenHUD2();

    // The native pacing loop reads the minimum frame time every frame.
    pattern = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 89 54 24 10 89 44 24 18");
    injector::WriteMemory<float*>(pattern.get_first(2), &SplinterCellFrameLimit::minFrameTime, true);

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
        CTMenu::EnableSkipLoadingPrompt();
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
    // Steam injects its overlay renderer but never starts the overlay for this game. Started by the fix,
    // the overlay costs frame rate on some PCs (streaming from another graphics card), so it is opt-in
    if (!ReadFixInt("MAIN.SteamOverlay"))
        return;

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
