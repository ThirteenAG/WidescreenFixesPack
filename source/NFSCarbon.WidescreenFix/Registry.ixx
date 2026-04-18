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
            // FirstTime might cause settings reset on each startup here
            static auto stub = "_FirstTime";
            auto pattern = hook::pattern("68 ? ? ? ? F7 D8 52");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(1), stub, true);

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

                SettingsSavePath.append("NFS Carbon");
                SettingsSavePath.append("Settings.ini");

                RegistryWrapper("Need for Speed", SettingsSavePath);
                RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
                RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryWrapper::AddDefault("CD Drive", "D:\\");
                RegistryWrapper::AddDefault("CacheSize", "5697825792");
                RegistryWrapper::AddDefault("SwapSize", "73400320");
                RegistryWrapper::AddDefault("Language", "English US");
                RegistryWrapper::AddDefault("StreamingInstall", "0");
                RegistryWrapper::AddDefault("VERSION", "1");
                RegistryWrapper::AddDefault("SIZE", "100");
                RegistryWrapper::AddDefault("FirstTime", "0");
                RegistryWrapper::AddDefault("g_CarEnvironmentMapEnable", "3");
                RegistryWrapper::AddDefault("g_RoadReflectionEnable", "2");
                RegistryWrapper::AddDefault("g_MotionBlurEnable", "1");
                RegistryWrapper::AddDefault("g_ParticleSystemEnable", "1");
                RegistryWrapper::AddDefault("g_WorldLodLevel", "3");
                RegistryWrapper::AddDefault("g_CarLodLevel", "1");
                RegistryWrapper::AddDefault("g_FSAALevel", "2");
                RegistryWrapper::AddDefault("g_RainEnable", "1");
                RegistryWrapper::AddDefault("g_TextureFiltering", "2");
                RegistryWrapper::AddDefault("g_RacingResolution", "1");
                RegistryWrapper::AddDefault("g_PerformanceLevel", "5");
                RegistryWrapper::AddDefault("g_VSyncOn", "0");
                RegistryWrapper::AddDefault("g_ShadowDetail", "0");
                RegistryWrapper::AddDefault("g_VisualTreatment", "1");
                RegistryWrapper::AddDefault("g_ShaderDetailLevel", "3");
                RegistryWrapper::AddDefault("g_Brightness", "0");
                RegistryWrapper::AddDefault("g_AudioMode", "0");

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
                    "Software\\Electronic Arts\\Need for Speed Carbon",
                    "Software\\Electronic Arts\\Need for Speed Carbono"
                );

                RegistryFallback::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryFallback::AddDefault("CD Drive", "D:\\");
                RegistryFallback::AddDefault("CacheSize", "5697825792");
                RegistryFallback::AddDefault("SwapSize", "73400320");
                RegistryFallback::AddDefault("Language", "English US");
                RegistryFallback::AddDefault("StreamingInstall", "0");
                RegistryFallback::AddDefault("VERSION", "1");
                RegistryFallback::AddDefault("SIZE", "100");
                RegistryFallback::AddDefault("FirstTime", "0");
                RegistryFallback::AddDefault("g_CarEnvironmentMapEnable", "3");
                RegistryFallback::AddDefault("g_RoadReflectionEnable", "2");
                RegistryFallback::AddDefault("g_MotionBlurEnable", "1");
                RegistryFallback::AddDefault("g_ParticleSystemEnable", "1");
                RegistryFallback::AddDefault("g_WorldLodLevel", "3");
                RegistryFallback::AddDefault("g_CarLodLevel", "1");
                RegistryFallback::AddDefault("g_FSAALevel", "2");
                RegistryFallback::AddDefault("g_RainEnable", "1");
                RegistryFallback::AddDefault("g_TextureFiltering", "2");
                RegistryFallback::AddDefault("g_RacingResolution", "1");
                RegistryFallback::AddDefault("g_PerformanceLevel", "5");
                RegistryFallback::AddDefault("g_VSyncOn", "0");
                RegistryFallback::AddDefault("g_ShadowDetail", "0");
                RegistryFallback::AddDefault("g_VisualTreatment", "1");
                RegistryFallback::AddDefault("g_ShaderDetailLevel", "3");
                RegistryFallback::AddDefault("g_Brightness", "0");
                RegistryFallback::AddDefault("g_AudioMode", "0");

                std::string exePath = GetExeModulePath<std::string>();
                RegistryFallback::AddDefault("Install Dir", exePath);
                RegistryFallback::AddDefault("InstallDir", exePath);
                RegistryFallback::AddDefault("Path", exePath);

                RegistryFallback::EnsureDefaults();

                IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
                    std::forward_as_tuple("RegCloseKey", RegistryFallback::RegCloseKey),
                    std::forward_as_tuple("RegCreateKeyA", RegistryFallback::RegCreateKeyA),
                    std::forward_as_tuple("RegOpenKeyA", RegistryFallback::RegOpenKeyA),
                    std::forward_as_tuple("RegOpenKeyExA", RegistryFallback::RegOpenKeyExA),
                    std::forward_as_tuple("RegCreateKeyExA", RegistryFallback::RegCreateKeyExA),
                    std::forward_as_tuple("RegQueryValueExA", RegistryFallback::RegQueryValueExA),
                    std::forward_as_tuple("RegSetValueExA", RegistryFallback::RegSetValueExA),
                    std::forward_as_tuple("RegQueryValueA", RegistryFallback::RegQueryValueA),
                    std::forward_as_tuple("RegDeleteKeyA", RegistryFallback::RegDeleteKeyA),
                    std::forward_as_tuple("RegEnumKeyA", RegistryFallback::RegEnumKeyA)
                );
            }
        };
    }
} Registry;