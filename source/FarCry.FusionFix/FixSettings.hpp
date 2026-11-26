#pragma once
#include "Shared/AsiSettings/asi_settings.hpp"
#include <mutex>
#include <optional>

inline asi_settings::Plugin& FixSettings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

namespace FarCryLive
{
    struct Values
    {
        std::optional<float> hudAspect;
        float fov = 1.0f, padSpeed = 1.0f;
        int movieMode = 1, fps = 60, borders = 3, padIcons = 0;
        bool confine = true, pauseOnFocusLoss = true, animatedBorders = true;
        bool invertPad = false, vibration = true;
    };

    inline std::mutex SettingsMutex;
    inline Values Settings;

    inline int Integer(std::string_view id, int fallback)
    {
        asi_settings::detail::ParseInt(FixSettings().Get(id), fallback);
        return fallback;
    }

    inline bool Boolean(std::string_view id)
    {
        return FixSettings().Get(id) == "1";
    }

    inline void Refresh()
    {
        const std::lock_guard lock(SettingsMutex);
        auto& plugin = FixSettings();
        Settings.hudAspect = ParseWidescreenHudOffset(plugin.Get("MAIN.HudAspectRatioConstraint"));
        asi_settings::detail::ParseFloat(plugin.Get("MAIN.FOVFactor"), Settings.fov);
        asi_settings::detail::ParseFloat(plugin.Get("MAIN.GamepadCameraSpeed"), Settings.padSpeed);
        Settings.fov = std::clamp(Settings.fov, 0.5f, 2.0f);
        Settings.padSpeed = std::clamp(Settings.padSpeed, 0.25f, 3.0f);
        Settings.movieMode = std::clamp(Integer("MAIN.FMVWidescreenMode", 1), 0, 1);
        Settings.fps = Integer("MAIN.FPSLimit", 60);
        Settings.borders = Integer("MAIN.CutsceneBorders", 3);
        Settings.padIcons = Integer("MAIN.GamepadIcons", 0);
        Settings.confine = Boolean("MAIN.ConfineCursor");
        Settings.pauseOnFocusLoss = Boolean("MAIN.PauseOnFocusLoss");
        Settings.animatedBorders = Boolean("MAIN.AnimatedCutsceneBorders");
        Settings.invertPad = Boolean("MAIN.InvertGamepadY");
        Settings.vibration = Boolean("MAIN.Vibration");
    }

    inline Values Read()
    {
        const std::lock_guard lock(SettingsMutex);
        return Settings;
    }

    inline void Init()
    {
        for (const auto id : { "MAIN.HudAspectRatioConstraint", "MAIN.FOVFactor", "MAIN.GamepadCameraSpeed",
            "MAIN.FMVWidescreenMode", "MAIN.FPSLimit", "MAIN.CutsceneBorders", "MAIN.GamepadIcons",
            "MAIN.ConfineCursor", "MAIN.PauseOnFocusLoss", "MAIN.AnimatedCutsceneBorders", "MAIN.InvertGamepadY", "MAIN.Vibration" })
            FixSettings().OnChange(id, Refresh);
        Refresh();
    }
}
