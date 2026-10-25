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
