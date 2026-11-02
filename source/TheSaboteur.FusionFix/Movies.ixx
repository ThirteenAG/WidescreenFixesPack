module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Movies;

import Bindings;

export namespace SaboteurMovies
{
    inline std::atomic<int> Mode{ 0 };
    inline GameRef<uintptr_t>* Renderer = nullptr;
    inline SafetyHookMid DrawHook;

    inline void Install()
    {
        Mode.store(FixSettings().Int("MAIN.FMVWidescreenMode"));
        FixSettings().OnChange("MAIN.FMVWidescreenMode", []()
        {
            Mode.store(FixSettings().Int("MAIN.FMVWidescreenMode"));
        });
        Renderer = SaboteurBindings::Renderer();
        auto pattern = hook::pattern("D9 86 38 02 00 00 D9 5C 24 04 D9 05 ? ? ? ? D9 1C 24 51 52 E8 ? ? ? ? 83 C4 20");
        DrawHook = safetyhook::create_mid(pattern.get_first(21), [](SafetyHookContext& regs)
        {
            const auto renderer = Renderer->get();
            const auto bink = *reinterpret_cast<uintptr_t*>(regs.esi + 16);
            if (!renderer || !bink)
                return;
            const auto width = *reinterpret_cast<uint32_t*>(renderer + 1652);
            const auto height = *reinterpret_cast<uint32_t*>(renderer + 1656);
            const auto filmWidth = *reinterpret_cast<uint32_t*>(bink);
            const auto filmHeight = *reinterpret_cast<uint32_t*>(bink + 4);
            if (!width || !height || !filmWidth || !filmHeight)
                return;
            const auto mode = Mode.load();
            const double areaWidth = mode == 0 ? std::min(double(width), double(height) * 4.0 / 3.0) : double(width);
            const double areaHeight = mode == 0 ? std::min(double(height), double(width) * 3.0 / 4.0) : double(height);
            const double aspect = double(filmWidth) / filmHeight;
            const float scaleX = static_cast<float>(std::min(areaWidth, areaHeight * aspect) / width);
            const float scaleY = static_cast<float>(std::min(areaHeight, areaWidth / aspect) / height);
            *reinterpret_cast<float*>(regs.esp + 8) = -scaleX;
            *reinterpret_cast<float*>(regs.esp + 12) = -scaleY;
            *reinterpret_cast<float*>(regs.esp + 16) = scaleX * 2.0f;
            *reinterpret_cast<float*>(regs.esp + 20) = scaleY * 2.0f;
        });
    }
}
