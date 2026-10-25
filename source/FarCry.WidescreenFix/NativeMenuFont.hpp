#pragma once
#include "NativeMenuState.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <optional>

namespace FarCryMenu
{
    struct NativeExtent { float x{}, y{}; };
#ifdef _WIN64
    constexpr uintptr_t fontServiceTableRva = 0x3B200, fontTableRva = 0x47D60;
    constexpr size_t fontStyleOffset = 48, fontEffectOffset = 104, fontObjectBytes = 464;
    constexpr size_t fontSystemOffset = 384, textureWidthOffset = 120, textureHeightOffset = 124, passBeginOffset = 48;
#else
    constexpr uintptr_t fontServiceTableRva = 0x233B8, fontTableRva = 0x23628;
    constexpr size_t fontStyleOffset = 44, fontEffectOffset = 80, fontObjectBytes = 304;
    constexpr size_t fontSystemOffset = 264, textureWidthOffset = 92, textureHeightOffset = 96, passBeginOffset = 12;
#endif

    // The native UI localizer interprets @ tokens and converts ordinary byte
    // text through the CRT, not UTF-8. Until a different encoding route is
    // proved, refuse unsupported literals instead of publishing replacements.
    // CryFont's metric and draw both interpret $$ as one literal dollar.
    inline std::optional<std::string> NativeLiteral(const std::string& text)
    {
        if (text.size() > 4096) return {};
        std::string output;
        output.reserve(text.size());
        for (size_t index = 0; index < text.size(); ++index)
        {
            const auto c = static_cast<unsigned char>(text[index]);
            if (c < 32 || c > 126 || c == '@' ||
                (c == '\\' && index + 1 < text.size() && text[index + 1] == 'n')) return {};
            if (c == '$') output += '$';
            output += static_cast<char>(c);
        }
        return output;
    }

    // Call only while the owning font/UI service is protected by the native
    // traversal lease. Font layouts differ between architectures.
    inline std::optional<NativeExtent> MeasureLiteral(void* service, HMODULE module,
        const std::string& text, float size)
    {
        auto literal = NativeLiteral(text);
        if (!literal || !std::isfinite(size) || size < 8.0f || size > 64.0f ||
            !Table(service, module, fontServiceTableRva)) return {};
        auto font = Virtual<void*>(service, 2, "default");
        if (!Table(font, module, fontTableRva) || !Readable(font, fontObjectBytes)) return {};
        auto bytes = static_cast<unsigned char*>(font);
        struct Restore
        {
            unsigned char* object;
            std::array<unsigned char, 24> style{};
            uintptr_t effect{};
            explicit Restore(unsigned char* value) : object(value)
            {
                std::memcpy(style.data(), object + fontStyleOffset, style.size());
                std::memcpy(&effect, object + fontEffectOffset, sizeof(effect));
            }
            ~Restore()
            {
                std::memcpy(object + fontStyleOffset, style.data(), style.size());
                std::memcpy(object + fontEffectOffset, &effect, sizeof(effect));
            }
        } restore(bytes);
        Virtual<void>(font, 5, "default");
        uintptr_t effect{};
        std::memcpy(&effect, bytes + fontEffectOffset, sizeof(effect));
        if (!effect || Field<int>(font, textureWidthOffset) <= 0 || Field<int>(font, textureHeightOffset) <= 0) return {};
        // Both actual metrics traverse forty-byte passes. The vector's own
        // field offsets differ between architectures; verify all records
        // before dispatch rather than treating a non-null effect as ready.
        const auto begin = Field<uintptr_t>(reinterpret_cast<void*>(effect), passBeginOffset);
        const auto end = Field<uintptr_t>(reinterpret_cast<void*>(effect), passBeginOffset + sizeof(void*));
        if (end <= begin || (end - begin) % 40 || end - begin > 64 * 40 ||
            !Readable(reinterpret_cast<void*>(begin), end - begin)) return {};
        for (auto current = begin; current < end; current += 40)
        {
            auto pass = reinterpret_cast<void*>(current);
            const auto x = Field<float>(pass, 16), y = Field<float>(pass, 20);
            if (!std::isfinite(x) || !std::isfinite(y) || x <= 0 || y <= 0 ||
                !std::isfinite(Field<float>(pass, 24)) || !std::isfinite(Field<float>(pass, 28))) return {};
        }
        Virtual<void>(font, 9, true);
        Virtual<void>(font, 15, false);
        Virtual<void>(font, 17, 1.0f);
        const NativeExtent base{size, size};
        Virtual<void>(font, 11, &base);
        std::wstring wide(literal->begin(), literal->end());
        NativeExtent extent{};
        // Actual wide metric: x86 thiscall(self,out,text,multiline)/RET12;
        // x64 RCX=self,RDX=out,R8=text,R9=multiline. Both return out.
        if (Virtual<NativeExtent*>(font, 23, &extent, wide.c_str(), false) != &extent ||
            !std::isfinite(extent.x) || !std::isfinite(extent.y) || extent.x < 0 || extent.y < 0 ||
            (!text.empty() && (extent.x <= 0 || extent.y <= 0))) return {};
        return extent;
    }
}
