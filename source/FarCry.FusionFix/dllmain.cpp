#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import Misc;
import Startup;
import Frontend;
import Updates;
import Focus;
import Runtime;

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            Init();
            CallbackHandler::RegisterModuleLoadCallback(L"CryScriptSystem.dll", []()
            {
                FarCryStartup::Install(FixSettings().Bool("MAIN.SkipIntro"), GetModuleHandleW(L"CryScriptSystem.dll"));
                FarCryMenu::Init();
            });
            CallbackHandler::RegisterModuleLoadCallback(L"XRenderD3D9.dll", InitXRenderD3D9);
            CallbackHandler::RegisterModuleLoadCallback(L"CryGame.dll", []()
            {
                FarCryUpdates::BindModule(GetModuleHandleW(L"CryGame.dll"));
                FarCryFocus::Init();
                InitCryGame();
                FarCryMenu::Init();
            });
            CallbackHandler::RegisterModuleLoadCallback(L"Cry3DEngine.dll", InitCry3DEngine);
            CallbackHandler::RegisterModuleLoadCallback(L"CrySystem.dll", []()
            {
                FarCryUpdates::BindModule(GetModuleHandleW(L"CrySystem.dll"));
                InitCrySystem();
                FarCryMenu::Init();
            });
            CallbackHandler::RegisterModuleLoadCallback(L"CryInput.dll", []()
            {
                FarCryRuntime::InitInput();
                FarCryMenu::Init();
            });
            CallbackHandler::RegisterModuleLoadCallback(L"CryFont.dll", []()
            {
                InitCryFont();
                FarCryMenu::Init();
            });
        });
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
            InitializeASI();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        ProcessExiting = lpReserved != nullptr;
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
