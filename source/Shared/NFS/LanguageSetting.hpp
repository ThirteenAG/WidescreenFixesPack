#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "../AsiSettings/asi_settings.hpp"

// The game's language as a setting of the fix: a choice of the names the game reads from the "Language" value of its
// registry key, which the installer writes. The registry isn't changed: the game's own read of the value gets the
// setting (AnswerQuery around the RegQueryValueExA the game calls, or the game's language id swapped in memory).
//
// Installs only have the files of their own languages, and a language whose files are missing crashes the game or
// leaves it stuck (#2066, #665, #2266), so only the languages whose files are all there can be used (Availability):
// menus offer only those, and a stored language that isn't installed stays in the .cfg while the default, or the first
// installed language, is in effect. On the first start the installer's language is stored.
namespace WFPLanguageSetting
{
    struct Entry
    {
        std::string name;                   // the registry value, as the game compares it ("English US", "German", ...)
        std::vector<std::string> files;     // files the language needs, relative to the game's folder
    };

    // Whether all of the language's files are there. A file whose folder isn't there at all doesn't count: the
    // install keeps those files elsewhere (speech streamed from the disc, for example).
    inline bool HasFiles(const Entry& entry)
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        auto folder = std::filesystem::path(path).parent_path();
        std::error_code ec;
        for (auto& file : entry.files)
        {
            auto full = folder / file;
            if (!std::filesystem::exists(full, ec) && std::filesystem::exists(full.parent_path(), ec))
                return false;
        }
        return true;
    }

    // At startup, before the game reads its language. entries: the languages the game has and their files;
    // installer: the installer's language (the registry's value), stored when the .cfg has no language yet.
    inline void Startup(asi_settings::Plugin& settings, std::string_view id, std::vector<Entry> entries, const std::string& installer)
    {
        // When the file lists match nothing (an install laid out differently), every language counts as installed
        // rather than none
        static std::vector<std::string> installed;
        installed.clear();
        for (auto& entry : entries)
        {
            if (HasFiles(entry))
                installed.push_back(entry.name);
        }
        if (installed.empty())
        {
            for (auto& entry : entries)
                installed.push_back(entry.name);
        }

        if (!settings.Stored(id) && !installer.empty())
            settings.Set(id, installer, true);
        settings.Availability(id, [](std::string_view value)
        {
            return std::any_of(installed.begin(), installed.end(), [&](const std::string& name) { return asi_settings::text::IEquals(name, value); });
        });
    }

    // For the RegQueryValueExA the game calls: its read of the "Language" value gets language; true when it was the
    // language and result is the answer
    inline bool AnswerQuery(const std::string& language, LPCSTR valueName, LPDWORD type, LPBYTE data, LPDWORD size, LSTATUS& result)
    {
        if (language.empty() || !valueName || _stricmp(valueName, "Language") != 0)
            return false;
        DWORD needed = DWORD(language.size() + 1);
        if (type)
            *type = REG_SZ;
        if (data && size && *size < needed)
            result = ERROR_MORE_DATA;
        else
        {
            if (data)
                memcpy(data, language.c_str(), needed);
            result = ERROR_SUCCESS;
        }
        if (size)
            *size = needed;
        return true;
    }
}
