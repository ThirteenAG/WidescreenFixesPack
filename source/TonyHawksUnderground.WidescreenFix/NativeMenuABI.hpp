#pragma once
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace THUGMenu
{
    // THUG's CStruct has a single head pointer. These components are borrowed
    // inputs only: the native script/parser copies them before retaining values.
    // Never pass this arena to a native owning destructor.
    struct Component
    {
        uint8_t type = 0, flags = 0;
        uint16_t size = 0;
        uint32_t name = 0, value = 0;
        Component* next = nullptr;
    };
    struct Structure { Component* head = nullptr; };
    struct Array { uint32_t data = 0, type = 0, count = 0; };
    static_assert(sizeof(Component) == 16 && sizeof(Structure) == 4 && sizeof(Array) == 12);

    constexpr uint32_t Checksum(const char* text)
    {
        uint32_t value = 0xFFFFFFFF;
        for (; *text; ++text)
        {
            auto c = static_cast<uint8_t>(*text);
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            value ^= c;
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ ((value & 1) ? 0xEDB88320 : 0);
        }
        return value;
    }
    static_assert(Checksum("TextElement") == 0x5200DFB6);
    static_assert(Checksum("ContainerElement") == 0x5B9DA842);
    static_assert(Checksum("VMenu") == 0x130EF802);

    class Parameters
    {
    public:
        Structure value;
        Parameters() = default;
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;
        Parameters(Parameters&&) = delete;
        Parameters& operator=(Parameters&&) = delete;

        void Integer(const char* name, uint32_t data) { Add(1, Checksum(name), data); }
        void Name(const char* name, uint32_t data) { Add(13, Checksum(name), data); }
        void Name(const char* name, const char* data) { Name(name, Checksum(data)); }
        void Flag(const char* name) { Add(13, 0, Checksum(name)); }
        void Flag(uint32_t name) { Add(13, 0, name); }
        void Float(const char* name, float data) { uint32_t bits; std::memcpy(&bits, &data, sizeof(bits)); Add(2, Checksum(name), bits); }
        void Text(const char* name, const std::string& data)
        {
            strings.push_back(data);
            Add(3, Checksum(name), Pointer(strings.back().c_str()));
        }
        void Pair(const char* name, float x, float y)
        {
            pairs.push_back({ x, y });
            Add(5, Checksum(name), Pointer(&pairs.back()));
        }
        void Nested(const char* name, Structure* data) { Add(10, Checksum(name), Pointer(data)); }
        void List(const char* name, Array* data) { Add(12, Checksum(name), Pointer(data)); }
        static uint32_t Pointer(const void* data) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data)); }

    private:
        struct XY { float x, y; };
        std::deque<Component> components;
        std::deque<std::string> strings;
        std::deque<XY> pairs;
        Component* tail = nullptr;

        void Add(uint8_t type, uint32_t name, uint32_t data)
        {
            components.push_back({ type, 0, 0, name, data, nullptr });
            auto* item = &components.back();
            if (tail) tail->next = item;
            else value.head = item;
            tail = item;
        }
    };

    // SetText494280 consumes one escape layer, then native font4AE4B0 consumes
    // another. Four input slashes leave a doubled font escape for one glyph.
    // The bridge selects the native font encoding before this conversion.
    inline std::string EscapeNativeText(const std::string& text)
    {
        std::string result;
        result.reserve(text.size());
        for (char c : text)
        {
            if (c == '\\') result.append(3, '\\');
            result += c;
        }
        return result;
    }
}
