module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <cstddef>
#include <cstdint>
#include <Windows.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include "Shared/RWGTA/FixMenu.hpp"
#include <functional>
#include <deque>
#include <memory>
#include "safetyhook.hpp"
#include <set>
#include <intrin.h>

export module Frontend;

import ComVars;


export namespace CthulhuMenu::Native
{
    // Native menu object sizes.
    inline constexpr size_t PageSize = 2240;
    inline constexpr size_t TextRowSize = 360;
    inline constexpr size_t TargetCapacity = 256;

    // Native menu functions, globals and class tables, found by pattern once.
    struct Addresses
    {
        // Hooked functions
        uintptr_t mapper, mapperCleanup, broadcast, message, update, serialize, draw, managerClear, managerDestroy;
        uintptr_t pageDestroy, rowDestroy, font, action;
        // Called functions
        uintptr_t allocateBuffer, freeBuffer, allocateObject, freeObject, constructOptions, addRow, rowAction, label;
        uintptr_t bounds, headingBounds, buildRenderers, clearRows, copyText, layoutText, headerBack, headerStart;
        uintptr_t select, removeHistory, managerGet, publish, deleteOptions, showPage, erasePage;
        // Globals and class tables (history entries are { page, flag } pairs)
        uintptr_t rowTable, pageTable, managerTable, rendererTable, backendTable;
    };

    inline const Addresses& Address()
    {
        static const Addresses addresses = []()
        {
            auto at = [](const char* signature, ptrdiff_t offset = 0)
            {
                return reinterpret_cast<uintptr_t>(hook::get_pattern(signature, offset));
            };
            auto operand = [](const char* signature, ptrdiff_t offset)
            {
                return *hook::get_pattern<uintptr_t>(signature, offset);
            };
            Addresses a{};
            a.mapper = at("83 EC 14 55 8B E9 E8 ? ? ? ? 84 C0");
            a.mapperCleanup = at("83 EC 18 53 55 56 57 8B F9 33 DB 38 9F 1C 02 00 00");
            a.broadcast = at("56 57 8B 7C 24 0C 8A 17 32 C0 81 E2 FF 00 00 00");
            a.message = at("80 79 0C 00 8B 54 24 04 74 07 66 81 3A C8 00");
            a.update = at("51 56 8B F1 83 BE F4 00 00 00 01 75 20");
            a.serialize = at("B8 01 00 00 00 83 EC 08 84 05 ? ? ? ?");
            a.draw = at("51 53 8B D9 8D 43 04 50 8D 4C 24 08");
            a.managerClear = at("56 8B F1 57 8D 4E 58 E8 ? ? ? ?");
            a.managerDestroy = at("51 53 55 56 8B F1 57 C7 06 ? ? ? ?");
            a.pageDestroy = at("53 56 57 8B F9 C7 07 ? ? ? ? E8 ? ? ? ? 8D B7 B8 08 00 00");
            a.rowDestroy = at("56 8B F1 8B 86 48 01 00 00 85 C0 C7 06 ? ? ? ? 74 13 50 E8 ? ? ? ? 83 C4 04 C7 86 48 01 00 00 00 00 00 00 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ?");
            a.font = at("8B 44 24 04 A3 ? ? ? ? C3 CC CC CC CC CC CC A1 ? ? ? ? C3");
            a.action = at("56 8B F1 83 BE 38 01 00 00 00 74 41");
            a.allocateBuffer = at("E8 ? ? ? ? 8B 4C 24 14 8B 40 04 8B 10 51");
            a.freeBuffer = at("E8 ? ? ? ? 8B 40 04 8B 4C 24 04 8B 10 51 8B C8 FF 52 24");
            a.allocateObject = at("E8 ? ? ? ? 8B 54 24 14 8B 48 04");
            a.freeObject = at("E8 ? ? ? ? 8B 40 04 8B 4C 24 04 8B 10 51 8B C8 FF 52 1C");
            a.constructOptions = at("56 8B F1 E8 ? ? ? ? C7 06 ? ? ? ? 8B C6 5E C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 56 8B F1 C7 06");
            a.addRow = at("83 EC 18 8B 44 24 1C 56 57 8B F1 56");
            a.rowAction = at("8B 44 24 0C 53 56 8B 74 24 10 85 F6");
            a.label = at("8B 44 24 04 89 81 3C 01 00 00 C2 04 00");
            a.bounds = at("8B 54 24 04 56 8B 32 8D 81 4C 02 00 00");
            a.headingBounds = at("8B 44 24 04 8B 10 81 C1 3C 02 00 00");
            a.buildRenderers = at("51 56 8B F1 8B 86 5C 02 00 00 85 C0");
            a.clearRows = at("56 8B F1 8B 06 57 FF 50 28 8B 7E 08");
            a.copyText = at("55 8B 6C 24 08 8B C5 56 8B F1 8D 50 01");
            a.layoutText = at("83 EC 20 56 8B F1 8B 86 2C 01 00 00");
            a.headerBack = at("56 8B 74 24 08 83 FE 02 57 8B F9 7C 17 68 D2 02 00 00");
            a.headerStart = at("8B 44 24 0C 8B 54 24 08 50 8B 44 24 08 52 8D 14 40");
            a.select = at("83 EC 18 53 55 56 57 8B F1 8B BE 1C 02 00 00");
            a.removeHistory = at("8B 0D ? ? ? ? 83 F9 01 72 48 33 C0");
            a.managerGet = at("B8 01 00 00 00 84 05 ? ? ? ? 75 1D 09 05 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 B8 ? ? ? ? C3 B8 01 00 00 00");
            a.publish = at("83 EC 1C 55 8B 6C 24 28 56 8B 74 24 28 85 F6 57 8B F9 74 04 85 ED 75 17 68 27 03 00 00");
            a.deleteOptions = at("56 8B F1 C7 06 ? ? ? ? E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 CC CC CC CC CC CC CC CC CC CC CC CC 56 8B F1 8B 86 8C 01 00 00");
            a.showPage = at("8B 44 24 0C 8B 54 24 08 50 8B 44 24 08 52 50");
            // Page list erase: the call in the manager's page clear loop
            a.erasePage = injector::GetBranchDestination(at("8B 4E 28 85 C9 74 06 8B 01 6A 01 FF 10 56 8D 4C 24 10 51 8B CB E8 ? ? ? ? 8B 30 3B 77 40", 21)).as_int();
            MenuManager.SetAddress(operand("B8 ? ? ? ? C3 B8 01 00 00 00 83 EC 08", 1));
            MenuHistoryCount.SetAddress(operand("A1 ? ? ? ? 83 EC 08 83 F8 01 56", 1));
            MenuHistory.SetAddress(operand("39 14 C5 ? ? ? ? 74 08 83 C0 01", 3));
            FontBackend.SetAddress(operand("8B 0D ? ? ? ? D8 0D ? ? ? ?", 2));
            FontMask.SetAddress(operand("85 14 8D ? ? ? ? 74 0E 83 C0 01", 3));
            FontPool.SetAddress(operand("8D 0C C5 ? ? ? ? E8 ? ? ? ? F7 D6", 3));
            a.rowTable = operand("C7 06 ? ? ? ? C7 86 3C 01 00 00 FF FF FF FF", 2);
            a.pageTable = operand("C7 06 ? ? ? ? 8B C6 5E C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 56 8B F1 C7 06", 2);
            a.managerTable = operand("C7 06 ? ? ? ? E8 ? ? ? ? 33 DB 8D 7E 3C", 2);
            a.rendererTable = operand("C7 06 ? ? ? ? C6 86 5C 01 00 00 01", 2);
            a.backendTable = operand("C7 06 ? ? ? ? C7 46 0C 00 00 00 00", 2);
            return a;
        }();
        return addresses;
    }

    inline void* AllocateBuffer(size_t size, int category, const char* file, int line, int flags)
    {
        return reinterpret_cast<void* (__cdecl*)(size_t, int, const char*, int, int)>(Address().allocateBuffer)(
            size, category, file, line, flags);
    }

