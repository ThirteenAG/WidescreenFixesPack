#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <algorithm>
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
inline void WatchNeoSetting(std::string_view id, Publish publish)
{
    auto refresh = [id = std::string(id), publish]()
    {
        // A newer callback cannot publish ahead of an older snapshot of the same setting.
        static std::mutex mutex;
        const std::lock_guard lock(mutex);
        publish(FixSettings().Get(id));
    };
    FixSettings().OnChange(id, refresh);
    refresh();
}

template <typename T>
inline void WatchNeoNumber(std::string_view id, std::atomic<T>& destination, T minimum, T maximum)
{
    WatchNeoSetting(id, [&destination, minimum, maximum](const std::string& text)
    {
        T value{};
        const auto end = text.data() + text.size();
        const auto result = std::from_chars(text.data(), end, value);
        if (result.ec == std::errc{} && result.ptr == end)
            destination = std::clamp(value, minimum, maximum);
    });
}
