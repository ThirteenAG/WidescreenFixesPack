#pragma once
#include <cstdint>
#include <cstring>

namespace lcsws::icon_pack
{
struct Entry
{
    char name[32];
    uint8_t width, height, depth, levels;
    int16_t stride;
    uint16_t flags;
    uint32_t bytes, runs;
};
struct Run { uint32_t offset, length; };
static_assert(sizeof(Entry) == 48 && sizeof(Run) == 8);

struct View
{
    const uint8_t* data;
    uint32_t size;

    uint32_t EntrySize(uint32_t at) const
    {
        if (!data || (at & 3) || at > size || sizeof(Entry) > size - at) return 0;
        const auto entry = reinterpret_cast<const Entry*>(data + at);
        uint32_t bytes = sizeof(Entry);
        if (entry->bytes < 16 || entry->bytes > 96 * 1024 || entry->runs > (size - at - bytes) / sizeof(Run)) return 0;
        for (uint32_t i = 0; i < entry->runs; ++i)
        {
            if (sizeof(Run) > size - at - bytes) return 0;
            const auto run = reinterpret_cast<const Run*>(data + at + bytes);
            if (run->offset > entry->bytes || run->length > entry->bytes - run->offset) return 0;
            if (run->length > size - at - bytes - sizeof(Run)) return 0;
            const uint32_t advance = (sizeof(Run) + run->length + 3) & ~3u;
            if (advance > size - at - bytes) return 0;
            bytes += advance;
        }
        return bytes;
    }
    const Entry* Find(const char* name) const
    {
        if (!name) return nullptr;
        for (uint32_t at = 0; at < size;)
        {
            const uint32_t bytes = EntrySize(at);
            if (!bytes) return nullptr;
            const auto entry = reinterpret_cast<const Entry*>(data + at);
            if (!std::strncmp(name, entry->name, sizeof(entry->name))) return entry;
            at += bytes;
        }
        return nullptr;
    }
};
}
