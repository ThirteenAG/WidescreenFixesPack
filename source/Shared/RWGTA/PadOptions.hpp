#pragma once

#include "Shared/AsiSettings/asi_settings.hpp"
#include "Shared/RWGTA/GameThread.hpp"
#include <atomic>
#include <charconv>

namespace RWGTA::PadOptions
{
    inline std::atomic<int> ModernControls{}, GamepadIcons{}, InvertGamepadY{}, Vibration{}, FreeCamera{}, SpeedFOV{};
    inline std::atomic<float> CameraSpeed{ 1.0f };

    // The right stick's look up and down, up positive: pushing up looks up unless the look is inverted
    inline float LookUp(float y)
    {
        return InvertGamepadY.load(std::memory_order_relaxed) ? -y : y;
    }

    template<class T>
    void Bind(asi_settings::Plugin& plugin, const char* id, std::atomic<T>& destination)
    {
        const auto read = [&plugin, id, &destination]()
        {
            const auto value = plugin.Get(id);
            T number{};
            if (std::from_chars(value.data(), value.data() + value.size(), number).ec == std::errc{})
                destination.store(number, std::memory_order_relaxed);
        };
        plugin.OnChange(id, [read] { GameThread::Post(read); });
        read();
    }

    inline void Install(asi_settings::Plugin& plugin, bool hasFreeCamera)
    {
        static std::once_flag once;
        std::call_once(once, [&]
        {
            Bind(plugin, "MAIN.ModernControls", ModernControls);
            Bind(plugin, "MAIN.GamepadIcons", GamepadIcons);
            Bind(plugin, "MAIN.InvertGamepadY", InvertGamepadY);
            Bind(plugin, "MAIN.Vibration", Vibration);
            Bind(plugin, "MAIN.GamepadCameraSpeed", CameraSpeed);
            if (hasFreeCamera)
            {
                Bind(plugin, "MAIN.FreeCamera", FreeCamera);
                Bind(plugin, "MISC.CarSpeedDependantFOV", SpeedFOV);
            }
        });
    }
}
