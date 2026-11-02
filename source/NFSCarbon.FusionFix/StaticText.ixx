module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <array>
#include <cstdint>
#include <cstring>


export module StaticText;

import ComVars;

export namespace NFSCStaticText
{
    inline float RenderScale(uint32_t font, int language)
    {
        return (language == 11 || language == 12 || language == 13) && font == 0x9583AA1A ? 2.0f : 1.0f;
    }

    inline uint8_t* Clone(uint8_t* original)
    {
        if (!original || *reinterpret_cast<uint32_t*>(original + 0x18) != 2 ||
            !*reinterpret_cast<uintptr_t*>(original + 0x2C) ||
            *reinterpret_cast<uint32_t*>(original + 0x30) != 68)
            return nullptr;
        auto vtable = *reinterpret_cast<void***>(original);
        if (!vtable || !vtable[1])
            return nullptr;
        std::array<uint8_t, 0x78> source;
        std::memcpy(source.data(), original, source.size());
        // A static caption inherits the native string/font/state/scripts, without row actions. The game updates the
        // current script of every object of a package, so the copy keeps its own copies of the scripts.
        for (size_t offset : { 0x38u, 0x3Cu, 0x40u })
            *reinterpret_cast<uint32_t*>(source.data() + offset) = 0;
        auto copy = reinterpret_cast<uint8_t* (__thiscall*)(uint8_t*, int)>(vtable[1])(source.data(), 0);
        if (copy)
        {
            auto state = *reinterpret_cast<void**>(copy + 0x2C);
            if (state)
                std::memcpy(state, *reinterpret_cast<const void**>(original + 0x2C), 68);
        }
        if (copy)
            *reinterpret_cast<uintptr_t*>(copy + 0x58) = 0;
        return copy;
    }

    // The position keys of an object's scripts (track type 4 at offset 7): a base key the animation is relative to
    template<typename Function>
    void PositionTracks(uint8_t* object, Function&& function)
    {
        auto script = *reinterpret_cast<uint8_t**>(object + 0x4C);
        for (size_t depth = 0; script && depth < 64; ++depth, script = *reinterpret_cast<uint8_t**>(script + 4))
        {
            auto count = *reinterpret_cast<uint32_t*>(script + 0x1C);
            auto tracks = *reinterpret_cast<uint8_t**>(script + 0x20);
            if (!tracks || count > 512)
                continue;
            for (size_t i = 0; i < count; ++i)
            {
                auto track = tracks + i * 0x38;
                if (track[0] == 4 && track[7] == 7)
                    function(*reinterpret_cast<float*>(track + 0x18));
            }
        }
    }

    // Moves an object and its scripts' position keys, which the animation is relative to
    inline void MoveTo(uint8_t* object, float* data, float x, float y)
    {
        float baseX = data[7];
        float baseY = data[8];
        bool found{};
        PositionTracks(object, [&](float& base)
        {
            if (!found)
            {
                baseX = base;
                baseY = (&base)[1];
                found = true;
            }
        });
        auto dx = x - baseX;
        auto dy = y - baseY;
        if (dx != 0.0f || dy != 0.0f)
        {
            PositionTracks(object, [dx, dy](float& base)
            {
                base += dx;
                (&base)[1] += dy;
            });
        }
    }

    inline void Destroy(uint8_t* object)
    {
        if (object)
            reinterpret_cast<void (__thiscall*)(uint8_t*, int)>((*reinterpret_cast<void***>(object))[0])(object, 1);
    }
}
