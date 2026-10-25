#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <new>

namespace NFSPSStaticText
{
    inline void* StatePool = nullptr;
    inline uint8_t* (__thiscall* Construct)(uint8_t*) = nullptr;
    inline void* (__thiscall* AllocateState)(void*, size_t) = nullptr;
    inline void* (__cdecl* Allocate)(size_t) = nullptr;
    inline void (__cdecl* Free)(void*) = nullptr;

    struct StaticAnimation;
    inline StaticAnimation* __fastcall DeleteAnimation(StaticAnimation* animation, void*, int flags)
    {
        if (flags & 1)
            Free(animation);
        return animation;
    }

    inline void* AnimationTable[]{ reinterpret_cast<void*>(&DeleteAnimation) };

    struct StaticAnimation
    {
        void** table = AnimationTable;
        StaticAnimation* next{};
        StaticAnimation* previous{};
        int duration{};
        int position{};
        uint32_t flags = 1;
        StaticAnimation* following{};
        uint32_t tracks{};
        void* trackData{};
        uint32_t events{};
        void* eventData{};
        const char* name{};
        uint32_t hash = 0x001744B3;
    };
    static_assert(sizeof(StaticAnimation) == 52);

    inline void Destroy(uint8_t* object)
    {
        if (object)
            reinterpret_cast<void (__thiscall*)(uint8_t*, int)>((*reinterpret_cast<void***>(object))[0])(object, 1);
    }

    inline uint8_t* Clone(uint8_t* original)
    {
        if (!Construct || !AllocateState || !Allocate || !Free || !StatePool || !original ||
            *reinterpret_cast<uint32_t*>(original + 0x18) != 2 ||
            *reinterpret_cast<uint32_t*>(original + 0x30) != 68 ||
            !*reinterpret_cast<void**>(original + 0x2C))
            return nullptr;
        // Capture the style before native allocations can retire the original screen.
        std::array<uint8_t, 68> style;
        std::memcpy(style.data(), *reinterpret_cast<void**>(original + 0x2C), style.size());
        auto font = *reinterpret_cast<uint32_t*>(original + 0x24);
        auto object = static_cast<uint8_t*>(Allocate(120));
        if (!object)
            return nullptr;
        std::memset(object, 0, 120);
        Construct(object);
        auto state = AllocateState(StatePool, style.size());
        if (!state)
        {
            Destroy(object);
            return nullptr;
        }
        std::memcpy(state, style.data(), style.size());
        *reinterpret_cast<void**>(object + 0x2C) = state;
        *reinterpret_cast<uint32_t*>(object + 0x30) = uint32_t(style.size());
        *reinterpret_cast<uint32_t*>(object + 0x24) = font;
        // The native updater visits every caption, including static ones.
        auto animation = static_cast<StaticAnimation*>(Allocate(sizeof(StaticAnimation)));
        if (!animation)
        {
            Destroy(object);
            return nullptr;
        }
        new (animation) StaticAnimation;
        *reinterpret_cast<uint32_t*>(object + 0x48) = 1;
        *reinterpret_cast<StaticAnimation**>(object + 0x4C) = animation;
        *reinterpret_cast<StaticAnimation**>(object + 0x50) = animation;
        *reinterpret_cast<StaticAnimation**>(object + 0x54) = animation;
        return object;
    }
}
