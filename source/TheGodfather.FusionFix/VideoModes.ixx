module;

#include <stdafx.h>
#include "FloatingPoint.hpp"
#include <array>
#include <charconv>
#include <string_view>
#include <stdexcept>

export module VideoModes;

import WindowModes;
import ResolutionPicker;

namespace GodfatherVideoModes
{
    struct NativeString
    {
        uint32_t allocator{};
        std::array<char, 16> text{};
        uint32_t length{};
        uint32_t capacity{ 15 };

        explicit NativeString(int value)
        {
            const auto result = std::to_chars(text.data(), text.data() + text.size() - 1, value);
            length = static_cast<uint32_t>(result.ptr - text.data());
        }
    };
    static_assert(sizeof(NativeString) == 28);
    SafetyHookInline defaultsHook;

    export std::string_view Text(const void* value)
    {
        if (!value)
            return {};
        const auto words = static_cast<const uint32_t*>(value);
        const auto size = words[5];
        const auto capacity = words[6];
        if (size > 1024 || size > capacity || (!size && !capacity))
            return {};
        const auto text = capacity < 16 ? static_cast<const char*>(value) + 4
            : *reinterpret_cast<const char* const*>(words + 1);
        return text ? std::string_view(text, size) : std::string_view{};
    }
    void* __cdecl Defaults(void* output, const void* section, const void* key, const void* fallback)
    {
        if (Text(section) != "VideoSettings")
            return defaultsHook.ccall<void*>(output, section, key, fallback);
        const auto name = Text(key);
        if (name != "SizeX" && name != "SizeY")
            return defaultsHook.ccall<void*>(output, section, key, fallback);
        Godfather::FloatingPointScope floatingPoint;
        auto [width, height] = GetDesktopRes();
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
        {
            width = 640;
            height = 480;
        }
        const NativeString desktop(name == "SizeX" ? width : height);
        const auto result = defaultsHook.ccall<void*>(output, section, key, &desktop);
        const auto text = Text(result);
        if (!text.empty())
        {
            int requested = 0;
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), requested);
            if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size())
                GodfatherWindowModes::Requested(name == "SizeX", requested);
        }
        return result;
    }
    export void Init()
    {
        GodfatherWindowModes::Init();
        GodfatherResolutions::Init();
        auto read = hook::pattern("51 8B 44 24 14 8B 4C 24 10 8B 54 24 0C 56 8B 74 24 0C 50 51 52 68 01 00 00 80 "
            "56 C7 44 24 18 00 00 00 00 E8 ? ? ? ? 83 C4 14 8B C6 5E 59 C3");
        defaultsHook = safetyhook::create_inline(read.get_first(), Defaults);
        if (!defaultsHook)
            throw std::runtime_error("Desktop-resolution hook unavailable");
    }
}
