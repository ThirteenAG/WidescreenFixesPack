#pragma once
#include "MenuLayout.hpp"
#include <string>

namespace CTControlRows
{
    using CTSettings::Read;
    using CTSettings::Set;
    inline int Count(void* list)
    {
        if (!list)
            return -1;
        const auto begin = Read<uintptr_t>(list, 108), end = Read<uintptr_t>(list, 112), capacity = Read<uintptr_t>(list, 116);
        if (!begin)
            return !end && !capacity ? 0 : -1;
        return end >= begin && capacity >= end && end - begin <= 60 * 256 && (end - begin) % 60 == 0 ? int((end - begin) / 60) : -1;
    }
    inline bool Assign(void* string, const std::wstring& text)
    {
        if (!string || text.size() > 512)
            return false;
        auto old = Read<uint32_t>(string, 24) >= 8 ? Read<void*>(string, 4) : nullptr;
        if (text.size() < 8)
        {
            std::memcpy(static_cast<uint8_t*>(string) + 4, text.c_str(), (text.size() + 1) * 2);
            Set<uint32_t>(string, 24, 7);
        }
        else
        {
            auto value = CTLayout::Memory((text.size() + 1) * 2);
            if (!value)
                return false;
            std::memcpy(value, text.c_str(), (text.size() + 1) * 2);
            Set<void*>(string, 4, value);
            Set<uint32_t>(string, 24, uint32_t(text.size()));
        }
        Set<uint32_t>(string, 20, uint32_t(text.size()));
        if (old)
            CTLayout::release(old);
        return true;
    }
    struct Rows
    {
        void *list = nullptr, *data = nullptr;
        uintptr_t oldBegin = 0, oldEnd = 0, oldCapacity = 0;
        int original = 0, count = 0;
        Rows() = default;
        Rows(const Rows&) = delete;
        Rows& operator=(const Rows&) = delete;
        ~Rows()
        {
            if (!data)
                return;
            for (int i = original; i < count; ++i)
            {
                auto row = static_cast<uint8_t*>(data) + i * 60;
                if (Read<uint32_t>(row, 24) >= 8)
                    CTLayout::release(Read<void*>(row, 4));
            }
            CTLayout::release(data);
        }
        bool Prepare(void* target, int additional)
        {
            original = Count(target);
            if (original < 1 || additional < 1 || original + additional > 256)
                return false;
            list = target;
            count = original + additional;
            oldBegin = Read<uintptr_t>(list, 108);
            oldEnd = Read<uintptr_t>(list, 112);
            oldCapacity = Read<uintptr_t>(list, 116);
            data = CTLayout::Memory(count * 60);
            if (!data)
                return false;
            std::memcpy(data, reinterpret_cast<void*>(oldBegin), original * 60);
            for (int i = original; i < count; ++i)
            {
                auto row = static_cast<uint8_t*>(data) + i * 60;
                Set<uint32_t>(row, 24, 7);
                Set<uint8_t>(row, 52, 1);
            }
            return true;
        }
        bool Text(int row, const std::wstring& text, bool disabled)
        {
            if (!data || row < 0 || original + row >= count)
                return false;
            auto target = static_cast<uint8_t*>(data) + (original + row) * 60;
            Set<uint8_t>(target, 53, disabled ? 1 : 0);
            return Assign(target, text);
        }
        bool Current() const
        {
            return data && Read<uintptr_t>(list, 108) == oldBegin && Read<uintptr_t>(list, 112) == oldEnd &&
                   Read<uintptr_t>(list, 116) == oldCapacity;
        }
        void Publish()
        {
            Set<void*>(list, 108, data);
            Set<void*>(list, 112, static_cast<uint8_t*>(data) + count * 60);
            Set<void*>(list, 116, static_cast<uint8_t*>(data) + count * 60);
            Set<uint8_t>(list, 180, uint8_t(Read<uint8_t>(list, 180) | 0x40));
            data = nullptr;
            CTLayout::release(reinterpret_cast<void*>(oldBegin));
        }
    };
    struct Integers
    {
        void *array = nullptr, *data = nullptr, *original = nullptr;
        int count = 0, capacity = 0;
        Integers() = default;
        Integers(const Integers&) = delete;
        Integers& operator=(const Integers&) = delete;
        ~Integers()
        {
            if (data)
                CTLayout::release(data);
        }
        bool Prepare(void* target, int expected)
        {
            array = target;
            original = Read<void*>(array, 0);
            count = Read<int>(array, 4);
            capacity = Read<int>(array, 8);
            if (!original || count != expected || count < 1 || count > 128 || capacity < count)
                return false;
            data = CTLayout::Memory((count + 6) * 4);
            if (data)
                std::memcpy(data, original, count * 4);
            return data != nullptr;
        }
        bool Current() const
        {
            return data && Read<void*>(array, 0) == original && Read<int>(array, 4) == count && Read<int>(array, 8) == capacity;
        }
        void Publish()
        {
            Set<void*>(array, 0, data);
            Set<int>(array, 4, count + 6);
            Set<int>(array, 8, count + 6);
            data = nullptr;
            CTLayout::release(original);
        }
    };
    struct Pointers
    {
        void *object = nullptr, *data = nullptr;
        size_t offset = 0, count = 0;
        uintptr_t begin = 0, end = 0, capacity = 0;
        Pointers() = default;
        Pointers(const Pointers&) = delete;
        Pointers& operator=(const Pointers&) = delete;
        ~Pointers()
        {
            if (data)
                CTLayout::release(data);
        }
        bool Prepare(void* target, size_t field, void* value)
        {
            object = target;
            offset = field;
            if (!CTLayout::Vector(object, offset, 128, begin, count))
                return false;
            end = Read<uintptr_t>(object, offset + 4);
            capacity = Read<uintptr_t>(object, offset + 8);
            if ((begin && capacity < end) || (!begin && capacity))
                return false;
            data = CTLayout::Memory((count + 1) * 4);
            if (!data)
                return false;
            if (count)
                std::memcpy(data, reinterpret_cast<void*>(begin), count * 4);
            static_cast<void**>(data)[count] = value;
            return true;
        }
        bool Current() const
        {
            return data && Read<uintptr_t>(object, offset) == begin && Read<uintptr_t>(object, offset + 4) == end &&
                   Read<uintptr_t>(object, offset + 8) == capacity;
        }
        void Publish()
        {
            Set<void*>(object, offset, data);
            Set<void*>(object, offset + 4, static_cast<uint8_t*>(data) + (count + 1) * 4);
            Set<void*>(object, offset + 8, static_cast<uint8_t*>(data) + (count + 1) * 4);
            data = nullptr;
            if (begin)
                CTLayout::release(reinterpret_cast<void*>(begin));
        }
    };
} // namespace CTControlRows
