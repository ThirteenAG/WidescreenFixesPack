#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <cstdlib>
#include <functional>
#include <map>
#include <mutex>
#include <utility>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

inline const auto& AppliedRestartSettings()
{
    static const auto values = []()
    {
        auto& settings = FixSettings();
        std::map<std::string, std::string, std::less<>> startup;
        for (const auto& setting : settings.GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                startup.emplace(setting.id, settings.Get(setting.id));
        return startup;
    }();
    return values;
}

inline bool StartupOption(std::string_view id)
{
    const auto& values = AppliedRestartSettings();
    const auto value = values.find(id);
    return value != values.end() && value->second == "1";
}

inline void BindLiveInt(std::string_view id, std::function<void(int)> apply)
{
    const auto update = [key = std::string(id), apply = std::move(apply)]()
    {
        static std::mutex writers;
        const std::lock_guard lock(writers);
        apply(std::atoi(FixSettings().Get(key).c_str()));
    };
    FixSettings().OnChange(id, update);
    update();
}
