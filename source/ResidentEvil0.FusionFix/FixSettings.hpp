#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <atomic>
#include <cstdlib>
#include <mutex>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

inline void BindLiveInt(std::string_view id, std::atomic<int>& value)
{
    const auto update = [key = std::string(id), &value]()
    {
        // Settings edits can invoke callbacks on different threads.
        static std::mutex writers;
        const std::lock_guard lock(writers);
        value.store(std::atoi(FixSettings().Get(key).c_str()), std::memory_order_relaxed);
    };
    FixSettings().OnChange(id, update);
    update();
}
