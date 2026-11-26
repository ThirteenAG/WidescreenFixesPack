#pragma once
#include <charconv>
#include "Shared/AsiSettings/asi_settings.hpp"

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

template<typename Number>
inline Number ReadNumericSetting(std::string_view id, Number fallback)
{
    const auto text = FixSettings().Get(id);
    Number value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() ? value : fallback;
}
