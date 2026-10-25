#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <Windows.h>

// Installed 1.2 native menu ABI. This header does not publish hooks or callbacks.
// Addressed machine proofs are in mafia-final-menu/native-*-proof.json.
namespace MafiaMenu
{
    struct Control;
    struct Record
    {
        uint32_t id, type;
        float x, y, width, height;
        uint32_t textKey, flags;
    };
    struct Control
    {
        uintptr_t* table;
        uint32_t id, flags, textKey;
        const char* text;
        float x, y, width, height, relativeX, relativeY;
        Control* parent;
        uint32_t allocator;
        Control** begin;
        Control** end;
        Control** capacity;
    };
    struct Page
    {
        uintptr_t* table;
        Page* parent;
        Control* root;
        Control* selected;
        uint32_t allocator;
        Control** begin;
        Control** end;
        Control** capacity;
        uint8_t closed;
        uint8_t closedPadding[3];
        int result, command;
        uint32_t flags;
    };
    static_assert(sizeof(Record) == 32);
    static_assert(sizeof(Control) == 64 && offsetof(Control, parent) == 44);
    static_assert(sizeof(Page) == 48 && offsetof(Page, selected) == 12 && offsetof(Page, result) == 36);
    inline bool Readable(const void* address, size_t length)
    {
        if (!address || !length) return false;
        auto current = reinterpret_cast<uintptr_t>(address);
        if (length > UINTPTR_MAX - current) return false;
        const auto end = current + length;
        while (current < end)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQuery(reinterpret_cast<void*>(current), &info, sizeof(info)) || info.State != MEM_COMMIT ||
                (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
            const auto next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
            if (next <= current) return false;
            current = next;
        }
        return true;
    }
    inline bool Controls(const Control* parent)
    {
        if (!Readable(parent, sizeof(Control))) return false;
        const auto first = reinterpret_cast<uintptr_t>(parent->begin);
        const auto last = reinterpret_cast<uintptr_t>(parent->end);
        const auto limit = reinterpret_cast<uintptr_t>(parent->capacity);
        return last >= first && limit >= last && (last - first) % sizeof(Control*) == 0 &&
            last - first <= 4096 * sizeof(Control*) && (!first ? last == 0 && limit == 0 : Readable(parent->begin, std::max<size_t>(sizeof(Control*), last - first)));
    }
    inline bool Contains(const Page* page, const Control* control)
    {
        if (!Readable(page, sizeof(Page)) || !control) return false;
        const auto first = reinterpret_cast<uintptr_t>(page->begin);
        const auto last = reinterpret_cast<uintptr_t>(page->end);
        if (last < first || (last - first) % sizeof(Control*) || last - first > 4096 * sizeof(Control*) ||
            !Readable(page->begin, std::max<size_t>(sizeof(Control*), last - first))) return false;
        for (auto** item = page->begin; item < page->end; ++item)
            if (*item == control) return Readable(control, sizeof(Control));
        return false;
    }
    inline Page* Top() { return *reinterpret_cast<Page**>(0x6BD890); }
    inline void Select(Page* page, uint32_t id)
    {
        reinterpret_cast<void (__thiscall*)(Page*, uint32_t)>(0x5EB210)(page, id);
    }
    inline void Close(Page* page, int result)
    {
        // Calls parent slot24 and own slot8 before marking native close state.
        reinterpret_cast<void (__thiscall*)(Page*, int)>(0x5EAE10)(page, result);
    }
    inline void Push(Page* page, int command)
    {
        // Native global stack publication precedes Init/parent notification.
        reinterpret_cast<void (__stdcall*)(Page*, int)>(0x5EAD50)(page, command);
    }
    inline Control* CreateButton(const Record& record, Control* parent)
    {
        //5EA370 factory dispatch proves ECX=record, EDX=current parent.
        return reinterpret_cast<Control* (__fastcall*)(const Record*, Control*)>(0x5DB730)(&record, parent);
    }
    inline void* Allocate(size_t length)
    {
        return reinterpret_cast<void* (__cdecl*)(size_t)>(0x6106FC)(length);
    }
    inline void Free(void* memory)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(0x60FCF6)(memory);
    }
    inline void DestroyControl(Control* control)
    {
        reinterpret_cast<void (__thiscall*)(Control*, uint32_t)>(control->table[0])(control, 1);
    }
    inline bool Append(Page* page, Control* control)
    {
        // Same native allocator and pointer-vector layout as5EA370/5EB7D0.
        // The factory already linked parent.children; this second vector is
        // the page's destructor ownership, not another child relationship.
        if (!Readable(page, sizeof(Page)) || !Readable(control, sizeof(Control))) return false;
        const auto begin = reinterpret_cast<uintptr_t>(page->begin);
        const auto end = reinterpret_cast<uintptr_t>(page->end);
        const auto capacity = reinterpret_cast<uintptr_t>(page->capacity);
        if (end < begin || capacity < end || (end - begin) % 4 || (capacity - begin) % 4 ||
            capacity - begin > 4096 * sizeof(Control*) || (!begin && (end || capacity))) return false;
        const auto count = (end - begin) / sizeof(Control*);
        if (begin && !Readable(page->begin, capacity - begin)) return false;
        if (end == capacity)
        {
            const auto available = std::min<size_t>(4096, std::max<size_t>(8, count * 2));
            if (available <= count) return false;
            auto** replacement = static_cast<Control**>(Allocate(available * sizeof(Control*)));
            if (!replacement) return false;
            if (count) std::memcpy(replacement, page->begin, count * sizeof(Control*));
            Free(page->begin);
            page->begin = replacement;
            page->end = replacement + count;
            page->capacity = replacement + available;
        }
        *page->end++ = control;
        return true;
    }
    inline bool Detach(Control* control)
    {
        // Only the exact newly created node may be detached during rollback.
        // The native scalar destructor does not unlink it from its parent.
        if (!Readable(control, sizeof(Control))) return false;
        auto* parent = control->parent;
        if (!parent) return true;
        if (!Controls(parent)) return false;
        for (auto** item = parent->begin; item < parent->end; ++item)
        {
            if (*item != control) continue;
            std::memmove(item, item + 1, static_cast<size_t>(parent->end - item - 1) * sizeof(Control*));
            --parent->end;
            control->parent = nullptr;
            return true;
        }
        return false;
    }
    inline float* FontBank(unsigned int font)
    {
        // Four native map slots occupy6C4078..6C4087;6C4088 begins the
        // language-map records (5F6F30), not another font pointer.
        if (font >= 4) return nullptr;
        auto* fonts = reinterpret_cast<float*>(0x6BF980);
        auto* bank = fonts + 1031 * font;
        if (!Readable(bank, 1031 * sizeof(float))) return nullptr;
        auto* maps = reinterpret_cast<const uint8_t* const*>(0x6C4078);
        if (!Readable(maps + font, sizeof(void*)) || !Readable(maps[font], 512)) return nullptr;
        if (!std::isfinite(bank[2]) || bank[2] <= 0 || !std::isfinite(bank[3]) || bank[3] <= 0 ||
            !std::isfinite(bank[5])) return nullptr;
        return bank;
    }
    inline float Measure(const std::string& text, float height, unsigned int font)
    {
        auto* bank = FontBank(font);
        if (!bank || text.size() > 4096 || !std::isfinite(height) || height <= 0) return INFINITY;
        auto* map = reinterpret_cast<const uint8_t* const*>(0x6C4078)[font];
        for (auto character : text)
        {
            const auto byte = static_cast<uint8_t>(character);
            const auto width = bank[4 * map[2 * byte] + 10];
            // A negative glyph width is the native blank/space sentinel;
            //6036D0 deliberately substitutes0.3em for it.
            if (!std::isfinite(width)) return INFINITY;
        }
        //603880 passes h*bank[2] to6036D0; measure uses the identical scale.
        const auto result = reinterpret_cast<double (__thiscall*)(float*, const char*, float, unsigned int, uintptr_t)>(0x6036D0)(
            reinterpret_cast<float*>(0x6BF980), text.c_str(), height * bank[2], font, 0);
        return std::isfinite(result) && result >= 0 && result <= std::numeric_limits<float>::max() ? static_cast<float>(result) : INFINITY;
    }
    inline std::string Language()
    {
        //5F70D0 returns the exact two bytes used by localized MNU filenames.
        const auto language = *reinterpret_cast<uint32_t*>(0x6C42C4);
        std::string tag{static_cast<char>(language), static_cast<char>(language >> 8)};
        for (auto& character : tag)
        {
            if (character >= 'A' && character <= 'Z') character += 'a' - 'A';
            if (character < 'a' || character > 'z') return "en";
        }
        // Native5F7000 explicitly loads tables/lang_cz.def; resources usecs.
        if (tag == "cz") return "cs";
        for (const auto* supported : { "en", "de", "fr", "it", "es", "pl", "cs", "ru" })
            if (tag == supported) return tag;
        // Nonstandard native aliases require their actual DEF provenance.
        return "en";
    }
}
