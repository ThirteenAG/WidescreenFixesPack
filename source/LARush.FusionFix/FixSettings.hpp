#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <atomic>
#include <mutex>
#include <type_traits>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

template<class T, class OnUpdate>
void BindLiveSetting(const char* id, std::atomic<T>& destination, OnUpdate onUpdate)
{
    const auto update = [id, &destination, onUpdate]()
    {
        static std::mutex mutex;
        std::lock_guard lock(mutex);
        const auto value = FixSettings().Get(id);
        T parsed{};
        if constexpr (std::is_same_v<T, bool>)
            parsed = value == "1";
        else if constexpr (std::is_same_v<T, int>)
        {
            if (!asi_settings::detail::ParseInt(value, parsed))
                return;
        }
        else
        {
            static_assert(std::is_same_v<T, float>);
            if (!asi_settings::detail::ParseFloat(value, parsed))
                return;
        }
        destination.store(parsed, std::memory_order_relaxed);
        onUpdate();
    };
    FixSettings().OnChange(id, update);
    update();
}

template<class T>
void BindLiveSetting(const char* id, std::atomic<T>& destination)
{
    BindLiveSetting(id, destination, []() {});
}
