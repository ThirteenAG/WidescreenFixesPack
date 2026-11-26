#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <charconv>
#include <mutex>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

template <class Apply>
void ObservePunisherSetting(const char* id, Apply apply)
{
    const auto refresh = [apply]()
    {
        // Serialize the initial read with updates from the menu and LiveLink.
        static std::mutex mutex;
        const std::lock_guard lock(mutex);
        apply();
    };
    FixSettings().OnChange(id, refresh);
    refresh();
}

template <class T>
T PunisherNumericSetting(const char* id, T fallback)
{
    const auto value = FixSettings().Get(id);
    T number{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
    return result.ec == std::errc{} && result.ptr == value.data() + value.size() ? number : fallback;
}
