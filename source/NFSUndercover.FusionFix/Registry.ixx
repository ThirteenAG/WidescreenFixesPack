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

// The game's language: the "Language" registry value names it, the game compares it with its table of names (it
// spells English "Engish") and takes the language id from it; an unknown name keeps English. A second table gives
// each id the name of its text files, LANGUAGES\<name>_GLOBAL.BIN, _FRONTEND.BIN, _INGAME.BIN and _SUBTITLES.BIN,
// and ids that aren't in it load "UNKNOWN" files, which no install has. The builds differ there: the Steam and 1.0
// builds have no Korean, Chinese and Thai, 1.1.2.1 has Chinese (Simplified) and Thai and shows Chinese
// (Traditional) in English, and none has Korean. The speech, the movies and the memory card texts fall back to
// English when a language has none, and the language textures are shared, so the text files are what a language
// needs.
namespace Language
{
    // The registry's names (the settings' languages, LANGUAGE.Language) and their language ids
    struct GameLanguage
    {
        const char* registry;
        int32_t id;
    };

    constexpr GameLanguage GameLanguages[] = {
        { "Engish (US)", 0 }, { "Engish (UK)", 0 }, { "French", 1 }, { "German", 2 }, { "Italian", 3 }, { "Spanish", 4 },
        { "Mexican Spanish", 10 }, { "Swedish", 6 }, { "Danish", 7 }, { "Czech", 9 }, { "Hungarian", 8 }, { "Dutch", 5 },
        { "Korean", 11 }, { "Chinese (Traditional)", 12 }, { "Chinese (Simplified)", 13 }, { "Japanese", 14 }, { "Thai", 16 },
        { "Polish", 17 }, { "Finnish", 18 }, { "Russian", 15 },
    };

    // The name of a language's text files, from the game's table ({id, type, name, filter, locale}); the Steam
    // build's table when it isn't found
    std::string FileName(int32_t id)
    {
        struct LanguageInfo
        {
            int32_t id;
            int32_t type;
            const char* name;
            const char* filter;
            void* locale;
        };
        static_assert(sizeof(LanguageInfo) == 0x14);

        auto pattern = hook::pattern("8B 54 24 04 33 C9 B8 ? ? ? ? EB 03 8D 49 00 39 10 74 12");
        if (!pattern.empty())
        {
            auto begin = *pattern.get_first<LanguageInfo*>(7);
            auto end = *pattern.get_first<LanguageInfo*>(0x1B);
            for (auto info = begin; info < end; ++info)
            {
                if (info->id == id)
                    return info->name ? info->name : "UNKNOWN";
            }
            return "UNKNOWN";
        }

        static const std::pair<int32_t, const char*> steam[] = {
            { 0, "ENGLISH" }, { 1, "FRENCH" }, { 2, "GERMAN" }, { 3, "ITALIAN" }, { 4, "SPANISH" }, { 5, "DUTCH" },
            { 6, "SWEDISH" }, { 17, "POLISH" }, { 18, "FINNISH" }, { 7, "DANISH" }, { 8, "HUNGARIAN" }, { 9, "CZECH" },
            { 10, "MEXICAN" }, { 15, "RUSSIAN" }, { 19, "NORWEGIAN" }, { 14, "JAPANESE" },
        };
        for (auto& [languageId, name] : steam)
        {
            if (languageId == id)
                return name;
        }
        return "UNKNOWN";
    }

    std::vector<WFPLanguageSetting::Entry> Entries()
    {
        std::vector<WFPLanguageSetting::Entry> entries;
        for (auto& language : GameLanguages)
        {
            auto name = FileName(language.id);
            entries.push_back({ language.registry, {
                "LANGUAGES\\" + name + "_GLOBAL.BIN", "LANGUAGES\\" + name + "_FRONTEND.BIN",
                "LANGUAGES\\" + name + "_INGAME.BIN", "LANGUAGES\\" + name + "_SUBTITLES.BIN" } });
        }
        return entries;
    }

    // The installer's language, from the real registry; portable installs have none
    std::string Installer()
    {
        std::string language;
        HKEY hKey = nullptr;
        if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\EA Games\\Need for Speed Undercover", &hKey) == ERROR_SUCCESS)
        {
            char szLang[256] = {};
            DWORD cbData = sizeof(szLang) - 1;
            if (RegQueryValueExA(hKey, "Language", nullptr, nullptr, (LPBYTE)szLang, &cbData) == ERROR_SUCCESS && cbData > 1)
                language = szLang;
            RegCloseKey(hKey);
        }
        return language;
    }
}

// The language setting: only languages whose text files are there can be used, and the installer's is stored on the
// first start (see LanguageSetting.hpp). Before the registry is redirected.
namespace Language
{
    void Startup()
    {
        WFPLanguageSetting::Startup(FixSettings(), "LANGUAGE.Language", Entries(), Installer());
    }
}

// The game's read of its language gets the language setting; the registry itself isn't changed
static LSTATUS WINAPI WrapperQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    LSTATUS result;
    if (WFPLanguageSetting::AnswerQuery(FixSettings().String("LANGUAGE.Language"), lpValueName, lpType, lpData, lpcbData, result))
        return result;
    return RegistryWrapper::RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

