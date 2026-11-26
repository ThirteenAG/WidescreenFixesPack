#pragma once
#include <charconv>
#include "Shared/AsiSettings/asi_settings.hpp"

// The fix's settings are described in the embedded schema and stored in the plugin's CFG.
inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

template<typename Number>
inline Number FixSettingNumber(std::string_view id, Number fallback)
{
    const auto value = FixSettings().Get(id);
    Number number{};
    const auto end = value.data() + value.size();
    const auto result = std::from_chars(value.data(), end, number);
    return result.ec == std::errc{} && result.ptr == end ? number : fallback;
}
