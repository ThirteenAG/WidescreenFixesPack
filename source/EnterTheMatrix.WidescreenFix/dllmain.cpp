#include "stdafx.h"
#include "FixSettings.hpp"

import ComVars;
import Matrix;
import MatrixOptions;

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
        const auto name = std::filesystem::path(path).filename().wstring();
        if (_wcsicmp(name.c_str(), L"MatrixOptions.exe") == 0 || _wcsicmp(name.c_str(), L"MatrixOptions12.exe") == 0)
        {
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitMatrixOptions, hook::pattern("8B 45 04 85 C0 74"));
        }
        else
        {
            // The options dialog keeps system scaling; the game itself gets real pixels on scaled desktops
            WindowedModeWrapper::EnableDpiAwareness();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitMatrix, hook::pattern("55 8B EC 83 EC 34 56 6A 00 6A FF"));
        }
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