    inline void FreeBuffer(void* memory)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(Address().freeBuffer)(memory);
    }

    inline void* AllocateObject(size_t size, int category, const char* file, int line, int flags)
    {
        return reinterpret_cast<void* (__cdecl*)(size_t, int, const char*, int, int)>(Address().allocateObject)(
            size, category, file, line, flags);
    }

    inline void FreeObject(void* memory)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(Address().freeObject)(memory);
    }

    inline void* ConstructOptions(void* allocation)
    {
        return reinterpret_cast<void* (__thiscall*)(void*)>(Address().constructOptions)(allocation);
    }

    inline void* AddRow(void* page, const char* type)
    {
        return reinterpret_cast<void* (__thiscall*)(void*, const char*)>(Address().addRow)(page, type);
    }

    inline void Action(void* row, int action, const char* target, int extra)
    {
        // The native warning does not prevent an oversized copy. Callers must
        // validate the complete target length before entering this method.
        reinterpret_cast<void (__thiscall*)(void*, int, const char*, int)>(Address().rowAction)(
            row, action, target, extra);
    }

    inline void Label(void* row, int resource)
    {
        reinterpret_cast<void (__thiscall*)(void*, int)>(Address().label)(row, resource);
    }

    inline void Bounds(void* page, const float* rectangle)
    {
        reinterpret_cast<void (__thiscall*)(void*, const float*)>(Address().bounds)(page, rectangle);
    }

    inline void HeadingBounds(void* page, const float* rectangle)
    {
        reinterpret_cast<void (__thiscall*)(void*, const float*)>(Address().headingBounds)(page, rectangle);
    }

    inline void BuildRenderers(void* page)
    {
        reinterpret_cast<void (__thiscall*)(void*)>(Address().buildRenderers)(page);
    }

    inline void ClearRows(void* page)
    {
        reinterpret_cast<void (__thiscall*)(void*)>(Address().clearRows)(page);
    }

    inline void CopyText(void* renderer, const char* text)
    {
        reinterpret_cast<void (__thiscall*)(void*, const char*)>(Address().copyText)(renderer, text);
    }

    inline void LayoutText(void* renderer)
    {
        reinterpret_cast<void (__thiscall*)(void*)>(Address().layoutText)(renderer);
    }

    inline void HeaderBack(void* page, int configuration, int action, const char* target)
    {
        reinterpret_cast<void (__thiscall*)(void*, int, int, const char*)>(Address().headerBack)(
            page, configuration, action, target);
    }

    inline void HeaderStart(void* page, int configuration, int action, const char* target)
    {
        // thiscall with three stack arguments (ret 12).
        reinterpret_cast<void (__thiscall*)(void*, int, int, const char*)>(Address().headerStart)(
            page, configuration, action, target);
    }

    inline void Select(void* page, int direction, int quiet)
    {
        reinterpret_cast<int (__thiscall*)(void*, int, int)>(Address().select)(page, direction, quiet);
    }

    inline void RemoveHistory(void* page)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(Address().removeHistory)(page);
    }

    inline void* Manager()
    {
        return reinterpret_cast<void* (__cdecl*)()>(Address().managerGet)();
    }

    inline void Publish(void* manager, const char* name, void* page)
    {
        reinterpret_cast<void (__thiscall*)(void*, const char*, void*)>(Address().publish)(manager, name, page);
    }

    inline void ErasePage(void* manager, void* node)
    {
        void* next = nullptr;
        reinterpret_cast<void* (__thiscall*)(void*, void**, void*)>(Address().erasePage)(
            static_cast<uint8_t*>(manager) + 60, &next, node);
    }

    inline void DeleteOptions(void* page)
    {
        reinterpret_cast<void* (__thiscall*)(void*, unsigned)>(Address().deleteOptions)(page, 1);
    }

    inline void ShowPage(void* manager, const char* name, int mode, int flags)
    {
        reinterpret_cast<void (__thiscall*)(void*, const char*, int, int)>(Address().showPage)(
            manager, name, mode, flags);
    }
}

export namespace CthulhuMenu
{
    template<class T> inline T Read(const void* object, size_t offset)
    {
        T value{};
        if (object)
            std::memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(value));
        return value;
    }

    // Use an already generated native Options row's font. Resolving a fresh font
    // through the resource manager would introduce another callback/lifetime path.
    struct FontProbe
    {
        void* renderer = nullptr;
        void* font = nullptr;
        uint32_t pool = 0;
        uint32_t* bits = nullptr;
        uint8_t* glyphs = nullptr;
        uint32_t glyphCount = 0;
        uint32_t characterCount = 0;
        float denominator = 0.0f;
        float horizontal = 0.0f;
        float vertical = 0.0f;

        bool Capture(void* row)
        {
            renderer = Read<void*>(row, 320);
            if (!renderer)
                return false;
            pool = Read<uint32_t>(renderer, 308);
            if (pool >= 350 || !(Read<uint32_t>(FontMask.get_ptr(), (pool >> 5) * 4) &
                (uint32_t{1} << (pool & 31))))
                return false;
            font = Read<void*>(FontPool.get_ptr(), pool * 8);
            if (!font)
                return false;
            bits = Read<uint32_t*>(font, 4);
            denominator = Read<float>(font, 8);
            glyphCount = Read<uint32_t>(font, 28);
            characterCount = Read<uint32_t>(font, 24);
            glyphs = Read<uint8_t*>(font, 32);
            horizontal = Read<float>(renderer, 316);
            vertical = Read<float>(renderer, 320);
            return characterCount >= 127 && characterCount <= 65536 &&
                glyphCount > 1 && glyphCount <= 65536 &&
                bits && glyphs &&
                std::isfinite(denominator) && denominator > 0.0f &&
                std::isfinite(horizontal) && horizontal > 0.0f &&
                std::isfinite(vertical) && vertical > 0.0f;
        }

        bool Current() const
        {
            return pool < 350 && renderer && font &&
                bits && glyphCount <= 65536 && glyphs &&
                (Read<uint32_t>(FontMask.get_ptr(), (pool >> 5) * 4) &
                    (uint32_t{1} << (pool & 31))) &&
                Read<uint32_t>(renderer, 308) == pool &&
                Read<void*>(FontPool.get_ptr(), pool * 8) == font &&
                Read<uint32_t*>(font, 4) == bits && Read<uint8_t*>(font, 32) == glyphs &&
                Read<uint32_t>(font, 28) == glyphCount && Read<float>(font, 8) == denominator &&
                Read<uint32_t>(font, 24) == characterCount &&
                Read<float>(renderer, 316) == horizontal && Read<float>(renderer, 320) == vertical;
        }

        bool Width(const std::string& text, float& width) const
        {
            width = 0.0f;
            if (!Current() || text.size() >= 4096)
                return false;
            for (unsigned char c : text)
            {
                if (c < 32 || c == 127 || c >= characterCount)
                    return false;
                if (c == 32)
                {
                    width += horizontal * (0.315f / 0.42f);
                    continue;
                }
                const auto word = Read<uint32_t>(bits, (c >> 5) * 4);
                if (!(word & (uint32_t{1} << (c & 31))))
                    return false;
                uint32_t rank = 0;
                for (unsigned value = 0; value < c; ++value)
                    rank += (Read<uint32_t>(bits, (value >> 5) * 4) >> (value & 31)) & 1;
                const auto glyph = rank + 1;
                if (glyph >= glyphCount)
                    return false;
                const auto advance = Read<int16_t>(glyphs, glyph * 52 + 20);
                if (advance <= 0)
                    return false;
                width += horizontal * advance / (denominator * 0.42f);
            }
            // Native focus draws a 1.04-scale copy. Fit the larger state too.
            width *= 1.04f;
            return std::isfinite(width) && Current();
        }
    };

    struct FontMetrics
    {
        std::array<float, 256> advances{};
        bool ready = false;

        bool Capture(const FontProbe& probe)
        {
            ready = false;
            advances.fill(-1.0f);
            for (unsigned c = 32; c < advances.size(); ++c)
            {
                float width = 0.0f;
                if (probe.Width(std::string(1, static_cast<char>(c)), width))
                    advances[c] = width;
            }
            ready = probe.Current() && advances[32] > 0.0f;
            return ready;
        }

        bool Width(const std::string& text, float& width) const
        {
            width = 0.0f;
            if (!ready || text.size() >= 4096)
                return false;
            for (unsigned char c : text)
            {
                if (c < 32 || c == 127 || advances[c] <= 0.0f)
                    return false;
                width += advances[c];
            }
            return std::isfinite(width);
        }

        bool Encode(const std::string& text, std::string& output) const
        {
            output.clear();
            if (!ready || text.size() >= 16384) return false;
            const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
            if (!count && !text.empty()) return false;
            std::wstring wide(size_t(count), L'\0');
            if (count) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), count);
            auto available = [&](unsigned char c) { return c >= 32 && c != 127 && advances[c] > 0.0f; };
            auto fallback = [&]() -> char
            {
                for (unsigned char c : { '?', '-', '*', '#' }) if (available(c)) return char(c);
                return 0;
            };
            for (size_t i = 0; i < wide.size(); ++i)
            {
                auto code = wide[i];
                if (code < 32 || code == 8232 || code == 8233) code = L' ';
                char value = 0; BOOL substituted = FALSE;
                // The native glyph parser consumes unsigned bytes, not UTF-8.
                // Preserve available western glyphs, then use visible fallbacks.
                if (code < 0xD800 || code > 0xDFFF)
                    if (WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &code, 1, &value, 1, nullptr, &substituted) != 1 || substituted || !available(uint8_t(value))) value = 0;
                if (!value)
                {
                    wchar_t decomposed[8]{};
                    const auto size = FoldStringW(MAP_COMPOSITE, &code, 1, decomposed, int(std::size(decomposed)));
                    if (size > 0 && decomposed[0] >= 32 && decomposed[0] < 127 && available(uint8_t(decomposed[0]))) value = char(decomposed[0]);
                }
                if (!value)
                {
                    if (code >= 0xD800 && code <= 0xDBFF && i + 1 < wide.size() && wide[i + 1] >= 0xDC00 && wide[i + 1] <= 0xDFFF) ++i;
                    value = fallback();
                }
                if (!value) return false;
                output.push_back(value);
                if (output.size() >= 4096) return false;
            }
            return true;
        }

        bool Fit(std::string text, float maximum, std::string& output) const
        {
            output.clear();
            float width = 0.0f;
            if (!std::isfinite(maximum) || maximum < 0.0f || !Width(text, width))
                return false;
            if (width < maximum || text.empty())
            {
                output = std::move(text);
                return true;
            }
            const std::string dots = "...";
            if (!Width(dots, width))
                return false;
            if (width >= maximum)
                return true;
            while (!text.empty())
            {
                text.pop_back();
                if (!Width(text + dots, width))
                    return false;
                if (width < maximum)
                {
                    output = text + dots;
                    return true;
                }
            }
            output = dots;
            return true;
        }

        bool Split(const std::string& text, float maximum, std::vector<std::string>& output) const
        {
            output.clear();
            if (!ready || !std::isfinite(maximum) || maximum <= 0.0f || text.size() >= 4096)
                return false;
            if (text.empty())
            {
                output.emplace_back();
                return true;
            }
            size_t first = 0;
            while (first < text.size())
            {
                float width = 0.0f;
                size_t last = first;
                size_t boundary = first;
                while (last < text.size())
                {
                    const auto c = static_cast<unsigned char>(text[last]);
                    if (c < 32 || c == 127 || advances[c] <= 0.0f)
                        return false;
                    const auto next = width + advances[c];
                    if (!std::isfinite(next) || next >= maximum)
                        break;
                    width = next;
                    ++last;
                    if (c == ' ' || c == '/' || c == '\\' || c == '-' || c == ':' || c == '.')
                        boundary = last;
                }
                if (last == first)
                    return false;
                if (last < text.size() && boundary > first)
                    last = boundary;
                output.emplace_back(text.substr(first, last - first));
                first = last;
                if (output.size() > 128)
                    return false;
            }
            return true;
        }
    };
}

