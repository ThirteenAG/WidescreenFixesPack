module;

#include <stdafx.h>

export module MenuBindings;

import EngineText;

export namespace RE0Menu
{
    using RE0Text::Read;
    using RE0Text::Write;
    inline uintptr_t* menuVtable = nullptr;
    inline uint8_t* initialize = nullptr;
    inline uint8_t* processInput = nullptr;
    inline uint8_t* dispatch = nullptr;
    inline uint8_t* inputReturn = nullptr;
    inline uint8_t* prepareRows = nullptr;
    inline GameRef<void*> pad, keyboard, mouse;
    inline uint32_t (__cdecl* pressed)() = nullptr;
    inline uint32_t (__cdecl* held)() = nullptr;
    inline void (__thiscall* remap)(void*, uint32_t*, uint32_t*) = nullptr;
    inline void (__thiscall* refreshLabels)(void*) = nullptr;

    inline void* Relative(const uint8_t* call)
    {
        return const_cast<uint8_t*>(call) + 5 + Read<int32_t>(call, 1);
    }

    inline bool Bind()
    {
        menuVtable = nullptr;
        initialize = processInput = dispatch = inputReturn = prepareRows = nullptr;
        auto constructor = hook::pattern("56 8B F1 E8 ? ? ? ? C7 06 ? ? ? ? C7 86 60 02 00 00 00 00 00 00 C6 86 64 02 00 00 00");
        auto input = hook::pattern("55 8B EC 83 E4 F0 83 EC 34 53 56 8B F1 57 C7 44 24 20 00 00 00 00 8D BE B8 02 00 00");
        auto accept = hook::pattern("8A 8E 64 02 00 00 80 F9 01 74 ? 8B 86 98 02 00 00");
        auto rows = hook::pattern("83 EC 14 C7 04 24 00 00 00 00 56 8B F1 C7 86 94 02 00 00 00 00 00 00");
        auto bits = hook::pattern("8B 0D ? ? ? ? 56 8B 35 ? ? ? ? 8B 91 C0 01 00 00");
        auto table = Read<uintptr_t*>(constructor.get_first<uint8_t>(), 10);
        const auto init = table[55];
        initialize = hook::range_pattern(init, init + 40,
            "56 57 8B F1 8B 0D ? ? ? ? 6A 01 8B 01 68 ? ? ? ? 68 ? ? ? ? FF 50 30 8B F8").get_first<uint8_t>();
        auto source = input.get_first<uint8_t>();
        auto tail = accept.get_first<uint8_t>();
        auto keys = Read<void**>(bits.get_first<uint8_t>(), 2);
        auto controller = Read<void**>(bits.get_first<uint8_t>(), 9);
        auto mouseRead = bits.get_first<uint8_t>() + 0xC6;
        auto pointer = Read<void**>(mouseRead, 2);
        auto labelTail = initialize + 0x192;
        hook::range_pattern(reinterpret_cast<uintptr_t>(labelTail), reinterpret_cast<uintptr_t>(labelTail) + 9,
            "5F 8B CE 5E E9 ? ? ? ?").get_first();
        menuVtable = table;
        processInput = source;
        dispatch = tail;
        inputReturn = source + 0x70F;
        prepareRows = rows.get_first<uint8_t>();
        held = reinterpret_cast<decltype(held)>(Relative(source + 0x14D));
        pressed = reinterpret_cast<decltype(pressed)>(Relative(source + 0x156));
        remap = reinterpret_cast<decltype(remap)>(Relative(source + 0x16F));
        refreshLabels = reinterpret_cast<decltype(refreshLabels)>(Relative(labelTail + 4));
        pad.SetAddress(controller);
        keyboard.SetAddress(keys);
        mouse.SetAddress(pointer);
        return true;
    }
}
