#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

extern "C"
{
    extern int PCSX2Data[PCSX2Data_Size];
    extern char FrameLimitUnthrottle;
}

namespace tcny
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

    // Settings (read again while the game runs).
    inline float forcedAspect = 0, hudAspect = 0, fovFactor = 1.0f;
    inline int cutsceneBorders = 3;
    inline bool animatedBorders = true;

    // Screen layout. The game draws its 2D in a 640-wide frame; `scale` is the
    // width of the 4:3 picture relative to the screen.
    struct Display
    {
        float aspect = 4.0f / 3.0f, width = 640.0f;
        float scale = 1.0f, hudScale = 1.0f, centered = 0, normalized = 0;
        float right = 0, left = 0, radar = -5.0f;
        void refresh()
        {
            aspect = forcedAspect > 0 ? forcedAspect : console::ps2Aspect(PCSX2Data);
            scale = (4.0f / 3.0f) / aspect;
            // Screens narrower than 4:3 keep the 2D at the screen width.
            hudScale = std::min(scale, 1.0f);
            centered = width * (1 - hudScale) * 0.5f;
            normalized = (1 / hudScale - 1) * 0.5f;
            // Side-anchored HUD: how far it moves from its 4:3 place, in 4:3
            // frame widths (the HUD box, or the whole screen in Auto).
            const float box = hudAspect > 0 ? std::clamp(hudAspect, 4.0f / 3.0f, std::max(aspect, 4.0f / 3.0f)) : aspect;
            right = std::max((box / (4.0f / 3.0f) - 1) * 0.5f, 0.0f);
            left = right;
            // The radar is placed in screen pixels of the 640 frame.
            radar = -5.0f + centered - width * (1 - (4.0f / 3.0f) / std::max(box, 4.0f / 3.0f)) * 0.5f;
        }
    };
    inline Display display;
    inline bool enable60 = false, unthrottle = true;
    inline int* loading = nullptr;
    inline uintptr_t mapCallers[2];
    inline uint32_t frame;

    // Cutscenes: the world (pointer), its cinematic flag, the game camera.
    inline uintptr_t worldPointer, cinemaOffset, gameCamera;
    // The 2D manager that lays out and draws the HUD and menus.
    inline uintptr_t hudManager;
    inline bool Cinema()
    {
        const auto world = Read<uintptr_t>(worldPointer);
        return world && Read<int32_t>(world + cinemaOffset) != 0;
    }

    void InstallHud(uintptr_t width, uintptr_t layout, uintptr_t subtitles, uintptr_t models, uintptr_t map,
                    uintptr_t blips, uintptr_t radar, uintptr_t radarDisc, uintptr_t scale3D);
    void InstallTiming(uintptr_t process, uintptr_t frameSite, uintptr_t vsyncCall, uintptr_t wide);
    void InstallCutscene(uintptr_t barsBegin, uintptr_t barsHeight, uintptr_t barsEnd, uintptr_t barsReset);
    void InstallCamera(uintptr_t projectionCall);
    void InstallLauncher(uintptr_t sprite, uintptr_t picture, uintptr_t splash);
    void PollSettings();
} // namespace tcny
