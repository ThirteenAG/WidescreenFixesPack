#pragma once

#include "NativeMenuHost.hpp"

namespace THPS2Menu
{
    inline constexpr int CategoryCommand = 0x6F01;

    template<typename T>
    T* Field(void* owner, size_t offset)
    {
        auto address = static_cast<uint8_t*>(owner) + offset;
        return Contracts::Readable(address, sizeof(T)) ? reinterpret_cast<T*>(address) : nullptr;
    }

    struct TitleContext
    {
        void* owner = nullptr;
        Native::List* base = nullptr;
        Native::TextProperties* style = nullptr;
        Native::TextRow* entry = nullptr;
        bool exiting = false;
        bool deletionRequested = false;

        bool Alive() const
        {
            if (!owner || !Contracts::Readable(owner, 2560) || *static_cast<void**>(owner) != reinterpret_cast<void*>(0x5163B4))
                return false;
            if (*Field<Native::List*>(owner, 844) != base || !base || !Contracts::Readable(base, sizeof(*base)) || base->vtable != reinterpret_cast<void*>(0x515EE8) || base->count <= 0 || base->count > 96 || base->selected < 0 || base->selected >= base->count)
                return false;
            if (*Field<Native::TextProperties*>(owner, 860) != style || !style || !Contracts::Readable(style, sizeof(*style)) || style->vtable != reinterpret_cast<void*>(0x515E40) || style->font.vtable != reinterpret_cast<void*>(0x5159E4))
                return false;
            if (entry && (*Field<Native::TextRow*>(owner, 904) != entry || !Contracts::Readable(entry, sizeof(*entry)) || entry->vtable != reinterpret_cast<void*>(0x515E44) || entry->command != CategoryCommand || entry->properties != style))
                return false;
            if (entry && std::find(base->rows.begin(), base->rows.begin() + base->count, entry) == base->rows.begin() + base->count)
                return false;
            return !exiting && !deletionRequested;
        }

        bool Eligible(bool privateOpen) const
        {
            if (!Alive())
                return false;
            const auto current = *Field<int>(owner, 2412);
            const auto next = *Field<int>(owner, 2416);
            // Discovery/population runs at the native stock20 boundary; the
            // adapter commits both states32 atomically after full construction.
            return current == next && current == (privateOpen ? 32 : 20);
        }

        int Selected() const
        {
            if (!Alive())
                return -1;
            auto row = base->rows[base->selected];
            return row && Contracts::Readable(row, sizeof(*row)) && row->enabled ? row->command : -1;
        }

        bool Append(void* value, const std::string& caption)
        {
            owner = value;
            if (!owner || !Contracts::Readable(owner, 2560) || *static_cast<void**>(owner) != reinterpret_cast<void*>(0x5163B4))
                return false;
            base = *Field<Native::List*>(owner, 844);
            style = *Field<Native::TextProperties*>(owner, 860);
            if (!Alive() || base->count > 12 || *Field<Native::TextRow*>(owner, 904) || caption.empty() || caption.size() > 31 || !CaptionGlyphs(style->font, caption))
                return false;
            for (int i = 0; i < base->count; ++i)
            {
                const auto row = base->rows[i];
                if (!row || !Contracts::Readable(row, sizeof(*row)) || row->command == CategoryCommand)
                    return false;
            }
            auto allocation = static_cast<Native::TextRow*>(AllocateNative(sizeof(Native::TextRow)));
            if (!allocation)
                return false;
            NativeWrite(allocation, Native::TextRow{});
            const auto created = FunctionAt<Native::TextConstructor>(0x46F820)(allocation, CategoryCommand, caption.c_str(), style);
            if (created != allocation || allocation->vtable != reinterpret_cast<void*>(0x515E44))
            {
                FreeNative(allocation);
                return false;
            }
            TextPointer transaction(allocation);
            if (allocation->width <= 0 || allocation->width > 440 || allocation->height < 6 || allocation->height > 24 || !Alive())
                return false;
            int total = allocation->height + base->rowSpacing;
            for (int i = 0; i < base->count; ++i)
            {
                if (base->rows[i]->height < 0 || base->rows[i]->height > 24)
                    return false;
                total += base->rows[i]->height + base->rowSpacing;
            }
            if (total <= 0 || total > 216)
                return false;
            // Native47FD70 owns row-array slot226, unused in this executable.
            // Publish only after construction/bounds. Native470FE0 has no
            // callback/allocator and cannot fail below the guarded96 capacity.
            NativeWrite(Field<Native::TextRow*>(owner, 904), allocation);
            FunctionAt<int (__thiscall*)(Native::List*, Native::TextRow*)>(0x470FE0)(base, allocation);
            entry = transaction.release();
            return true;
        }

        bool Enter()
        {
            if (!Eligible(false) || Selected() != CategoryCommand)
                return false;
            NativeWrite(Field<int>(owner, 2412), 32);
            NativeWrite(Field<int>(owner, 2416), 32);
            return true;
        }

        void Leave()
        {
            if (!Alive())
                return;
            if (*Field<int>(owner, 2412) == 32 && *Field<int>(owner, 2416) == 32)
            {
                NativeWrite(Field<int>(owner, 2412), 20);
                NativeWrite(Field<int>(owner, 2416), 20);
                FunctionAt<int (__thiscall*)(Native::List*, int)>(0x470AB0)(base, CategoryCommand);
            }
        }
    };

    struct PauseContext
    {
        Native::PauseMenu* base = nullptr;
        std::string caption;
        uint8_t entry = 255;

        bool Alive() const
        {
            const auto root = reinterpret_cast<Native::PauseMenu* const*>(0x55E8A0);
            return base && Contracts::Readable(root, sizeof(*root)) && *root == base && Contracts::Readable(base, sizeof(*base)) && base->vtable == reinterpret_cast<void*>(0x5159F0) && base->count && base->count <= 15 && base->selected < base->count;
        }

        bool Eligible() const
        {
            return Alive() && *reinterpret_cast<const int*>(0x55E85C) == 1 && *reinterpret_cast<const int*>(0x55E864) == 1 && entry < base->count && base->rows[entry].text == caption.c_str();
        }

        bool Append(Native::PauseMenu* value, std::string label)
        {
            base = value;
            if (!Alive() || base->count > 7 || base->rowSpacing <= 0 || base->rowSpacing > 24 || label.empty() || label.size() > 31 || std::any_of(label.begin(), label.end(), [](char code) { return static_cast<uint8_t>(code) >= 0x80 || code == 0; }))
                return false;
            auto font = reinterpret_cast<const Native::Font*>(0x567600);
            PauseFont metrics;
            const auto& slot = base->rows[base->count];
            const int largestScale = std::max<int16_t>(slot.focusedScale, slot.unfocusedScale);
            if (!Contracts::Readable(font, sizeof(*font)) || font->proportional != 0 || !CaptionGlyphs(*font, label) || largestScale < 256 || largestScale > 512 || !metrics.Prepare(*font, { { label, CategoryCommand, true } }, 16 * largestScale, base->rowSpacing))
                return false;
            // Stock pause rows are maintained by pointer-labelled native
            // lookups every frame. Append; never replace those original rows.
            caption = std::move(label);
            entry = base->count;
            FunctionAt<char (__thiscall*)(Native::PauseMenu*, const char*)>(0x44B830)(base, caption.c_str());
            if (base->count != entry + 1 || base->rows[entry].text != caption.c_str())
                return false;
            return true;
        }
    };
}
