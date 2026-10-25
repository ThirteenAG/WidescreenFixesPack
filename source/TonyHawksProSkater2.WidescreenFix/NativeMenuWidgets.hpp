#pragma once

#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"
#include "NativeMenuText.hpp"
#include "NativeMenuDiagnostics.hpp"
#include <memory>
#include <vector>

namespace THPS2Menu
{
    // The guarded host supplies its exact native
    // title/pause context and complete input/draw unwind. This layer never installs a
    // callback or transfers native row ownership to an unrelated container.
    inline bool nativeReady = false;

    template<typename Function>
    Function FunctionAt(uintptr_t address)
    {
        return reinterpret_cast<Function>(address);
    }

    template<typename T>
    void NativeWrite(T* address, const T& value)
    {
        ProtectedGameRef<T>{ address } = value;
    }

    inline void* AllocateNative(size_t size)
    {
        return nativeReady ? FunctionAt<void* (__cdecl*)(size_t)>(0x466B00)(size) : nullptr;
    }

    inline void FreeNative(void* value)
    {
        if (value && nativeReady)
            FunctionAt<void (__cdecl*)(void*)>(0x466B40)(value);
    }

    template<uintptr_t Destructor>
    struct NativeDelete
    {
        template<typename T>
        void operator()(T* value) const
        {
            if (value && nativeReady)
                FunctionAt<void* (__thiscall*)(void*, unsigned)>(Destructor)(value, 1);
        }
    };

    using ListPointer = std::unique_ptr<Native::List, NativeDelete<0x470C80>>;
    using TextPointer = std::unique_ptr<Native::TextRow, NativeDelete<0x46F8A0>>;
    using PausePointer = std::unique_ptr<Native::PauseMenu, NativeDelete<0x44B690>>;

    inline bool FontUsable(const Native::Font& font)
    {
        return font.glyphs && Contracts::Readable(font.glyphs, 8) && !font.effect && font.spacing >= 0 && font.spacing <= 64 && font.scale > 0 && font.scale <= 8192;
    }

    inline bool CaptionGlyphs(const Native::Font& font, const std::string& text)
    {
        if (!FontUsable(font))
            return false;
        for (const unsigned char code : text)
        {
            if (!code || code >= 0x80)
                return false;
            const auto glyph = font.glyphMap[code];
            if (code != ' ' && glyph == 255)
                return false;
            if (glyph != 255 && !Contracts::Readable(static_cast<const uint8_t*>(font.glyphs) + 8 * glyph, 8))
                return false;
        }
        return true;
    }

    struct DisplayLine
    {
        std::string text;
        int command = -1;
        bool enabled = false;
        bool continuation = false;
        bool heading = false;
    };

    class TextList
    {
        struct Cell
        {
            // The engine text row borrows these private mutable properties.
            // Native text-row destruction does not free them (46F8C0/46F650).
            std::unique_ptr<Native::TextProperties> properties;
            TextPointer row;
        };

        ListPointer list;
        std::vector<std::unique_ptr<Cell>> cells;

    public:
        TextList() = default;
        TextList(const TextList&) = delete;
        TextList& operator=(const TextList&) = delete;
        TextList(TextList&&) = delete;
        TextList& operator=(TextList&&) = delete;

        ~TextList() { Clear(); }

        void Clear()
        {
            // No native input/draw may be in flight here. List470CA0 is a
            // non-owning pointer array; retire rows before their properties.
            list.reset();
            cells.clear();
        }

        Native::List* Get() const { return list.get(); }

