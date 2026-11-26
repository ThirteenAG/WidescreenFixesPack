#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <atomic>
#include <charconv>
#include <mutex>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

template <typename Publish>
inline void WatchMatrixSetting(std::string_view id, Publish publish)
{
    auto refresh = [id = std::string(id), publish]()
    {
        // Keep the snapshot and its publication ordered across concurrent callbacks.
        static std::mutex mutex;
        const std::lock_guard lock(mutex);
        publish(FixSettings().Get(id));
    };
    FixSettings().OnChange(id, refresh);
    refresh();
}

template <typename T>
inline void WatchMatrixNumber(std::string_view id, std::atomic<T>& destination)
{
    WatchMatrixSetting(id, [&destination](const std::string& text)
    {
        T value{};
        const auto end = text.data() + text.size();
        const auto result = std::from_chars(text.data(), end, value);
        if (result.ec == std::errc{} && result.ptr == end)
            destination = value;
    });
}
