#pragma once
#include <string>
#include "../AsiSettings/asi_settings.hpp"

// Menu hosts choose the language of every plugin's texts; auto follows the game's language.
namespace WFPMenu
{
    inline std::string TextLanguage(asi_settings::Plugin& settings, const std::string& gameLanguage)
    {
        auto selected = settings.Get("MAIN.TextLanguage");
        static const std::pair<std::string_view, std::string_view> languages[] = {
            { "en", "english" },
            { "fr", "french" },
            { "de", "german" },
            { "it", "italian" },
            { "es", "spanish" },
            { "nl", "dutch" },
            { "sv", "swedish" },
            { "da", "danish" },
            { "fi", "finnish" },
            { "pl", "polish" },
            { "cs", "czech" },
            { "hu", "hungarian" },
            { "ru", "russian" },
            { "pt", "portuguese" },
            { "pt-BR", "brazilian" },
        };
        for (auto& [tag, name] : languages)
            if (selected == tag) return std::string(name);
        return gameLanguage;
    }
}
