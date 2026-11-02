#include "stdafx.h"
#include "FixSettings.hpp"
#include <shlobj.h>
#include <filesystem>
#include <FunctionHookMinHook.hpp>
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import Misc;
import Xidi;
import ControllerDefaults;

std::unique_ptr<FunctionHookMinHook> mhSHGetFolderPathA = { nullptr };
HRESULT WINAPI SHGetFolderPathAHook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    HRESULT r;
    if ((csidl & ~CSIDL_FLAG_CREATE) == CSIDL_COMMON_DOCUMENTS)
    {
        r = mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(hwnd, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, hToken, dwFlags, pszPath);

        if (FAILED(r))
            return r;

        static bool once = false;
        if (!once)
        {
            std::error_code ec;
            auto dest = std::filesystem::path(std::string(pszPath) + "\\Monolith Productions\\Condemned\\");
            if (!std::filesystem::exists(dest, ec))
            {
                CHAR szPath[MAX_PATH];
                if (SUCCEEDED(mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(NULL, CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
                {
                    auto src = std::filesystem::path(std::string(szPath) + "\\Monolith Productions\\Condemned\\");
                    std::filesystem::create_directories(dest, ec);
                    std::filesystem::copy(src, dest, std::filesystem::copy_options::recursive, ec);
                }
            }
            once = true;
        }
    }
    else
    {
        r = mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(hwnd, csidl, hToken, dwFlags, pszPath);
    }

    return r;
}

void Init()
{
    WFP::onInitEvent().executeAll();
}

void InitGameClient()
{
    WFP::onGameInitEvent().executeAll();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        WindowedModeWrapper::EnableDpiAwareness();
        auto& settings = FixSettings();
        asi_settings::LiveLink::Start();
        IniFile.FixSavePath = settings.Bool("MAIN.FixSavePath");
        IniFile.WindowMode = settings.Int("MAIN.WindowMode");
        IniFile.SkipIntro = settings.Bool("MAIN.SkipIntro");

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("E8 ? ? ? ? 8B C6 5E 83 C4 10 C3"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitConfig, hook::pattern("0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F"));
        CallbackHandler::RegisterModuleLoadCallback(L"GameClient.dll", InitGameClient);
        CallbackHandler::RegisterModuleLoadCallback(L"Xidi.32.dll", InitXidi);

        if (IniFile.FixSavePath)
        {
            mhSHGetFolderPathA = std::make_unique<FunctionHookMinHook>((uintptr_t)SHGetFolderPathA, (uintptr_t)SHGetFolderPathAHook);
            mhSHGetFolderPathA->create();
        }
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
            InitializeASI();
    }
    return TRUE;
}
