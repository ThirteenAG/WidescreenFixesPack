#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/Window.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import CrashHandler;
import Settings;
import FrameRate;
import Core;
import Engine;
import D3DDrv;
import Echelon;
import WinDrv;
import Window;
import GUI;
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

// The retail release and the current one (Steam, Ubisoft Connect): other builds differ throughout

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        FixSettings();
        CursorConfinement::SetEnabled(FixSettings().Int("MAIN.ConfineCursor") != 0);
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Int("MAIN.ConfineCursor") != 0);
        });
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Core.dll", [] { InitCore(); PTMenu::Init(); InitUpdateCheck(); });
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"Echelon.dll", [] { InitEchelon(); Gamepad::InitEchelonHUD(); });
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"GUI.dll", [] { InitGUI(); PTMenu::InitDrawing(); });
        CallbackHandler::RegisterCallback(L"Window.dll", InitWindow);
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
