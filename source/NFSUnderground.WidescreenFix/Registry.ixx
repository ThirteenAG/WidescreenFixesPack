module;

#include <stdafx.h>
#include <FunctionHookMinHook.hpp>
#include "FixSettings.hpp"

export module Registry;

import ComVars;

// The game's language. At startup the game reads the "Language" value of its HKLM key, which the installer writes,
// and looks the name up in a table of { name, language id, region }; without the key, the value or a match the
// language is English. The id picks the text (LANGUAGES\<language>.BIN) and the speech: the bank
// sound\strmBank\STRM_<language>_Speech_MB.abk and its stream SPEECHSTREAMS\STRM_<language>_Speech_MB.ast. French,
// German, Italian and Spanish have speech of their own, the other languages use the English one. Without the text
// file the game can't load its text, and without the speech stream the first car purchase never completes, because the
// game waits for Samantha's comment (#2266). The other per-language files don't matter: the movies are the same
// for all languages (the exe's table of localized intro movies is unused), and the online terms (LANGUAGES\agree.*)
// are skipped when they're missing. The game never writes the value, and the language select screen of the boot
// flow (ShowLangSelect) only changes the language until the game is closed.
namespace Language
{
    struct GameLanguage
    {
        const char* name;
        int32_t id;
        int32_t region;
    };

    GameLanguage* Table = nullptr;
    int32_t* Current = nullptr;

    // The settings' languages (LANGUAGE.Language) and the files each needs: the text and its speech
    struct Files
    {
        const char* name;
        const char* text;
        const char* speech;
    };
    constexpr Files Known[] = {
        { "English US", "ENGLISH", "English" }, { "English UK", "ENGLISH", "English" }, { "French", "FRENCH", "French" },
        { "German", "GERMAN", "German" }, { "Italian", "ITALIAN", "Italian" }, { "Spanish", "SPANISH", "Spanish" },
        { "Dutch", "DUTCH", "English" }, { "Swedish", "SWEDISH", "English" }, { "Russian", "RUSSIAN", "English" },
        { "Korean", "KOREAN", "English" }, { "Chinese (Traditional)", "CHINESE", "English" },
    };

    const GameLanguage* Find(std::string_view name)
    {
        for (auto info = Table; info && info->name; ++info)
        {
            if (name.size() == strlen(info->name) && _strnicmp(info->name, name.data(), name.size()) == 0)
                return info;
        }
        return nullptr;
    }

    // A language can be used when the exe has it and its files are there. A file whose folder is missing altogether
    // doesn't count: such installs stream the speech from the disc.
    bool Usable(std::string_view name)
    {
        if (!Find(name))
            return false;
        auto files = std::find_if(std::begin(Known), std::end(Known), [&](const Files& f) { return _stricmp(f.name, std::string(name).c_str()) == 0; });
        if (files == std::end(Known))
            return false;
        auto folder = GetExeModulePath<std::filesystem::path>();
        for (auto path : { folder / "LANGUAGES" / (std::string(files->text) + ".BIN"),
                           folder / "sound" / "strmBank" / ("STRM_" + std::string(files->speech) + "_Speech_MB.abk"),
                           folder / "SPEECHSTREAMS" / ("STRM_" + std::string(files->speech) + "_Speech_MB.ast") })
        {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec) && std::filesystem::exists(path.parent_path(), ec))
                return false;
        }
        return true;
    }

    // The installer's language, from the key the game reads (European builds use "NFS Underground")
    std::string InstallerLanguage()
    {
        for (auto key : { "Software\\EA Games\\Need For Speed Underground", "Software\\EA Games\\NFS Underground" })
        {
            char value[64] = {};
            DWORD size = sizeof(value) - 1;
            if (RegGetValueA(HKEY_LOCAL_MACHINE, key, "Language", RRF_RT_REG_SZ, nullptr, value, &size) == ERROR_SUCCESS && value[0])
                return value;
        }
        return {};
    }

    void Startup()
    {
        auto table = hook::pattern("8B 0C 85 ? ? ? ? 89 0D ? ? ? ? EB 06 89 3D");
        auto read = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? A2 ? ? ? ? E8");
        if (table.empty() || read.empty())
            return;
        Table = reinterpret_cast<GameLanguage*>(*table.get_first<uintptr_t>(3) - offsetof(GameLanguage, id));
        Current = *table.get_first<int32_t*>(9);

        // The installer's language until the player picks one; only installed languages can be used (a choice that
        // isn't installed stays in the .cfg, and the default or the first installed language is in effect meanwhile)
        auto& settings = FixSettings();
        if (!settings.Stored("LANGUAGE.Language"))
        {
            if (auto installer = InstallerLanguage(); !installer.empty())
                settings.Set("LANGUAGE.Language", installer, true);
        }
        settings.Availability("LANGUAGE.Language", [](std::string_view value) { return Usable(value); });

        // After the game has read the registry: the language in effect replaces the installer's
        static auto shLanguageRead = safetyhook::create_mid(read.get_first(5), [](SafetyHookContext& regs)
        {
            if (auto info = Find(FixSettings().String("LANGUAGE.Language")))
                *Current = info->id;
        });
    }
}

static std::filesystem::path CustomUserDir;
static bool bUseCustomUserDir = false;

static std::unique_ptr<FunctionHookMinHook> mhSHGetFolderPathA;
static HRESULT WINAPI SHGetFolderPathAHook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    if (bUseCustomUserDir)
    {
        int folder = csidl & ~CSIDL_FLAG_CREATE;
        if (folder == CSIDL_COMMON_APPDATA)
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

        // Here, when the plugin is loaded, so that the options menu, set up later, has the languages
        Language::Startup();
    }
} Registry;