static LSTATUS WINAPI FallbackQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    LSTATUS result;
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
            auto ModuleStart = (uintptr_t)GetModuleHandle(NULL);
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
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

                // The installer's language as the redirected registry's default; the game's read of it gets the language
                // setting (an installed language, which may differ from what the key holds)
                Language::Startup();
                static std::string defaultLanguage = Language::Installer();
                if (defaultLanguage.empty())
                    defaultLanguage = "Engish (US)";

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

                    SettingsSavePath.append("NFS Undercover");
                    SettingsSavePath.append("Settings.ini");

                    RegistryWrapper("Need for Speed", SettingsSavePath);
                    RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
                    RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
                    RegistryWrapper::AddDefault("CD Drive", "D:\\");
                    RegistryWrapper::AddDefault("FirstTime", "0");
                    RegistryWrapper::AddDefault("CacheSize", "5697825792");
                    RegistryWrapper::AddDefault("SwapSize", "73400320");
                    RegistryWrapper::AddDefault("Language", defaultLanguage.c_str());
                    RegistryWrapper::AddDefault("StreamingInstall", "0");
                    RegistryWrapper::AddDefault("g_CarEffects", "3");
                    RegistryWrapper::AddDefault("g_WorldFXLevel", "3");
                    RegistryWrapper::AddDefault("g_RoadReflectionEnable", "0");
                    RegistryWrapper::AddDefault("g_WorldLodLevel", "2");
                    RegistryWrapper::AddDefault("g_CarLodLevel", "0");
                    RegistryWrapper::AddDefault("g_FSAALevel", "3");
                    RegistryWrapper::AddDefault("g_RainEnable", "0");
                    RegistryWrapper::AddDefault("g_TextureFiltering", "2");
                    RegistryWrapper::AddDefault("g_PerformanceLevel", "0");
                    RegistryWrapper::AddDefault("g_VSyncOn", "0");
                    RegistryWrapper::AddDefault("g_ShadowEnable", "3");
                    RegistryWrapper::AddDefault("g_SmokeEnable", "1");
                    RegistryWrapper::AddDefault("g_Brightness", "68");
                    RegistryWrapper::AddDefault("g_ShaderDetailLevel", "0");
                    RegistryWrapper::AddDefault("g_AudioDetail", "0");
                    RegistryWrapper::AddDefault("g_AudioMode", "1");
                    RegistryWrapper::AddDefault("g_Width", std::to_string(DesktopResW));
                    RegistryWrapper::AddDefault("g_Height", std::to_string(DesktopResH));
                    RegistryWrapper::AddDefault("g_Refresh", "60");
                    RegistryWrapper::AddDefault("g_CarDamageDetail", "2");
                    RegistryWrapper::AddDefault("AllowR32FAA", "0");
                    RegistryWrapper::AddDefault("ForceR32AA", "0");

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

                    // get the ShadowLevel setting from the settings ini -- it's important to get this before the game uses it
                    CIniReader shadowini(SettingsSavePath);
                    ShadowLevel = shadowini.ReadInteger("Need for Speed Undercover", "g_ShadowEnable", 3);
                }
                else
                {
                    auto [DesktopResW, DesktopResH] = GetDesktopRes();

                    RegistryFallback::Init(
                        "Software\\EA Games\\Need for Speed Undercover",
                        "Software\\Electronic Arts\\EA Games\\Need for Speed Undercover"
                    );

                    RegistryFallback::AddDefault("@", "INSERTYOURCDKEYHERE");
                    RegistryFallback::AddDefault("CD Drive", "D:\\");
                    RegistryFallback::AddDefault("FirstTime", "0");
                    RegistryFallback::AddDefault("CacheSize", "5697825792");
                    RegistryFallback::AddDefault("SwapSize", "73400320");
                    RegistryFallback::AddDefault("Language", defaultLanguage.c_str());
                    RegistryFallback::AddDefault("StreamingInstall", "0");
                    RegistryFallback::AddDefault("g_CarEffects", "3");
                    RegistryFallback::AddDefault("g_WorldFXLevel", "3");
                    RegistryFallback::AddDefault("g_RoadReflectionEnable", "0");
                    RegistryFallback::AddDefault("g_WorldLodLevel", "2");
                    RegistryFallback::AddDefault("g_CarLodLevel", "0");
                    RegistryFallback::AddDefault("g_FSAALevel", "3");
                    RegistryFallback::AddDefault("g_RainEnable", "0");
                    RegistryFallback::AddDefault("g_TextureFiltering", "2");
                    RegistryFallback::AddDefault("g_PerformanceLevel", "0");
                    RegistryFallback::AddDefault("g_VSyncOn", "0");
                    RegistryFallback::AddDefault("g_ShadowEnable", "3");
                    RegistryFallback::AddDefault("g_SmokeEnable", "1");
                    RegistryFallback::AddDefault("g_Brightness", "68");
                    RegistryFallback::AddDefault("g_ShaderDetailLevel", "0");
                    RegistryFallback::AddDefault("g_AudioDetail", "0");
                    RegistryFallback::AddDefault("g_AudioMode", "1");
                    RegistryFallback::AddDefault("g_Width", std::to_string(DesktopResW));
                    RegistryFallback::AddDefault("g_Height", std::to_string(DesktopResH));
                    RegistryFallback::AddDefault("g_Refresh", "60");
                    RegistryFallback::AddDefault("g_CarDamageDetail", "2");
                    RegistryFallback::AddDefault("AllowR32FAA", "0");
                    RegistryFallback::AddDefault("ForceR32AA", "0");

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

                    // get the ShadowLevel setting from the registry -- it's important to get this before the game uses it
                    HKEY hUCKey = 0;
                    DWORD type = REG_DWORD;
                    DWORD cbtype = REG_DWORD;
                    LSTATUS status = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\EA Games\\Need for Speed Undercover", &hUCKey);
                    if (status == ERROR_SUCCESS)
                    {
                        RegQueryValueExA(hUCKey, "g_ShadowEnable", NULL, &type, (LPBYTE)&ShadowLevel, &cbtype);
                        RegCloseKey(hUCKey);
                    }
                }

            }, hook::range_pattern(ModuleStart, RangeEnd, "FF 15 ? ? ? ? 8D 54 24 40 52 68 3F 00 0F 00"));
        };
    }
} Registry;