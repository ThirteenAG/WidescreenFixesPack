#pragma once
#include <array>
#include <cstring>
#include <memory>
#include "Settings.hpp"

namespace CTLayout
{
    using CTSettings::Read;
    using CTSettings::Set;
    inline void*(__cdecl* allocate)(size_t, const wchar_t*) = nullptr;
    inline void(__cdecl* release)(void*) = nullptr;

    inline bool Vector(void* object, size_t offset, size_t limit, uintptr_t& begin, size_t& count)
    {
        begin = Read<uintptr_t>(object, offset);
        auto end = Read<uintptr_t>(object, offset + 4);
        count = 0;
        if (!begin)
            return !end;
        if (end < begin || (end - begin) % 4 || end - begin > 4 * limit)
            return false;
        count = (end - begin) / 4;
        return true;
    }
    inline void* Memory(size_t size)
    {
        auto result = allocate(size, nullptr);
        if (result)
            std::memset(result, 0, size);
        return result;
    }
    inline bool Stretch(void* point, int sourceWidth, int width)
    {
        for (auto offset : { 8u, 10u })
        {
            const auto x = Read<int16_t>(point, offset);
            if (x < -32 || x > sourceWidth + 32)
                return false;
            if (x > sourceWidth / 2)
                Set<int16_t>(point, offset, int16_t(x + width - sourceWidth));
        }
        return true;
    }
    inline void* ClonePoint(void* source, size_t size, int sourceWidth, int width)
    {
        auto point = Memory(size);
        if (!point)
            return nullptr;
        std::memcpy(point, source, size);
        if (!Stretch(point, sourceWidth, width))
        {
            release(point);
            return nullptr;
        }
        return point;
    }
    inline void* CloneItem(void* source, int sourceWidth, int width, bool copyFrames = true)
    {
        const auto kind = CTSettings::Kind(source);
        if (kind != 5 && kind != 6 && kind != 8)
            return nullptr;
        // Basic wrappers and resource frames own their arrays and parameters.
        // Font, texture and draw-state handles in these three renderers are
        // package-owned; the native destructors leave them untouched.
        if (Read<void*>(source, 16))
            return nullptr;
        auto sourceBody = Read<void*>(source, 8);
        const size_t bodySize = kind == 5 ? 52 : kind == 6 ? 40 : 92;
        const size_t pointOffset = kind == 8 ? 12 : 8;
        const size_t pointSize = kind == 5 ? 36 : 28;
        auto sourcePoint = Read<void*>(sourceBody, 4);
        if (sourcePoint != static_cast<uint8_t*>(sourceBody) + pointOffset)
            return nullptr;
        auto item = Memory(28);
        if (!item)
            return nullptr;
        Set<void*>(item, 0, Read<void*>(source, 0));
        auto body = Memory(bodySize);
        if (!body)
        {
            release(item);
            return nullptr;
        }
        Set<void*>(item, 8, body);
        if (kind == 8)
        {
            std::memcpy(body, sourceBody, 56);
            std::memcpy(static_cast<uint8_t*>(body) + 84, static_cast<uint8_t*>(sourceBody) + 84, 8);
            Set<uint32_t>(body, 80, 7); // Independently owned empty native wide string.
        }
        else
            std::memcpy(body, sourceBody, bodySize);
        auto point = static_cast<uint8_t*>(body) + pointOffset;
        Set<void*>(body, 4, point);
        if (!Stretch(point, sourceWidth, width))
        {
            CTSettings::Destroy(item);
            return nullptr;
        }
        Set<uint32_t>(item, 4, Read<uint32_t>(source, 4));
        Set<uint32_t>(item, 20, Read<uint32_t>(source, 20));
        Set<uint8_t>(item, 24, Read<uint8_t>(source, 24));
        if (!copyFrames)
        {
            Set<uint32_t>(item, 20, (Read<uint32_t>(item, 20) & ~0xFFFu) | 0xFFEu);
            return item;
        }
        auto frames = Read<void*>(source, 12);
        if (!frames)
            return item;
        uintptr_t begin = 0;
        size_t count = 0;
        if (!Vector(frames, 4, 32, begin, count))
        {
            CTSettings::Destroy(item);
            return nullptr;
        }
        auto copied = Memory(16);
        if (!copied)
        {
            CTSettings::Destroy(item);
            return nullptr;
        }
        Set<void*>(item, 12, copied);
        if (!count)
            return item;
        auto entries = Memory(count * 4);
        if (!entries)
        {
            CTSettings::Destroy(item);
            return nullptr;
        }
        Set<void*>(copied, 4, entries);
        Set<void*>(copied, 8, entries);
        Set<void*>(copied, 12, static_cast<uint8_t*>(entries) + count * 4);
        for (size_t i = 0; i < count; ++i)
        {
            auto original = reinterpret_cast<void**>(begin)[i];
            uintptr_t events = 0;
            size_t eventCount = 0;
            auto eventList = original ? Read<void*>(original, 8) : nullptr;
            if (!original || !Read<void*>(original, 4) || Read<uintptr_t>(Read<void*>(original, 4), 0) != Read<uintptr_t>(sourcePoint, 0) ||
                (eventList && (!Vector(eventList, 4, 16, events, eventCount) || eventCount)))
            {
                CTSettings::Destroy(item);
                return nullptr;
            }
            auto frame = Memory(16);
            if (!frame)
            {
                CTSettings::Destroy(item);
                return nullptr;
            }
            std::memcpy(frame, original, 16);
            Set<void*>(frame, 4, nullptr);
            Set<void*>(frame, 8, nullptr);
            reinterpret_cast<void**>(entries)[i] = frame;
            Set<void*>(copied, 8, static_cast<uint8_t*>(entries) + (i + 1) * 4);
            auto parameter = ClonePoint(Read<void*>(original, 4), pointSize, sourceWidth, width);
            if (!parameter)
            {
                CTSettings::Destroy(item);
                return nullptr;
            }
            Set<void*>(frame, 4, parameter);
        }
        return item;
    }
    inline void* Clone(void* source, int width)
    {
        if (!source || !allocate || !release)
            return nullptr;
        auto kind = reinterpret_cast<int(__fastcall*)(void*, void*)>(Read<void**>(source, 0)[4])(source, nullptr);
        // The title list's five-state row resource has no controller callbacks.
        if (kind != 2)
            return nullptr;
        uintptr_t begin = 0, states = 0;
        size_t count = 0, stateCount = 0;
        if (!Vector(source, 8, 32, begin, count) || !count || !Vector(source, 40, 8, states, stateCount) || stateCount != 5)
            return nullptr;
        // Every authored row has one fixed-width glyph rectangle with 5px
        // side margins. Derive its width without mutating the package page.
        int sourceWidth = 0;
        for (size_t i = 0; i < count; ++i)
        {
            auto child = reinterpret_cast<void**>(begin)[i];
            if (CTSettings::Kind(child) != 8)
                continue;
            if (sourceWidth)
                return nullptr;
            auto point = Read<void*>(Read<void*>(child, 8), 4);
            if (!point || Read<int16_t>(point, 8) != 5)
                return nullptr;
            sourceWidth = Read<int16_t>(point, 10) + 5;
        }
        if (sourceWidth < 100 || sourceWidth > 480 || width < sourceWidth || width > 540)
            return nullptr;
        // The native constructors only initialize these fields and the two
        // vectors. Build checked vectors so allocation refusal cannot publish
        // a partial row or overwrite the game's shared template.
        auto page = Memory(52);
        if (!page)
            return nullptr;
        Set<void*>(page, 0, Read<void*>(source, 0));
        Set<uint32_t>(page, 20, Read<uint32_t>(source, 20));
        Set<uint32_t>(page, 24, Read<uint32_t>(source, 24));
        Set<uint32_t>(page, 28, Read<uint32_t>(source, 28));
        Set<uint8_t>(page, 32, Read<uint8_t>(source, 32));
        auto stateData = Memory(stateCount * 4), children = Memory(count * 4);
        if (!stateData || !children)
        {
            if (stateData)
                release(stateData);
            if (children)
                release(children);
            CTSettings::Destroy(page);
            return nullptr;
        }
        std::memcpy(stateData, reinterpret_cast<void*>(states), stateCount * 4);
        Set<void*>(page, 40, stateData);
        Set<void*>(page, 44, static_cast<uint8_t*>(stateData) + stateCount * 4);
        Set<void*>(page, 48, static_cast<uint8_t*>(stateData) + stateCount * 4);
        Set<void*>(page, 8, children);
        Set<void*>(page, 12, children);
        Set<void*>(page, 16, static_cast<uint8_t*>(children) + count * 4);
        for (size_t i = 0; i < count; ++i)
        {
            auto child = CloneItem(reinterpret_cast<void**>(begin)[i], sourceWidth, width);
            if (!child)
            {
                CTSettings::Destroy(page);
                return nullptr;
            }
            reinterpret_cast<void**>(children)[i] = child;
            Set<void*>(page, 12, static_cast<uint8_t*>(children) + (i + 1) * 4);
        }
        return page;
    }
    struct Templates
    {
        std::array<void*, 3> pages{};
        ~Templates()
        {
            for (auto page : pages)
                CTSettings::Destroy(page);
        }
    };
} // namespace CTLayout