export namespace CthulhuMenu
{
    // The map's key is a native std::string. Inspect it without invoking a native
    // allocator or inserting a missing key during discovery/collision checks.
    inline bool FindPageNode(void* manager, const std::string& name, void*& result)
    {
        result = nullptr;
        const auto sentinel = Read<void*>(manager, 64);
        if (!sentinel || !Read<uint8_t>(sentinel, 45))
            return false;
        auto node = Read<void*>(sentinel, 4);
        for (size_t visited = 0; visited < 512 && node != sentinel; ++visited)
        {
            if (!node || Read<uint8_t>(node, 45))
                return false;
            const auto length = Read<uint32_t>(node, 32);
            const auto capacity = Read<uint32_t>(node, 36);
            if (length > 255 || capacity < length)
                return false;
            const auto data = capacity < 16 ? static_cast<const char*>(node) + 16 : Read<const char*>(node, 16);
            if (!data || data[length] != '\0')
                return false;
            const auto comparison = name.compare(0, name.size(), data, length);
            if (!comparison)
            {
                result = node;
                return true;
            }
            node = Read<void*>(node, comparison < 0 ? 0 : 8);
        }
        return node == sentinel;
    }

    inline void* FindPage(void* manager, const std::string& name)
    {
        void* node = nullptr;
        return FindPageNode(manager, name, node) && node ? Read<void*>(node, 40) : nullptr;
    }

    struct Context
    {
        void* manager = nullptr;
        void* options = nullptr;
        void** begin = nullptr;
        std::array<void*, 7> rows{};
        int mode = 0;
        int header = 0;

        bool Capture()
        {
            manager = MenuManager.get_ptr();
            if (Read<uintptr_t>(manager, 0) != Native::Address().managerTable)
                return false;
            options = FindPage(manager, "OPTIONS");
            if (!options || Read<uintptr_t>(options, 0) != Native::Address().pageTable)
                return false;
            begin = Read<void**>(options, 8);
            const auto end = Read<void**>(options, 12);
            if (!begin ||
                reinterpret_cast<uintptr_t>(end) != reinterpret_cast<uintptr_t>(begin) + rows.size() * sizeof(void*))
                return false;
            for (size_t i = 0; i < rows.size(); ++i)
            {
                rows[i] = Read<void*>(begin, i * sizeof(void*));
                if (!rows[i] || Read<uintptr_t>(rows[i], 0) != Native::Address().rowTable ||
                    Read<void*>(rows[i], 268) != options)
                    return false;
            }
            const auto literal = Read<const char*>(rows[4], 328);
            if (Read<int>(rows[4], 264) || Read<uint8_t>(rows[4], 273) ||
                (literal && *literal))
                return false;
            mode = Read<int>(manager, 244);
            header = Read<int>(options, 2232);
            return (mode == 1 || mode == 2) && header >= 0 && header < 2 &&
                Read<uint8_t>(manager, 248) == header && Alive();
        }

        bool Alive() const
        {
            if (Read<uintptr_t>(manager, 0) != Native::Address().managerTable ||
                FindPage(manager, "OPTIONS") != options || !options ||
                Read<uintptr_t>(options, 0) != Native::Address().pageTable || Read<void**>(options, 8) != begin ||
                reinterpret_cast<uintptr_t>(Read<void**>(options, 12)) !=
                    reinterpret_cast<uintptr_t>(begin) + rows.size() * sizeof(void*) ||
                Read<int>(manager, 244) != mode || Read<int>(options, 2232) != header ||
                Read<uint8_t>(manager, 248) != header)
                return false;
            for (size_t i = 0; i < rows.size(); ++i)
                if (Read<void*>(begin, i * sizeof(void*)) != rows[i] || !rows[i] ||
                    Read<uintptr_t>(rows[i], 0) != Native::Address().rowTable || Read<void*>(rows[i], 268) != options)
                    return false;
            return true;
        }

        bool Current(void* page) const
        {
            if (!Alive() || Read<uint8_t>(manager, 12) || !Read<uint8_t>(manager, 33) ||
                Read<void*>(manager, 44) || Read<void*>(manager, 52) || Read<void*>(manager, 48) != page)
                return false;
            const auto count = MenuHistoryCount.get();
            return count > 0 && count <= 10 &&
                Read<void*>(MenuHistory.get_ptr(), (count - 1) * 8) == page &&
                !Read<uint8_t>(MenuHistory.get_ptr() + 4, (count - 1) * 8);
        }

        bool SelectedEntry() const
        {
            return Current(options) && Read<int>(options, 540) == 4;
        }

        bool Pause() const
        {
            // The title menu uses mode 1, the in-game pause menu mode 2.
            return mode == 2;
        }
    };
}

export namespace CthulhuMenu
{
    inline std::string notice; // an update notice, shown in the help line while the first row is selected

    enum class Kind { Link, Row, Previous, Next, Back };

    struct Item
    {
        Kind kind = Kind::Back;
        std::string text;
        RWGTAMenu::Row row{};
        size_t target = 0;
        bool enabled = true;
        std::string help;
    };

    struct View
    {
        static constexpr size_t ContentRows = 3;
        int page = -1;
        size_t physical = 0;
        std::string title, help, restart, update;
        std::vector<Item> items;

        bool Footer(size_t selected, const FontMetrics& font, float width, std::string& output) const
        {
            const auto text = !restart.empty() ? restart : selected == 0 && !update.empty() ? update :
                selected < items.size() ? items[selected].help : std::string{};
            return font.Fit(text, width, output);
        }

        bool Prepare(RWGTAMenu::Model& model, const FontMetrics& font, bool pause,
                     const std::function<bool()>& current, float width)
        {
            if (!current() || !model.IsOpen() || model.Pages().empty())
                return false;
            bool encodingValid = true;
            auto native = [&](const std::string& text)
            {
                std::string encoded;
                encodingValid &= font.Encode(text, encoded);
                return encoded;
            };
            std::vector<Item> content;
            auto append = [&](Item item, const std::string& text)
            {
                if (!font.Fit(text, width, item.text) || !current() || content.size() >= 2048)
                    return false;
                content.push_back(std::move(item));
                return true;
            };
            if (page < 0)
            {
                title = native(model.Text("menu.plugins"));
                const auto pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                    if (!append(Item{Kind::Link, {}, {}, i, true}, native(pages[i].title)))
                        return false;
            }
            else
            {
                if (size_t(page) >= model.Pages().size())
                    return false;
                title = native(model.Pages()[size_t(page)].title);
                auto visible = model.VisibleRows(size_t(page));
                if (!current())
                    return false;
                std::vector<RWGTAMenu::Row> copied;
                for (auto row : visible)
                    copied.push_back(*row);
                for (auto row : copied)
                {
                    auto label = native(model.Label(row));
                    if (!current())
                        return false;
                    // Presets belong to their plugin; the page already names it.
                    if (!row.plugin && (row.kind == RWGTAMenu::RowKind::Preset ||
                        row.kind == RWGTAMenu::RowKind::ApplyPreset))
                        continue;
                    auto value = native(model.Value(row));
                    if (!current())
                        return false;
                    auto enabled = model.Enabled(row, pause);
                    if (!current())
                        return false;
                    std::string suffix = value.empty() ? std::string{} : ": " + value;
                    if (!enabled)
                        suffix += " " + native(model.Text("menu.unavailable"));
                    std::string fitted;
                    float suffixWidth = 0.0f;
                    if (!font.Fit(suffix, width, suffix) || !font.Width(suffix, suffixWidth) ||
                        !font.Fit(label, std::max(0.0f, width - suffixWidth), fitted))
                        return false;
                    label = fitted + suffix;
                    auto description = native(model.Help(row));
                    if (!current()) return false;
                    if (!append(Item{Kind::Row, {}, row, 0, enabled, std::move(description)}, label))
                        return false;
                }
            }
            if (!current())
                return false;
            restart = model.RestartRequired() ? native(model.Text("menu.restartwarning")) : std::string{};
            if (update.clear(); !notice.empty() && !font.Encode(notice, update))
                update.clear();
            if (!font.Fit(title, width, title))
                return false;
            const auto count = std::max(size_t(1), (content.size() + ContentRows - 1) / ContentRows);
            physical = std::min(physical, count - 1);
            items.clear();
            for (size_t i = physical * ContentRows; i < content.size() && i < (physical + 1) * ContentRows; ++i)
                items.push_back(std::move(content[i]));
            if (physical)
                items.push_back(Item{Kind::Previous, native(model.Text("menu.previous")), {}, 0, true});
            if (physical + 1 < count)
                items.push_back(Item{Kind::Next, native(model.Text("menu.next")), {}, 0, true});
            items.push_back(Item{Kind::Back, native(model.Text("menu.back")), {}, 0, true});
            for (const auto& item : items)
            {
                float measured = 0.0f;
                if (!font.Width(item.text, measured) || measured >= width)
                    return false;
            }
            return encodingValid && current() && !items.empty() && items.size() <= 6;
        }
    };
}

