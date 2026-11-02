module;

#include <stdafx.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string_view>

export module EngineText;

export namespace REText
{
    template<class T> T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    template<class T> void Write(void* object, size_t offset, T value)
    {
        std::memcpy(static_cast<uint8_t*>(object) + offset, &value, sizeof(value));
    }

    using Factory = void* (__cdecl*)();
    inline Factory factory = nullptr;
    inline uintptr_t* textVtable = nullptr;

    inline auto DataPattern(std::string_view signature)
    {
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto pe = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto sections = IMAGE_FIRST_SECTION(pe);
        const auto data = std::find_if(sections, sections + pe->FileHeader.NumberOfSections,
            [](const IMAGE_SECTION_HEADER& section) { return std::memcmp(section.Name, ".rdata", 6) == 0; });
        const auto begin = base + data->VirtualAddress;
        return hook::range_pattern(begin, begin + data->Misc.VirtualSize, signature);
    }

    inline bool Bind()
    {
        auto current = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 83 C4 04 8B C8 8B 10 FF 35 ? ? ? ? 6A 10 68 80 01 00 00 FF 52 1C 85 C0 74 07 8B C8 E9 94 00 00 00 33 C0 C3");
        auto original = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B 10 83 C4 04 8B C8 FF 35 ? ? ? ? 6A 10 68 80 01 00 00 FF 52 1C 85 C0 74 07 8B C8 E9 44 BC FF FF 33 C0 C3");
        const auto entry = current.empty() ? original.get_first<uint8_t>() : current.get_first<uint8_t>();
        const auto constructor = entry + 44 + Read<int32_t>(entry, 40);
        factory = reinterpret_cast<Factory>(entry);
        textVtable = Read<uintptr_t*>(constructor, 10);
        const auto setter = textVtable[22];
        hook::range_pattern(setter, setter + 112, "83 EC 74 56 8B F1 8B 06 FF 50 60").get_first();
        return true;
    }

    // Nodes are allocated and destroyed by the game's GUI tree, including its font buffers.
    inline void* Append(void* gui, void* parent, uint32_t priority)
    {
        if (!factory || !gui || !parent || !Read<void*>(gui, 240)) return nullptr;
        auto node = factory();
        if (!node) return nullptr;
        Write(node, 8, priority);
        Write(node, 104, parent);
        Write(node, 108, gui);
        // The options resource contains image labels. A null local resource selects the global UI font bank.
        Write<void*>(node, 112, nullptr);
        auto flags = Read<uint32_t>(node, 84) | 9;
        if (Read<uint32_t>(gui, 328) & 0x400) flags |= 0x200000;
        Write(node, 84, flags);
        auto sibling = Read<void*>(parent, 96);
        if (sibling)
        {
            while (auto next = Read<void*>(sibling, 100)) sibling = next;
            Write(sibling, 100, node);
        }
        else Write(parent, 96, node);
        Write(parent, 84, Read<uint32_t>(parent, 84) | 0x40000);
        return node;
    }

    inline void Set(void* node, std::string_view value)
    {
        if (!node || value.size() > static_cast<size_t>(std::numeric_limits<int>::max())) return;
        using SetText = void (__thiscall*)(void*, const char*, int, int);
        // Slot 23 is the four-argument format overload; slot 22 owns a UTF-8 copy.
        reinterpret_cast<SetText>(Read<uintptr_t*>(node, 0)[22])(
            node, value.data(), static_cast<int>(value.size()), -1);
    }

    inline void Place(void* node, float x, float y, int size, uint32_t color = 0xFFFFFFFF)
    {
        if (!node) return;
        Write(node, 160, x);
        Write(node, 164, y);
        Write(node, 228, color);
        Write(node, 232, size);
        Write(node, 236, size);
        Write(node, 280, Read<uint32_t>(node, 280) | 0x10000);
        Write(node, 84, Read<uint32_t>(node, 84) | 0x10000);
    }

    inline void Show(void* node, bool visible)
    {
        if (node) Write(node, 84, (Read<uint32_t>(node, 84) & ~1u) | (visible ? 1u : 0u));
    }
}
