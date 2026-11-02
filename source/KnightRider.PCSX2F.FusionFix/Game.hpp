#pragma once
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

extern "C" int PCSX2Data[];

namespace kr
{
    template <class T> T Read(uintptr_t address)
    {
        return *reinterpret_cast<const T*>(address);
    }
    template <class T> void Write(uintptr_t address, T value)
    {
        *reinterpret_cast<T*>(address) = value;
    }
    inline uint32_t Bits(float value)
    {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    }

    inline float forcedAspect, hudAspect, fovFactor = 1.0f;
    inline int cutsceneBorders = 3;
    inline bool skipIntro = true;

    // Frames rendered (main scene draws), and the last frames that showed the
    // cockpit dashboard or a scripted camera.
    inline uint32_t frame, cockpitFrame = ~0u - 8, cinematicFrame = ~0u - 8;

    inline float Aspect()
    {
        return forcedAspect > 0 ? forcedAspect : console::ps2Aspect(PCSX2Data);
    }
    // Width of the 4:3 design frame relative to the screen.
    inline float Fit()
    {
        const float k = (4.0f / 3.0f) / Aspect();
        return k > 0 && k < 1 ? k : 1;
    }
    // Height of the 4:3 design frame on screens narrower than 4:3.
    inline float VerticalFit()
    {
        return std::min(Aspect() / (4.0f / 3.0f), 1.0f);
    }
    inline bool Recent(uint32_t when)
    {
        return frame - when <= 2;
    }
    inline bool Cockpit()
    {
        return Recent(cockpitFrame);
    }
    inline bool Cinematic()
    {
        return Recent(cinematicFrame);
    }
    // How far side-anchored HUD elements move from their 4:3 place, in screen
    // widths. The cockpit and pillarboxed scenes keep the whole HUD inside
    // their 4:3 frame.
    inline float HudShift()
    {
        if (Cockpit() || (Cinematic() && (cutsceneBorders & 2)))
        {
            return 0;
        }
        const float aspect = Aspect(), k = Fit();
        const float box = std::min(1.0f, (hudAspect > 0 ? hudAspect : aspect) / aspect);
        return box > k ? 0.5f * (box - k) : 0;
    }
} // namespace kr
