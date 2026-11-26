#pragma once
#include <atomic>
#include <charconv>
#include <mutex>
#include <type_traits>
#include "Shared/AsiSettings/asi_settings.hpp"

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

namespace Driv3rSettings
{
    template<class T>
    inline T Read(std::string_view id, T fallback)
    {
        const auto text = FixSettings().Get(id);
        std::conditional_t<std::is_same_v<T, bool>, int, T> value{};
        const auto end = text.data() + text.size();
        const auto [next, error] = std::from_chars(text.data(), end, value);
        return error == std::errc{} && next == end ? static_cast<T>(value) : fallback;
    }

    template<class T, class Apply>
    inline void Bind(std::string_view id, T fallback, Apply apply)
    {
        const auto update = [=]()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            apply(Read(id, fallback));
        };
        FixSettings().OnChange(id, update);
        update();
    }

    template<class T>
    inline void Publish(std::string_view id, std::atomic<T>& value)
    {
        Bind(id, value.load(), [&value](T current) { value.store(current); });
    }

    inline std::atomic<float> MouseSensitivity{ 1.0f }, GamepadCameraSpeed{ 1.0f };
    inline std::atomic<float> CameraReturnTimeout{ 3.0f }, CameraReturnSpeed{ 2.0f };
    inline std::atomic<float> GamepadDeadzone{ 0.15f }, CursorSensitivity{ 1.0f };
    inline std::atomic<bool> InvertLook{ false }, Vibration{ true }, PauseOnFocusLoss{ true };
    inline std::atomic<int> FPSLimit{ 60 }, GamepadIcons{ 0 }, MovieMode{ 1 };

    inline void Install()
    {
        Publish("MAIN.MouseSensitivity", MouseSensitivity);
        Publish("MAIN.GamepadCameraSpeed", GamepadCameraSpeed);
        Publish("CAMERA.CameraReturnTimeout", CameraReturnTimeout);
        Publish("CAMERA.CameraReturnSpeed", CameraReturnSpeed);
        Publish("CAMERA.InvertLook", InvertLook);
        Publish("MAIN.GamepadDeadzone", GamepadDeadzone);
        Publish("MAIN.Vibration", Vibration);
        Publish("MAIN.PauseOnFocusLoss", PauseOnFocusLoss);
        Publish("MAIN.FPSLimit", FPSLimit);
        Publish("MAIN.CursorSensitivity", CursorSensitivity);
        Publish("MAIN.GamepadIcons", GamepadIcons);
        Publish("MAIN.FMVWidescreenMode", MovieMode);
    }
}