        bool Build(const std::vector<DisplayLine>& lines, const Native::TextProperties& style)
        {
            if (!nativeReady || lines.empty() || lines.size() > 12 || style.vtable != reinterpret_cast<void*>(0x515E40) || style.font.vtable != reinterpret_cast<void*>(0x5159E4) || !FontUsable(style.font) || std::none_of(lines.begin(), lines.end(), [](const DisplayLine& line) { return line.enabled; }))
                return false;
            Clear();
            cells.reserve(lines.size());
            auto allocation = static_cast<Native::List*>(AllocateNative(sizeof(Native::List)));
            if (!allocation)
                return false;
            NativeWrite(allocation, Native::List{});
            auto created = FunctionAt<Native::ListConstructor>(0x470BF0)(allocation, 64, 30, 384, 180, 0, 0, 2, 0);
            if (created != allocation || allocation->vtable != reinterpret_cast<void*>(0x515EE8))
            {
                FreeNative(allocation);
                return false;
            }
            list.reset(allocation);

            for (const auto& line : lines)
            {
                if (line.text.size() > 31 || std::any_of(line.text.begin(), line.text.end(), [](char code) { return static_cast<uint8_t>(code) >= 0x80 || code == 0; }))
                {
                    Clear();
                    return false;
                }
                auto cell = std::make_unique<Cell>();
                cell->properties = std::make_unique<Native::TextProperties>(style);
                auto& properties = *cell->properties;
                properties.font.effect = nullptr;
                properties.font.proportional = 0;
                properties.font.scale = 3072;
                properties.focusedScale = properties.unfocusedScale = 3072;
                if (!LiteralFont(properties.font))
                {
                    Clear();
                    return false;
                }
                if (line.continuation)
                    properties.disabledColor = properties.unfocusedColor;
                if (line.heading)
                    properties.disabledColor = properties.focusedColor;

                auto row = static_cast<Native::TextRow*>(AllocateNative(sizeof(Native::TextRow)));
                if (!row)
                {
                    Clear();
                    return false;
                }
                NativeWrite(row, Native::TextRow{});
                auto result = FunctionAt<Native::TextConstructor>(0x46F820)(row, line.command, line.text.c_str(), cell->properties.get());
                if (result != row || row->vtable != reinterpret_cast<void*>(0x515E44))
                {
                    FreeNative(row);
                    Clear();
                    return false;
                }
                cell->row.reset(row);
                NativeWrite(&row->enabled, static_cast<uint8_t>(line.enabled));
                // Native spacing is part of the measured width. Fit long rows
                // using private font properties, then let the engine remeasure.
                for (int attempt = 0; row->width > 384 && attempt < 3; ++attempt)
                {
                    const int scale = std::max(2048, properties.focusedScale * 376 / row->width);
                    if (scale >= properties.focusedScale)
                        break;
                    properties.font.scale = scale;
                    properties.focusedScale = properties.unfocusedScale = scale;
                    FunctionAt<int (__thiscall*)(Native::TextRow*, const char*)>(0x46F8F0)(row, line.text.c_str());
                }
                if (row->width < 0 || row->width > 384 || row->height < 6 || row->height > 15)
                {
                    Diagnose(Diagnostic::Layout, row->width < 0 || row->width > 384
                        ? "title row exceeds panel width" : "title row height unavailable", cells.size());
                    Clear();
                    return false;
                }
                // Publish only fully constructed rows. All private row/style
                // allocations have stable addresses before the native append.
                FunctionAt<Native::TextRow* (__thiscall*)(Native::List*, Native::TextRow*)>(0x470FE0)(list.get(), row);
                cells.push_back(std::move(cell));
            }

            FunctionAt<int (__thiscall*)(Native::List*)>(0x470CB0)(list.get());
            if (list->count != static_cast<int>(lines.size()) || list->height <= 0 || list->height > 204 || list->selected < 0 || list->selected >= list->count || !list->rows[list->selected]->enabled)
            {
                Diagnose(Diagnostic::Layout, list->height <= 0 || list->height > 204
                    ? "title list exceeds panel height" : "title selection unavailable", cells.size());
                Clear();
                return false;
            }
            return true;
        }

        int SelectedCommand() const
        {
            if (!list || list->selected < 0 || list->selected >= list->count || list->count > 96)
                return -1;
            auto row = list->rows[list->selected];
            return row && row->enabled ? row->command : -1;
        }

        void Select(int command)
        {
            if (!list)
                return;
            for (int i = 0; i < list->count; ++i)
            {
                if (list->rows[i]->enabled && list->rows[i]->command == command)
                {
                    FunctionAt<int (__thiscall*)(Native::List*, int)>(0x470AB0)(list.get(), command);
                    return;
                }
            }
        }

