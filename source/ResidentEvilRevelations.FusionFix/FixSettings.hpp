#pragma once
#include "../Shared/AsiSettings/asi_settings.hpp"

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

template<typename T, typename Address>
inline void WriteGameValue(Address address, T value)
{
    ProtectedGameRef<T>{reinterpret_cast<T*>(address)} = value;
}
