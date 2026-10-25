#pragma once
#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include "../AsiSettings/asi_settings.h"
#include "../AsiSettings/asi_settings_json.hpp"
#pragma comment(lib, "Normaliz.lib")

// Text of the fixes' menu entries: text/<family>/<language tag>.json holds the entries of one language as
// { "Key": "Text" }, in the format of the ASI Settings translations (source/Shared/AsiSettings). The files are embedded
// into the plugin as ASISETTINGS_TEXT resources named by language tag (en, fr, pt-BR, ...). Text is UTF-8; Convert()
// turns it into what the game's fonts expect. Missing entries fall back to English.
namespace WFPText
{
    namespace Detail
    {
        using Table = std::unordered_map<std::string, std::string>;

        inline Table Parse(std::string_view text)
        {
            Table table;
            asi_settings::json::Value root;
            if (!asi_settings::json::Parse(text, root) || !root.IsObject())
                return table;
            for (auto& [key, value] : root.members)
            {
                if (value.IsString())
                    table[key] = value.string;
            }
            return table;
        }

        // The language tag of a game language's name, as the games call them
        inline const wchar_t* Tag(std::string_view language)
        {
            static const std::pair<std::string_view, const wchar_t*> tags[] = {
                { "english", L"en" }, { "french", L"fr" }, { "german", L"de" }, { "italian", L"it" }, { "spanish", L"es" },
                { "dutch", L"nl" }, { "swedish", L"sv" }, { "danish", L"da" }, { "finnish", L"fi" }, { "polish", L"pl" },
                { "czech", L"cs" }, { "hungarian", L"hu" }, { "russian", L"ru" }, { "portuguese", L"pt" }, { "brazilian", L"pt-BR" },
            };
            for (auto& [name, tag] : tags)
            {
                if (name == language)
                    return tag;
            }
            return nullptr;
        }

        inline Table Load(const std::string& language)
        {
            auto tag = Tag(language);
            if (!tag)
                return {};
            HMODULE module = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Parse), &module);
            HRSRC resource = FindResourceW(module, tag, ASI_SETTINGS_TEXT_RESOURCE_TYPE);
            HGLOBAL handle = resource ? LoadResource(module, resource) : nullptr;
            auto data = handle ? static_cast<const char*>(LockResource(handle)) : nullptr;
            if (!data)
                return {};
            return Parse(std::string_view(data, SizeofResource(module, resource)));
        }

        inline std::string& Language()
        {
            static std::string language = "english";
            return language;
        }

        inline Table& Current()
        {
            static Table table;
            return table;
        }

        inline Table& English()
        {
            static Table table = Load("english");
            return table;
        }
    }

    // The game's language, by its English name (english, french, german, brazilian, ...)
    inline void SetLanguage(std::string language)
    {
        for (auto& c : language)
            c = char(tolower(uint8_t(c)));
        if (language == Detail::Language() && !Detail::Current().empty())
            return;
        Detail::Language() = language;
        Detail::Current() = Detail::Load(language);
    }

    inline const std::string& Language()
    {
        return Detail::Language();
    }

    // UTF-8 text of an entry
    inline const std::string& Get(const std::string& key)
    {
        if (auto it = Detail::Current().find(key); it != Detail::Current().end())
            return it->second;
        if (auto it = Detail::English().find(key); it != Detail::English().end())
            return it->second;
        return key;
    }

    // UTF-8 to a Windows code page, for games with 8-bit fonts
    inline std::string ToCodePage(const std::string& text, UINT codePage)
    {
        int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), wide.data(), length);
        int size = WideCharToMultiByte(codePage, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
        std::string result(size, '\0');
        WideCharToMultiByte(codePage, 0, wide.data(), length, result.data(), size, nullptr, nullptr);
        return result;
    }

    inline std::wstring ToWide(const std::string& text)
    {
        int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), wide.data(), length);
        return wide;
    }

    // UTF-8 text in capitals by the casing rules of the game's language, for games whose rows are upper case
    inline std::string Upper(const std::string& text)
    {
        auto wide = ToWide(text);
        auto tag = Detail::Tag(Detail::Language());
        int length = LCMapStringEx(tag ? tag : L"en", LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING, wide.data(), int(wide.size()), nullptr, 0, nullptr, nullptr, 0);
        if (length <= 0)
            return text;
        std::wstring upper(length, L'\0');
        LCMapStringEx(tag ? tag : L"en", LCMAP_UPPERCASE | LCMAP_LINGUISTIC_CASING, wide.data(), int(wide.size()), upper.data(), length, nullptr, nullptr, 0);
        int size = WideCharToMultiByte(CP_UTF8, 0, upper.data(), length, nullptr, 0, nullptr, nullptr);
        std::string result(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, upper.data(), length, result.data(), size, nullptr, nullptr);
        return result;
    }

    // UTF-8 to the 8-bit text of EA's fonts (Most Wanted to Undercover): Latin-1, except that the Polish, Czech and
    // Hungarian fonts put their own letters at 0x81 and up. Letters a font doesn't have lose their accent.
    inline std::string ToEAFont(const std::string& text, std::string_view language)
    {
        static const std::pair<wchar_t, char> polish[] = {
            { L'ę', '\x81' }, { L'ł', '\x82' }, { L'ś', '\x83' }, { L'ć', '\x84' }, { L'ż', '\x85' }, { L'ą', '\x86' },
            { L'Ł', '\x87' }, { L'Ś', '\x88' }, { L'ń', '\x89' }, { L'Ę', '\x8A' }, { L'Ń', '\x8B' }, { L'ź', '\x8C' },
            { L'Ż', '\x8D' }, { L'Ą', '\x8E' }, { L'Ć', '\x90' }, { L'Ź', '\x91' },
        };
        static const std::pair<wchar_t, char> czech[] = {
            { L'ě', '\x81' }, { L'ž', '\x82' }, { L'š', '\x83' }, { L'ř', '\x84' }, { L'č', '\x85' }, { L'ů', '\x86' },
            { L'Č', '\x87' }, { L'Ě', '\x88' }, { L'ň', '\x89' }, { L'Ř', '\x8A' }, { L'Š', '\x8B' }, { L'Ž', '\x8C' },
            { L'ď', '\x8D' }, { L'ť', '\x8E' }, { L'Ů', '\x90' }, { L'Ť', '\x91' }, { L'Ň', '\x92' },
        };
        static const std::pair<wchar_t, char> hungarian[] = {
            { L'ő', '\x81' }, { L'Ő', '\x82' }, { L'ű', '\x83' }, { L'Ű', '\x84' },
        };

        std::span<const std::pair<wchar_t, char>> table;
        if (language == "polish")
            table = polish;
        else if (language == "czech")
            table = czech;
        else if (language == "hungarian")
            table = hungarian;

        std::string result;
        for (wchar_t c : ToWide(text))
        {
            if (c < 0x80)
            {
                result += char(c);
                continue;
            }
            auto it = std::find_if(table.begin(), table.end(), [c](const auto& entry) { return entry.first == c; });
            if (it != table.end())
                result += it->second;
            else if (c < 0x100 && c >= 0xA0)
                result += char(c);
            else
            {
                // The letter without its accent
                wchar_t decomposed[8] = {};
                if (NormalizeString(NormalizationD, &c, 1, decomposed, 8) > 0 && decomposed[0] < 0x80)
                    result += char(decomposed[0]);
                else
                    result += '?';
            }
        }
        return result;
    }
}
