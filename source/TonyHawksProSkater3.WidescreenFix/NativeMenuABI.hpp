#pragma once
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace THPS3Menu
{
    struct Component
    {
        uint8_t type = 0, flags = 0;
        uint16_t reserved = 0;
        uint32_t name = 0, value = 0;
        Component* next = nullptr;
    };
    struct Structure { Component* head = nullptr; Component* tail = nullptr; };
    struct Array { uint32_t data = 0; uint8_t type = 0, reserved = 0; uint16_t count = 0; };
    static_assert(sizeof(Component) == 16 && sizeof(Structure) == 8 && sizeof(Array) == 8);

    constexpr uint32_t Checksum(const char* text)
    {
        uint32_t value = 0xFFFFFFFF;
        for (; *text; ++text)
        {
            auto c = static_cast<uint8_t>(*text);
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (c == '/') c = '\\';
            value ^= c;
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ ((value & 1) ? 0xEDB88320 : 0);
        }
        return value;
    }
    static_assert(Checksum("textmenuelement") == 0x6B7AE77F);
    static_assert(Checksum("verticalmenu") == 0x87FCAE56);

    // Borrowed synchronous inputs only. Native429630/4292B0 clone strings,
    // structures and arrays before retention; native destructors never own this.
    class Parameters
    {
    public:
        Structure value;
        Parameters() = default;
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;
        Parameters(Parameters&&) = delete;
        Parameters& operator=(Parameters&&) = delete;
        static uint32_t Pointer(const void* value) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value)); }
        void Integer(const char* name, uint32_t number) { Add(1, Checksum(name), number); }
        void Float(const char* name, float number)
        {
            uint32_t bits;
            std::memcpy(&bits, &number, sizeof(bits));
            Add(2, Checksum(name), bits);
        }
        void Name(const char* name, uint32_t checksum) { Add(13, Checksum(name), checksum); }
        void Name(const char* name, const char* text) { Name(name, Checksum(text)); }
        void Flag(const char* name) { Add(13, 0, Checksum(name)); }
        void Text(const char* name, const std::string& text)
        {
            strings.push_back(text);
            Add(3, Checksum(name), Pointer(strings.back().c_str()));
        }
        void Nested(const char* name, Structure* nested) { Add(11, Checksum(name), Pointer(nested)); }
        void List(const char* name, Array* array) { Add(12, Checksum(name), Pointer(array)); }

    private:
        std::deque<Component> components;
        std::deque<std::string> strings;
        void Add(uint8_t type, uint32_t name, uint32_t data)
        {
            components.push_back({ type, 0, 0, name, data, nullptr });
            auto* next = &components.back();
            if (value.tail) value.tail->next = next;
            else value.head = next;
            value.tail = next;
        }
    };
}
