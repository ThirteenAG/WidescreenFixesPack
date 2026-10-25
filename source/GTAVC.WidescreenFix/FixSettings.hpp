#pragma once
#include "../Shared/AsiSettings/asi_settings.hpp"
#include "../Shared/RWGTA/GameThread.hpp"

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

// Applies a changed setting on the game's main thread, whoever changed it (the menu, a preset, an editor)
inline void OnSettingChange(std::string_view id, std::function<void()> apply)
{
    FixSettings().OnChange(id, [apply]() { RWGTA::GameThread::Post(apply); });
}
