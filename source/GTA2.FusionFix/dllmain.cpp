#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"

import ComVars;
import Log;
import GameRegistry;
import Desktop;
import Window;
import Camera;
import Input;
import Prompts;
import Resolution;
import Hud;
import Frontend;
import Misc;
import PauseMenu;
import Updates;
import Movies;
import Quicksave;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    Log::Write("GTA2 patches initializing.");
    // Per-monitor DPI awareness: the window and the resolution are in real pixels at any scaling
    using SetContext = BOOL(WINAPI*)(HANDLE);
    auto setContext = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext"));
    if (!setContext || !setContext(reinterpret_cast<HANDLE>(-4)))
        SetProcessDPIAware();

    FixSettings().Availability("SETUP.Option.Language", [](std::string_view value)
    {
        return value == "auto" || std::filesystem::exists(std::filesystem::path("data") / (std::string(value) + ".gxt"));
    }, [] { return "Only installed game languages are available."; });
    SettingsReader iniReader("");
    nResX = iniReader.ReadInteger("MAIN", "ResX", 0);
    nResY = iniReader.ReadInteger("MAIN", "ResY", 0);
    // Changes from FusionFixHub or a preset apply at once where the in-game options apply them at once too
    static auto readLive = []
    {
        fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        bSkipCredits = FixSettings().Int("MISC.SkipCredits") != 0;
        bFillBackground = FixSettings().Bool("MAIN.FillFrontendBackground");
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        nQuicksaveKey = FixSettings().Int("MISC.QuicksaveKey");
        nZoomIncreaseKey = FixSettings().Int("MISC.ZoomIncreaseKey");
        nZoomDecreaseKey = FixSettings().Int("MISC.ZoomDecreaseKey");
    };
    for (auto id : { "MAIN.HudAspectRatioConstraint", "MISC.SkipCredits", "MAIN.FillFrontendBackground", "MAIN.ConfineCursor", "MISC.QuicksaveKey",
        "MISC.ZoomIncreaseKey", "MISC.ZoomDecreaseKey" })
        FixSettings().OnChange(id, readLive);

    bSkipMovie = iniReader.ReadBoolean("MAIN", "SkipIntro", true);
    bSkipCredits = iniReader.ReadInteger("MISC", "SkipCredits", 1) != 0;
    bFillBackground = iniReader.ReadBoolean("MAIN", "FillFrontendBackground", false);

    nQuicksaveKey = iniReader.ReadInteger("MISC", "QuicksaveKey", VK_F5);
    nZoomIncreaseKey = iniReader.ReadInteger("MISC", "ZoomIncreaseKey", VK_OEM_PLUS);
    nZoomDecreaseKey = iniReader.ReadInteger("MISC", "ZoomDecreaseKey", VK_OEM_MINUS);
    readLive();

    Log::Read(iniReader);
    InputDevices::Read(iniReader);
    InputDevices::Watch();
    Desktop::Resolution(nResX, nResY);
    GTA2Window::Install();
    GTA2Camera::Install(iniReader);
    GTA2Input::Install(iniReader);
    GTA2Prompts::Install();
    GTA2Resolution::Install();
    GTA2Hud::Install();
    GTA2Frontend::Install();
    GTA2Misc::Install();
    GTA2PauseMenu::Install();
    GTA2Updates::Install();
    GTA2Movies::Install();
    Log::Write("GTA2 presentation and input hooks installed.");
    GTA2Quicksave::Install();
}

// Run from the game's folder: the game opens its files relative to it.
static void SetGameDirectory()
{
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, path, MAX_PATH))
        SetCurrentDirectoryW(std::filesystem::path(path).parent_path().c_str());
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        FixSettings();
        asi_settings::LiveLink::Start();
        SetGameDirectory();
        GameRegistry::Install();
        Log::Install();
        // The manager uses this ASI through the same local DirectDraw loader.
        // Keep its 16-bit format and minimum-size checks; remove only 4:3 filtering.
        auto managerCount = hook::pattern("DF E0 F6 C4 44 7A 01 43 8B 49 38");
        auto managerFill = hook::pattern("DF E0 F6 C4 44 7A 41 81 FF 80 02 00 00");
        if (managerCount.size() == 1 && managerFill.size() == 1)
        {
            injector::MakeNOP(managerCount.get_first(5), 2, true);
            injector::MakeNOP(managerFill.get_first(5), 2, true);
            return;
        }
        GameRegistry::DefaultString("Software\\DMA Design Ltd\\GTA2\\screen", "rendername", "d3ddll.dll");
        GameRegistry::DefaultString("Software\\DMA Design Ltd\\GTA2\\screen", "videoname", "dmavideo.dll");
        GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\screen", "start_mode", 0);
        GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\screen", "renderdevice", 1);
        GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\screen", "videodevice", 1);
        const int keys[] = { 0xC8, 0xD0, 0xCB, 0xCD, 0x1D, 0x1C, 0x39, 0x2C, 0x2D, 0x0F, 0x38, 0x36 };
        for (int i = 0; i < 12; ++i)
            GameRegistry::DefaultValue("Software\\DMA Design Ltd\\GTA2\\Control", std::to_string(i).c_str(), keys[i]);
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("83 EC 68 55 56 8B 74 24 74"));
        CallbackHandler::RegisterCallback(L"d3dim.dll", GTA2Misc::InitD3DDim);       // crash fix for
        CallbackHandler::RegisterCallback(L"d3dim700.dll", GTA2Misc::InitD3DDim700); // resolutions > 2048
        CallbackHandler::RegisterCallback(L"d3ddll.dll", InitD3DDLL); // frontend background scale
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
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
