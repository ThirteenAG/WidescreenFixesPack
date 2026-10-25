#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace NFSU2StaticText
{
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
        // Clone native animation states independently; the package update requires a current state.
        // Remove only row actions from the caption.
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

    inline void Destroy(uint8_t* object)
    {
        if (object)
            reinterpret_cast<void (__thiscall*)(uint8_t*, int)>((*reinterpret_cast<void***>(object))[0])(object, 1);
    }
}
