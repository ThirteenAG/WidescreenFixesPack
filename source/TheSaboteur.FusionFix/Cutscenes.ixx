module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Cutscenes;

import Bindings;
import ComVars;

export namespace SaboteurCutscenes
{
    inline std::atomic<int> Borders{ 3 };
    inline std::atomic<bool> Animated{ true };
    inline GameRef<uintptr_t>* Camera = nullptr;
    inline GameRef<int>* CameraCount = nullptr;
    inline GameRef<uintptr_t>* HUD = nullptr;
    inline GameRef<uintptr_t>* Movies = nullptr;
    inline std::atomic<uintptr_t> ReadyMovie{ 0 };

    inline bool Ready()
    {
        const auto movie = ReadyMovie.load();
        return movie && HUD && HUD->get() && *reinterpret_cast<uintptr_t*>(HUD->get() + 2688) == movie;
    }

    inline bool Active()
    {
        if (!Camera || !CameraCount || CameraCount->get() <= 0 || !Camera->get() || !HUD || !HUD->get() || !Movies || !Movies->get())
            return false;
        if (!*reinterpret_cast<uintptr_t*>(HUD->get() + 2688) || *reinterpret_cast<uintptr_t*>(Movies->get() + 16))
            return false;
        const auto mode = *reinterpret_cast<int*>(Camera->get() + 44);
        return mode == 1 || mode == 2 || (mode == 0 && (*reinterpret_cast<uint8_t*>(HUD->get() + 3212) & 1));
    }

    inline void Install()
    {
        HUD = SaboteurBindings::HUD();
        Movies = SaboteurBindings::Movies();
        auto pattern = hook::pattern("83 EC 50 56 57 8B 7C 24 5C 3B 3D ? ? ? ? 8B F1 7D 10 8B C7 69 C0 30 19 00 00 03 05 ? ? ? ? EB 02 33 C0 D9 40 50");
        CameraCount = BindGameRef(cutsceneCameraCount, *pattern.get_first<int*>(11));
        Camera = BindGameRef(cutsceneGameCamera, *pattern.get_first<uintptr_t*>(29));
        const auto update = []()
        {
            Borders.store(FixSettings().Int("MAIN.CutsceneBorders"));
            Animated.store(FixSettings().Bool("MAIN.AnimatedCutsceneBorders"));
        };
        update();
        FixSettings().OnChange("MAIN.CutsceneBorders", update);
        FixSettings().OnChange("MAIN.AnimatedCutsceneBorders", update);
    }
}
