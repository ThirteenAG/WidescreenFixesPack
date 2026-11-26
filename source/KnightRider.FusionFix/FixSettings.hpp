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
inline T ReadFixNumber(const char* key, T fallback)
{
    const auto value = FixSettings().Get(key);
    T parsed{};
    const auto last = value.data() + value.size();
    const auto result = std::from_chars(value.data(), last, parsed);
    return result.ec == std::errc{} && result.ptr == last ? parsed : fallback;
}
