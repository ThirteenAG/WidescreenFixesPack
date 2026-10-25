#pragma once

#include "NativeMenuABI.hpp"
#include <cmath>

namespace KongMenu
{
    struct Context
    {
        void* world = nullptr;
        void* owner = nullptr;
        void* behavior = nullptr;
        void* parameters = nullptr;
        void* page = nullptr;
        void* text = nullptr;
        void* model = nullptr;
        void* descriptors = nullptr;
        void* glyphs = nullptr;
        void* fontGroup = nullptr;
        uint32_t glyphCapacity = 0;
        int first = 0;
        uint8_t slotCount = 0;
        uint16_t width = 0, height = 0;
        Native::TextSlot style{};

        static bool ScratchReady()
        {
            // The explicit setters use a native scratch object's graphic pair
            // for their font-group assignment before walking any source glyphs.
            auto scratch = Native::Read<void*>(reinterpret_cast<void*>(0xCC0E10), 0);
            if (!Native::Readable(scratch, 24)) return false;
            auto graphic = Native::Read<void*>(scratch, 20);
            if (!Native::Readable(graphic, 4)) return false;
            auto pair = Native::Read<void*>(graphic, 0);
            if (!Native::Readable(pair, 8)) return false;
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQuery(static_cast<uint8_t*>(pair) + 4, &info, sizeof(info))) return false;
            auto protection = info.Protect & 0xFF;
            return protection == PAGE_READWRITE || protection == PAGE_WRITECOPY ||
                protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
        }

        void* FontReady() const
        {
            // Follow the exact index-zero resource route used by 9C2700, but
            // require an existing cache entry. No font registry allocation is
            // permitted while publishing a private page.
            auto resource = fontGroup;
            if (!Native::Readable(resource, 36)) return nullptr;
            auto kind = Native::Read<void*>(resource, 0);
            if (!Native::Readable(kind, 4)) return nullptr;
            if (Native::Read<uint32_t>(kind, 0) == 4)
            {
                auto count = Native::Read<uint32_t>(resource, 8);
                auto table = Native::Read<void*>(resource, 12);
                if (!count || count > 4096 || !Native::Readable(table, count * 4)) return nullptr;
                auto index = Native::Read<uint32_t>(reinterpret_cast<void*>(0xBE2D18), 0) == 12 ? 22U : 0U;
                if (index >= count) index = 0;
                resource = Native::Read<void*>(table, index * 4);
                if (!Native::Readable(resource, 36)) return nullptr;
                kind = Native::Read<void*>(resource, 0);
                if (!Native::Readable(kind, 4)) return nullptr;
            }
            int handle = -1;
            switch (Native::Read<uint32_t>(kind, 0))
            {
                case 3: handle = Native::Read<int>(resource, 32); break;
                case 5:
                {
                    auto pointer = Native::Read<void*>(resource, 32);
                    if (!Native::Readable(pointer, 2)) return nullptr;
                    handle = Native::Read<int16_t>(pointer, 0);
                    break;
                }
                default: return nullptr;
            }
            void* texture = nullptr;
            if (handle != -1)
            {
                if (handle < 0 || handle > 65535) return nullptr;
                auto table = Native::Read<uintptr_t>(reinterpret_cast<void*>(0xF4B08C), 0);
                if (!table || table > std::numeric_limits<uintptr_t>::max() - 12 * 65536) return nullptr;
                auto entry = reinterpret_cast<void*>(table + size_t(handle) * 12);
                if (!Native::Readable(entry, 12)) return nullptr;
                if (Native::Read<uint8_t>(entry, 8) & 0x40)
                {
                    handle = Native::Read<int16_t>(entry, 6);
                    if (handle < 0) return nullptr;
                    entry = reinterpret_cast<void*>(table + size_t(handle) * 12);
                    if (!Native::Readable(entry, 12)) return nullptr;
                }
                texture = Native::Read<void*>(entry, 0);
                if (!texture) return nullptr;
            }
            auto cache = Native::Read<void*>(reinterpret_cast<void*>(0xCC0DE0), 0);
            auto capacity = Native::Read<int>(reinterpret_cast<void*>(0xCC0DE4), 0);
            auto count = Native::Read<int>(reinterpret_cast<void*>(0xCC0DE8), 0);
            if (count <= 0 || capacity < count || capacity > 4096 ||
                !Native::Readable(cache, size_t(capacity) * 28)) return nullptr;
            // Native first searches the texture identity, then its resource key.
            for (int pass = 0; pass < 2; ++pass)
                for (int i = 0; i < count; ++i)
                {
                    auto entry = static_cast<uint8_t*>(cache) + size_t(i) * 28;
                    if ((pass == 0 && texture && (Native::Read<void*>(entry, 0) == texture ||
                                                Native::Read<void*>(entry, 4) == texture)) ||
                        (pass == 1 && Native::Read<void*>(entry, 8) == resource))
                        return entry;
                }
            return nullptr;
        }

