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
import Input;
import Presentation;
import Raster;
import Hud;
import Camera;
import Rotation;
import Timing;
import Modes;
import Options;
import Prompts;
import Updates;
import Resolution;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

// One executable layout per game: the first entry point found wins
void Init(GTA1Build::Kind kind)
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    GTA1Build::Set(kind);

    SettingsReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

    Log::Read(iniReader);
    InputDevices::Read(iniReader);
    InputDevices::Watch();
    Desktop::Resolution(Screen.Width, Screen.Height);
    InitComVars();
    GTA1Window::Install();
    GTA1Input::Install();
    GTA1Presentation::Install();
    GTA1Raster::Install();
    GTA1Hud::Install();
    GTA1Camera::Install();
    GTA1Rotation::Install();
    GTA1Timing::Install();

    Screen.Width43 = static_cast<uint32_t>(Screen.Height * (4.0f / 3.0f));
    GTA1Modes::Install(Screen.Width, Screen.Height, Screen.Width43);

    GTA1Options::Install();
    GTA1Options::Build(Screen.Width, Screen.Height);
    GTA1Options::ApplyResolution = GTA1Resolution::ApplyResolution;
    GTA1Prompts::Install();
    GTA1Updates::Install();
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", [] { CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor")); });

    GTA1Resolution::Install();

    auto pattern = hook::pattern("3B 05 ? ? ? ? 0F 94 C3 85 DB"); //0x4B4FB8
    GTA1Presentation::WatchWindow(*pattern.get_first<HWND*>(2));
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
        GTA1Options::LoadSetup();
        Log::Install();
        GameRegistry::DefaultValue("Software\\DMA Design\\Grand Theft Auto", "Language", 0);
        // DirectInput scan codes: left, right, up, down, fire, jump, enter, previous, next weapon, special.
        const int keys[] = { 203, 205, 200, 208, 57, 28, 29, 45, 44, 15 };
        for (int i = 0; i < 10; ++i)
            GameRegistry::DefaultValue("Software\\DMA Design\\Grand Theft Auto\\Controls",
                ("Control " + std::to_string(i)).c_str(), keys[i]);
        // - Classics: GTA1 Rockstar Classics / Steam and London 1969 / 1961 (WINO)
        // - London: GTA London 1969 / 1961 retail
        // - Retail: GTA1 retail
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([] { Init(GTA1Build::Kind::Classics); }, hook::pattern("83 EC 0C 33 C0 53 57 33 DB"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([] { Init(GTA1Build::Kind::London); }, hook::pattern("51 53 55 56 68 ? ? ? ? 33 DB"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([] { Init(GTA1Build::Kind::Retail); }, hook::pattern("53 56 57 55 33 ED 68 ? ? ? ? E8"));
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
