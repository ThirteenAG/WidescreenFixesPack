#pragma once
#include "NativeMenuContext.hpp"
#include <cmath>
#include <functional>
#include <string>

namespace HD2Menu
{
    struct TextProbe
    {
        void* text = nullptr;
        void* button = nullptr;
        void* font = nullptr;
        void* properties = nullptr;
        int32_t height = 0;
        float available = 0;
        std::function<bool()> current;

        bool Capture(void* source, const std::function<bool()>& valid)
        {
            button = source;
            current = valid;
            uint16_t screenWidth = 0, screenHeight = 0;
            float width = 0, boxHeight = 0;
            uint8_t flags = 0;
            if (!current() || !Context::Read(button, 108, text)
                || !Context::Read(text, 36, font) || !Context::Read(text, 44, properties)
                || !Context::Read(properties, 48, flags) || (flags & 4)
                || !Context::Read(text, 4, screenWidth) || !Context::Read(text, 6, screenHeight)
                || screenWidth < 320 || screenHeight < 240
                || !Context::Read(text, 24, width) || !Context::Read(text, 28, boxHeight)
                || !std::isfinite(width) || !std::isfinite(boxHeight)
                || width < 96 || width > screenWidth || boxHeight < 12 || boxHeight > screenHeight)
                return false;
            uintptr_t table = 0, lookup = 0;
            if (!Context::Read(font, 0, table) || !Context::Read(reinterpret_cast<void*>(table), 20, lookup)) return false;
            uintptr_t expected = 0;
            if (table == ABI::game + 0x413B68) expected = ABI::game + 0x2402C0;
            else if (table == ABI::game + 0x413B48) expected = ABI::game + 0x2409A0;
            else if (table == ABI::game + 0x413B10) expected = ABI::game + 0x23CB20;
            if (!expected || lookup != expected) return false;
            auto measured = ABI::FontHeight(text);
            if (!current() || measured < 12 || measured > 96 || measured > boxHeight)
                return false;
            height = static_cast<int32_t>(measured);
            available = width - 12;
            return Valid();
        }

        bool Fits(const std::string& value)
        {
            if (!Valid() || !text || !font || value.size() > 8192)
                return false;
            for (unsigned char letter : value)
            {
                if (letter < 32 || letter > 126) return false;
                auto glyph = ABI::Virtual<void*(__thiscall*)(void*, int32_t)>(font, 20)(font, letter);
                // Dynamic system-font lookup can allocate a glyph. Keep the
                // native owner check after that call, not just before it.
                if (!Valid()) return false;
                float width = 0;
                if (!Context::Read(glyph, 16, width) || !std::isfinite(width)
                    || width < 0 || (letter != ' ' && width == 0)) return false;
            }
            ABI::String string;
            if (!string.Assign(value) || !Valid()) return false;
            auto width = ABI::FontWidth(text, string.Get(), height);
            return Valid() && width >= 0 && static_cast<float>(width) <= available;
        }

        bool Valid() const
        {
            void* currentText = nullptr;
            void* currentFont = nullptr;
            void* currentProperties = nullptr;
            return current && current() && Context::Read(button, 108, currentText) && currentText == text
                && Context::Read(text, 36, currentFont) && currentFont == font
                && Context::Read(text, 44, currentProperties) && currentProperties == properties;
        }

        static bool Convert(const std::string& value, std::string& result)
        {
            // The literal native font has a limited glyph range. Fold western
            // accents to readable Latin letters and retain unknown code points
            // as visible U+XXXX tokens.
            if (value.size() > 8192) return false;
            auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                static_cast<int>(value.size()), nullptr, 0);
            if (!count && !value.empty()) return false;
            std::wstring wide(static_cast<size_t>(count), L'\0');
            if (count && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                static_cast<int>(value.size()), wide.data(), count) != count) return false;
            result.clear();
            static constexpr char hex[] = "0123456789ABCDEF";
            for (size_t i = 0; i < wide.size(); ++i)
            {
                uint32_t code = wide[i];
                if (code >= 0xD800 && code <= 0xDBFF)
                {
                    if (++i >= wide.size() || wide[i] < 0xDC00 || wide[i] > 0xDFFF) return false;
                    code = 0x10000 + ((code - 0xD800) << 10) + (wide[i] - 0xDC00);
                }
                if (code == 0xDF) { result += "ss"; continue; }
                if (code == 0xC6) { result += "AE"; continue; }
                if (code == 0xE6) { result += "ae"; continue; }
                if (code == 0x152) { result += "OE"; continue; }
                if (code == 0x153) { result += "oe"; continue; }
                if (code >= 0xC0 && code <= 0x17F)
                {
                    const wchar_t source = static_cast<wchar_t>(code);
                    wchar_t folded[8]{};
                    if (FoldStringW(MAP_COMPOSITE, &source, 1, folded, static_cast<int>(std::size(folded))) > 0
                        && ((folded[0] >= L'A' && folded[0] <= L'Z') || (folded[0] >= L'a' && folded[0] <= L'z')))
                    {
                        result += static_cast<char>(folded[0]);
                        continue;
                    }
                }
                if (code >= 32 && code <= 126) result += static_cast<char>(code);
                else
                {
                    result += " U+";
                    for (int shift = code > 0xFFFF ? 20 : 12; shift >= 0; shift -= 4)
                        result += hex[(code >> shift) & 15];
                    result += ' ';
                }
                if (result.size() > 8192) return false;
            }
            return true;
        }
    };
}