export namespace CthulhuMenu
{
    template<class T> inline void Write(void* object, size_t offset, const T& value)
    {
        *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset) = value;
    }

    inline bool ReplaceLiteral(void* row, const std::string& text,
                               const std::function<bool()>& current, char*& previous)
    {
        previous = nullptr;
        if (!current() || !row ||
            Read<uintptr_t>(row, 0) != Native::Address().rowTable || text.size() >= 4096)
            return false;
        for (unsigned char c : text)
            if (c < 32 || c == 127)
                return false;
        auto buffer = static_cast<char*>(Native::AllocateBuffer(
            text.size() + 1, 7, ".\\CoCControlTextOption.cpp", 83, 1));
        if (!buffer)
            return false;
        std::memcpy(buffer, text.c_str(), text.size() + 1);
        if (!current() || !row || Read<uintptr_t>(row, 0) != Native::Address().rowTable)
        {
            Native::FreeBuffer(buffer);
            return false;
        }
        previous = Read<char*>(row, 328);
        Write<char*>(row, 328, buffer);
        // The row owns this checked allocation after publication. Its exact native
        // destructor frees it; the caller retains/restores the previous allocation.
        return true;
    }

    struct Entry
    {
        Entry() = default;
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
        Entry(Entry&&) = delete;
        Entry& operator=(Entry&&) = delete;
        static constexpr int ActionID = 0x7F00;
        void* row = nullptr;
        char* original = nullptr;
        char* caption = nullptr;
        int action = 0;
        int extra = 0;
        uint32_t flags = 0;
        uint8_t selectable = 0;
        bool installed = false;

        bool SameSource(const Context& context) const
        {
            if (!row || row != context.rows[4] ||
                Read<uintptr_t>(row, 0) != Native::Address().rowTable || Read<void*>(row, 268) != context.options ||
                FindPage(context.manager, "OPTIONS") != context.options || !context.options)
                return false;
            auto children = Read<void**>(context.options, 8);
            return children == context.begin &&
                reinterpret_cast<uintptr_t>(Read<void**>(context.options, 12)) ==
                    reinterpret_cast<uintptr_t>(children) + 7 * sizeof(void*) &&
                Read<void*>(children, 4 * sizeof(void*)) == row;
        }

        bool Matches(const Context& context) const
        {
            return installed && SameSource(context) && Read<char*>(row, 328) == caption &&
                Read<int>(row, 264) == ActionID && Read<uint8_t>(row, 273) == 1;
        }

        bool CopyCaption(const std::string& text, const std::function<bool()>& current)
        {
            if (!current())
                return false;
            auto renderer = Read<void*>(row, 320);
            if (!renderer || Read<uintptr_t>(renderer, 0) != Native::Address().rendererTable)
                return false;
            Native::CopyText(renderer, text.c_str());
            if (!current())
                return false;
            auto stored = Read<const char*>(renderer, 300);
            return stored && !std::memcmp(stored, text.c_str(), text.size() + 1);
        }

        bool Build(const Context& context, const std::string& title, const FontMetrics& font,
                   const std::function<bool()>& current)
        {
            if (row || !current())
                return false;
            row = context.rows[4];
            if (!SameSource(context) || Read<int>(row, 264) || Read<uint8_t>(row, 273) ||
                Read<char>(row, 8) || Read<void*>(row, 312))
                return false;
            float measured = 0.0f;
            const auto width = Read<float>(row, 288) - Read<float>(row, 280);
            if (!font.Width(title, measured) || !std::isfinite(width) || measured >= width)
                return false;
            action = Read<int>(row, 264);
            extra = Read<int>(row, 4);
            flags = Read<uint32_t>(row, 308);
            selectable = Read<uint8_t>(row, 273);
            if (!ReplaceLiteral(row, title, current, original))
                return false;
            caption = Read<char*>(row, 328);
            if (!CopyCaption(title, current) || !current())
                return false;
            // The empty stock spacer can have a different style from row0.
            // Verify the actual copied entry's native layout/font, not only the
            // source scalar estimate used to bound construction before copying.
            auto renderer = Read<void*>(row, 320);
            auto backend = FontBackend.get();
            if (!backend || Read<uintptr_t>(backend, 0) != Native::Address().backendTable)
                return false;
            Native::LayoutText(renderer);
            FontProbe probe;
            if (!current() || !SameSource(context) || !probe.Capture(row) ||
                !probe.Width(title, measured) || measured >= width)
                return false;
            Native::Action(row, ActionID, "", 0);
            if (!current())
                return false;
            Write<uint8_t>(row, 273, 1);
            installed = true;
            return Matches(context);
        }

        bool Restore(const Context& context, const std::function<bool()>& authorized)
        {
            if (!row)
                return true;
            if (!caption)
            {
                row = nullptr;
                return true;
            }
            if (!authorized() || !SameSource(context) || Read<char*>(row, 328) != caption)
                return false;
            // Restore the copied renderer caption before changing row semantics.
            // On allocation/copy failure keep the exact installed transaction for
            // a later safe retry instead of exposing stock actions with our label.
            auto renderer = Read<void*>(row, 320);
            if (renderer && ((original && *original) ||
                !CopyCaption("", authorized)))
                return false;
            if (!authorized() || !SameSource(context) || Read<char*>(row, 328) != caption)
                return false;
            auto retained = caption;
            Write<char*>(row, 328, original);
            caption = nullptr;
            original = nullptr;
            Write<int>(row, 264, action);
            Write<int>(row, 4, extra);
            Write<uint32_t>(row, 308, flags);
            Write<uint8_t>(row, 273, selectable);
            installed = false;
            if (Read<void*>(context.manager, 48) == context.options && Read<int>(context.options, 540) == 4 &&
                !Read<void*>(context.manager, 44) && !Read<void*>(context.manager, 52))
                Native::Select(context.options, 1, 1);
            Native::FreeBuffer(retained);
            row = nullptr;
            return true;
        }

        void Forgotten()
        {
            // Native row retirement frees the active caption. The previous
            // allocation remains retained until an authorized outer cleanup.
            row = nullptr;
            caption = nullptr;
            installed = false;
        }

        void ReleaseOriginal(const std::function<bool()>& authorized)
        {
            if (!row && original && authorized())
            {
                auto retained = original;
                original = nullptr;
                Native::FreeBuffer(retained);
            }
        }
    };

    struct Page
    {
        Page() = default;
        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;
        Page(Page&&) = delete;
        Page& operator=(Page&&) = delete;
        static constexpr const char* Name = "WFPADV_CTHULHU";
        void* object = nullptr;
        void* node = nullptr;
        bool registered = false;
        bool constructed = false;
        bool publicationAttempted = false;

        bool Create(const Context& context, const std::function<bool()>& current)
        {
            if (object || !current())
                return false;
            void* collision = nullptr;
            if (!FindPageNode(context.manager, Name, collision) || collision)
                return false;
            object = Native::AllocateObject(Native::PageSize, 7, ".\\CoCMenuBuilder.cpp", 291, 1);
            if (!object)
                return false;
            if (!current())
                return false;
            Native::ConstructOptions(object);
            constructed = true;
            if (!current() || !object ||
                Read<uintptr_t>(object, 0) != Native::Address().pageTable)
                return false;
            // Track the allocated object before any map publication. A failed or
            // replaced source context must not strand an unregistered native page.
            if (!FindPageNode(context.manager, Name, collision) || collision || !current())
                return false;
            publicationAttempted = true;
            Native::Publish(context.manager, Name, object);
            if (!current())
                return false;
            if (!FindPageNode(context.manager, Name, node) || !node || Read<void*>(node, 40) != object)
                return false;
            registered = true;
            return current();
        }

        bool Remove(void* manager, const std::function<bool()>& authorized)
        {
            if (!object)
                return true;
            // The host authorizes this only after complete input/update/render
            // unwind, with the exact native manager retained and teardown excluded.
            if (!authorized())
                return false;
            if (publicationAttempted)
            {
                void* current = nullptr;
                if (!manager || Read<uintptr_t>(manager, 0) != Native::Address().managerTable ||
                    Read<void*>(manager, 44) == object || Read<void*>(manager, 48) == object ||
                    Read<void*>(manager, 52) == object || !FindPageNode(manager, Name, current))
                    return false;
                const auto history = MenuHistoryCount.get();
                if (history > 10)
                    return false;
                for (uint32_t i = 0; i < history; ++i)
                    if (Read<void*>(MenuHistory.get_ptr(), i * 8) == object)
                        return false;
                if (current && (Read<void*>(current, 40) != object || (node && current != node)))
                    return false;
                if (current)
                    Native::ErasePage(manager, current);
                if (!object)
                    return true;
                if (!authorized())
                    return false;
                node = nullptr;
                registered = false;
                publicationAttempted = false;
            }
            if (constructed && (!object || Read<uintptr_t>(object, 0) != Native::Address().pageTable))
                return false;
            auto allocation = object;
            object = nullptr;
            if (constructed)
                Native::DeleteOptions(allocation);
            else
                Native::FreeObject(allocation);
            constructed = false;
            publicationAttempted = false;
            return true;
        }

        bool Alive(const Context& context) const
        {
            void* current = nullptr;
            return object && registered && context.Alive() &&
                FindPageNode(context.manager, Name, current) && current == node &&
                Read<void*>(current, 40) == object && object &&
                Read<uintptr_t>(object, 0) == Native::Address().pageTable;
        }

        void Forgotten()
        {
            // Called before the native owner destroys/clears its registered page.
            // Never delete the same engine-owned allocation from the C++ host.
            object = nullptr;
            node = nullptr;
            registered = false;
            constructed = false;
            publicationAttempted = false;
        }
    };
}

export namespace CthulhuMenu
{
    // The native page owns every row and its renderer/literal allocations. This
    // record is only an identity snapshot, cleared before native row retirement.
    struct Surface
    {
        static constexpr size_t Count = 8;
        static constexpr int ActionID = 0x7F01;
        std::array<void*, Count> rows{};
        void** begin = nullptr;
        void* page = nullptr;
        char* helpBuffer = nullptr;

        void Forgotten()
        {
            rows.fill(nullptr);
            begin = nullptr;
            page = nullptr;
            helpBuffer = nullptr;
        }

