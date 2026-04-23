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

            static std::string defaultLanguage = "English US";

            HKEY hKeyOrig = nullptr;
            if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Electronic Arts\\Need for Speed Carbon", &hKeyOrig) == ERROR_SUCCESS)
            {
                char szLang[256] = {};
                DWORD cbData = sizeof(szLang);
                if (RegQueryValueExA(hKeyOrig, "Language", nullptr, nullptr, (LPBYTE)szLang, &cbData) == ERROR_SUCCESS && cbData > 1)
                {
                    defaultLanguage = szLang;
                }
                RegCloseKey(hKeyOrig);
            }

            auto InstallRegistryInlineHooks = [](
                decltype(&RegCloseKey)      fRegCloseKey,
                decltype(&RegCreateKeyA)    fRegCreateKeyA,
                decltype(&RegOpenKeyA)      fRegOpenKeyA,
                decltype(&RegOpenKeyExA)    fRegOpenKeyExA,
                decltype(&RegCreateKeyExA)  fRegCreateKeyExA,
                decltype(&RegQueryValueExA) fRegQueryValueExA,
                decltype(&RegSetValueExA)   fRegSetValueExA,
                decltype(&RegQueryValueA)   fRegQueryValueA,
                decltype(&RegDeleteKeyA)    fRegDeleteKeyA,
                decltype(&RegEnumKeyA)      fRegEnumKeyA)
            {
                static auto IsCallerFromModule = [](HMODULE hModule) -> bool
                {
                    for (const auto& entry : std::stacktrace::current(1, 4))
                    {
                        HMODULE hCaller = NULL;
                        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)entry.native_handle(), &hCaller) && hCaller == hModule)
                            return true;
                    }
                    return false;
                };

                static decltype(&RegCloseKey)      sRegCloseKey;
                static decltype(&RegCreateKeyA)    sRegCreateKeyA;
                static decltype(&RegOpenKeyA)      sRegOpenKeyA;
                static decltype(&RegOpenKeyExA)    sRegOpenKeyExA;
                static decltype(&RegCreateKeyExA)  sRegCreateKeyExA;
                static decltype(&RegQueryValueExA) sRegQueryValueExA;
                static decltype(&RegSetValueExA)   sRegSetValueExA;
                static decltype(&RegQueryValueA)   sRegQueryValueA;
                static decltype(&RegDeleteKeyA)    sRegDeleteKeyA;
                static decltype(&RegEnumKeyA)      sRegEnumKeyA;

                sRegCloseKey = fRegCloseKey;
                sRegCreateKeyA = fRegCreateKeyA;
                sRegOpenKeyA = fRegOpenKeyA;
                sRegOpenKeyExA = fRegOpenKeyExA;
                sRegCreateKeyExA = fRegCreateKeyExA;
                sRegQueryValueExA = fRegQueryValueExA;
                sRegSetValueExA = fRegSetValueExA;
                sRegQueryValueA = fRegQueryValueA;
                sRegDeleteKeyA = fRegDeleteKeyA;
                sRegEnumKeyA = fRegEnumKeyA;

                static SafetyHookInline shRegCloseKey = {};
                shRegCloseKey = safetyhook::create_inline(RegCloseKey, static_cast<decltype(&RegCloseKey)>([](HKEY hKey) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegCloseKey(hKey);
                    return shRegCloseKey.stdcall<LSTATUS>(hKey);
                }));

                static SafetyHookInline shRegCreateKeyA = {};
                shRegCreateKeyA = safetyhook::create_inline(RegCreateKeyA, static_cast<decltype(&RegCreateKeyA)>([](HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegCreateKeyA(hKey, lpSubKey, phkResult);
                    return shRegCreateKeyA.stdcall<LSTATUS>(hKey, lpSubKey, phkResult);
                }));

                static SafetyHookInline shRegOpenKeyA = {};
                shRegOpenKeyA = safetyhook::create_inline(RegOpenKeyA, static_cast<decltype(&RegOpenKeyA)>([](HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegOpenKeyA(hKey, lpSubKey, phkResult);
                    return shRegOpenKeyA.stdcall<LSTATUS>(hKey, lpSubKey, phkResult);
                }));

                static SafetyHookInline shRegOpenKeyExA = {};
                shRegOpenKeyExA = safetyhook::create_inline(RegOpenKeyExA, static_cast<decltype(&RegOpenKeyExA)>([](HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
                    return shRegOpenKeyExA.stdcall<LSTATUS>(hKey, lpSubKey, ulOptions, samDesired, phkResult);
                }));

                static SafetyHookInline shRegCreateKeyExA = {};
                shRegCreateKeyExA = safetyhook::create_inline(RegCreateKeyExA, static_cast<decltype(&RegCreateKeyExA)>([](HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, CONST LPSECURITY_ATTRIBUTES lpSA, PHKEY phkResult, LPDWORD lpdwDisp) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegCreateKeyExA(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSA, phkResult, lpdwDisp);
                    return shRegCreateKeyExA.stdcall<LSTATUS>(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSA, phkResult, lpdwDisp);
                }));

                static SafetyHookInline shRegQueryValueExA = {};
                shRegQueryValueExA = safetyhook::create_inline(RegQueryValueExA, static_cast<decltype(&RegQueryValueExA)>([](HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
                    return shRegQueryValueExA.stdcall<LSTATUS>(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
                }));

                static SafetyHookInline shRegSetValueExA = {};
                shRegSetValueExA = safetyhook::create_inline(RegSetValueExA, static_cast<decltype(&RegSetValueExA)>([](HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
                    return shRegSetValueExA.stdcall<LSTATUS>(hKey, lpValueName, Reserved, dwType, lpData, cbData);
                }));

                static SafetyHookInline shRegQueryValueA = {};
                shRegQueryValueA = safetyhook::create_inline(RegQueryValueA, static_cast<decltype(&RegQueryValueA)>([](HKEY hKey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegQueryValueA(hKey, lpSubKey, lpData, lpcbData);
                    return shRegQueryValueA.stdcall<LSTATUS>(hKey, lpSubKey, lpData, lpcbData);
                }));

                static SafetyHookInline shRegDeleteKeyA = {};
                shRegDeleteKeyA = safetyhook::create_inline(RegDeleteKeyA, static_cast<decltype(&RegDeleteKeyA)>([](HKEY hKey, LPCSTR lpSubKey)->LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegDeleteKeyA(hKey, lpSubKey);
                    return shRegDeleteKeyA.stdcall<LSTATUS>(hKey, lpSubKey);
                }));

                static SafetyHookInline shRegEnumKeyA = {};
                shRegEnumKeyA = safetyhook::create_inline(RegEnumKeyA, static_cast<decltype(&RegEnumKeyA)>([](HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cchName) -> LSTATUS
                {
                    if (IsCallerFromModule(GetModuleHandleA(NULL))) return sRegEnumKeyA(hKey, dwIndex, lpName, cchName);
                    return shRegEnumKeyA.stdcall<LSTATUS>(hKey, dwIndex, lpName, cchName);
                }));
            };

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
                RegistryWrapper::AddDefault("Language", defaultLanguage.c_str());
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

                auto hooks = IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
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

                if (hooks.size() <= 1)
                {
                    InstallRegistryInlineHooks(
                        RegistryWrapper::RegCloseKey,
                        RegistryWrapper::RegCreateKeyA,
                        RegistryWrapper::RegOpenKeyA,
                        RegistryWrapper::RegOpenKeyExA,
                        RegistryWrapper::RegCreateKeyExA,
                        RegistryWrapper::RegQueryValueExA,
                        RegistryWrapper::RegSetValueExA,
                        RegistryWrapper::RegQueryValueA,
                        RegistryWrapper::RegDeleteKeyA,
                        RegistryWrapper::RegEnumKeyA
                    );
                }
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
                RegistryFallback::AddDefault("Language", defaultLanguage.c_str());
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

                auto hooks = IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
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

                if (hooks.empty())
                {
                    InstallRegistryInlineHooks(
                        RegistryFallback::RegCloseKey,
                        RegistryFallback::RegCreateKeyA,
                        RegistryFallback::RegOpenKeyA,
                        RegistryFallback::RegOpenKeyExA,
                        RegistryFallback::RegCreateKeyExA,
                        RegistryFallback::RegQueryValueExA,
                        RegistryFallback::RegSetValueExA,
                        RegistryFallback::RegQueryValueA,
                        RegistryFallback::RegDeleteKeyA,
                        RegistryFallback::RegEnumKeyA
                    );
                }
            }

            auto szLanguage = iniReader.ReadString("LANGUAGE", "Language", "");
            if (!szLanguage.empty())
            {
                HKEY hKey = nullptr;
                if (bWriteSettingsToFile)
                {
                    if (RegistryWrapper::RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Electronic Arts\\Need for Speed Carbon", &hKey) == ERROR_SUCCESS)
                    {
                        RegistryWrapper::RegSetValueExA(hKey, "Language", 0, REG_SZ, (const BYTE*)szLanguage.c_str(), (DWORD)szLanguage.size() + 1);
                        RegistryWrapper::RegCloseKey(hKey);
                    }
                }
                else
                {
                    if (RegistryFallback::RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Electronic Arts\\Need for Speed Carbon", &hKey) == ERROR_SUCCESS)
                    {
                        RegistryFallback::RegSetValueExA(hKey, "Language", 0, REG_SZ, (const BYTE*)szLanguage.c_str(), (DWORD)szLanguage.size() + 1);
                        RegistryFallback::RegCloseKey(hKey);
                    }
                }
            }
        };
    }
} Registry;