        static void* TextModel(void* object, void** group = nullptr)
        {
            if (!Native::Readable(object, 40) || !(Native::Read<uint32_t>(object, 0) & 0x4000))
                return nullptr;
            auto graphic = Native::Read<void*>(object, 20);
            if (!Native::Readable(graphic, 4))
                return nullptr;
            auto pair = Native::Read<void*>(graphic, 0);
            if (!Native::Readable(pair, 8))
                return nullptr;
            auto candidate = Native::Read<void*>(pair, 0);
            if (!Native::Readable(candidate, 3992))
                return nullptr;
            auto descriptor = Native::Read<void*>(candidate, 0);
            if (!Native::Readable(descriptor, 4) || Native::Read<uint32_t>(descriptor, 0) != 10)
                return nullptr;
            if (group)
                *group = Native::Read<void*>(pair, 4);
            return candidate;
        }

        bool Member() const
        {
            // The behavior iterator in 406340 uses eight-byte object entries.
            if (!Native::Readable(world, 308))
                return false;
            auto begin = Native::Read<uintptr_t>(world, 304);
            auto end = Native::Read<uintptr_t>(world, 292);
            if (!begin || end < begin || (end - begin) % 8 || end - begin > 65536 * 8 ||
                !Native::Readable(reinterpret_cast<void*>(begin), end - begin))
                return false;
            for (auto at = begin; at < end; at += 8)
                if (Native::Read<void*>(reinterpret_cast<void*>(at), 0) == owner)
                    return true;
            return false;
        }

        bool Alive() const
        {
            if (!Member() || !Native::Readable(owner, 40) || !Native::Readable(behavior, 88) ||
                !Native::Readable(parameters, 1396) || !Native::Readable(page, 24))
                return false;
            auto component = Native::Read<void*>(owner, 24);
            if (!Native::Readable(component, 8) || Native::Read<void*>(component, 4) != behavior ||
                Native::Read<void*>(behavior, 64) != parameters ||
                Native::Read<void*>(parameters, 8) != text ||
                Native::Read<int>(parameters, 248) != first ||
                Native::Read<uintptr_t>(page, 20) != 0x623150)
                return false;
            void* group = nullptr;
            if (TextModel(text, &group) != model || group != fontGroup ||
                Native::Read<void*>(model, 28) != glyphs ||
                Native::Read<void*>(model, 32) != descriptors ||
                Native::Read<uint32_t>(model, 8) != glyphCapacity ||
                Native::Read<uint8_t>(model, 12) != slotCount ||
                Native::Read<uint16_t>(model, 24) != width ||
                Native::Read<uint16_t>(model, 26) != height)
                return false;
            return true;
        }

        bool Current() const
        {
            return Alive() && Native::Read<void*>(behavior, 36) == page &&
                !Native::Read<void*>(behavior, 32) &&
                Native::Read<uint32_t>(parameters, 24) &&
                !Native::Read<uint32_t>(parameters, 32) &&
                !Native::Read<uint32_t>(parameters, 216) &&
                !Native::Read<uint32_t>(parameters, 1388);
        }

        static bool Capture(void* world, Context& result)
        {
            Context next;
            next.world = world;
            next.owner = Native::Read<void*>(reinterpret_cast<void*>(0xB420E4), 0);
            next.behavior = Native::Read<void*>(reinterpret_cast<void*>(0xB420DC), 0);
            if (!Native::Readable(next.behavior, 88))
                return false;
            next.parameters = Native::Read<void*>(next.behavior, 64);
            next.page = Native::Read<void*>(next.behavior, 36);
            if (!Native::Readable(next.parameters, 1396) || !Native::Readable(next.page, 24))
                return false;
            next.text = Native::Read<void*>(next.parameters, 8);
            next.model = TextModel(next.text, &next.fontGroup);
            if (!next.model || !next.fontGroup)
                return false;
            next.first = Native::Read<int>(next.parameters, 248);
            next.slotCount = Native::Read<uint8_t>(next.model, 12);
            next.glyphCapacity = Native::Read<uint32_t>(next.model, 8);
            next.glyphs = Native::Read<void*>(next.model, 28);
            next.descriptors = Native::Read<void*>(next.model, 32);
            next.width = Native::Read<uint16_t>(next.model, 24);
            next.height = Native::Read<uint16_t>(next.model, 26);
            if (next.slotCount < 30 || next.first < 0 || next.first > int(next.slotCount) - 30 ||
                !next.glyphCapacity || next.glyphCapacity > 65535 ||
                !Native::Readable(next.glyphs, next.glyphCapacity * sizeof(Native::Glyph)) ||
                !Native::Readable(next.descriptors, next.slotCount * sizeof(Native::TextSlot)) ||
                next.width < 320 || next.height < 240 || next.width > 16384 || next.height > 16384)
                return false;
            auto slot = static_cast<Native::TextSlot*>(next.descriptors) + next.first + 5;
            std::memcpy(&next.style, slot, sizeof(next.style));
            if (!std::isfinite(next.style.x) || !std::isfinite(next.style.y) ||
                !std::isfinite(next.style.z) || next.style.x < 0.0f || next.style.x > 1.0f ||
                next.style.y < 0.0f || next.style.y > 1.0f || !next.Current())
                return false;
            result = next;
            return true;
        }
    };
}