        bool Alive() const
        {
            if (!page || Read<uintptr_t>(page, 0) != Native::Address().pageTable ||
                Read<void**>(page, 8) != begin || !begin ||
                reinterpret_cast<uintptr_t>(Read<void**>(page, 12)) !=
                    reinterpret_cast<uintptr_t>(begin) + Count * sizeof(void*))
                return false;
            for (size_t i = 0; i < Count; ++i)
                if (!rows[i] || Read<void*>(begin, i * sizeof(void*)) != rows[i] ||
                    !rows[i] || Read<uintptr_t>(rows[i], 0) != Native::Address().rowTable ||
                    Read<void*>(rows[i], 268) != page)
                    return false;
            return true;
        }

        bool Selected(size_t item) const
        {
            return item < Count - 1 && Alive() && Read<int>(page, 540) == int(item + 1) &&
                Read<uint8_t>(rows[item + 1], 273) && Read<int>(rows[item + 1], 264) == ActionID;
        }

        static bool PlanRows(const std::array<std::array<float, 4>, 7>& source,
                             std::array<std::array<float, 4>, Count>& rectangles)
        {
            for (size_t i = 0; i < source.size(); ++i)
            {
                const auto& rect = source[i];
                for (auto value : rect) if (!std::isfinite(value) || value < 0 || value > 1) return false;
                if (rect[2] - rect[0] <= 0.05f || rect[3] - rect[1] < 0.012f || (i && rect[1] < source[i - 1][3])) return false;
                rectangles[i] = rect;
            }
            const auto& last = source.back();
            rectangles.back() = { last[0], last[3] + 0.005f, last[2], last[3] + 0.005f + last[3] - last[1] };
            return rectangles.back()[3] <= 0.90f;
        }

        bool Build(void* destination, const Context& source, const View& view, float fontHeight,
                   const std::function<bool()>& current)
        {
            if (page || view.items.empty() || view.items.size() >= Count || !current() ||
                !destination || Read<uintptr_t>(destination, 0) != Native::Address().pageTable ||
                Read<void**>(destination, 8) != Read<void**>(destination, 12))
                return false;
            // Keep all seven stock positions, with a disabled help row below.
            // The private page alone grows; stock Options geometry is borrowed.
            // Copy only proved scalar geometry/style, never native owner pointers.
            std::array<std::array<float, 4>, 7> borrowed{};
            std::array<std::array<float, 4>, Count> rectangles{};
            for (size_t i = 0; i < source.rows.size(); ++i)
            {
                if (!source.Alive()) return false;
                for (size_t j = 0; j < 4; ++j) borrowed[i][j] = Read<float>(source.rows[i], 280 + j * 4);
            }
            // The disabled help row never receives the native focus scale.
            // The active page's renderer was checked before ClearRows. Stock
            // Options renderers are released while the private page is shown.
            if (!PlanRows(borrowed, rectangles) || !std::isfinite(fontHeight) || fontHeight <= 0.0f ||
                fontHeight > rectangles.back()[3] - rectangles.back()[1]) return false;
            page = destination;
            // The constructor does not initialize these rectangles. Native
            // AddRow reads page588/596/currentY before assigning row geometry,
            // so establish its normal setter preconditions before any factory.
            std::array<float, 4> bounds{}, heading{};
            for (size_t j = 0; j < 4; ++j)
            {
                bounds[j] = Read<float>(source.options, 588 + j * 4);
                heading[j] = Read<float>(source.options, 572 + j * 4);
                if (!std::isfinite(bounds[j]) || !std::isfinite(heading[j]) ||
                    bounds[j] < 0.0f || bounds[j] > 1.0f || heading[j] < 0.0f || heading[j] > 1.0f)
                    return false;
            }
            if (bounds[2] <= bounds[0] || bounds[3] <= bounds[1] ||
                heading[2] <= heading[0] || heading[3] <= heading[1] || !current())
                return false;
            bounds[3] = std::max(bounds[3], rectangles.back()[3]);
            Native::Bounds(page, bounds.data());
            if (!current())
                return false;
            Native::HeadingBounds(page, heading.data());
            if (!current())
                return false;
            const auto style = Read<uint8_t>(source.options, 620);
            if (style > 1)
                return false;
            Write<uint8_t>(page, 620, style);
            for (int configuration = 0; configuration < 2; ++configuration)
            {
                Native::HeaderBack(page, configuration, 1, "OPTIONS");
                if (!current())
                    return false;
                // Stock Start goes through the current selected-row Accept.
                Native::HeaderStart(page, configuration, 23, "");
                if (!current())
                    return false;
            }
            // Constructor leaves native localized heading disabled (-1). Own
            // heading row preserves the complete shared page title instead.
            for (size_t i = 0; i < Count; ++i)
            {
                if (!current())
                    return false;
                rows[i] = Native::AddRow(page, "TEXTOPTION");
                if (!current() || !rows[i] ||
                    Read<uintptr_t>(rows[i], 0) != Native::Address().rowTable || Read<void*>(rows[i], 268) != page)
                    return false;
                auto row = rows[i];
                for (size_t j = 0; j < 4; ++j)
                    Write<float>(row, 280 + j * 4, rectangles[i][j]);
                // All private labels use the normal source font/scales and color.
                for (size_t offset : {size_t(332), size_t(336), size_t(344), size_t(348), size_t(352), size_t(356)})
                    Write<uint32_t>(row, offset, Read<uint32_t>(source.rows[0], offset));
                for (size_t offset : {size_t(276), size_t(277), size_t(340), size_t(341)})
                    Write<uint8_t>(row, offset, Read<uint8_t>(source.rows[0], offset));
                const auto footer = i == Count - 1;
                const auto active = i > 0 && !footer && i - 1 < view.items.size();
                const auto text = i == 0 ? view.title : footer ? view.help : active ? view.items[i - 1].text : std::string{};
                Native::Label(row, -1);
                if (!current())
                    return false;
                char* previous = nullptr;
                if (footer)
                {
                    if (text.size() >= 4096 || Read<void*>(row, 328)) return false;
                    helpBuffer = static_cast<char*>(Native::AllocateBuffer(4096, 7, ".\\CoCControlTextOption.cpp", 83, 1));
                    if (!helpBuffer || !current())
                    {
                        if (helpBuffer) Native::FreeBuffer(helpBuffer);
                        helpBuffer = nullptr; return false;
                    }
                    std::memcpy(helpBuffer, text.c_str(), text.size() + 1);
                    Write<char*>(row, 328, helpBuffer);
                }
                else if (!ReplaceLiteral(row, text, current, previous)) return false;
                // Newly constructed TEXTOPTION must not carry an old allocation.
                // If it does, retain the page for checked native deletion.
                if (previous)
                {
                    Native::FreeBuffer(previous);
                    return false;
                }
                Native::Action(row, active ? ActionID : 0, "", 0);
                if (!current())
                    return false;
                Write<uint8_t>(row, 273, active ? 1 : 0);
                // Availability affects activation, not navigation: disabled rows
                // remain readable and reachable, and an enabled Back always exists.
                const auto editable = active && view.items[i - 1].kind == Kind::Row &&
                    (view.items[i - 1].row.kind == RWGTAMenu::RowKind::Setting ||
                     view.items[i - 1].row.kind == RWGTAMenu::RowKind::Preset);
                Write<uint32_t>(row, 308, active ? (editable ? 0x1Cu : 0x10u) : 0u);
            }
            begin = Read<void**>(page, 8);
            return current() && Alive();
        }

        bool Help(const std::string& text, const std::function<bool()>& current) const
        {
            if (!current() || !Alive() || text.size() >= 4096 || !helpBuffer || Read<char*>(rows.back(), 328) != helpBuffer) return false;
            auto renderer = Read<void*>(rows.back(), 320);
            if (!renderer || Read<uintptr_t>(renderer, 0) != Native::Address().rendererTable) return false;
            std::memcpy(helpBuffer, text.c_str(), text.size() + 1);
            Native::CopyText(renderer, helpBuffer);
            if (!current() || !Alive()) return false;
            auto stored = Read<const char*>(renderer, 300);
            if (!stored || std::memcmp(stored, text.c_str(), text.size() + 1)) return false;
            if (text.empty()) return true;
            Native::LayoutText(renderer);
            FontProbe probe; float width = 0.0f;
            return current() && Alive() && probe.Capture(rows.back()) && probe.Width(text, width) &&
                width < Read<float>(rows.back(), 288) - Read<float>(rows.back(), 280);
        }

        bool Rendered(const View& view, const std::function<bool()>& current) const
        {
            if (!Alive())
                return false;
            for (size_t i = 0; i < view.items.size() + 1; ++i)
            {
                if (!current())
                    return false;
                auto renderer = Read<void*>(rows[i], 320);
                auto backend = FontBackend.get();
                if (!renderer || Read<uintptr_t>(renderer, 0) != Native::Address().rendererTable ||
                    !backend || Read<uintptr_t>(backend, 0) != Native::Address().backendTable)
                    return false;
                // Native BuildRenderers copies text with layout=false. Generate
                // the owning renderer's pool entry before measuring it; a fresh
                // empty pool is not evidence of unsupported font readiness.
                Native::LayoutText(renderer);
                if (!current() || !Alive())
                    return false;
                FontProbe probe;
                const auto& text = i ? view.items[i - 1].text : view.title;
                float width = 0.0f;
                if (!probe.Capture(rows[i]) || !probe.Width(text, width) ||
                    width >= Read<float>(rows[i], 288) - Read<float>(rows[i], 280))
                    return false;
            }
            return Help(view.help, current);
        }
    };
}

export namespace CthulhuMenu
{
    struct Host : std::enable_shared_from_this<Host>
    {
        Context context;
        Entry entry;
        Page page;
        Surface surface;
        View view;
        FontMetrics font;
        HMODULE module = nullptr;
        void* escape = nullptr;
        const char* preparationFailure = nullptr;
        RWGTAMenu::Model model;
        uint64_t generation = 1, serial = 1;
        bool invalid = false, open = false, ready = false, busy = false, opening = false;
        bool expectedRows = false, restore = false, retiring = false;
        size_t selected = 0;
        std::array<std::array<uint32_t, 4>, 7> geometry{};
        std::array<uint32_t, 6> style{};

