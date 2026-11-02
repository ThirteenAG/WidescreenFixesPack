module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Borders;
import ComVars;


export namespace OnimushaBorders
{
    inline std::atomic<int> Mode{ 3 };
    inline std::atomic<bool> Animated{ true };
    inline std::function<float()> Aspect;
    inline uint32_t* (__thiscall* Allocate)(void*, int) = nullptr;
    inline uint32_t (__thiscall* NativeSubmit)(void*, uint32_t*, int) = nullptr;

    inline bool Active()
    {
        return Script < 0;
    }

    inline float CameraScale(float aspect, int mode)
    {
        return (mode & 2) && aspect < 4.0f / 3.0f ? (4.0f / 3.0f) / aspect : 1.0f;
    }

    inline uint32_t Position(float x, float y)
    {
        // Native GS coordinates use separate horizontal and interlaced vertical origins.
        return uint16_t(std::lround((1792.0f + x) * 16.0f)) |
            (uint32_t(uint16_t(std::lround((3872.0f + y) * 8.0f))) << 16);
    }

    inline void Rectangle(uint32_t* command, float left, float top, float right, float bottom)
    {
        command[4] = Position(left, top);
        command[6] = Position(right, bottom);
    }

    inline void Letterbox(uint32_t* command, bool bottom, float aspect, int mode)
    {
        const float scale = 1.0f / CameraScale(aspect, mode);
        const float edge = 224.0f * (1.0f - scale) + ((mode & 1) ? 48.0f * scale : 0.0f);
        Rectangle(command, 0.0f, bottom ? 448.0f - edge : 0.0f, 512.0f, bottom ? 448.0f : edge);
    }

    inline uint32_t __fastcall Submit(void* owner, void*, uint32_t* command, int list)
    {
        const bool bottom = command[4] == 0x85807000;
        const int mode = Mode.load(std::memory_order_relaxed);
        const float aspect = Aspect();
        const uint32_t alpha = Animated.load(std::memory_order_relaxed) ? command[2] : Active() ? 0x80000000 : 0;
        command[2] = alpha;
        Letterbox(command, bottom, aspect, mode);
        const auto result = NativeSubmit(owner, command, list);
        if (bottom && (mode & 2) && aspect > 4.0f / 3.0f)
        {
            const float edge = 256.0f * (1.0f - (4.0f / 3.0f) / aspect);
            for (int side = 0; side < 2; ++side)
            {
                auto pillar = Allocate(Allocator.get_ptr(), 32);
                std::memcpy(pillar, command, 32);
                pillar[0] = 0x80000000;
                Rectangle(pillar, side ? 512.0f - edge : 0.0f, 0.0f, side ? 512.0f : edge, 448.0f);
                NativeSubmit(owner, pillar, list);
            }
        }
        return result;
    }

    inline void Init(std::function<float()> aspect)
    {
        Aspect = std::move(aspect);
        const auto update = []()
        {
            Mode = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
            Animated = FixSettings().Bool("MAIN.AnimatedCutsceneBorders");
        };
        update();
        FixSettings().OnChange("MAIN.CutsceneBorders", update);
        FixSettings().OnChange("MAIN.AnimatedCutsceneBorders", update);
        Script.SetAddress(*hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8D 6E 20 8B CD E8").get_first<int32_t*>(1));
        // These are the two cinematic rectangles only. Their native depth and fade counter remain intact.
        auto bars = hook::pattern("6A 20 B9 ? ? ? ? E8 ? ? ? ? 0F B7 F6 6A 00 C1 E6 18 50 B9 ? ? ? ? C7 00 00 00 00 80 C7 40 04 06 00 18 14");
        Allocator.SetAddress(*bars.get_first<void*>(3));
        Allocate = reinterpret_cast<decltype(Allocate)>(injector::GetBranchDestination(bars.get_first(7)).as_int());
        NativeSubmit = reinterpret_cast<decltype(NativeSubmit)>(injector::GetBranchDestination(bars.get_first(62)).as_int());
        injector::MakeCALL(bars.get_first(62), Submit, true);
        injector::MakeCALL(bars.get_first(123), Submit, true);
    }
}