        void Input(int direction)
        {
            if (list && list->count > 0 && list->selected >= 0 && list->selected < list->count)
                FunctionAt<Native::ListInput>(0x470F10)(list.get(), static_cast<uint32_t>(direction));
        }

        void Draw()
        {
            if (!list || list->count <= 0 || list->selected < 0 || list->selected >= list->count)
                return;
            FunctionAt<int (__thiscall*)(Native::List*)>(0x470EE0)(list.get());
            FunctionAt<int (__thiscall*)(Native::List*)>(0x470E50)(list.get());
        }
    };

    class PauseList
    {
        // 44B830 borrows these strings. A whole frame/input/draw lease is
        // required before Clear/Build, not merely a controller callback scope.
        std::vector<DisplayLine> lines;
        PausePointer menu;

    public:
        PauseList() = default;
        PauseList(const PauseList&) = delete;
        PauseList& operator=(const PauseList&) = delete;
        PauseList(PauseList&&) = delete;
        PauseList& operator=(PauseList&&) = delete;
        ~PauseList() { Clear(); }

        void Clear()
        {
            menu.reset();
            lines.clear();
        }

        Native::PauseMenu* Get() const { return menu.get(); }

        bool Build(const std::vector<DisplayLine>& source)
        {
            if (!nativeReady || source.empty() || source.size() > 12 || std::none_of(source.begin(), source.end(), [](const DisplayLine& line) { return line.enabled; }))
                return false;
            for (const auto& line : source)
            {
                if (line.text.size() > 31 || std::any_of(line.text.begin(), line.text.end(), [](char code) { return static_cast<uint8_t>(code) >= 0x80 || code == 0; }))
                    return false;
            }
            Clear();
            lines = source;
            auto allocation = static_cast<Native::PauseMenu*>(AllocateNative(sizeof(Native::PauseMenu)));
            if (!allocation)
                return false;
            NativeWrite(allocation, Native::PauseMenu{});
            auto result = FunctionAt<Native::PauseConstructor>(0x44B5E0)(allocation, 256, 0, 0, 256, 256, 16);
            if (result != allocation || allocation->vtable != reinterpret_cast<void*>(0x5159F0))
            {
                FreeNative(allocation);
                return false;
            }
            menu.reset(allocation);
            for (size_t i = 0; i < lines.size(); ++i)
            {
                FunctionAt<char (__thiscall*)(Native::PauseMenu*, const char*)>(0x44B830)(menu.get(), lines[i].text.c_str());
                auto& row = menu->rows[i];
                NativeWrite(&row.enabled, static_cast<uint8_t>(lines[i].enabled));
                if (lines[i].continuation)
                    NativeWrite(&row.unfocusedColor, std::array<uint8_t, 3>{ 122, 122, 122 });
                if (lines[i].heading)
                    NativeWrite(&row.unfocusedColor, row.focusedColor);
            }
            for (size_t i = 0; i < lines.size(); ++i)
            {
                if (lines[i].enabled)
                {
                    FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(0x44B710)(menu.get(), static_cast<char>(i));
                    break;
                }
            }
            FunctionAt<int (__thiscall*)(Native::PauseMenu*)>(0x44B6C0)(menu.get());
            if (menu->count != lines.size() || menu->selected >= menu->count || !menu->rows[menu->selected].enabled)
            {
                Clear();
                return false;
            }
            return true;
        }

        int SelectedCommand() const
        {
            if (!menu || menu->selected >= lines.size() || !menu->rows[menu->selected].enabled)
                return -1;
            return lines[menu->selected].command;
        }

        void Select(int command)
        {
            if (!menu)
                return;
            for (size_t i = 0; i < lines.size(); ++i)
            {
                if (lines[i].enabled && lines[i].command == command)
                {
                    FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(0x44B710)(menu.get(), static_cast<char>(i));
                    return;
                }
            }
        }
    };
}