        struct Foreign
        {
            Host& host;
            bool previous;
            explicit Foreign(Host& owner) : host(owner), previous(owner.busy) { host.busy = true; }
            ~Foreign() { host.busy = previous; }
        };
        struct Flag
        {
            bool& flag;
            bool previous;
            explicit Flag(bool& value) : flag(value), previous(value) { flag = true; }
            ~Flag() { flag = previous; }
        };
        struct Pending
        {
            uint64_t generation, serial;
            int logical;
            size_t physical, selected;
            void* row;
            Item item;
            int direction;
            bool back;
        };
        std::deque<Pending> pending;

        bool CaptureStyle()
        {
            if (!context.Alive())
                return false;
            for (size_t i = 0; i < geometry.size(); ++i)
                for (size_t j = 0; j < 4; ++j)
                    geometry[i][j] = Read<uint32_t>(context.rows[i], 280 + j * 4);
            for (size_t i = 0; i < style.size(); ++i)
                style[i] = Read<uint32_t>(context.rows[0], std::array<size_t, 6>{332, 336, 344, 348, 352, 356}[i]);
            return true;
        }

        bool Alive() const
        {
            if (invalid || !context.Alive())
                return false;
            for (size_t i = 0; i < geometry.size(); ++i)
                for (size_t j = 0; j < 4; ++j)
                    if (geometry[i][j] != Read<uint32_t>(context.rows[i], 280 + j * 4))
                        return false;
            for (size_t i = 0; i < style.size(); ++i)
                if (style[i] != Read<uint32_t>(context.rows[0], std::array<size_t, 6>{332, 336, 344, 348, 352, 356}[i]))
                    return false;
            return true;
        }

        bool Current(bool privatePage) const
        {
            return Alive() && (privatePage ? page.Alive(context) && context.Current(page.object) : context.Current(context.options));
        }

        std::function<bool()> Snapshot(bool privatePage, bool selection = false)
        {
            auto lease = shared_from_this();
            const auto version = generation, rows = serial;
            const auto selectedRow = privatePage ? Read<int>(page.object, 540) : Read<int>(context.options, 540);
            return [lease, version, rows, privatePage, selection, selectedRow]
            {
                return lease->generation == version && lease->serial == rows && lease->Current(privatePage) &&
                    (!selection || Read<int>(privatePage ? lease->page.object : lease->context.options, 540) == selectedRow);
            };
        }

        float Width() const
        {
            float width = 1.0f;
            for (size_t i = 0; i < context.rows.size(); ++i)
                width = std::min(width, Read<float>(context.rows[i], 288) - Read<float>(context.rows[i], 280));
            return width - 0.01f;
        }

        void Cancel() noexcept
        {
            ++generation;
            ready = false;
            restore = true;
            pending.clear();
            // Shared discovery/read/preset snapshots observe Close immediately.
            // Native repaint/pop/deletion always waits for complete outer unwind.
            model.Close();
        }

        void Retire() noexcept
        {
            if (invalid)
                return;
            invalid = retiring = true;
            Cancel();
        }

        bool Prepare(const std::string& title)
        {
            auto lease = shared_from_this();
            preparationFailure = "CoC native menu: Options context changed before preparation";
            if (!context.Capture())
                return false;
            preparationFailure = "CoC native menu: Options style or active-page authority unavailable";
            if (!CaptureStyle() || !Current(false))
                return false;
            escape = FindPage(context.manager, context.Pause() ? "INGAMEPAUSE" : "MAIN");
            preparationFailure = "CoC native menu: return page identity unavailable";
            if (!escape)
                return false;
            FontProbe probe;
            preparationFailure = "CoC native menu: Options source font unavailable";
            if (!probe.Capture(context.rows[0]) || !font.Capture(probe) || !probe.Current())
                return false;
            auto current = Snapshot(false);
            Foreign foreign(*this);
            preparationFailure = "CoC native menu: spare-row caption or font layout refused";
            if (!current() || !probe.Current() || !entry.Build(context, title, font, current))
                return false;
            preparationFailure = "CoC native menu: Options entry changed during publication";
            if (!current() || !entry.Matches(context))
                return false;
            preparationFailure = nullptr;
            return true;
        }

        bool Populate(View candidate, size_t focus, bool privatePage)
        {
            auto lease = shared_from_this();
            if (!open || !Current(privatePage))
                return false;
            ready = false;
            auto current = Snapshot(privatePage, true);
            FontProbe probe;
            auto fontRow = privatePage && surface.Alive() ? surface.rows[1] : context.rows[0];
            if (!probe.Capture(fontRow) || !font.Capture(probe) || !probe.Current())
                return false;
            {
                Foreign foreign(*this);
                if (!candidate.Prepare(model, font, context.Pause(), current, Width()) ||
                    !current() || !probe.Current() || !candidate.Footer(focus, font, Width(), candidate.help))
                    return false;
            }
            // Stop old-row replay before native invalidation. Failure closes the
            // entire private view; old widgets are never paired with new mappings.
            ++serial;
            pending.clear();
            auto continuing = Snapshot(privatePage);
            {
                Flag constructing(expectedRows);
                Foreign foreign(*this);
                surface.Forgotten();
                Native::ClearRows(page.object);
                if (!continuing())
                    return false;
                if (!surface.Build(page.object, context, candidate, probe.vertical, continuing) || !continuing())
                    return false;
                if (privatePage)
                {
                    Native::BuildRenderers(page.object);
                    if (!continuing() || !surface.Rendered(candidate, continuing))
                        return false;
                }
            }
            view = std::move(candidate);
            selected = std::min(focus, view.items.size() - 1);
            // Do not raw-write selection: native focus/secondary renderer state
            // is coupled to it. SelectDown finds the first enabled row (heading0
            // is disabled), then bounded native steps reach the retained index.
            Write<int>(page.object, 540, -1);
            for (size_t i = 0; i <= selected; ++i)
            {
                Native::Select(page.object, 1, 1);
                if (!continuing())
                    return false;
            }
            ready = privatePage;
            return continuing() && surface.Selected(selected);
        }

        bool Open(const std::string& language)
        {
            auto lease = shared_from_this();
            if (open || !Current(false) || !entry.Matches(context) || !context.SelectedEntry())
                return false;
            ++generation;
            open = true;
            ready = false;
            restore = false;
            Flag construction(opening);
            auto current = Snapshot(false, true);
            {
                Foreign foreign(*this);
                model.OpenPlugins(module, language, 4096);
            }
            if (!current() || !context.SelectedEntry() || !model.IsOpen() || model.Pages().empty())
            {
                Cancel();
                return false;
            }
            if ((!page.object && !page.Create(context, current)) || !current() || !page.Alive(context) ||
                !Populate(View{}, 0, false) || !Current(false) || !context.SelectedEntry())
            {
                Cancel();
                return false;
            }
            {
                Foreign foreign(*this);
                Native::ShowPage(context.manager, Page::Name, 0, 1);
            }
            auto committed = Snapshot(true);
            if (!Current(true) || !surface.Rendered(view, committed) || !surface.Selected(0))
            {
                Cancel();
                return false;
            }
            selected = 0;
            ready = true;
            return true;
        }

        bool Queue(int direction, bool back = false)
        {
            auto lease = shared_from_this();
            if (!open || invalid || pending.size() >= 32)
                return false;
            if (opening && busy && back && !direction)
            {
                if (!Current(false) || !context.SelectedEntry())
                    return false;
                Cancel();
                return true;
            }
            if (!Current(true) || !surface.Alive())
                return false;
            const auto nativeSelected = Read<int>(page.object, 540);
            if (nativeSelected <= 0 || size_t(nativeSelected - 1) >= view.items.size())
                return false;
            selected = size_t(nativeSelected - 1);
            if (!surface.Selected(selected))
                return false;
            if (!direction && !back && view.items[selected].kind == Kind::Back)
                back = true;
            if (back && !direction && busy)
            {
                Cancel();
                return true;
            }
            if (!ready)
                return false;
            pending.push_back(Pending{generation, serial, view.page, view.physical, selected,
                surface.rows[selected + 1], view.items[selected], direction, back});
            return true;
        }

        bool Activate(const Pending& event)
        {
            auto lease = shared_from_this();
            if (!open || !ready || event.generation != generation || event.serial != serial ||
                event.logical != view.page || event.physical != view.physical ||
                !Current(true) || !surface.Selected(event.selected) ||
                surface.rows[event.selected + 1] != event.row)
                return true; // stale events are discarded, never retargeted
            selected = event.selected;
            auto current = Snapshot(true, true);
            auto candidate = view;
            const auto item = event.back ? Item{Kind::Back, {}, {}, 0, true} : event.item;
            switch (item.kind)
            {
                case Kind::Link:
                    if (event.direction) return true;
                    if (item.target >= model.Pages().size()) return false;
                    candidate.page = int(item.target);
                    candidate.physical = 0;
                    model.PageShown(item.target);
                    break;
                case Kind::Previous:
                    if (event.direction || !candidate.physical) return true;
                    --candidate.physical;
                    break;
                case Kind::Next:
                    if (event.direction) return true;
                    ++candidate.physical;
                    break;
                case Kind::Back:
                    if (event.direction) return true;
                    if (candidate.page < 0) { Cancel(); return true; }
                    model.PageShown(size_t(candidate.page));
                    candidate.page = -1;
                    candidate.physical = 0;
                    break;
                case Kind::Row:
                {
                    Foreign foreign(*this);
                    const auto enabled = model.Enabled(item.row, context.Pause());
                    if (!current()) return false;
                    if (!enabled) return true;
                    model.Change(item.row, event.direction, context.Pause());
                    if (!current()) return false;
                    break;
                }
            }
            const bool samePage = candidate.page == view.page && candidate.physical == view.physical;
            return current() && Populate(std::move(candidate), samePage ? selected : 0, true);
        }

