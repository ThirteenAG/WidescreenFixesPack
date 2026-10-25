#pragma once
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include "safetyhook.hpp"
#include "gameref.hpp"

namespace CTHudLive
{
class Positions
{
    struct Entry
    {
        void *body, *point, *type;
        uint32_t id;
        DWORD thread;
        int direction, extra, count;
        std::array<int16_t, 2> baseline{}, written{};
    };
    std::mutex mutex;
    std::map<void*, Entry> entries;
    template <class T> static T Read(void* at, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(at) + offset);
    }
    static void Set(void* at, size_t offset, int16_t value)
    {
        ProtectedGameRef<int16_t>{ reinterpret_cast<int16_t*>(static_cast<uint8_t*>(at) + offset) } = value;
    }
    static int Shift(float offset, int direction, int extra)
    {
        if (!std::isfinite(offset))
            return 0;
        return int(std::clamp(direction * (double(offset) + extra), -32768.0, 32767.0));
    }
    static int16_t Coordinate(int base, int shift)
    {
        return int16_t(std::clamp(base + shift, int(std::numeric_limits<int16_t>::min()), int(std::numeric_limits<int16_t>::max())));
    }

  public:
    // The parser is about to copy this temporary parameter into its owning
    // renderer. Retain the renderer's embedded destination, never the temp.
    bool Add(void* owner, void* point, void* source, uint32_t id, void* type, int count, int direction, int extra, float offset)
    {
        if (!owner || !point || !source || !type || (count != 1 && count != 2) || (direction != -1 && direction != 1) ||
            Read<uint32_t>(owner, 4) != id || Read<void*>(point, 0) != type || Read<void*>(source, 0) != type)
            return false;
        auto body = Read<void*>(owner, 8);
        if (!body || Read<void*>(body, 4) != point)
            return false;
        std::lock_guard lock(mutex);
        Entry entry{ body, point, type, id, GetCurrentThreadId(), direction, extra, count };
        const auto shift = Shift(offset, direction, extra);
        for (int i = 0; i < count; ++i)
        {
            entry.baseline[i] = Read<int16_t>(source, 8 + i * 2);
            entry.written[i] = Coordinate(entry.baseline[i], shift);
            Set(source, 8 + i * 2, entry.written[i]);
        }
        entries[owner] = entry;
        return true;
    }
    void Remove(void* owner)
    {
        std::lock_guard lock(mutex);
        entries.erase(owner);
    }
    void SetOffset(float offset)
    {
        if (!std::isfinite(offset))
            return;
        std::lock_guard lock(mutex);
        for (auto at = entries.begin(); at != entries.end();)
        {
            auto owner = at->first;
            auto& entry = at->second;
            if (entry.thread != GetCurrentThreadId())
            {
                ++at;
                continue;
            }
            bool current = Read<void*>(owner, 8) == entry.body && Read<uint32_t>(owner, 4) == entry.id &&
                           Read<void*>(entry.body, 4) == entry.point && Read<void*>(entry.point, 0) == entry.type;
            for (int i = 0; current && i < entry.count; ++i)
                current = Read<int16_t>(entry.point, 8 + i * 2) == entry.written[i];
            // A script or native state has taken over this geometry. Leave
            // it alone until a new checked parse supplies its authored data.
            if (!current)
            {
                at = entries.erase(at);
                continue;
            }
            const auto shift = Shift(offset, entry.direction, entry.extra);
            for (int i = 0; i < entry.count; ++i)
            {
                entry.written[i] = Coordinate(entry.baseline[i], shift);
                Set(entry.point, 8 + i * 2, entry.written[i]);
            }
            ++at;
        }
    }
    size_t Count()
    {
        std::lock_guard lock(mutex);
        return entries.size();
    }
};
inline Positions positions;
inline SafetyHookInline destructorHook;
inline int __fastcall Destroyed(void* owner, void*)
{
    positions.Remove(owner);
    return destructorHook.thiscall<int>(owner);
}
} // namespace CTHudLive
