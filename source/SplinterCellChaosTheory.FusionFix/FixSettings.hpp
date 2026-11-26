#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <mutex>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

inline int ReadFixInt(std::string_view id)
{
    int value = 0;
    asi_settings::detail::ParseInt(FixSettings().Get(id), value);
    return value;
}

// Publish each live value from a locked SDK snapshot. Concurrent notifications must leave the latest value in effect.
template<class T, class Apply>
inline void ObserveFixSetting(const char* id, bool (*parse)(std::string_view, T&), Apply apply)
{
    auto refresh = [id, parse, apply]()
    {
        static std::mutex mutex;
        std::lock_guard lock(mutex);
        T value{};
        if (parse(FixSettings().Get(id), value))
            apply(value);
    };
    FixSettings().OnChange(id, refresh);
    refresh();
}