        bool Restore()
        {
            auto lease = shared_from_this();
            model.Close();
            ready = open = false;
            pending.clear();
            if (!Alive())
                return false;
            if (Current(true))
            {
                Foreign foreign(*this);
                Native::ShowPage(context.manager, "OPTIONS", 0, 0);
            }
            if (!Current(false))
                return false; // temporary modal: retain pending restoration
            restore = false;
            view = {};
            ++serial;
            return entry.Matches(context);
        }

        void Frame()
        {
            auto lease = shared_from_this();
            if (invalid)
                return;
            if (!Alive()) { Retire(); return; }
            if (restore)
            {
                Restore();
                return;
            }
            if (!open)
                return;
            if (!Current(true))
            {
                // A temporary dialog loses input authority without discarding
                // borrowed page restoration or touching native hidden widgets.
                Cancel();
                return;
            }
            while (!pending.empty() && Current(true) && open)
            {
                auto event = std::move(pending.front());
                pending.pop_front();
                if (!Activate(event))
                {
                    Cancel();
                    return;
                }
            }
            if (open && Current(true) && RWGTAMenu::Model::Changed().exchange(false))
            {
                const auto index = Read<int>(page.object, 540);
                if (index <= 0 || size_t(index - 1) >= view.items.size() ||
                    !Populate(view, size_t(index - 1), true))
                    Cancel();
            }
            if (open && ready && Current(true))
            {
                const auto index = Read<int>(page.object, 540);
                std::string description;
                if (index > 0 && size_t(index - 1) < view.items.size() &&
                    view.Footer(size_t(index - 1), font, Width(), description) && description != view.help)
                {
                    auto current = Snapshot(true, true);
                    Foreign foreign(*this);
                    if (!surface.Help(description, current)) { Cancel(); return; }
                    if (current()) view.help = std::move(description);
                }
            }
        }

        void NativePageRetiring(void* object) noexcept
        {
            if (object == escape)
            {
                escape = nullptr;
                Retire();
            }
            if (object == context.options)
            {
                Retire();
                entry.Forgotten();
            }
            if (object == page.object)
            {
                Retire();
                surface.Forgotten();
                page.Forgotten();
            }
        }

        void NativeRowRetiring(void* object) noexcept
        {
            if (object == entry.row)
            {
                Retire();
                entry.Forgotten();
            }
            if (!expectedRows)
                for (auto row : surface.rows)
                    if (object == row)
                    {
                        Retire();
                        surface.Forgotten();
                        break;
                    }
        }

        bool ManagerAlive() const
        {
            // Independent of the source Options owner: only used for deferred
            // escape/cleanup after that source's destructor has invalidated it.
            void* ignored = nullptr;
            return context.manager && Read<uintptr_t>(context.manager, 0) == Native::Address().managerTable &&
                FindPageNode(context.manager, Page::Name, ignored);
        }

        bool OwnsNativePage() const
        {
            if (!ManagerAlive() || !page.object ||
                Read<uintptr_t>(page.object, 0) != Native::Address().pageTable ||
                FindPage(context.manager, Page::Name) != page.object ||
                Read<void*>(context.manager, 48) != page.object || Read<void*>(context.manager, 44) ||
                Read<void*>(context.manager, 52) || Read<uint8_t>(context.manager, 12) ||
                !Read<uint8_t>(context.manager, 33))
                return false;
            const auto count = MenuHistoryCount.get();
            return count > 0 && count <= 10 &&
                Read<void*>(MenuHistory.get_ptr(), (count - 1) * 8) == page.object &&
                !Read<uint8_t>(MenuHistory.get_ptr() + 4, (count - 1) * 8);
        }

        bool Cleanup(const std::function<bool()>& authorized)
        {
            auto lease = shared_from_this();
            if (!authorized())
                return false;
            model.Close();
            ready = open = false;
            pending.clear();
            if (OwnsNativePage())
            {
                const auto sourceExists = entry.SameSource(context);
                const auto name = sourceExists ? "OPTIONS" : context.Pause() ? "INGAMEPAUSE" : "MAIN";
                const auto destination = sourceExists ? context.options : escape;
                if (!destination || FindPage(context.manager, name) != destination ||
                    !destination || !authorized())
                    return false;
                Native::ShowPage(context.manager, name, 0, 0);
                if (!authorized() || !ManagerAlive() || OwnsNativePage())
                    return false;
            }
            // A modal may retain our page beneath it. Never erase/free a page
            // still referenced by current slots or native history.
            if (!page.Remove(context.manager, authorized))
                return false;
            surface.Forgotten();
            if (!entry.Restore(context, authorized))
                return false;
            entry.ReleaseOriginal(authorized);
            return authorized() && !page.object && !entry.row && !entry.original;
        }

        void NativeManagerRetiring()
        {
            Retire();
            entry.Forgotten();
            // A successfully published page is owned by the native map's full
            // deletion loop. Forget it before that loop, never delete it twice.
            if (page.object && FindPage(context.manager, Page::Name) == page.object)
            {
                page.Forgotten();
                surface.Forgotten();
            }
        }
    };
}

export namespace CthulhuMenu
{
    inline SafetyHookInline mapperHook, mapperCleanupHook, targetHook, broadcastHook;
    inline SafetyHookInline messageHook, updateHook, serializationHook, drawHook;
    inline SafetyHookInline managerClearHook, managerDestroyHook, pageDestroyHook, rowDestroyHook;
    inline SafetyHookInline fontHook;
    inline SafetyHookInline actionHook;
    inline std::shared_ptr<Host> host;
    inline HMODULE module = nullptr;
    inline void* frameDispatchReturn = nullptr;
    inline bool installed = false, servicing = false, nativeResources = false;
    inline unsigned mapperDepth = 0, eventDepth = 0, messageDepth = 0, updateDepth = 0;
    inline unsigned serializationDepth = 0, drawDepth = 0, retirementDepth = 0, actionDepth = 0;

    struct Depth
    {
        unsigned& depth;
        explicit Depth(unsigned& value) : depth(value) { ++depth; }
        ~Depth() { --depth; }
    };

    inline bool OutsideNative()
    {
        return !mapperDepth && !eventDepth && !messageDepth && !updateDepth &&
            !serializationDepth && !drawDepth && !retirementDepth && !actionDepth;
    }

    // Failures only; each distinct message is reported once.
    inline void Log(const char* message) noexcept
    {
        try
        {
            static std::set<std::string> written;
            if (written.size() >= 64 || !written.emplace(message).second)
                return;
            OutputDebugStringA(message);
            OutputDebugStringA("\n");
        }
        catch (...) {}
    }

    inline void CancelFailure() noexcept
    {
        if (auto lease = host)
            lease->Cancel();
        Log("CoC native menu: private transaction refused; deferred stock restore");
    }

    // Cheap check on the static menu manager (no page walk).
    inline bool MenuShown() noexcept
    {
        const auto manager = MenuManager.get_ptr();
        const auto count = MenuHistoryCount.get();
        return manager[33] != 0 && count > 0 && count <= 10;
    }

    inline void Service() noexcept
    {
        if (!installed || servicing || !OutsideNative() || !nativeResources)
            return;
        Host::Flag guard(servicing);
        try
        {
            auto lease = host;
            if (lease && lease->invalid)
            {
                if (lease->Cleanup([] { return OutsideNative() && nativeResources; }) && host == lease)
                    host.reset();
                return;
            }
            if (lease)
            {
                // Idle between visits: nothing to do while no native menu is shown.
                if (!lease->open && !lease->restore && lease->pending.empty() && !MenuShown())
                    return;
                lease->Frame();
                return;
            }
            Context source;
            if (!MenuShown() || !source.Capture() || !source.Current(source.options))
                return;
            auto candidate = std::make_shared<Host>();
            candidate->module = module;
            host = candidate;
            const auto title = FixSettings().GetTexts().Resolve("@menu.title");
            if (!candidate->Prepare(title))
            {
                candidate->Retire();
                Log(candidate->preparationFailure ? candidate->preparationFailure : "CoC native menu: Options entry refused");
                return;
            }
        }
        catch (...) { CancelFailure(); }
    }

    inline bool Input(void* manager, uint16_t event) noexcept
    {
        if (!installed)
            return false;
        try
        {
            auto lease = host;
            if (!lease || lease->context.manager != manager)
                return false;
            if (lease->invalid)
            {
                // Never allow a private header to dereference a retired source.
                // Its copied input waits for source-independent safe recovery.
                return lease->OwnsNativePage() &&
                    (event == 0x243 || event == 0x244 || event == 0x245 || event == 0x246 ||
                     event == 0x248 || event == 0x251);
            }
            if (!lease->open)
            {
                if ((event == 0x245 || event == 0x248 || event == 0x251) &&
                    lease->Current(false) && lease->entry.Matches(lease->context) && lease->context.SelectedEntry())
                {
                    // Primitive opening request. Model discovery/UI work cannot
                    // run while manager/bus/mapper native continuations are live.
                    lease->restore = false;
                    lease->pending.clear();
                    lease->pending.push_back(Host::Pending{lease->generation, lease->serial, -2, 0, 0,
                        lease->entry.row, {}, 0, false});
                    return true;
                }
                return false;
            }
            if (event == 0x246)
                return lease->Queue(0, true);
            if (event == 0x245 || event == 0x248 || event == 0x251)
                return lease->Queue(0);
            if (event == 0x243 || event == 0x244)
                return lease->Queue(event == 0x243 ? -1 : 1);
            return false; // stock up/down/forced-close/quit always remain native
        }
        catch (...) { CancelFailure(); return false; }
    }

