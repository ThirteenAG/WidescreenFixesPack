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

// The game's language: the "Language" registry value names it, the game compares it with its table of names
// ("English US", "German", ...) and takes the language id and region from it; an unknown name is English US. The
// id picks LANGUAGES\<file>.BIN, and the movies are MOVIES\<movie>_<language>_<ntsc|pal>.vp6 with the language's
// name from the same table and "_pal" for every European region (all names but English US); a language without
// its movies plays the English ones of the same region, and without those the story movies and tutorials are
// skipped. The speech is in shared banks and the language textures in LanguageTextures.bin. An install has only
// some of the files (a German install only GERMAN.BIN); with a language whose text file is missing every text is
// missing and the game crashed making the default profile name. Korean, Chinese, Japanese and Thai are in the
// table of names but have no text file in the European and American builds, which crash looking them up, so they
// aren't offered.
namespace Language
{
    // A movie of every release in the language's movies, or in the English ones the game falls back to
    std::string Movie(const char* language, bool pal)
    {
        auto file = [&](const char* name) { return std::string("MOVIES\\blacklist_01_") + name + (pal ? "_pal.vp6" : "_ntsc.vp6"); };
        std::error_code ec;
        if (std::filesystem::exists(GetExeModulePath<std::filesystem::path>() / file(language), ec))
            return file(language);
        return file("english");
    }

    // The settings' languages (LANGUAGE.Language) and the files each needs
    std::vector<WFPLanguageSetting::Entry> Entries()
    {
        return {
            { "English US", { "LANGUAGES\\ENGLISH.BIN", Movie("english", false) } },
            { "English UK", { "LANGUAGES\\ENGLISH.BIN", Movie("english", true) } },
            { "French", { "LANGUAGES\\FRENCH.BIN", Movie("french", true) } },
            { "German", { "LANGUAGES\\GERMAN.BIN", Movie("german", true) } },
            { "Italian", { "LANGUAGES\\ITALIAN.BIN", Movie("italian", true) } },
            { "Spanish", { "LANGUAGES\\SPANISH.BIN", Movie("spanish", true) } },
            { "Swedish", { "LANGUAGES\\SWEDISH.BIN", Movie("swedish", true) } },
            { "Danish", { "LANGUAGES\\DANISH.BIN", Movie("danish", true) } },
            { "Dutch", { "LANGUAGES\\DUTCH.BIN", Movie("dutch", true) } },
            { "Polish", { "LANGUAGES\\POLISH.BIN", Movie("polish", true) } },
            { "Russian", { "LANGUAGES\\RUSSIAN.BIN", Movie("russian", true) } },
        };
    }

    // The installer's language, from the real registry; portable installs have none
    std::string Installer()
    {
        std::string language;
        HKEY hKey = nullptr;
        if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\EA Games\\Need for Speed Most Wanted", &hKey) == ERROR_SUCCESS)
        {
            char szLang[256] = {};
            DWORD cbData = sizeof(szLang) - 1;
            if (RegQueryValueExA(hKey, "Language", nullptr, nullptr, (LPBYTE)szLang, &cbData) == ERROR_SUCCESS && cbData > 1)
                language = szLang;
            RegCloseKey(hKey);
        }
        return language;
    }

    // The language setting: only languages whose files are there can be used, and the installer's is stored on the
    // first start (see LanguageSetting.hpp). Before the registry is redirected.
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
                defaultLanguage = "English US";

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

                SettingsSavePath.append("NFS Most Wanted");
                SettingsSavePath.append("Settings.ini");

                RegistryWrapper("Need for Speed", SettingsSavePath);
                RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
                RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryWrapper::AddDefault("CD Drive", "D:\\");
                RegistryWrapper::AddDefault("CacheSize", "2936691712");
                RegistryWrapper::AddDefault("SwapSize", "73400320");
                RegistryWrapper::AddDefault("Language", defaultLanguage.c_str());
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
                RegistryFallback::Init(
                    "Software\\EA Games\\Need for Speed Most Wanted"
                );

                RegistryFallback::AddDefault("@", "INSERTYOURCDKEYHERE");
                RegistryFallback::AddDefault("CD Drive", "D:\\");
                RegistryFallback::AddDefault("CacheSize", "2936691712");
                RegistryFallback::AddDefault("SwapSize", "73400320");
                RegistryFallback::AddDefault("Language", defaultLanguage.c_str());
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

        };
    }
} Registry;