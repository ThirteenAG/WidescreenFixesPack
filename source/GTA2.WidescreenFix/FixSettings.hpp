#pragma once
#include "../Shared/AsiSettings/asi_settings.hpp"

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
public:
    explicit SettingsReader(const char* = "") {}
    int ReadInteger(const char* section, const char* key, int) { return FixSettings().Int(Id(section, key)); }
    bool ReadBoolean(const char* section, const char* key, bool) { return FixSettings().Bool(Id(section, key)); }
    float ReadFloat(const char* section, const char* key, float) { return FixSettings().Float(Id(section, key)); }
    void WriteString(const char* section, const char* key, const std::string& value) { FixSettings().Set(Id(section, key), value); }
};
