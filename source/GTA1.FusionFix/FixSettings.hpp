#pragma once
#include <charconv>
#include "Shared/AsiSettings/asi_settings.hpp"

// The fix's settings: described in settings.json (embedded as a resource), stored in the plugin's .cfg. Loaded on first
// use, since modules read them while the plugin loads. Read the values through the references of Int, Float and
// String; change them with Set, so that the .cfg, menus and editors follow.
inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

// Typed access for the existing consumers. The legacy INI is never read or imported.
class SettingsReader
{
    static std::string Id(const char* section, const char* key) { return std::string(section) + "." + key; }
    template<class T>
    T Number(const char* section, const char* key, T fallback)
    {
        const auto text = FixSettings().Get(Id(section, key));
        T value{};
        return std::from_chars(text.data(), text.data() + text.size(), value).ec == std::errc{} ? value : fallback;
    }

public:
    explicit SettingsReader(const char* = "") {}
    int ReadInteger(const char* section, const char* key, int fallback) { return Number(section, key, fallback); }
    bool ReadBoolean(const char* section, const char* key, bool fallback) { return Number(section, key, int(fallback)) != 0; }
    float ReadFloat(const char* section, const char* key, float fallback) { return Number(section, key, fallback); }
    void WriteString(const char* section, const char* key, const std::string& value) { FixSettings().Set(Id(section, key), value); }
};
