module;

#include <stdafx.h>
#include <FunctionHookMinHook.hpp>
#include "FixSettings.hpp"
#include "../Shared/NFS/LanguageSetting.hpp"

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

// The game's languages: the names the registry's "Language" value is compared with, each standing for a language
// id (0x710890 reads it), and the text files of each id (the table 0x594F90 looks ids up in). A name whose id has no
// text files in this exe can't be used: the language loader dereferences the missing entry and the game crashes.
// That is the case for Korean, both Chinese and Thai in the 1.1 exe (#665), even where their LANGUAGES files are
// installed; only the Asian exe has text tables for them. Japanese uses the English files. The text files are the
// files a language needs: movies fall back to the English ones, subtitles to the ones without a language suffix,
// speech is English only and LANGUAGES\LanguageTextures.bin is shared.
static std::vector<WFPLanguageSetting::Entry> GameLanguages()
{
    struct RegistryName
    {
        const char* name;
        int32_t id;
        int32_t region;
    };

    struct LanguageInfo
    {
        int32_t id;
        const char* name;
        const char* code;
        const char* files[4];
        uint32_t unknown[2];
    };
    static_assert(sizeof(LanguageInfo) == 0x24);

    auto names = hook::pattern("33 F6 39 35 ? ? ? ? 74 ? B8 ? ? ? ? 8B 08 51");
    auto info = hook::pattern("8B 54 24 04 33 C9 B8 ? ? ? ? EB 03 8D 49 00 39 10 74 10 83 C0 24 83 C1 01 3D ? ? ? ? 7C EF 33 C0 C3");
    if (names.empty() || info.empty())
        return {};

    auto infoBegin = *info.get_first<LanguageInfo*>(7);
    auto infoEnd = *info.get_first<LanguageInfo*>(0x1B);

    std::vector<WFPLanguageSetting::Entry> entries;
    for (auto name = *names.get_first<RegistryName*>(4); name->name; ++name)
    {
        auto language = std::find_if(infoBegin, infoEnd, [&](const LanguageInfo& entry) { return entry.id == name->id; });
        if (language == infoEnd)
            continue;

        WFPLanguageSetting::Entry entry{ name->name, {} };
        for (auto file : language->files)
        {
            if (file)
                entry.files.emplace_back(file);
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

// The game's registry reads, with its language answered by the fix
static LSTATUS WINAPI WrapperQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    LSTATUS result = ERROR_SUCCESS;
    if (WFPLanguageSetting::AnswerQuery(FixSettings().String("LANGUAGE.Language"), lpValueName, lpType, lpData, lpcbData, result))
        return result;
    return RegistryWrapper::RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

static LSTATUS WINAPI FallbackQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    LSTATUS result = ERROR_SUCCESS;
    if (WFPLanguageSetting::AnswerQuery(FixSettings().String("LANGUAGE.Language"), lpValueName, lpType, lpData, lpcbData, result))
        return result;
    return RegistryFallback::RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

class Registry
{
public:
    Registry()
    {
        WFP::onInitEvent() += []()
        {
            bool bWriteSettingsToFile = FixSettings().Bool("MAIN.PortableSettings");
            auto szCustomUserFilesDirectoryInGameDir = FixSettings().String("MAIN.CustomUserFilesDirectoryInGameDir");

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

            static std::string defaultLanguage = "Engish (US)";

            HKEY hKeyOrig = nullptr;
            if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Electronic Arts\\Need for Speed ProStreet", &hKeyOrig) == ERROR_SUCCESS)
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

                SettingsSavePath.append("NFS ProStreet");
                SettingsSavePath.append("Settings.ini");

                RegistryWrapper("Need for Speed", SettingsSavePath);
                RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
                RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryWrapper::AddDefault("CD Drive", "D:\\");
                RegistryWrapper::AddDefault("CacheSize", "5697825792");
                RegistryWrapper::AddDefault("SwapSize", "73400320");
                RegistryWrapper::AddDefault("Language", defaultLanguage.c_str());
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

                auto hooks = IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
                    std::forward_as_tuple("RegCloseKey", RegistryWrapper::RegCloseKey),
                    std::forward_as_tuple("RegCreateKeyA", RegistryWrapper::RegCreateKeyA),
                    std::forward_as_tuple("RegOpenKeyA", RegistryWrapper::RegOpenKeyA),
                    std::forward_as_tuple("RegOpenKeyExA", RegistryWrapper::RegOpenKeyExA),
                    std::forward_as_tuple("RegCreateKeyExA", RegistryWrapper::RegCreateKeyExA),
                    std::forward_as_tuple("RegQueryValueExA", WrapperQueryValueExA),
                    std::forward_as_tuple("RegSetValueExA", RegistryWrapper::RegSetValueExA),
                    std::forward_as_tuple("RegQueryValueA", RegistryWrapper::RegQueryValueA),
                    std::forward_as_tuple("RegDeleteKeyA", RegistryWrapper::RegDeleteKeyA),
                    std::forward_as_tuple("RegEnumKeyA", RegistryWrapper::RegEnumKeyA)
                );

                if (hooks.empty())
                {
                    InstallRegistryInlineHooks(
                        RegistryWrapper::RegCloseKey,
                        RegistryWrapper::RegCreateKeyA,
                        RegistryWrapper::RegOpenKeyA,
                        RegistryWrapper::RegOpenKeyExA,
                        RegistryWrapper::RegCreateKeyExA,
                        WrapperQueryValueExA,
                        RegistryWrapper::RegSetValueExA,
                        RegistryWrapper::RegQueryValueA,
                        RegistryWrapper::RegDeleteKeyA,
                        RegistryWrapper::RegEnumKeyA
                    );
                }
            }
            else
            {
                auto [DesktopResW, DesktopResH] = GetDesktopRes();

                RegistryFallback::Init(
                    "Software\\Electronic Arts\\Need for Speed ProStreet",
                    "Software\\Electronic Arts\\Electronic Arts\\Need for Speed ProStreet"
                );

                RegistryFallback::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryFallback::AddDefault("CD Drive", "D:\\");
                RegistryFallback::AddDefault("CacheSize", "5697825792");
                RegistryFallback::AddDefault("SwapSize", "73400320");
                RegistryFallback::AddDefault("Language", defaultLanguage.c_str());
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

                auto hooks = IATHook::Replace(GetModuleHandleA(NULL), "ADVAPI32.DLL",
                    std::forward_as_tuple("RegCloseKey", RegistryFallback::RegCloseKey),
                    std::forward_as_tuple("RegCreateKeyA", RegistryFallback::RegCreateKeyA),
                    std::forward_as_tuple("RegOpenKeyA", RegistryFallback::RegOpenKeyA),
                    std::forward_as_tuple("RegOpenKeyExA", RegistryFallback::RegOpenKeyExA),
                    std::forward_as_tuple("RegCreateKeyExA", RegistryFallback::RegCreateKeyExA),
                    std::forward_as_tuple("RegQueryValueExA", FallbackQueryValueExA),
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
                        FallbackQueryValueExA,
                        RegistryFallback::RegSetValueExA,
                        RegistryFallback::RegQueryValueA,
                        RegistryFallback::RegDeleteKeyA,
                        RegistryFallback::RegEnumKeyA
                    );
                }
            }

            // The language setting: only languages the exe can load and whose files are there can be used, and the
            // installer's is stored on the first start (see LanguageSetting.hpp). The registry keeps the installer's;
            // the game's query of the "Language" value gets the setting.
            WFPLanguageSetting::Startup(FixSettings(), "LANGUAGE.Language", GameLanguages(), defaultLanguage);
        };
    }
} Registry;