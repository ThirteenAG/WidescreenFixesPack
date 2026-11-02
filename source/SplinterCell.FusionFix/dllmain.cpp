#include <stdafx.h>
#include "FixSettings.hpp"
#include "automation/asi_automation_frames.h"
#include "Shared/SplinterCell/Window.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/WidescreenFixConflict.hpp"

ASI_SETTINGS_EXPORT(FixSettings())
ASI_AUTOMATION_FRAMES_EXPORT()

import ComVars;
import CrashHandler;
import Settings;
import FrameRate;
import Core;
import Engine;
import D3DDrv;
import WinDrv;
import Window;
import Echelon;
import EchelonHUD;
import EchelonMenus;
import Xidi;
import Eax;
import Gamepad;
import NativeMenu;
import Updates;

void Init()
{
    SCWindow::EnableDpi();
    WFP::onInitEvent().executeAll();
}

// Only version 1.3 (the retail patch, Steam, GOG, Ubisoft Connect): the modules of 1.0 differ throughout

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        FixSettings();
        asi_settings::LiveLink::Start();
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        Gamepad::Init();
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
        CallbackHandler::RegisterCallback(L"Core.dll", [] { InitCore(); SCMenu::Init(); InitUpdateCheck(); });
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
        CallbackHandler::RegisterCallback(L"EchelonHUD.dll", [] { InitEchelonHUD(); Gamepad::InitEchelonHUD(); });
        CallbackHandler::RegisterCallback(L"EchelonMenus.dll", InitEchelonMenus);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
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