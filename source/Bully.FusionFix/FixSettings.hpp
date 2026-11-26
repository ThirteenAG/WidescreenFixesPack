#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <atomic>
#include <charconv>

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

template<typename T>
inline void BindLiveValue(const char* id, std::atomic<T>& target)
{
    auto read = [id, &target]()
    {
        const auto text = FixSettings().Get(id);
        T value{};
        if (std::from_chars(text.data(), text.data() + text.size(), value).ec == std::errc{})
            target.store(value, std::memory_order_relaxed);
    };
    FixSettings().OnChange(id, read);
    read();
}
