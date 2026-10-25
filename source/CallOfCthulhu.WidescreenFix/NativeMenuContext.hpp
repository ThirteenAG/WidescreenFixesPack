#pragma once

#include "NativeMenuText.hpp"
#include <array>
#include <string>

namespace CthulhuMenu
{
    // The map's key is a native std::string. Inspect it without invoking a native
    // allocator or inserting a missing key during discovery/collision checks.
    inline bool FindPageNode(void* manager, const std::string& name, void*& result)
    {
        result = nullptr;
        const auto sentinel = Read<void*>(manager, 64);
        if (!Readable(sentinel, 46) || !Read<uint8_t>(sentinel, 45))
            return false;
        auto node = Read<void*>(sentinel, 4);
        for (size_t visited = 0; visited < 512 && node != sentinel; ++visited)
        {
            if (!Readable(node, 46) || Read<uint8_t>(node, 45))
                return false;
            const auto length = Read<uint32_t>(node, 32);
            const auto capacity = Read<uint32_t>(node, 36);
            if (length > 255 || capacity < length)
                return false;
            const auto data = capacity < 16 ? static_cast<const char*>(node) + 16 : Read<const char*>(node, 16);
            if (!Readable(data, length + 1) || data[length] != '\0')
                return false;
            const auto comparison = name.compare(0, name.size(), data, length);
            if (!comparison)
            {
                result = node;
                return true;
            }
            node = Read<void*>(node, comparison < 0 ? 0 : 8);
        }
        return node == sentinel;
    }

    inline void* FindPage(void* manager, const std::string& name)
    {
        void* node = nullptr;
        return FindPageNode(manager, name, node) && node ? Read<void*>(node, 40) : nullptr;
    }

    struct Context
    {
        void* manager = nullptr;
        void* options = nullptr;
        void** begin = nullptr;
        std::array<void*, 7> rows{};
        int mode = 0;
        int header = 0;

        bool Capture()
        {
            manager = reinterpret_cast<void*>(0x6C8560);
            if (!Readable(manager, 249) || Read<uintptr_t>(manager, 0) != 0x62BD68)
                return false;
            options = FindPage(manager, "OPTIONS");
            if (!Readable(options, 2240) || Read<uintptr_t>(options, 0) != 0x62CAEC)
                return false;
            begin = Read<void**>(options, 8);
            const auto end = Read<void**>(options, 12);
            if (!Readable(begin, rows.size() * sizeof(void*)) ||
                reinterpret_cast<uintptr_t>(end) != reinterpret_cast<uintptr_t>(begin) + rows.size() * sizeof(void*))
                return false;
            for (size_t i = 0; i < rows.size(); ++i)
            {
                rows[i] = Read<void*>(begin, i * sizeof(void*));
                if (!Readable(rows[i], 360) || Read<uintptr_t>(rows[i], 0) != 0x62C7AC ||
                    Read<void*>(rows[i], 268) != options)
                    return false;
            }
            const auto literal = Read<const char*>(rows[4], 328);
            if (Read<int>(rows[4], 264) || Read<uint8_t>(rows[4], 273) ||
                (literal && (!Readable(literal, 1) || *literal)))
                return false;
            mode = Read<int>(manager, 244);
            header = Read<int>(options, 2232);
            return (mode == 1 || mode == 2) && header >= 0 && header < 2 &&
                Read<uint8_t>(manager, 248) == header && Alive();
        }

        bool Alive() const
        {
            if (!Readable(manager, 249) || Read<uintptr_t>(manager, 0) != 0x62BD68 ||
                FindPage(manager, "OPTIONS") != options || !Readable(options, 2240) ||
                Read<uintptr_t>(options, 0) != 0x62CAEC || Read<void**>(options, 8) != begin ||
                reinterpret_cast<uintptr_t>(Read<void**>(options, 12)) !=
                    reinterpret_cast<uintptr_t>(begin) + rows.size() * sizeof(void*) ||
                Read<int>(manager, 244) != mode || Read<int>(options, 2232) != header ||
                Read<uint8_t>(manager, 248) != header)
                return false;
            for (size_t i = 0; i < rows.size(); ++i)
                if (Read<void*>(begin, i * sizeof(void*)) != rows[i] || !Readable(rows[i], 360) ||
                    Read<uintptr_t>(rows[i], 0) != 0x62C7AC || Read<void*>(rows[i], 268) != options)
                    return false;
            return true;
        }

        bool Current(void* page) const
        {
            if (!Alive() || Read<uint8_t>(manager, 12) || !Read<uint8_t>(manager, 33) ||
                Read<void*>(manager, 44) || Read<void*>(manager, 52) || Read<void*>(manager, 48) != page)
                return false;
            const auto count = Read<uint32_t>(reinterpret_cast<void*>(0x6C8890), 0);
            return count > 0 && count <= 10 &&
                Read<void*>(reinterpret_cast<void*>(0x6C8840), (count - 1) * 8) == page &&
                !Read<uint8_t>(reinterpret_cast<void*>(0x6C8844), (count - 1) * 8);
        }

        bool SelectedEntry() const
        {
            return Current(options) && Read<int>(options, 540) == 4;
        }

        bool Pause() const
        {
            // The title menu uses mode 1, the in-game pause menu mode 2.
            return mode == 2;
        }
    };
}