    inline void OpenRequested() noexcept
    {
        try
        {
            auto lease = host;
            if (!installed || !lease || lease->invalid || lease->open || lease->pending.empty())
                return;
            auto request = std::move(lease->pending.front());
            lease->pending.pop_front();
            if (request.logical != -2 || request.generation != lease->generation || request.serial != lease->serial ||
                request.row != lease->entry.row || !lease->Current(false) || !lease->context.SelectedEntry())
                return;
            // The native renderer uses western byte glyphs. Explicit supported
            // menu languages use checked conversion; auto keeps the English fallback.
            const auto chosen = FixSettings().String("MAIN.TextLanguage");
            const auto language = chosen == "fr" || chosen == "de" || chosen == "it" || chosen == "es" ? chosen : std::string("en");
            if (!lease->Open(language))
                Log("CoC native menu: private publication/layout refused");
        }
        catch (...) { CancelFailure(); }
    }

    inline void Completed() noexcept
    {
        if (!OutsideNative() || servicing)
            return;
        // Both operations have their own reentrancy guard and retained Host.
        Service();
        if (installed && nativeResources && OutsideNative() && !servicing)
        {
            Host::Flag guard(servicing);
            OpenRequested();
        }
    }

    inline int __fastcall Mapper(void* self, void*)
    {
        int result;
        { Depth scope(mapperDepth); result = mapperHook.unsafe_thiscall<int>(self); }
        Completed();
        return result;
    }
    inline int __fastcall MapperCleanup(void* self, void*)
    {
        Depth scope(mapperDepth);
        return mapperCleanupHook.unsafe_thiscall<int>(self);
    }
    inline uint8_t __fastcall Target(void* self, void*, void* event)
    {
        const auto frame = _ReturnAddress() == frameDispatchReturn && event &&
            Read<uint16_t>(event, 0) == 201 && Read<int>(event, 4) == -1 && Read<int>(event, 8) == 0;
        uint8_t result;
        { Depth scope(eventDepth); result = targetHook.unsafe_thiscall<uint8_t>(self, event); }
        // The native frame uses source -1 and target 0; finish dispatch before menu work.
        if (frame)
            Completed();
        return result;
    }
    inline uint8_t __fastcall Broadcast(void* self, void*, void* event)
    {
        Depth scope(eventDepth);
        return broadcastHook.unsafe_thiscall<uint8_t>(self, event);
    }
    inline uint8_t __fastcall Message(void* self, void*, void* event)
    {
        Depth scope(messageDepth);
        if (event && Input(self, Read<uint16_t>(event, 0)))
            return 1;
        return messageHook.unsafe_thiscall<uint8_t>(self, event);
    }
    inline int __fastcall Action(void* self, void*)
    {
        Depth scope(actionDepth);
        if (installed)
            try
            {
                if (auto lease = host)
                {
                    const auto source = !lease->open && self == lease->entry.row &&
                        lease->entry.Matches(lease->context) && lease->context.SelectedEntry();
                    bool own = false;
                    if (lease->open && lease->Current(true) && lease->surface.Alive())
                    {
                        const auto selected = Read<int>(lease->page.object, 540);
                        own = selected > 0 && size_t(selected) < Surface::Count &&
                            self == lease->surface.rows[size_t(selected)];
                    }
                    if ((source || own) && Input(lease->context.manager, 0x245))
                        return 0;
                }
            }
            catch (...) { CancelFailure(); }
        // Every unowned/stock/modal action retains its native confirmation and
        // dispatch; this callback never drains a primitive queued action.
        return actionHook.unsafe_thiscall<int>(self);
    }
    inline int __fastcall Update(void* self, void*)
    {
        Depth scope(updateDepth);
        return updateHook.unsafe_thiscall<int>(self);
    }
    inline int __cdecl Serialize()
    {
        int result;
        // Save/load serialization is not a menu frame boundary.
        { Depth scope(serializationDepth); result = serializationHook.unsafe_ccall<int>(); }
        return result;
    }
    inline int __fastcall Draw(void* self, void*, int target)
    {
        Depth scope(drawDepth);
        return drawHook.unsafe_thiscall<int>(self, target);
    }
    inline int __fastcall ManagerClear(void* self, void*)
    {
        Depth scope(retirementDepth);
        if (installed)
            try { if (auto lease = host; lease && lease->context.manager == self) lease->NativeManagerRetiring(); }
            catch (...) { CancelFailure(); }
        return managerClearHook.unsafe_thiscall<int>(self);
    }
    inline int __fastcall ManagerDestroy(void* self, void*)
    {
        Depth scope(retirementDepth);
        if (installed)
            try { if (auto lease = host; lease && lease->context.manager == self) lease->NativeManagerRetiring(); }
            catch (...) { CancelFailure(); }
        return managerDestroyHook.unsafe_thiscall<int>(self);
    }
    inline int __fastcall PageDestroy(void* self, void*)
    {
        const bool enclosingRetirement = retirementDepth != 0;
        Depth scope(retirementDepth);
        if (installed)
            try
            {
                if (auto lease = host)
                {
                    const auto tracked = self == lease->context.options || self == lease->page.object;
                    // A standalone source destruction must not leave a dead
                    // history entry for later ShowPage/deactivation to follow.
                    if (tracked && !enclosingRetirement && lease->ManagerAlive() &&
                        MenuHistoryCount.get() <= 10)
                    {
                        Native::RemoveHistory(self);
                        for (size_t offset : {size_t(44), size_t(48), size_t(52)})
                            if (Read<void*>(lease->context.manager, offset) == self)
                                Write<void*>(lease->context.manager, offset, nullptr);
                    }
                    lease->NativePageRetiring(self);
                }
            }
            catch (...) { CancelFailure(); }
        return pageDestroyHook.unsafe_thiscall<int>(self);
    }
    inline void* __fastcall RowDestroy(void* self, void*, unsigned flags)
    {
        Depth scope(retirementDepth);
        if (installed)
            if (auto lease = host)
                lease->NativeRowRetiring(self);
        return rowDestroyHook.unsafe_thiscall<void*>(self, flags);
    }
    inline int __cdecl Font(void* backend)
    {
        Depth scope(retirementDepth);
        if (installed && !backend)
        {
            nativeResources = false;
            if (auto lease = host) lease->Retire();
        }
        const auto result = fontHook.unsafe_ccall<int>(backend);
        if (installed && backend)
            nativeResources = Read<uintptr_t>(backend, 0) == Native::Address().backendTable;
        return result;
    }

    inline void Rollback() noexcept
    {
        installed = false;
        nativeResources = false;
        frameDispatchReturn = nullptr;
        if (auto lease = host) lease->Retire();
        actionHook.reset(); fontHook.reset(); rowDestroyHook.reset(); pageDestroyHook.reset();
        managerDestroyHook.reset(); managerClearHook.reset(); drawHook.reset(); serializationHook.reset();
        updateHook.reset(); messageHook.reset(); broadcastHook.reset(); targetHook.reset();
        mapperCleanupHook.reset(); mapperHook.reset();
    }

    inline void Init()
    {
        if (installed)
            return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        auto frameDispatch = hook::pattern("8D 44 24 44 83 CE FF 50 66 C7 44 24 48 C9 00 89 5C 24 58 66 89 5C 24 54 89 74 24 4C 89 5C 24 50 E8 ? ? ? ? 8B C8 E8 ? ? ? ? E8 ? ? ? ? 8B C8 E8");
        auto targetDispatch = hook::pattern("56 8B 74 24 08 8B 56 08 83 FA FF 75 0A 56 E8 ? ? ? ? 5E C2 04 00 8B C2 25 FF 01 00 00 8B 04 81 85 C0 74 0B 39 10");
        frameDispatchReturn = frameDispatch.get_first(44);
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &pinned))
        {
            Log("CoC native menu: process lifetime pin refused");
            return;
        }
        // The module is pinned before any hook, so it cannot unload while hooked.
        try
        {
            mapperHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().mapper), Mapper, SafetyHookInline::StartDisabled);
            mapperCleanupHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().mapperCleanup), MapperCleanup, SafetyHookInline::StartDisabled);
            targetHook = safetyhook::create_inline(targetDispatch.get_first(), Target, SafetyHookInline::StartDisabled);
            broadcastHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().broadcast), Broadcast, SafetyHookInline::StartDisabled);
            messageHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().message), Message, SafetyHookInline::StartDisabled);
            updateHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().update), Update, SafetyHookInline::StartDisabled);
            serializationHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().serialize), Serialize, SafetyHookInline::StartDisabled);
            drawHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().draw), Draw, SafetyHookInline::StartDisabled);
            managerClearHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().managerClear), ManagerClear, SafetyHookInline::StartDisabled);
            managerDestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().managerDestroy), ManagerDestroy, SafetyHookInline::StartDisabled);
            pageDestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().pageDestroy), PageDestroy, SafetyHookInline::StartDisabled);
            rowDestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().rowDestroy), RowDestroy, SafetyHookInline::StartDisabled);
            fontHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().font), Font, SafetyHookInline::StartDisabled);
            actionHook = safetyhook::create_inline(reinterpret_cast<void*>(Native::Address().action), Action, SafetyHookInline::StartDisabled);
            // Every original trampoline exists before any patched entry is live.
            for (auto hook : { &mapperHook, &mapperCleanupHook, &targetHook, &broadcastHook, &messageHook, &updateHook, &serializationHook,
                &drawHook, &managerClearHook, &managerDestroyHook, &pageDestroyHook, &rowDestroyHook, &fontHook, &actionHook })
                (void)hook->enable();
            auto backend = FontBackend.get();
            nativeResources = backend && Read<uintptr_t>(backend, 0) == Native::Address().backendTable;
            installed = true;
        }
        catch (...)
        {
            Rollback();
            Log("CoC native menu: installation exception refused");
        }
    }
}
