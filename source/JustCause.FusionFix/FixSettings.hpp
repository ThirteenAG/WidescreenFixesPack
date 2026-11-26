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

template<class T>
inline T GetFixNumber(std::string_view id, T fallback)
{
    const auto text = FixSettings().Get(id);
    T value{};
    const auto end = text.data() + text.size();
    const auto [next, error] = std::from_chars(text.data(), end, value);
    return error == std::errc{} && next == end ? value : fallback;
}
