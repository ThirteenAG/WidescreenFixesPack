module;

#include <stdafx.h>
#include <FunctionHookMinHook.hpp>

export module Registry;

import ComVars;

static std::filesystem::path CustomUserDir;
static bool bUseCustomUserDir = false;

static std::unique_ptr<FunctionHookMinHook> mhSHGetFolderPathA;
static HRESULT WINAPI SHGetFolderPathAHook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    if (bUseCustomUserDir)
    {
        int folder = csidl & ~CSIDL_FLAG_CREATE;
        if (folder == CSIDL_COMMON_DOCUMENTS)
        {
            CreateDirectoryW(CustomUserDir.c_str(), NULL);
            memcpy(pszPath, CustomUserDir.u8string().data(), CustomUserDir.u8string().size() + 1);
            return S_OK;
        }
    }

    return mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(hwnd, csidl, hToken, dwFlags, pszPath);
}

class Registry
{
public:
    Registry()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 0) != 0;
            auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");

            if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
                szCustomUserFilesDirectoryInGameDir.clear();

            if (!szCustomUserFilesDirectoryInGameDir.empty())
            {
                CustomUserDir = GetExeModulePath<std::filesystem::path>();
                CustomUserDir.append(szCustomUserFilesDirectoryInGameDir);
                bUseCustomUserDir = true;
            }

            mhSHGetFolderPathA = std::make_unique<FunctionHookMinHook>((uintptr_t)SHGetFolderPathA, (uintptr_t)SHGetFolderPathAHook);
            mhSHGetFolderPathA->create();

            if (bWriteSettingsToFile)
            {
                auto [DesktopResW, DesktopResH] = GetDesktopRes();

                std::filesystem::path SettingsSavePath;
                if (bUseCustomUserDir)
                {
                    SettingsSavePath = CustomUserDir;
                }
                else
                {
                    char szSettingsSavePath[MAX_PATH];
                    SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szSettingsSavePath);
                    SettingsSavePath = szSettingsSavePath;
                }

                SettingsSavePath.append("NFS ProStreet");
                SettingsSavePath.append("Settings.ini");

                RegistryWrapper("Need for Speed", SettingsSavePath);
                RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
                RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryWrapper::AddDefault("CD Drive", "D:\\");
                RegistryWrapper::AddDefault("CacheSize", "5697825792");
                RegistryWrapper::AddDefault("SwapSize", "73400320");
                RegistryWrapper::AddDefault("Language", "Engish (US)");
                RegistryWrapper::AddDefault("StreamingInstall", "0");
                RegistryWrapper::AddDefault("FirstTime", "0");
                RegistryWrapper::AddDefault("g_CarEffects", "2");
                RegistryWrapper::AddDefault("g_WorldFXLevel", "3");
                RegistryWrapper::AddDefault("g_RoadReflectionEnable", "1");
                RegistryWrapper::AddDefault("g_WorldLodLevel", "2");
                RegistryWrapper::AddDefault("g_CarLodLevel", "0");
                RegistryWrapper::AddDefault("g_FSAALevel", "3");
                RegistryWrapper::AddDefault("g_RainEnable", "1");
                RegistryWrapper::AddDefault("g_TextureFiltering", "2");
                RegistryWrapper::AddDefault("g_SmokeEnable", "1");
                RegistryWrapper::AddDefault("g_CarDamageDetail", "2");
                RegistryWrapper::AddDefault("g_PerformanceLevel", "0");
                RegistryWrapper::AddDefault("g_VSyncOn", "0");
                RegistryWrapper::AddDefault("g_ShadowEnable", "3");
                RegistryWrapper::AddDefault("g_ShaderDetailLevel", "1");
                RegistryWrapper::AddDefault("g_AudioDetail", "1");
                RegistryWrapper::AddDefault("g_Brightness", "50");
                RegistryWrapper::AddDefault("g_AudioMode", "1");
                RegistryWrapper::AddDefault("g_Width", std::to_string(DesktopResW));
                RegistryWrapper::AddDefault("g_Height", std::to_string(DesktopResH));
                RegistryWrapper::AddDefault("g_Refresh", "60");

                IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
                    std::forward_as_tuple("RegCloseKey", RegistryWrapper::RegCloseKey),
                    std::forward_as_tuple("RegCreateKeyA", RegistryWrapper::RegCreateKeyA),
                    std::forward_as_tuple("RegOpenKeyA", RegistryWrapper::RegOpenKeyA),
                    std::forward_as_tuple("RegOpenKeyExA", RegistryWrapper::RegOpenKeyExA),
                    std::forward_as_tuple("RegCreateKeyExA", RegistryWrapper::RegCreateKeyExA),
                    std::forward_as_tuple("RegQueryValueExA", RegistryWrapper::RegQueryValueExA),
                    std::forward_as_tuple("RegSetValueExA", RegistryWrapper::RegSetValueExA),
                    std::forward_as_tuple("RegQueryValueA", RegistryWrapper::RegQueryValueA),
                    std::forward_as_tuple("RegDeleteKeyA", RegistryWrapper::RegDeleteKeyA),
                    std::forward_as_tuple("RegEnumKeyA", RegistryWrapper::RegEnumKeyA)
                );
            }
            else
            {
                auto [DesktopResW, DesktopResH] = GetDesktopRes();

                RegistryFallback::Init(
                    "Software\\Electronic Arts\\Need for Speed ProStreet",
                    "Software\\Electronic Arts\\Electronic Arts\\Need for Speed ProStreet"
                );

                RegistryFallback::UseHKCUFallback(true);
                RegistryFallback::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryFallback::AddDefault("CD Drive", "D:\\");
                RegistryFallback::AddDefault("CacheSize", "5697825792");
                RegistryFallback::AddDefault("SwapSize", "73400320");
                RegistryFallback::AddDefault("Language", "Engish (US)");
                RegistryFallback::AddDefault("StreamingInstall", "0");
                RegistryFallback::AddDefault("FirstTime", "0");
                RegistryFallback::AddDefault("g_CarEffects", "2");
                RegistryFallback::AddDefault("g_WorldFXLevel", "3");
                RegistryFallback::AddDefault("g_RoadReflectionEnable", "1");
                RegistryFallback::AddDefault("g_WorldLodLevel", "2");
                RegistryFallback::AddDefault("g_CarLodLevel", "0");
                RegistryFallback::AddDefault("g_FSAALevel", "3");
                RegistryFallback::AddDefault("g_RainEnable", "1");
                RegistryFallback::AddDefault("g_TextureFiltering", "2");
                RegistryFallback::AddDefault("g_SmokeEnable", "1");
                RegistryFallback::AddDefault("g_CarDamageDetail", "2");
                RegistryFallback::AddDefault("g_PerformanceLevel", "0");
                RegistryFallback::AddDefault("g_VSyncOn", "0");
                RegistryFallback::AddDefault("g_ShadowEnable", "3");
                RegistryFallback::AddDefault("g_ShaderDetailLevel", "1");
                RegistryFallback::AddDefault("g_AudioDetail", "1");
                RegistryFallback::AddDefault("g_Brightness", "50");
                RegistryFallback::AddDefault("g_AudioMode", "1");
                RegistryFallback::AddDefault("g_Width", std::to_string(DesktopResW));
                RegistryFallback::AddDefault("g_Height", std::to_string(DesktopResH));
                RegistryFallback::AddDefault("g_Refresh", "60");

                std::string exePath = GetExeModulePath<std::string>();
                RegistryFallback::AddDefault("Install Dir", exePath);
                RegistryFallback::AddDefault("InstallDir", exePath);
                RegistryFallback::AddDefault("Path", exePath);

                RegistryFallback::EnsureDefaults();
            }
        };
    }
} Registry;