#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <cstdlib>
#include <functional>
#include <mutex>
#include <utility>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

inline void BindLiveSetting(std::string_view id, std::function<void(const std::string&)> apply)
{
    const auto update = [key = std::string(id), apply = std::move(apply)]()
    {
        static std::mutex writers;
        const std::lock_guard lock(writers);
        apply(FixSettings().Get(key));
    };
    FixSettings().OnChange(id, update);
    update();
}

inline void BindLiveInt(std::string_view id, std::function<void(int)> apply)
{
    BindLiveSetting(id, [apply = std::move(apply)](const std::string& value)
    {
        apply(std::atoi(value.c_str()));
    });
}
