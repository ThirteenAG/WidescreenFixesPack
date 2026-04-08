module;

#include <stdafx.h>

export module Registry;

import ComVars;

class Registry
{
public:
    Registry()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 0) != 0;
            static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
            static std::filesystem::path CustomUserDir;
            if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
                szCustomUserFilesDirectoryInGameDir.clear();

            auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");

            if (!szCustomUserFilesDirectoryInGameDir.empty())
            {
                CustomUserDir = GetExeModulePath<std::filesystem::path>();
                CustomUserDir.append(szCustomUserFilesDirectoryInGameDir);

                auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
                {
                    CreateDirectoryW((LPCWSTR)(CustomUserDir.u16string().c_str()), NULL);
                    memcpy(pszPath, CustomUserDir.u8string().data(), CustomUserDir.u8string().size() + 1);
                    return S_OK;
                };

                for (size_t i = 0; i < GetFolderPathpattern.size(); i++)
                {
                    uint32_t* dword_6CBF17 = GetFolderPathpattern.get(i).get<uint32_t>(12);
                    if (*(BYTE*)dword_6CBF17 != 0xFF)
                        dword_6CBF17 = GetFolderPathpattern.get(i).get<uint32_t>(14);

                    injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
                    injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
                }
            }

            if (bWriteSettingsToFile)
            {
                std::filesystem::path SettingsSavePath;
                if (!szCustomUserFilesDirectoryInGameDir.empty())
                    SettingsSavePath = CustomUserDir;

                uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.get(0).get<uintptr_t>(14), true).as_int();

                if (GetFolderPathCallDest && szCustomUserFilesDirectoryInGameDir.empty())
                {
                    char szSettingsSavePath[MAX_PATH];
                    injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
                    SettingsSavePath = szSettingsSavePath;
                }

                SettingsSavePath.append("NFS Most Wanted");
                SettingsSavePath.append("Settings.ini");

                if (GetFolderPathCallDest || !szCustomUserFilesDirectoryInGameDir.empty())
                {
                    RegistryWrapper("Need for Speed", SettingsSavePath);
                    RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
                    RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
                    RegistryWrapper::AddDefault("CD Drive", "D:\\");
                    RegistryWrapper::AddDefault("CacheSize", "2936691712");
                    RegistryWrapper::AddDefault("SwapSize", "73400320");
                    RegistryWrapper::AddDefault("Language", "English");
                    RegistryWrapper::AddDefault("StreamingInstall", "0");
                    RegistryWrapper::AddDefault("VTMode", "0");
                    RegistryWrapper::AddDefault("VERSION", "0");
                    RegistryWrapper::AddDefault("SIZE", "0");
                    RegistryWrapper::AddDefault("g_CarEnvironmentMapEnable", "3");
                    RegistryWrapper::AddDefault("g_CarEnvironmentMapUpdateData", "1");
                    RegistryWrapper::AddDefault("g_RoadReflectionEnable", "3");
                    RegistryWrapper::AddDefault("g_MotionBlurEnable", "1");
                    RegistryWrapper::AddDefault("g_ParticleSystemEnable", "1");
                    RegistryWrapper::AddDefault("g_WorldLodLevel", "3");
                    RegistryWrapper::AddDefault("g_CarLodLevel", "1");
                    RegistryWrapper::AddDefault("g_OverBrightEnable", "1");
                    RegistryWrapper::AddDefault("g_FSAALevel", "7");
                    RegistryWrapper::AddDefault("g_RainEnable", "1");
                    RegistryWrapper::AddDefault("g_TextureFiltering", "2");
                    RegistryWrapper::AddDefault("g_RacingResolution", "1");
                    RegistryWrapper::AddDefault("g_PerformanceLevel", "5");
                    RegistryWrapper::AddDefault("g_VSyncOn", "0");
                    RegistryWrapper::AddDefault("g_ShadowDetail", "2");
                    RegistryWrapper::AddDefault("g_VisualTreatment", "1");

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
            }
            else
            {
                RegistryFallback::Init(
                    "Software\\EA Games\\Need for Speed Most Wanted"
                );

                RegistryFallback::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryFallback::AddDefault("CD Drive", "D:\\");
                RegistryFallback::AddDefault("CacheSize", "2936691712");
                RegistryFallback::AddDefault("SwapSize", "73400320");
                RegistryFallback::AddDefault("Language", "English");
                RegistryFallback::AddDefault("StreamingInstall", "0");
                RegistryFallback::AddDefault("VTMode", "0");
                RegistryFallback::AddDefault("VERSION", "0");
                RegistryFallback::AddDefault("SIZE", "0");
                RegistryFallback::AddDefault("g_CarEnvironmentMapEnable", "3");
                RegistryFallback::AddDefault("g_CarEnvironmentMapUpdateData", "1");
                RegistryFallback::AddDefault("g_RoadReflectionEnable", "3");
                RegistryFallback::AddDefault("g_MotionBlurEnable", "1");
                RegistryFallback::AddDefault("g_ParticleSystemEnable", "1");
                RegistryFallback::AddDefault("g_WorldLodLevel", "3");
                RegistryFallback::AddDefault("g_CarLodLevel", "1");
                RegistryFallback::AddDefault("g_OverBrightEnable", "1");
                RegistryFallback::AddDefault("g_FSAALevel", "7");
                RegistryFallback::AddDefault("g_RainEnable", "1");
                RegistryFallback::AddDefault("g_TextureFiltering", "2");
                RegistryFallback::AddDefault("g_RacingResolution", "1");
                RegistryFallback::AddDefault("g_PerformanceLevel", "5");
                RegistryFallback::AddDefault("g_VSyncOn", "0");
                RegistryFallback::AddDefault("g_ShadowDetail", "2");
                RegistryFallback::AddDefault("g_VisualTreatment", "1");

                std::string exePath = GetExeModulePath<std::string>();
                RegistryFallback::AddDefault("Install Dir", exePath);
                RegistryFallback::AddDefault("InstallDir", exePath);
                RegistryFallback::AddDefault("Path", exePath);

                RegistryFallback::EnsureDefaults();
            }
        };
    }
} Registry;