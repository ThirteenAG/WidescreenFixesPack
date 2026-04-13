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
        if (folder == CSIDL_PERSONAL)
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

                SettingsSavePath.append("NFS Underground 2");
                SettingsSavePath.append("Settings.ini");

                RegistryWrapper("Need for Speed", SettingsSavePath);
                RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
                RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryWrapper::AddDefault("CD Drive", "D:\\");
                RegistryWrapper::AddDefault("CacheSize", "1879040000");
                RegistryWrapper::AddDefault("SwapSize", "0");
                RegistryWrapper::AddDefault("Language", "English US");
                RegistryWrapper::AddDefault("StreamingInstall", "0");
                RegistryWrapper::AddDefault("g_CarEnvironmentMapEnable", "3");
                RegistryWrapper::AddDefault("g_CarEnvironmentMapUpdateData", "1");
                RegistryWrapper::AddDefault("g_CarShadowEnable", "2");
                RegistryWrapper::AddDefault("g_CarHeadlightEnable", "1");
                RegistryWrapper::AddDefault("g_CarLightingEnable", "0");
                RegistryWrapper::AddDefault("g_CarDamageEnable", "0");
                RegistryWrapper::AddDefault("g_CrowdEnable", "1");
                RegistryWrapper::AddDefault("g_RoadReflectionEnable", "3");
                RegistryWrapper::AddDefault("g_FogEnable", "1");
                RegistryWrapper::AddDefault("g_MotionBlurEnable", "1");
                RegistryWrapper::AddDefault("g_LightStreaksEnable", "1");
                RegistryWrapper::AddDefault("g_LightGlowEnable", "1");
                RegistryWrapper::AddDefault("g_AnimatedTextureEnable", "1");
                RegistryWrapper::AddDefault("g_ParticleSystemEnable", "1");
                RegistryWrapper::AddDefault("g_DepthOfFieldEnable", "1");
                RegistryWrapper::AddDefault("g_WorldLodLevel", "3");
                RegistryWrapper::AddDefault("g_CarLodLevel", "1");
                RegistryWrapper::AddDefault("g_OverBrightEnable", "1");
                RegistryWrapper::AddDefault("g_BleachByPassEnable", "1");
                RegistryWrapper::AddDefault("g_TintingEnable", "1");
                RegistryWrapper::AddDefault("g_FSAALevel", "7");
                RegistryWrapper::AddDefault("g_HorizonFogEnable", "1");
                RegistryWrapper::AddDefault("g_RainEnable", "1");
                RegistryWrapper::AddDefault("g_TextureFiltering", "2");
                RegistryWrapper::AddDefault("g_RacingResolution", "4");
                RegistryWrapper::AddDefault("g_PerformanceLevel", "5");
                RegistryWrapper::AddDefault("g_VSyncOn", "0");

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
                RegistryFallback::Init(
                    "Software\\EA Games\\Need for Speed Underground 2",
                    "Software\\Electronic Arts\\EA Games\\Need for Speed Underground 2"
                );

                RegistryFallback::UseHKCUFallback(true);
                RegistryFallback::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryFallback::AddDefault("CD Drive", "D:\\");
                RegistryFallback::AddDefault("CacheSize", "1879040000");
                RegistryFallback::AddDefault("SwapSize", "0");
                RegistryFallback::AddDefault("Language", "English US");
                RegistryFallback::AddDefault("StreamingInstall", "0");
                RegistryFallback::AddDefault("g_CarEnvironmentMapEnable", "3");
                RegistryFallback::AddDefault("g_CarEnvironmentMapUpdateData", "1");
                RegistryFallback::AddDefault("g_CarShadowEnable", "2");
                RegistryFallback::AddDefault("g_CarHeadlightEnable", "1");
                RegistryFallback::AddDefault("g_CarLightingEnable", "0");
                RegistryFallback::AddDefault("g_CarDamageEnable", "0");
                RegistryFallback::AddDefault("g_CrowdEnable", "1");
                RegistryFallback::AddDefault("g_RoadReflectionEnable", "3");
                RegistryFallback::AddDefault("g_FogEnable", "1");
                RegistryFallback::AddDefault("g_MotionBlurEnable", "1");
                RegistryFallback::AddDefault("g_LightStreaksEnable", "1");
                RegistryFallback::AddDefault("g_LightGlowEnable", "1");
                RegistryFallback::AddDefault("g_AnimatedTextureEnable", "1");
                RegistryFallback::AddDefault("g_ParticleSystemEnable", "1");
                RegistryFallback::AddDefault("g_DepthOfFieldEnable", "1");
                RegistryFallback::AddDefault("g_WorldLodLevel", "3");
                RegistryFallback::AddDefault("g_CarLodLevel", "1");
                RegistryFallback::AddDefault("g_OverBrightEnable", "1");
                RegistryFallback::AddDefault("g_BleachByPassEnable", "1");
                RegistryFallback::AddDefault("g_TintingEnable", "1");
                RegistryFallback::AddDefault("g_FSAALevel", "7");
                RegistryFallback::AddDefault("g_HorizonFogEnable", "1");
                RegistryFallback::AddDefault("g_RainEnable", "1");
                RegistryFallback::AddDefault("g_TextureFiltering", "2");
                RegistryFallback::AddDefault("g_RacingResolution", "4");
                RegistryFallback::AddDefault("g_PerformanceLevel", "5");
                RegistryFallback::AddDefault("g_VSyncOn", "0");

                std::string exePath = GetExeModulePath<std::string>();
                RegistryFallback::AddDefault("Install Dir", exePath);
                RegistryFallback::AddDefault("InstallDir", exePath);
                RegistryFallback::AddDefault("Path", exePath);

                RegistryFallback::EnsureDefaults();
            }
        };
    }
} Registry;