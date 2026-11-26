#pragma once
#include <atomic>
#include <mutex>
#include "Shared/AsiSettings/asi_settings.hpp"

// The embedded settings are loaded on first use. Changes through Set update the .cfg, menus and editors.
inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

template<class T, class Parse>
void ObserveFixSetting(std::string_view id, std::atomic<T>& target, Parse parse)
{
    auto publish = [id, &target, parse]()
    {
        // Menu changes and live editor updates can arrive on different threads.
        static std::mutex mutex;
        const std::lock_guard lock(mutex);
        target.store(parse(FixSettings().Get(id)), std::memory_order_relaxed);
    };
    FixSettings().OnChange(id, publish);
    publish();
}
