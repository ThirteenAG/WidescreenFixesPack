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
