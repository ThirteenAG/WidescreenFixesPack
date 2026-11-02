module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module SettingsFile;

import ComVars;

// WriteSettingsToFile: the game keeps its settings in the registry through CWinApp's profile
// functions, which keep them in the INI file m_pszProfileName names instead while there's no
// registry key. Instead of setting one, the file next to the savegames gets used, starting out with
// what the registry has the first time.
export namespace CWinApp
{
    enum
    {
        APP_NAME = 0x50,     // char* m_pszAppName
        PROFILE_NAME = 0x68, // char* m_pszProfileName
    };

    std::filesystem::path SettingsPath;

    // of the CRT mfc71.dll frees m_pszProfileName with
    char* (__cdecl* crtStrdup)(const char* szString) = nullptr;
    void(__cdecl* crtFree)(void* pBlock) = nullptr;

    // As CWinApp::WriteProfileInt and WriteProfileString would have written the values
    void CopyRegistrySettings(const std::string& szKey)
    {
        HKEY hApp = nullptr;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, szKey.c_str(), 0, KEY_READ, &hApp) != ERROR_SUCCESS)
            return;

        auto szFile = SettingsPath.string();
        char szSection[256];
        for (DWORD i = 0; ; ++i)
        {
            DWORD nSectionLength = sizeof(szSection);
            if (RegEnumKeyExA(hApp, i, szSection, &nSectionLength, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;

            HKEY hSection = nullptr;
            if (RegOpenKeyExA(hApp, szSection, 0, KEY_READ, &hSection) != ERROR_SUCCESS)
                continue;

            char szName[256];
            char data[4096];
            for (DWORD j = 0; ; ++j)
            {
                DWORD nNameLength = sizeof(szName);
                DWORD nDataSize = sizeof(data) - 1;
                DWORD nType = REG_NONE;
                auto status = RegEnumValueA(hSection, j, szName, &nNameLength, nullptr, &nType, (BYTE*)data, &nDataSize);
                if (status == ERROR_NO_MORE_ITEMS)
                    break;
                if (status != ERROR_SUCCESS)
                    continue;

                if (nType == REG_DWORD && nDataSize == sizeof(DWORD))
                    WritePrivateProfileStringA(szSection, szName, std::to_string(*(int32_t*)data).c_str(), szFile.c_str());
                else if (nType == REG_SZ)
                {
                    data[nDataSize] = '\0';
                    WritePrivateProfileStringA(szSection, szName, data, szFile.c_str());
                }
            }
            RegCloseKey(hSection);
        }
        RegCloseKey(hApp);
    }

    SafetyHookInline shSetRegistryKey = {};
    void __fastcall SetRegistryKey(uint8_t* _this, void* edx, const char* szRegistryKey)
    {
        std::error_code ec;
        if (!std::filesystem::exists(SettingsPath, ec))
            CopyRegistrySettings(std::string("Software\\") + szRegistryKey + "\\" + *(const char**)(_this + APP_NAME));

        auto& szProfileName = *(char**)(_this + PROFILE_NAME);
        crtFree(szProfileName);
        szProfileName = crtStrdup(SettingsPath.string().c_str());
    }
}

// Before the game sets its registry key
export void InitSettingsFile()
{
    auto& settings = FixSettings();
    if (!settings.Bool("MAIN.PortableSettings"))
        return;

    char szDocuments[MAX_PATH];
    if (settings.Bool("MISC.UseGameFolderForSavegames"))
        CWinApp::SettingsPath = GetExeModulePath() / "savegames";
    else if (SHGetSpecialFolderPathA(nullptr, szDocuments, CSIDL_PERSONAL, FALSE))
        CWinApp::SettingsPath = std::filesystem::path(szDocuments) / "Max Payne 2 Savegames";
    else
        return;

    std::error_code ec;
    std::filesystem::create_directories(CWinApp::SettingsPath, ec);
    CWinApp::SettingsPath /= "settings.ini";

    auto crt = GetModuleHandleA("msvcr71.dll");
    CWinApp::crtStrdup = (decltype(CWinApp::crtStrdup))GetProcAddress(crt, "_strdup");
    CWinApp::crtFree = (decltype(CWinApp::crtFree))GetProcAddress(crt, "free");
    if (!CWinApp::crtStrdup || !CWinApp::crtFree)
        return;

    CWinApp::shSetRegistryKey = safetyhook::create_inline(GetProcAddress(GetModuleHandleA("mfc71.dll"), MAKEINTRESOURCEA(5975)), CWinApp::SetRegistryKey); // CWinApp::SetRegistryKey(LPCTSTR)
}
