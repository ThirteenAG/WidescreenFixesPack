module;

#include <stdafx.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <Windows.h>
#include <cstdio>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include "Shared/RWGTA/FixMenu.hpp"
#include <functional>
#include <limits>
#include <map>
#include "FixSettings.hpp"
#include <deque>
#include <intrin.h>
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frontend;

import ComVars;
import Hud;
import Camera;

export
{
// THawk2.exe only. These two native row systems are unrelated; neither owns
// foreign callbacks. The adapter must verify the executable contracts before
// calling into either one. No menu is installed by this layout header.
namespace THPS2Menu::Native
{
    // 449C40 / 44A010 / 44A470. Glyph lookup sign-extends input bytes, so the
    // host must convert UTF-8 to safe 7-bit text before native measure/draw.
    struct Font
    {
        void* vtable;
        int32_t alignment;
        int32_t verticalAlignment;
        int32_t spacing;
        int32_t reserved16;
        int32_t alpha;
        int32_t red;
        int32_t green;
        int32_t blue;
        int32_t shadowX;
        int32_t shadowY;
        int32_t proportional;
        int32_t scale;
        void* glyphs;
        std::array<uint32_t, 2> reserved56;
        void* effect;
        uint32_t reserved68;
        std::array<uint8_t, 3> flags;
        std::array<uint8_t, 256> glyphMap;
        uint8_t reserved331;
    };

    // 46F700. Ctor/setter/draw change the font scale and RGB fields. Private
    // rows require private copies; the native Options style is not immutable.
    struct TextProperties
    {
        void* vtable;
        Font font;
        int32_t focusedScale;
        int32_t unfocusedScale;
        std::array<int32_t, 3> focusedColor;
        std::array<int32_t, 3> unfocusedColor;
        std::array<int32_t, 3> disabledColor;
    };

    struct PauseRow
    {
        const char* text;
        int16_t scale;
        int16_t targetScale;
        int16_t focusedScale;
        int16_t unfocusedScale;
        uint8_t extraSpacing;
        uint8_t visible;
        uint8_t enabled;
        std::array<uint8_t, 3> focusedColor;
        std::array<uint8_t, 3> unfocusedColor;
        uint8_t pulse;
        std::array<uint8_t, 2> reserved;
    };

    // 44B5E0 / 44B830 / 44BB60: exactly fifteen borrowed label pointers.
    struct PauseMenu
    {
        void* vtable;
        uint8_t alignment;
        uint8_t reserved5;
        uint8_t selected;
        std::array<uint8_t, 2> reserved7;
        uint8_t count;
        std::array<uint8_t, 2> reserved10;
        int32_t x;
        int32_t y;
        int32_t rowSpacing;
        std::array<PauseRow, 15> rows;
    };

    // 46F820 / 46F8F0: text is copied, strictly fewer than 32 bytes.
    // Properties are borrowed from the Options owner, not retained by a lease.
    struct TextRow
    {
        void* vtable;
        uint32_t flags;
        int32_t command;
        uint8_t enabled;
        uint8_t visible;
        uint8_t focused;
        uint8_t reserved15;
        int32_t value;
        int32_t width;
        int32_t height;
        int32_t x;
        int32_t y;
        int32_t animation;
        int32_t animate;
        int32_t alignment;
        std::array<char, 32> text;
        TextProperties* properties;
    };

    // 470BF0 / 470FE0. Deleting this list does not delete its row pointers:
    // 47FD70 destroys the four lists and then the separate 96x4 owner arrays.
    struct List
    {
        void* vtable;
        uint32_t background;
        void* leftArrow;
        void* rightArrow;
        int32_t arrowSpacing;
        std::array<TextRow*, 96> rows;
        int32_t count;
        int32_t selected;
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
        int32_t animate;
        int32_t alignment;
        int32_t verticalAlignment;
        int32_t rowSpacing;
    };

    static_assert(sizeof(void*) == 4);
    static_assert(sizeof(Font) == 332);
    static_assert(offsetof(Font, scale) == 48);
    static_assert(offsetof(Font, glyphs) == 52);
    static_assert(offsetof(Font, effect) == 64);
    static_assert(offsetof(Font, glyphMap) == 75);
    static_assert(sizeof(TextProperties) == 380);
    static_assert(offsetof(TextProperties, focusedScale) == 336);
    static_assert(sizeof(PauseRow) == 24);
    static_assert(sizeof(PauseMenu) == 384);
    static_assert(offsetof(PauseMenu, selected) == 6);
    static_assert(offsetof(PauseMenu, count) == 9);
    static_assert(offsetof(PauseMenu, rows) == 24);
    static_assert(sizeof(TextRow) == 84);
    static_assert(offsetof(TextRow, command) == 8);
    static_assert(offsetof(TextRow, text) == 48);
    static_assert(offsetof(TextRow, properties) == 80);
    static_assert(sizeof(List) == 444);
    static_assert(offsetof(List, rows) == 20);
    static_assert(offsetof(List, count) == 404);
    static_assert(offsetof(List, selected) == 408);

    // Machine cleanup, not inferred Hex-Rays pseudo-arguments: 470F10 uses
    // ECX + one stack argument and RET4; its PUSH EBP preserves the register.
    using ListInput = int (__thiscall*)(List*, uint32_t);
    using ListConstructor = List* (__thiscall*)(List*, int, int, int, int, int, int, int, int);
    using TextConstructor = TextRow* (__thiscall*)(TextRow*, int, const char*, TextProperties*);
    using PauseConstructor = PauseMenu* (__thiscall*)(PauseMenu*, int, int, char, int16_t, int16_t, int);
    using PauseInput = char (__thiscall*)(PauseMenu*);
    using PauseReadInput = int (__cdecl*)(int*, int*, int*, int*);
    using OptionsDraw = void (__thiscall*)(void*, int, int);
    using OptionsDelete = void* (__thiscall*)(void*, unsigned);
    using FontWidth = int (__thiscall*)(Font*, const char*);
    // 44A4E0 reads ECX and preserves it across 44A560/44A510, despite the
    // stale stdcall decompiler annotation; machine cleanup is RET4.
    using FontHeight = int (__thiscall*)(Font*, const char*);
    using FontDraw = char (__thiscall*)(Font*, int, int, const char*, int);
}

namespace THPS2Menu
{
    struct Addresses
    {
        uintptr_t direction, optionsCommand, pauseSelect, titleConstruct, titleDelete, titleInput, titleUpdate, pauseCenter,
            pauseDelete, pauseInput, pauseDraw, pauseRead, fontDraw, pauseDispatch, textConstruct, listAdd, listSelect, pauseAdd,
            fontWidth, fontHeight, modal, present, world, allocate, free, listDelete, textDelete, listConstruct, textSet,
            listLayout, listInput, listRefresh, listUpdate, pauseConstruct;
        uintptr_t titleFinalReturn;
        uintptr_t titleTable, listTable, propertiesTable, fontTable, rowTable, pauseTable;
    };

    inline const Addresses& Game()
    {
        static const Addresses game = []
        {
            auto at = [](const char* bytes) { return reinterpret_cast<uintptr_t>(hook::pattern(bytes).get_first()); };
            auto operand = [](const char* bytes, ptrdiff_t offset) { return *hook::pattern(bytes).get_first<uintptr_t>(offset); };
            Addresses a{};
            a.direction = at("6A 06 6A 00 6A 00 E8 ? ? ? ? 83 C4 0C 83 F8 FF 75 0E");
            a.optionsCommand = at("53 8B D9 56 33 F6 8B 83 A8 00 00 00");
            a.pauseSelect = at("8A 44 24 04 56 88 41 06 33 F6 8D 41");
            a.titleConstruct = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 20 53 55 56 57 6A FF 33");
            a.titleDelete = at("56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 90 90 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 53 8B D9 56 57 89 5C 24 0C C7 03 ? ? ? ? BF ? ? ? ? 83 C9 FF 33 C0 C7 05 ? ? ? ? F1");
            a.titleInput = at("8B D1 8B 82 6C 09 00 00 83 C0 EC 83");
            a.titleUpdate = at("51 53 55 8B E9 56 57 8B 85 70 09 00");
            a.pauseCenter = at("33 D2 56 8A 51 09 57 33 F6 33 FF 85");
            a.pauseDelete = at("56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 90 90 C7 01 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 33");
            a.pauseInput = at("53 56 57 8B F1 E8 ? ? ? ? 33 C0");
            a.pauseDraw = at("53 55 57 8B F9 8A 47 04 50 E8 ? ? ? ? 8A");
            a.pauseRead = at("A1 ? ? ? ? 33 C9 3B C1 74 10 39");
            a.fontDraw = at("81 EC 14 04 00 00 53 55 8B E9 56 57");
            a.pauseDispatch = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 50 A0");
            a.textConstruct = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 57 8B");
            a.listAdd = at("53 56 8B D9 57 BF ? ? ? ? 83 C9 FF 33 C0 C7 05 ? ? ? ? E4");
            a.listSelect = at("53 8B D9 56 57 8B 83 98 01 00 00 6A");
            a.pauseAdd = at("53 56 8B D9 57 BF ? ? ? ? 83 C9 FF 33 C0 C7 05 ? ? ? ? 0A");
            a.fontWidth = at("53 8B 5C 24 08 56 57 8A 03 33 FF 84");
            a.fontHeight = at("53 56 57 8B 7C 24 10 8B F1 57 E8 ? ? ? ? 57");
            a.modal = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 55 8B 2D ? ? ? ? 56 57 C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? 8B 0D ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 00");
            a.present = at("A1 ? ? ? ? 33 C9 8A 0D ? ? ? ? 50 A1 ? ? ? ? 8B D0 25 00 FF 00 FF 81 E2 FF 00 00 00 C1 E2 10 0B CA 0B C8 51 E8 ? ? ? ? 6A");
            a.world = at("A1 ? ? ? ? 53 33 DB 3B C3 74 33");
            a.allocate = at("56 57 8B 7C 24 0C 6A 00 6A 01 6A 00");
            a.free = at("8B 44 24 04 56 8B 35 ? ? ? ? 50");
            a.listDelete = at("56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 90 90 C7 01 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 83 EC 08 53 55 56 8B F1 57 33");
            a.textDelete = at("56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 90 90 C7 01 ? ? ? ? E9 ? ? ? ? 90 90 90 90 90 8B 44 24 04 8B");
            a.listConstruct = at("8B D1 57 B9 60 00 00 00 33 C0 8D 7A 14 C7 02");
            a.textSet = at("53 55 56 8B D9 57 BF ? ? ? ? 83 C9 FF 33 C0 F2 AE F7 D1 2B F9 C7 05 ? ? ? ? CF");
            a.listLayout = at("83 EC 08 53 55 56 8B F1 57 33 FF 8B");
            a.listInput = at("56 57 8B 7C 24 0C 8B F1 85 FF 74 05");
            a.listRefresh = at("53 8B D9 56 33 F6 8B 83 94 01 00 00");
            a.listUpdate = at("53 56 8B F1 57 8B 4E 08 85 C9 74 25");
            a.pauseConstruct = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 55 8B E9 56 57 89 6C 24 0C 8B");
            // The title owner's deleting destructor call in the native modal loop, before the fonts are unloaded
            a.titleFinalReturn = at("89 2D ? ? ? ? E8 ? ? ? ? 85 F6 74 08 8B 06 6A 01 8B CE FF 10") + 23;
            PauseRoot.SetAddress(operand("8B 0D ? ? ? ? E8 ? ? ? ? A0", 2));
            PauseState.SetAddress(operand("A1 ? ? ? ? 56 8B 74 24 0C 3B F0", 1));
            PauseMode.SetAddress(operand("8B 0D ? ? ? ? 57 3B C8 0F 85 37", 2));
            GamePauseFont.SetAddress(operand("B9 ? ? ? ? E8 ? ? ? ? 8A 17", 1));
            TitleChoose.SetAddress(operand("A0 ? ? ? ? 84 C0 0F 85 9F 00 00", 1));
            PauseChoose.SetAddress(operand("88 1D ? ? ? ? C7 86 3C 02 00 00", 2));
            TitleBack.SetAddress(operand("A0 ? ? ? ? 84 C0 74 93 6A 00 68", 1));
            PauseBack.SetAddress(operand("88 0D ? ? ? ? 8B 3E 8B 02 0B F8", 2));
            a.titleTable = operand("C7 06 ? ? ? ? A1 ? ? ? ? 3B", 2);
            a.listTable = operand("C7 02 ? ? ? ? F3 AB 8B 4C 24 08", 2);
            a.propertiesTable = operand("C7 06 ? ? ? ? C7 86 50 01 00 00", 2);
            a.fontTable = operand("C7 00 ? ? ? ? C7 40 3C FF FF FF", 2);
            a.rowTable = operand("C7 06 ? ? ? ? 89 7E 08 89 46 50", 2);
            a.pauseTable = operand("C7 45 00 ? ? ? ? 8D 45 28 BA 0F", 3);
            return a;
        }();
        return game;
    }
}

namespace THPS2Menu
{
    enum class Diagnostic : size_t
    {
        UnsupportedImage, PinFailed, HookFailed,
        TitleContext, PauseContext, Style, Widget, Layout, Discovery,
        TitleStyle, TitleWidget, PauseStyle, PauseWidget, PauseLayout,
        Rollback, OwnerRetired, FrameReentry, Count
    };

    // Failure reasons only, each reported at most three times.
    inline void Diagnose(Diagnostic reason, const char* detail = nullptr, size_t rows = 0) noexcept
    {
        static std::array<uint8_t, static_cast<size_t>(Diagnostic::Count)> seen{};
        auto index = static_cast<size_t>(reason);
        if (index >= seen.size() || seen[index] >= 3)
            return;
        ++seen[index];
        static constexpr std::array<const char*, static_cast<size_t>(Diagnostic::Count)> names =
        {
            "unsupported image", "module pin failed", "hook transaction failed",
            "title context unavailable", "pause context unavailable", "style unavailable", "widget refused", "layout refused", "discovery retired",
            "title style unavailable", "title widget refused", "pause style unavailable", "pause widget refused", "pause geometry refused",
            "construction rollback", "native owner retired", "frame reentry rejected"
        };
        if (!detail)
            detail = names[index];
        char line[256] = {};
        auto count = std::snprintf(line, sizeof(line), "THPS2 native menu: %s; rows=%zu\n", detail, rows);
        if (count > 0)
            OutputDebugStringA(line);
    }
}

namespace THPS2Menu
{
    // THPS2 does not have THUG's unsigned glyph lookup. Never send raw UTF-8
    // or another game's encoding to its signed-byte map. Fold western letters
    // and punctuation to ASCII; unsupported codepoints get one visible '?'.
    inline std::string PlainText(std::string_view utf8)
    {
        if (utf8.size() > 16384) return "?";
        std::string result;
        for (size_t i = 0; i < utf8.size();)
        {
            uint32_t code = static_cast<uint8_t>(utf8[i++]);
            if (code >= 0x80)
            {
                size_t count = code >= 0xC2 && code <= 0xDF ? 1 : code >= 0xE0 && code <= 0xEF ? 2 : code >= 0xF0 && code <= 0xF4 ? 3 : 0;
                uint32_t minimum = count == 1 ? 0x80 : count == 2 ? 0x800 : 0x10000;
                bool valid = count && count <= utf8.size() - i;
                uint32_t decoded = code & (count == 1 ? 0x1F : count == 2 ? 0x0F : 0x07);
                for (size_t k = 0; valid && k < count; ++k)
                {
                    auto byte = static_cast<uint8_t>(utf8[i + k]);
                    valid = (byte & 0xC0) == 0x80;
                    decoded = (decoded << 6) | (byte & 0x3F);
                }
                if (valid)
                {
                    valid = decoded >= minimum && decoded <= 0x10FFFF && !(decoded >= 0xD800 && decoded <= 0xDFFF);
                    if (valid)
                        i += count;
                }
                code = valid ? decoded : '?';
            }
            if (code == 0x2018 || code == 0x2019)
                code = '\'';
            else if (code == 0x201C || code == 0x201D)
                code = '"';
            else if (code == 0x2013 || code == 0x2014)
                code = '-';
            else if (code == 0xA0 || code == '\r' || code == '\n' || code == '\t')
                code = ' ';
            if (code == 0xDF) { result += "ss"; continue; }
            if (code == 0xC6 || code == 0xE6) { result += code == 0xC6 ? "AE" : "ae"; continue; }
            if (code == 0x152 || code == 0x153) { result += code == 0x152 ? "OE" : "oe"; continue; }
            if (code == 0xD8 || code == 0xF8) code = code == 0xD8 ? 'O' : 'o';
            if (code >= 0x80 && code <= 0xFFFF)
            {
                wchar_t original = wchar_t(code), decomposed[8]{};
                if (FoldStringW(MAP_COMPOSITE, &original, 1, decomposed, int(std::size(decomposed))) > 0 && decomposed[0] >= 32 && decomposed[0] <= 126)
                    code = uint32_t(decomposed[0]);
            }
            if (code < 0x20 || code > 0x7E)
                code = '?';
            result += static_cast<char>(code);
        }
        return result;
    }

    // The native font treats missing reserved characters as formatting controls
    // and other missing characters into blank advance. Private map copies use
    // a visible '?' for unsupported printable characters; spaces retain their
    // native blank advance. Native game fonts and shared drawers stay intact.
    inline bool LiteralFont(Native::Font& font)
    {
        auto replacement = font.glyphMap[static_cast<size_t>('?')];
        if (replacement == 255 || !font.glyphs || font.effect)
            return false;
        for (size_t code = 0x21; code <= 0x7E; ++code)
        {
            auto& mapped = font.glyphMap[code];
            if (mapped == 255)
                mapped = replacement;
        }
        return true;
    }

    inline std::string FitText(const std::string& text, size_t maximum)
    {
        if (text.size() <= maximum) return text;
        if (maximum < 3) return text.substr(0, maximum);
        return text.substr(0, maximum - 3) + "...";
    }

    // Preserve all converted text in continuation rows. Callers must bound the
    // resulting native layout and fail closed if the complete text cannot fit;
    // a 31-byte native button buffer is not permission to silently truncate.
    inline std::vector<std::string> TextLines(const std::string& text, size_t maximum)
    {
        std::vector<std::string> lines;
        if (!maximum)
            return lines;
        for (size_t offset = 0; offset < text.size();)
        {
            size_t count = std::min(maximum, text.size() - offset);
            if (count < text.size() - offset)
            {
                auto space = text.rfind(' ', offset + count - 1);
                if (space != std::string::npos && space > offset)
                    count = space - offset;
            }
            lines.emplace_back(text.substr(offset, count));
            offset += count;
            if (offset < text.size() && text[offset] == ' ')
                ++offset;
        }
        if (lines.empty())
            lines.emplace_back();
        return lines;
    }
}

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
        injector::WriteMemory<T>(address, value, true);
    }

    inline void* AllocateNative(size_t size)
    {
        return nativeReady ? FunctionAt<void* (__cdecl*)(size_t)>(Game().allocate)(size) : nullptr;
    }

    inline void FreeNative(void* value)
    {
        if (value && nativeReady)
            FunctionAt<void (__cdecl*)(void*)>(Game().free)(value);
    }

    template<uintptr_t Addresses::* Destructor>
    struct NativeDelete
    {
        template<typename T>
        void operator()(T* value) const
        {
            if (value && nativeReady)
                FunctionAt<void* (__thiscall*)(void*, unsigned)>(Game().*Destructor)(value, 1);
        }
    };

    using ListPointer = std::unique_ptr<Native::List, NativeDelete<&Addresses::listDelete>>;
    using TextPointer = std::unique_ptr<Native::TextRow, NativeDelete<&Addresses::textDelete>>;
    using PausePointer = std::unique_ptr<Native::PauseMenu, NativeDelete<&Addresses::pauseDelete>>;

    inline bool FontUsable(const Native::Font& font)
    {
        return font.glyphs && !font.effect && font.spacing >= 0 && font.spacing <= 64 && font.scale > 0 && font.scale <= 8192;
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
            if (!nativeReady || lines.empty() || lines.size() > 12 || style.vtable != reinterpret_cast<void*>(Game().propertiesTable) || style.font.vtable != reinterpret_cast<void*>(Game().fontTable) || !FontUsable(style.font) || std::none_of(lines.begin(), lines.end(), [](const DisplayLine& line) { return line.enabled; }))
                return false;
            Clear();
            cells.reserve(lines.size());
            auto allocation = static_cast<Native::List*>(AllocateNative(sizeof(Native::List)));
            if (!allocation)
                return false;
            NativeWrite(allocation, Native::List{});
            auto created = FunctionAt<Native::ListConstructor>(Game().listConstruct)(allocation, 64, 30, 384, 180, 0, 0, 2, 0);
            if (created != allocation || allocation->vtable != reinterpret_cast<void*>(Game().listTable))
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
                auto result = FunctionAt<Native::TextConstructor>(Game().textConstruct)(row, line.command, line.text.c_str(), cell->properties.get());
                if (result != row || row->vtable != reinterpret_cast<void*>(Game().rowTable))
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
                    FunctionAt<int (__thiscall*)(Native::TextRow*, const char*)>(Game().textSet)(row, line.text.c_str());
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
                FunctionAt<Native::TextRow* (__thiscall*)(Native::List*, Native::TextRow*)>(Game().listAdd)(list.get(), row);
                cells.push_back(std::move(cell));
            }

            FunctionAt<int (__thiscall*)(Native::List*)>(Game().listLayout)(list.get());
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
                    FunctionAt<int (__thiscall*)(Native::List*, int)>(Game().listSelect)(list.get(), command);
                    return;
                }
            }
        }

        void Input(int direction)
        {
            if (list && list->count > 0 && list->selected >= 0 && list->selected < list->count)
                FunctionAt<Native::ListInput>(Game().listInput)(list.get(), static_cast<uint32_t>(direction));
        }

        void Draw()
        {
            if (!list || list->count <= 0 || list->selected < 0 || list->selected >= list->count)
                return;
            FunctionAt<int (__thiscall*)(Native::List*)>(Game().listRefresh)(list.get());
            FunctionAt<int (__thiscall*)(Native::List*)>(Game().listUpdate)(list.get());
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
            auto result = FunctionAt<Native::PauseConstructor>(Game().pauseConstruct)(allocation, 256, 0, 0, 256, 256, 16);
            if (result != allocation || allocation->vtable != reinterpret_cast<void*>(Game().pauseTable))
            {
                FreeNative(allocation);
                return false;
            }
            menu.reset(allocation);
            for (size_t i = 0; i < lines.size(); ++i)
            {
                FunctionAt<char (__thiscall*)(Native::PauseMenu*, const char*)>(Game().pauseAdd)(menu.get(), lines[i].text.c_str());
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
                    FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(Game().pauseSelect)(menu.get(), static_cast<char>(i));
                    break;
                }
            }
            FunctionAt<int (__thiscall*)(Native::PauseMenu*)>(Game().pauseCenter)(menu.get());
            // Keep the list below the PAUSED title
            if (menu->y < 68 && 68 + menu->rowSpacing * (menu->count - 1) <= 224)
                NativeWrite(&menu->y, 68);
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
                    FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(Game().pauseSelect)(menu.get(), static_cast<char>(i));
                    return;
                }
            }
        }
    };
}

namespace THPS2Menu
{
    enum class ItemKind { Page, Row, Details, Back, Previous, Next };

    // Values of restart settings as the game started with them (other plugins: when first seen)
    inline std::map<std::pair<HMODULE, std::string>, std::string> restartBaseline;

    struct Item
    {
        ItemKind kind = ItemKind::Back;
        size_t page = 0;
        RWGTAMenu::Row row;
        std::string label;
        bool enabled = true;
    };

    struct View
    {
        static constexpr size_t Root = std::numeric_limits<size_t>::max();
        static constexpr size_t PageLinks = 3;
        // The pause list starts below the game's PAUSED title
        static constexpr size_t TitleLines = 7, PauseLines = 5;
        static constexpr int PhysicalPrevious = 0x7FF0;
        static constexpr int PhysicalNext = 0x7FF1;
        static constexpr int PhysicalBack = 0x7FF2;

        size_t page = Root;
        size_t rootStart = 0;
        size_t physical = 0;
        bool details = false;
        size_t returnPhysical = 0;
        int returnSelected = -1;
        std::string detailText;
        std::vector<Item> items;
        std::vector<DisplayLine> text;
        std::vector<size_t> starts;
        bool restart = false;
        size_t contentLines = TitleLines;

        void Reset()
        {
            page = Root;
            rootStart = physical = 0;
            details = false;
            returnPhysical = 0;
            returnSelected = -1;
            detailText.clear();
            items.clear();
            text.clear();
            starts.clear();
        }

        // Foreign Visible/Get/GetState returns can retire or reopen the host.
        // Copy row names before reads, then validate the exact host snapshot
        // immediately after each foreign return before touching another row.
        bool Prepare(RWGTAMenu::Model& model, bool pause, const std::function<bool()>& current)
        {
            if (!current() || !model.IsOpen())
                return false;
            std::vector<Item> next;
            std::string heading = model.Text("menu.plugins");
            if (details)
            {
                next.push_back({ ItemKind::Back, 0, {}, detailText, false });
                heading = model.Text("menu.details");
            }
            else if (page == Root)
            {
                auto count = model.Pages().size();
                if (rootStart >= count)
                    rootStart = count ? ((count - 1) / PageLinks) * PageLinks : 0;
                for (size_t i = rootStart; i < std::min(rootStart + PageLinks, count); ++i)
                    next.push_back({ ItemKind::Page, i, {}, model.Pages()[i].title, true });
                if (rootStart)
                    next.push_back({ ItemKind::Previous, 0, {}, model.Text("menu.previous"), true });
                if (rootStart + PageLinks < count)
                    next.push_back({ ItemKind::Next, 0, {}, model.Text("menu.next"), true });
            }
            else
            {
                if (page >= model.Pages().size())
                    return false;
                heading = model.Pages()[page].title;
                auto rows = model.VisibleRows(page);
                if (!current())
                    return false;
                for (const auto* described : rows)
                {
                    auto row = *described;
                    auto label = model.Label(row);
                    if (!current())
                        return false;
                    auto value = model.Value(row);
                    if (!current())
                        return false;
                    auto enabled = model.Enabled(row, pause);
                    if (!current())
                        return false;
                    if (!value.empty())
                        label += ": " + value;
                    auto help = model.Help(row);
                    if (!current())
                        return false;
                    next.push_back({ ItemKind::Row, page, row, std::move(label), enabled });
                    if (!help.empty())
                        next.push_back({ ItemKind::Details, page, row, model.Text("menu.details"), true });
                }
            }
            if (!current())
                return false;
            // The restart notice disappears again when the values are set back
            bool pending = false;
            for (const auto& candidate : model.Pages())
            {
                for (const auto& row : candidate.rows)
                {
                    if (!row.plugin || !row.setting || row.setting->apply != "restart" || !row.setting->HasValue())
                        continue;
                    auto value = OtherPlugins::Get(*row.plugin, *row.setting);
                    if (!current())
                        return false;
                    auto baseline = restartBaseline.emplace(std::make_pair(row.plugin->module, row.setting->id), value).first;
                    pending |= !asi_settings::text::IEquals(baseline->second, value);
                }
            }
            restart = pending;
            contentLines = pause ? PauseLines : TitleLines;
            std::vector<DisplayLine> nextText;
            for (const auto& line : TextLines(PlainText(heading), 31))
                nextText.push_back({ line, -1, false, false, true });
            for (size_t i = 0; i < next.size(); ++i)
            {
                auto lines = TextLines(PlainText(next[i].label), 31);
                for (size_t k = 0; k < lines.size(); ++k)
                    nextText.push_back({ lines[k], k == 0 ? static_cast<int>(i) : -1, k == 0 && next[i].enabled, k != 0, false });
            }
            if (!current())
                return false;
            items = std::move(next);
            text = std::move(nextText);

            // Content lines, two help rows and previous/next/back stay within
            // the native positions. Full values retain their nonselectable
            // continuation lines.
            starts.clear();
            for (size_t start = 0; start < text.size(); start += contentLines)
                starts.push_back(start);
            if (starts.empty())
                starts.push_back(0);
            if (physical >= starts.size())
                physical = starts.size() - 1;
            return true;
        }

        std::vector<DisplayLine> Lines(RWGTAMenu::Model& model, int selected = -1) const
        {
            if (starts.empty() || physical >= starts.size())
                return {};
            auto start = starts[physical];
            auto count = std::min(contentLines, text.size() - start);
            std::vector<DisplayLine> result(text.begin() + start, text.begin() + start + count);
            std::string help;
            if (!details && selected >= 0 && static_cast<size_t>(selected) < items.size() && items[selected].kind == ItemKind::Row)
                help = PlainText(model.Help(items[selected].row));
            if (restart) result.push_back({ FitText(PlainText(model.Text("menu.restartwarning")), 31), -1, false });
            if (!help.empty())
            {
                auto lines = TextLines(help, 31);
                auto maximum = restart ? size_t(1) : size_t(2);
                for (size_t i = 0; i < std::min(maximum, lines.size()); ++i)
                {
                    auto line = lines[i];
                    if (i + 1 == maximum && lines.size() > maximum) line = FitText(line + "...", 31);
                    result.push_back({ std::move(line), -1, false });
                }
            }
            if (physical)
                result.push_back({ PlainText(model.Text("menu.previous")), PhysicalPrevious, true });
            if (physical + 1 < starts.size())
                result.push_back({ PlainText(model.Text("menu.next")), PhysicalNext, true });
            result.push_back({ PlainText(model.Text("menu.back")), PhysicalBack, true });
            return result;
        }
    };
}

namespace THPS2Menu
{
    // 44BA30 -> 473480/473970 -> 44A010. This is the pause backend,
    // not the title TextProperties backend. Its native glyph objects remain
    // borrowed; the host must retain the native font owner for the whole draw.
    class PauseFont
    {
        Native::Font measured{};
        std::array<uint8_t, 256> sourceMap{};
        bool ready = false;

    public:
        void Clear() { measured = {}; sourceMap = {}; ready = false; }

        bool Prepare(const Native::Font& source, const std::vector<DisplayLine>& lines, int scale = 4096, int maximumHeight = 16)
        {
            Clear();
            if (!nativeReady || source.vtable != reinterpret_cast<void*>(Game().fontTable) || !FontUsable(source) || lines.empty() || lines.size() > 12 || scale < 4096 || scale > 8192 || maximumHeight < 8 || maximumHeight > 24)
                return false;
            measured = source;
            sourceMap = source.glyphMap;
            // All private rows use scale256; 473480 multiplies this by16.
            measured.scale = scale;
            measured.proportional = 0;
            measured.effect = nullptr;
            measured.alignment = 1;
            measured.verticalAlignment = 1;
            measured.shadowX = measured.shadowY = 0;
            measured.blue = 255; // Byte33 shadow switch is zero for RGB0..255.
            if (!LiteralFont(measured))
                return false;

            for (const auto& line : lines)
            {
                if (line.text.empty() || line.text.size() > 31 || std::any_of(line.text.begin(), line.text.end(), [](char code) { return static_cast<uint8_t>(code) >= 0x80 || code == 0; }))
                    return false;
                const int width = FunctionAt<Native::FontWidth>(Game().fontWidth)(&measured, line.text.c_str());
                const int height = FunctionAt<Native::FontHeight>(Game().fontHeight)(&measured, line.text.c_str());
                // Centered at256 in the native512x240 plane, with margins.
                // Space measurement is conservative versus the native drawer.
                if (width <= 0 || width > 420 || height < 8 || height > maximumHeight)
                    return false;
            }
            ready = true;
            return true;
        }

        bool Current(const Native::Font& source) const
        {
            // Scale/color/alignment are changed by the stock pause renderer.
            // The owned literal map and measured geometry only remain valid
            // while the underlying glyph owner/map/spacing are unchanged.
            return ready && source.vtable == measured.vtable && source.glyphs == measured.glyphs && source.glyphMap == sourceMap && !source.effect && source.spacing == measured.spacing;
        }

        bool DrawCopy(const Native::Font& source, Native::Font& output) const
        {
            // The stock renderer enlarges the selected row and sets the colours
            if (!Current(source) || source.scale < measured.scale || source.scale > 8192)
                return false;
            output = measured;
            output.scale = source.scale;
            output.alpha = source.alpha;
            output.red = source.red;
            output.green = source.green;
            output.blue = source.blue;
            return true;
        }
    };
}

namespace THPS2Menu
{
    enum class Backend { Title, Pause };

    inline std::string MenuLanguage()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        // Auto uses English until a native locale getter is established.
        return language.empty() || language == "auto" ? "en" : language;
    }

    // The native hooks supply an exact owner predicate. This game-local
    // state machine uses the native hooks' exact owner and frame predicates.
    // C++ retention does not retain the native owner: deleting hooks must
    // invalidate immediately and defer native deletion through its full frame.
    struct Host : std::enable_shared_from_this<Host>
    {
        Backend backend;
        void* owner = nullptr;
        HMODULE module = nullptr;
        uint64_t generation = 1;
        bool valid = true;
        bool open = false;
        bool entering = false;
        bool busy = false;
        bool closeRequested = false;
        int helpSelected = -1;
        std::function<bool(const Host&)> nativeCurrent;
        std::function<bool(Host&)> enterNative;
        std::function<void(Host&)> leaveNative;
        Native::TextProperties* titleStyle = nullptr;
        Native::Font* pauseSource = nullptr;
        RWGTAMenu::Model model;
        View view;
        std::unique_ptr<TextList> title;
        std::unique_ptr<PauseList> pause;
        PauseFont pauseFont;

        explicit Host(Backend kind) : backend(kind) {}
        Host(const Host&) = delete;
        Host& operator=(const Host&) = delete;
        Host(Host&&) = delete;
        Host& operator=(Host&&) = delete;

        struct Snapshot
        {
            std::shared_ptr<Host> host;
            uint64_t generation;
            bool open;
            explicit Snapshot(const std::shared_ptr<Host>& value)
                : host(value), generation(value->generation), open(value->open) {}
            bool Current() const
            {
                return host->valid && host->generation == generation && host->open == open && host->nativeCurrent && host->nativeCurrent(*host);
            }
        };

        struct Busy
        {
            Host& host;
            bool previous;
            explicit Busy(Host& value) : host(value), previous(value.busy) { host.busy = true; }
            ~Busy() { host.busy = previous; }
        };

        int Selected() const
        {
            return backend == Backend::Title ? (title ? title->SelectedCommand() : -1) : (pause ? pause->SelectedCommand() : -1);
        }

        const void* Widget() const
        {
            return backend == Backend::Title ? static_cast<const void*>(title ? title->Get() : nullptr) : static_cast<const void*>(pause ? pause->Get() : nullptr);
        }

        void Retire()
        {
            // Native storage reclamation is deliberately separate. Close now
            // aborts a shared-model preset/read burst on owner destruction.
            valid = false;
            ++generation;
            model.Close();
            closeRequested = true;
        }

        bool Populate(int selected = -1)
        {
            auto retained = shared_from_this();
            ++generation; // Any old physical-row input is now obsolete.
            Snapshot snapshot(retained);
            if (!snapshot.Current() || !open || !model.IsOpen())
                return false;
            Busy scope(*this);
            auto nextView = view;
            if (!nextView.Prepare(model, backend == Backend::Pause, [&snapshot] { return snapshot.Current(); }))
                return false;
            if (selected < 0 && !nextView.starts.empty())
            {
                auto start = nextView.starts[nextView.physical];
                for (size_t i = start; i < std::min(nextView.text.size(), start + nextView.contentLines); ++i)
                    if (nextView.text[i].enabled) { selected = nextView.text[i].command; break; }
            }
            auto lines = nextView.Lines(model, selected);
            if (!snapshot.Current())
                return false;
            if (backend == Backend::Title)
            {
                if (!titleStyle)
                {
                    Diagnose(Diagnostic::TitleStyle);
                    return false;
                }
                auto next = std::make_unique<TextList>();
                if (!next->Build(lines, *titleStyle))
                {
                    Diagnose(Diagnostic::TitleWidget);
                    return false;
                }
                if (!snapshot.Current())
                    return false;
                next->Select(selected);
                title = std::move(next);
            }
            else
            {
                if (!pauseSource)
                {
                    Diagnose(Diagnostic::PauseStyle);
                    return false;
                }
                PauseFont nextFont;
                if (!nextFont.Prepare(*pauseSource, lines))
                {
                    Diagnose(Diagnostic::PauseLayout);
                    return false;
                }
                auto next = std::make_unique<PauseList>();
                if (!next->Build(lines))
                {
                    Diagnose(Diagnostic::PauseWidget);
                    return false;
                }
                // 44B6C0 centers baselines in240 pixels. Width is centered512;
                // measured ascent/descent fit each16-pixel private row.
                const auto menu = next->Get();
                if (menu->x != 256 || menu->y < 16 || menu->rowSpacing != 16 || menu->y + 16 * (menu->count - 1) > 224 || !snapshot.Current())
                {
                    Diagnose(Diagnostic::PauseLayout);
                    return false;
                }
                next->Select(selected);
                pauseFont = nextFont;
                pause = std::move(next);
            }
            if (!snapshot.Current())
                return false;
            view = std::move(nextView);
            helpSelected = Selected();
            return snapshot.Current();
        }

        bool Refresh(int selected = -1)
        {
            if (Populate(selected))
                return true;
            // Activate may have navigated before population. Never leave old
            // visible commands bound to changed logical page/item state.
            Close();
            Diagnose(Diagnostic::Rollback, "private population refused; stock menu restored");
            return false;
        }

        bool Open()
        {
            auto retained = shared_from_this();
            Snapshot before(retained);
            if (busy || open || !before.Current())
                return false;
            Busy scope(*this);
            // Shared9c snapshots survive Close while discovery is in flight.
            model.OpenPlugins(module, MenuLanguage(), 4096);
            if (!before.Current() || !model.IsOpen())
            {
                model.Close();
                Diagnose(Diagnostic::Discovery);
                return false;
            }
            view.Reset();
            open = true;
            entering = true;
            ++generation;
            if (!Populate() || !valid || !enterNative || !enterNative(*this))
            {
                Close();
                Diagnose(Diagnostic::Rollback);
                return false;
            }
            entering = false;
            ++generation;
            Snapshot committed(retained);
            if (!committed.Current())
            {
                Close();
                Diagnose(Diagnostic::Rollback);
                return false;
            }
            return true;
        }

        void Close()
        {
            auto retained = shared_from_this();
            Busy scope(*this);
            ++generation;
            model.Close();
            if (open && valid && leaveNative)
                leaveNative(*this);
            open = false;
            entering = false;
            closeRequested = false;
            // Called only at complete modal iteration/world-frame unwind.
            title.reset();
            pause.reset();
            pauseFont.Clear();
            view.Reset();
        }

        bool Activate(int command, int direction)
        {
            auto retained = shared_from_this();
            Snapshot snapshot(retained);
            if (busy || !open || !snapshot.Current() || (command != View::PhysicalBack && Selected() != command))
                return false;
            Busy scope(*this);
            if (command == View::PhysicalBack)
            {
                if (direction)
                    return false;
                if (view.details)
                {
                    view.details = false;
                    view.physical = view.returnPhysical;
                    return Refresh(view.returnSelected);
                }
                if (view.page == View::Root)
                {
                    Close();
                    return true;
                }
                model.PageShown(view.page);
                view.page = View::Root;
                view.physical = 0;
            }
            else if (command == View::PhysicalPrevious || command == View::PhysicalNext)
            {
                if (direction)
                    return false;
                if (command == View::PhysicalPrevious && view.physical)
                    --view.physical;
                else if (command == View::PhysicalNext && view.physical + 1 < view.starts.size())
                    ++view.physical;
                else
                    return false;
            }
            else
            {
                if (command < 0 || static_cast<size_t>(command) >= view.items.size())
                    return false;
                const auto item = view.items[command];
                if (!item.enabled)
                    return false;
                if (item.kind == ItemKind::Row)
                {
                    model.Change(item.row, direction, backend == Backend::Pause);
                    if (!snapshot.Current())
                        return false;
                    return Refresh(command);
                }
                if (direction)
                    return false;
                if (item.kind == ItemKind::Details)
                {
                    auto help = model.Help(item.row);
                    if (!snapshot.Current())
                        return false;
                    view.detailText = model.Label(item.row) + ": " + help;
                    if (!snapshot.Current())
                        return false;
                    view.details = true;
                    view.returnPhysical = view.physical;
                    view.returnSelected = command;
                }
                else if (item.kind == ItemKind::Page)
                {
                    view.page = item.page;
                    model.PageShown(item.page);
                }
                else if (item.kind == ItemKind::Previous && view.rootStart >= View::PageLinks)
                    view.rootStart -= View::PageLinks;
                else if (item.kind == ItemKind::Next)
                    view.rootStart += View::PageLinks;
                else
                    return false;
                view.physical = 0;
            }
            return snapshot.Current() && Refresh();
        }
    };

    struct Pending
    {
        std::shared_ptr<Host> host;
        uint64_t generation;
        const void* widget;
        size_t page;
        size_t rootStart;
        size_t physical;
        int command;
        int selected;
        int direction;
        bool opening;

        bool Current() const
        {
            return Host::Snapshot(host).Current() && host->generation == generation && host->Widget() == widget && host->view.page == page && host->view.rootStart == rootStart && host->view.physical == physical && (opening ? !host->open : host->open && host->Selected() == selected);
        }
    };

    inline std::deque<Pending> pending;
    inline bool draining = false;

    inline bool Queue(const std::shared_ptr<Host>& host, int command, int direction, bool opening = false)
    {
        Host::Snapshot snapshot(host);
        if (!snapshot.Current() || pending.size() >= 32 || (opening ? host->open || host->busy : !host->open || (command != View::PhysicalBack && host->Selected() != command)))
            return false;
        if (!opening && command == View::PhysicalBack && host->busy && direction == 0)
        {
            // Abort foreign Set/preset bursts immediately; rebuild/stock return
            // stays deferred until the full native outer iteration unwinds.
            host->model.Close();
            ++host->generation;
            host->closeRequested = true;
            return true;
        }
        pending.push_back({ host, host->generation, host->Widget(), host->view.page, host->view.rootStart, host->view.physical, command, host->Selected(), direction, opening });
        return true;
    }

    inline void Drain(Backend backend)
    {
        if (draining)
            return;
        struct Scope { Scope() { draining = true; } ~Scope() { draining = false; } } scope;
        auto work = std::move(pending);
        pending.clear();
        for (auto& action : work)
        {
            if (action.host->backend != backend)
            {
                pending.push_back(std::move(action));
                continue;
            }
            if (!action.Current())
                continue;
            if (action.opening)
                action.host->Open();
            else
                action.host->Activate(action.command, action.direction);
        }
    }
}

namespace THPS2Menu
{
    inline constexpr int CategoryCommand = 0x6F01;

    template<typename T>
    T* Field(void* owner, size_t offset)
    {
        auto address = static_cast<uint8_t*>(owner) + offset;
        return reinterpret_cast<T*>(address);
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
            if (!owner || *static_cast<void**>(owner) != reinterpret_cast<void*>(Game().titleTable))
                return false;
            if (*Field<Native::List*>(owner, 844) != base || !base || base->vtable != reinterpret_cast<void*>(Game().listTable) || base->count <= 0 || base->count > 96 || base->selected < 0 || base->selected >= base->count)
                return false;
            if (*Field<Native::TextProperties*>(owner, 860) != style || !style || style->vtable != reinterpret_cast<void*>(Game().propertiesTable) || style->font.vtable != reinterpret_cast<void*>(Game().fontTable))
                return false;
            if (entry && (*Field<Native::TextRow*>(owner, 904) != entry || entry->vtable != reinterpret_cast<void*>(Game().rowTable) || entry->command != CategoryCommand || entry->properties != style))
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
            return row && row->enabled ? row->command : -1;
        }

        bool Append(void* value, const std::string& caption)
        {
            owner = value;
            if (!owner || *static_cast<void**>(owner) != reinterpret_cast<void*>(Game().titleTable))
                return false;
            base = *Field<Native::List*>(owner, 844);
            style = *Field<Native::TextProperties*>(owner, 860);
            if (!Alive() || base->count > 12 || *Field<Native::TextRow*>(owner, 904) || caption.empty() || caption.size() > 31 || !CaptionGlyphs(style->font, caption))
                return false;
            for (int i = 0; i < base->count; ++i)
            {
                const auto row = base->rows[i];
                if (!row || row->command == CategoryCommand)
                    return false;
            }
            auto allocation = static_cast<Native::TextRow*>(AllocateNative(sizeof(Native::TextRow)));
            if (!allocation)
                return false;
            NativeWrite(allocation, Native::TextRow{});
            const auto created = FunctionAt<Native::TextConstructor>(Game().textConstruct)(allocation, CategoryCommand, caption.c_str(), style);
            if (created != allocation || allocation->vtable != reinterpret_cast<void*>(Game().rowTable))
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
            FunctionAt<int (__thiscall*)(Native::List*, Native::TextRow*)>(Game().listAdd)(base, allocation);
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
                FunctionAt<int (__thiscall*)(Native::List*, int)>(Game().listSelect)(base, CategoryCommand);
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
            const auto root = static_cast<Native::PauseMenu*>(PauseRoot.get());
            return base && root == base && base->vtable == reinterpret_cast<void*>(Game().pauseTable) && base->count && base->count <= 15 && base->selected < base->count;
        }

        bool Eligible() const
        {
            return Alive() && PauseState == 1 && PauseMode == 1 && entry < base->count && base->rows[entry].text == caption.c_str();
        }

        bool Append(Native::PauseMenu* value, std::string label)
        {
            base = value;
            if (!Alive() || base->count > 7 || base->rowSpacing <= 0 || base->rowSpacing > 24 || label.empty() || label.size() > 31 || std::any_of(label.begin(), label.end(), [](char code) { return static_cast<uint8_t>(code) >= 0x80 || code == 0; }))
                return false;
            auto font = reinterpret_cast<const Native::Font*>(GamePauseFont.get_ptr());
            PauseFont metrics;
            const auto& slot = base->rows[base->count];
            const int largestScale = std::max<int16_t>(slot.focusedScale, slot.unfocusedScale);
            if (font->proportional != 0 || !CaptionGlyphs(*font, label) || largestScale < 256 || largestScale > 512 || !metrics.Prepare(*font, { { label, CategoryCommand, true } }, 16 * largestScale, base->rowSpacing))
                return false;
            // Stock pause rows are maintained by pointer-labelled native
            // lookups every frame. Append; never replace those original rows.
            caption = std::move(label);
            entry = base->count;
            FunctionAt<char (__thiscall*)(Native::PauseMenu*, const char*)>(Game().pauseAdd)(base, caption.c_str());
            if (base->count != entry + 1 || base->rows[entry].text != caption.c_str())
                return false;
            return true;
        }
    };
}

namespace THPS2Menu::Unwind
{
    inline unsigned modalDepth = 0;
    inline unsigned worldDepth = 0;
    inline unsigned inputDepth = 0;
    inline unsigned updateDepth = 0;
    inline unsigned drawDepth = 0;
    inline unsigned presentDepth = 0;
    inline unsigned pauseDispatchDepth = 0;
    inline bool servicing = false;
    inline std::function<void()> titleFrame;
    inline std::function<void()> worldFrame;
    inline std::function<void()> modalFinished;
    inline SafetyHookInline modalHook;
    inline SafetyHookInline presentHook;
    inline SafetyHookInline worldHook;

    struct Depth
    {
        unsigned& counter;
        explicit Depth(unsigned& value) : counter(value) { ++counter; }
        ~Depth() { --counter; }
    };

    inline bool OutsideNativeRows()
    {
        return !inputDepth && !updateDepth && !drawDepth && !pauseDispatchDepth;
    }

    inline void Service(const std::function<void()>& action) noexcept
    {
        if (!action || servicing || !OutsideNativeRows())
            return;
        struct Scope { Scope() { servicing = true; } ~Scope() { servicing = false; } } scope;
        action();
    }

    inline int __cdecl Modal()
    {
        int result;
        {
            Depth scope(modalDepth);
            result = modalHook.unsafe_ccall<int>();
        }
        if (!modalDepth && !worldDepth)
            Service(modalFinished);
        return result;
    }

    inline int __cdecl Present()
    {
        // Called once per title menu iteration after input, drawing and presentation.
        // Queued title actions run only here; the modal return is teardown.
        int result;
        {
            Depth scope(presentDepth);
            result = presentHook.unsafe_ccall<int>();
        }
        if (!presentDepth && modalDepth == 1 && !worldDepth)
            Service(titleFrame);
        return result;
    }

    inline int __cdecl World()
    {
        int result;
        {
            // Covers the game's scene and HUD work; pause callbacks only enqueue.
            Depth scope(worldDepth);
            result = worldHook.unsafe_ccall<int>();
        }
        if (!worldDepth && !modalDepth)
            Service(worldFrame);
        return result;
    }

    inline void Reset()
    {
        worldHook.reset();
        presentHook.reset();
        modalHook.reset();
    }

    inline bool Install()
    {
        // Caller has pinned the module.
        // The host commits nativeReady only after all hook creation succeeds.
        modalHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().modal), reinterpret_cast<void*>(&Modal));
        presentHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().present), reinterpret_cast<void*>(&Present));
        worldHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().world), reinterpret_cast<void*>(&World));
        return true;
    }
}

namespace THPS2Menu
{
    // Native Options and pause hosts for the guarded installed executable.
    struct TitleRecord
    {
        TitleContext context;
        std::shared_ptr<Host> host;
        unsigned deleteFlags = 0;
    };
    struct PauseRecord
    {
        PauseContext context;
        std::shared_ptr<Host> host;
        bool deletionRequested = false;
        unsigned deleteFlags = 0;
    };
    inline HMODULE ownModule = nullptr;
    inline std::map<void*, std::shared_ptr<TitleRecord>> titles;
    inline std::map<void*, std::shared_ptr<PauseRecord>> pauses;
    inline std::vector<std::shared_ptr<TitleRecord>> retiredTitles;
    inline std::shared_ptr<PauseRecord> painting;
    inline SafetyHookInline titleCtorHook, titleDeleteHook, titleInputHook, titleUpdateHook;
    inline SafetyHookInline pauseCenterHook, pauseDeleteHook, pauseInputHook, pauseDrawHook, pauseReadHook, fontDrawHook, pauseDispatchHook;
    inline std::function<void()> Frame;

    inline std::string Caption()
    {
        // No independently proved native locale getter: honest English fallback.
        return PlainText(RWGTAMenu::PluginText(FixSettings(), MenuLanguage(), "menu.plugins"));
    }

    inline std::shared_ptr<TitleRecord> Title(void* owner)
    {
        auto found = titles.find(owner);
        return found == titles.end() ? nullptr : found->second;
    }

    inline std::shared_ptr<PauseRecord> Pause(void* owner)
    {
        auto found = pauses.find(owner);
        return found == pauses.end() ? nullptr : found->second;
    }

    inline bool Pressed(const uint8_t* a, const uint8_t* b)
    {
        return *a || *b;
    }

    inline void Consume(uint8_t* a, uint8_t* b)
    {
        NativeWrite(a, uint8_t(0));
        NativeWrite(b, uint8_t(0));
    }

    inline void* __fastcall TitleConstruct(void* owner, void*)
    {
        auto result = titleCtorHook.unsafe_thiscall<void*>(owner);
        if (!nativeReady || result != owner || Unwind::modalDepth != 1 || Unwind::worldDepth || titles.count(owner))
            return result;
        auto record = std::make_shared<TitleRecord>();
        auto host = std::make_shared<Host>(Backend::Title);
        record->host = host;
        host->owner = owner;
        host->module = ownModule;
        std::weak_ptr<TitleRecord> weak = record;
        host->nativeCurrent = [weak](const Host& value)
        {
            auto kept = weak.lock();
            return kept && Unwind::modalDepth == 1 && !Unwind::worldDepth && kept->context.Eligible(value.open && !value.entering);
        };
        host->enterNative = [weak](Host&) { auto kept = weak.lock(); return kept && kept->context.Enter(); };
        host->leaveNative = [weak](Host&) { if (auto kept = weak.lock()) kept->context.Leave(); };
        // Register C++ lifetime before publishing any engine-owned row.
        titles.emplace(owner, record);
        if (!record->context.Append(owner, Caption()))
        {
            titles.erase(owner);
            Diagnose(Diagnostic::TitleContext, "title category construction refused");
            return result;
        }
        host->titleStyle = record->context.style;
        return result;
    }

    inline void* __fastcall TitleDelete(void* owner, void*, unsigned flags)
    {
        // Saved481959 CALL[eax] returns48195B, before all44B040 font unloads.
        // SafetyHook's entry JMP retains that original caller return address.
        const bool nativeFinal = reinterpret_cast<uintptr_t>(_ReturnAddress()) == Game().titleFinalReturn && flags == 1 && Unwind::modalDepth == 1 && Unwind::OutsideNativeRows();
        auto record = Title(owner);
        if (record)
        {
            record->host->Retire();
            if (Unwind::modalDepth && !nativeFinal)
            {
                // A local owner pointer remains in4817E0 across its loop. Keep
                // native allocation intact until the next input returns exit;
                // normal native480160/save/deleting-dtor then owns teardown.
                record->context.deletionRequested = true;
                record->deleteFlags |= flags;
                Diagnose(Diagnostic::OwnerRetired, "title deletion deferred to native modal exit");
                return owner;
            }
            record->host->Close();
            titles.erase(owner);
        }
        return titleDeleteHook.unsafe_thiscall<void*>(owner, flags);
    }

    inline int __fastcall TitleInput(void* owner, void*)
    {
        Unwind::Depth input(Unwind::inputDepth);
        auto record = Title(owner);
        if (!record)
            return titleInputHook.unsafe_thiscall<int>(owner);
        auto host = record->host;
        if (record->context.deletionRequested || !host->valid)
        {
            record->context.exiting = true;
            return 1;
        }
        const bool choose = Pressed(TitleChoose.get_ptr(), PauseChoose.get_ptr());
        const bool back = Pressed(TitleBack.get_ptr(), PauseBack.get_ptr());
        if (host->open && record->context.Eligible(true))
        {
            if (choose && Queue(host, host->Selected(), 0))
                Consume(TitleChoose.get_ptr(), PauseChoose.get_ptr());
            else if (back && Queue(host, View::PhysicalBack, 0))
                Consume(TitleBack.get_ptr(), PauseBack.get_ptr());
            return 0;
        }
        if (record->context.Eligible(false) && record->context.Selected() == CategoryCommand && choose)
        {
            Queue(host, CategoryCommand, 0, true);
            // Only our otherwise-unknown command is consumed. Every unknown
            // stock command and all stock modal/save behavior stay original.
            Consume(TitleChoose.get_ptr(), PauseChoose.get_ptr());
            return 0;
        }
        const int result = titleInputHook.unsafe_thiscall<int>(owner);
        if (result)
            record->context.exiting = true;
        return result;
    }

    inline int __fastcall TitleUpdate(void* owner, void*)
    {
        Unwind::Depth update(Unwind::updateDepth);
        auto record = Title(owner);
        if (!record || !record->host->open || !record->context.Eligible(true))
            return titleUpdateHook.unsafe_thiscall<int>(owner);
        auto host = record->host;
        const int direction = FunctionAt<int (__cdecl*)()>(Game().direction)();
        if (direction == 0 || direction == 1)
            Queue(host, host->Selected(), direction == 0 ? -1 : 1);
        else if (host->title)
            host->title->Input(direction);
        if (host->title)
            host->title->Draw();
        // Preserve native base UI animation; no private state reaches the
        // stock options switch's unsupported-command assertion.
        return FunctionAt<int (__thiscall*)(void*)>(Game().optionsCommand)(owner);
    }

    inline int __fastcall PauseCenter(Native::PauseMenu* owner, void*)
    {
        if (nativeReady && Unwind::worldDepth == 1 && !pauses.count(owner) && static_cast<Native::PauseMenu*>(PauseRoot.get()) == owner)
        {
            auto record = std::make_shared<PauseRecord>();
            auto host = std::make_shared<Host>(Backend::Pause);
            record->host = host;
            host->owner = owner;
            host->module = ownModule;
            host->pauseSource = reinterpret_cast<Native::Font*>(GamePauseFont.get_ptr());
            std::weak_ptr<PauseRecord> weak = record;
            host->nativeCurrent = [weak](const Host&)
            {
                auto kept = weak.lock();
                return kept && !kept->deletionRequested && kept->context.Eligible();
            };
            host->enterNative = [weak](Host&)
            {
                auto kept = weak.lock();
                return kept && kept->context.Eligible() && kept->context.base->selected == kept->context.entry;
            };
            host->leaveNative = [weak](Host&)
            {
                if (auto kept = weak.lock(); kept && kept->context.Eligible())
                    FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(Game().pauseSelect)(kept->context.base, static_cast<char>(kept->context.entry));
            };
            pauses.emplace(owner, record);
            if (!record->context.Append(owner, Caption()))
            {
                pauses.erase(owner);
                Diagnose(Diagnostic::PauseContext, "pause category construction refused");
            }
        }
        return pauseCenterHook.unsafe_thiscall<int>(owner);
    }

    inline void* __fastcall PauseDelete(Native::PauseMenu* owner, void*, unsigned flags)
    {
        auto record = Pause(owner);
        if (record)
        {
            record->host->Retire();
            if (Unwind::worldDepth || !Unwind::OutsideNativeRows() || Unwind::servicing)
            {
                record->deletionRequested = true;
                record->deleteFlags |= flags;
                return owner;
            }
            record->host->Close();
            pauses.erase(owner);
        }
        return pauseDeleteHook.unsafe_thiscall<void*>(owner, flags);
    }

    inline char __fastcall PauseInput(Native::PauseMenu* owner, void*)
    {
        Unwind::Depth input(Unwind::inputDepth);
        auto record = Pause(owner);
        if (Unwind::pauseDispatchDepth != 1 || !record || !record->host->open || !record->host->valid || !record->context.Eligible() || !record->host->pause)
            return pauseInputHook.unsafe_thiscall<char>(owner);
        auto host = record->host;
        auto result = pauseInputHook.unsafe_thiscall<char>(host->pause->Get());
        const int direction = FunctionAt<int (__cdecl*)()>(Game().direction)();
        if (direction == 0 || direction == 1)
            Queue(host, host->Selected(), direction == 0 ? -1 : 1);
        return result;
    }

    inline char __fastcall PauseDraw(Native::PauseMenu* owner, void*)
    {
        Unwind::Depth draw(Unwind::drawDepth);
        auto record = Pause(owner);
        if (Unwind::worldDepth != 1 || Unwind::modalDepth || !record || !record->host->open || !record->host->valid || !record->context.Eligible() || !record->host->pause)
            return pauseDrawHook.unsafe_thiscall<char>(owner);
        struct Scope
        {
            std::shared_ptr<PauseRecord> previous;
            explicit Scope(std::shared_ptr<PauseRecord> current) : previous(painting) { painting = std::move(current); }
            ~Scope() { painting = previous; }
        } scope(record);
        return pauseDrawHook.unsafe_thiscall<char>(record->host->pause->Get());
    }

    inline char __fastcall FontDraw(Native::Font* source, void*, int x, int y, const char* text, int mode)
    {
        auto record = painting;
        if (record && source == reinterpret_cast<Native::Font*>(GamePauseFont.get_ptr()))
        {
            Native::Font copy;
            if (!record->host->valid || !record->context.Eligible() || !record->host->pauseFont.DrawCopy(*source, copy))
            {
                record->host->Retire();
                Diagnose(Diagnostic::PauseLayout, "private pause font identity changed");
                return 0;
            }
            return fontDrawHook.unsafe_thiscall<char>(&copy, x, y, text, mode);
        }
        return fontDrawHook.unsafe_thiscall<char>(source, x, y, text, mode);
    }

    inline int __cdecl ReadPauseInput(int* choose, int* back, int* any, int* start)
    {
        const int result = pauseReadHook.unsafe_ccall<int>(choose, back, any, start);
        if (Unwind::worldDepth != 1 || Unwind::pauseDispatchDepth != 1)
            return result;
        auto root = static_cast<Native::PauseMenu*>(PauseRoot.get());
        auto record = Pause(root);
        if (!record || !record->host->valid || !record->context.Eligible())
            return result;
        auto host = record->host;
        bool consumed = false;
        if (host->open)
        {
            if (*choose)
                consumed = Queue(host, host->Selected(), 0);
            else if (*back)
                consumed = Queue(host, View::PhysicalBack, 0);
        }
        else if (*choose && root->selected == record->context.entry)
            consumed = Queue(host, CategoryCommand, 0, true);
        if (!consumed)
            return result;
        NativeWrite(choose, 0);
        NativeWrite(back, 0);
        NativeWrite(any, 0);
        NativeWrite(start, 0);
        return 0;
    }

    inline int __cdecl DispatchPause()
    {
        Unwind::Depth scope(Unwind::pauseDispatchDepth);
        return pauseDispatchHook.unsafe_ccall<int>();
    }

    inline void ServiceTitle()
    {
        Drain(Backend::Title);
        if (Frame)
            Frame();
        std::vector<std::shared_ptr<TitleRecord>> snapshot;
        for (const auto& [owner, record] : titles)
            snapshot.push_back(record);
        const bool changed = RWGTAMenu::Model::Changed().exchange(false);
        for (const auto& record : snapshot)
        {
            auto host = record->host;
            if (host->closeRequested)
                host->Close();
            else if (host->open && host->valid)
            {
                if (!Host::Snapshot(host).Current())
                    host->Retire();
                else if (changed || host->Selected() != host->helpSelected)
                    host->Refresh(host->Selected());
            }
        }
    }

    inline void ServiceWorld()
    {
        Drain(Backend::Pause);
        if (Frame)
            Frame();
        std::vector<std::shared_ptr<PauseRecord>> snapshot;
        for (const auto& [owner, record] : pauses)
            snapshot.push_back(record);
        const bool changed = RWGTAMenu::Model::Changed().exchange(false);
        for (const auto& record : snapshot)
        {
            auto host = record->host;
            if (host->open && host->valid && !Host::Snapshot(host).Current())
                host->Retire();
            if (host->closeRequested)
                host->Close();
            else if ((changed || host->Selected() != host->helpSelected) && host->open && host->valid)
                host->Refresh(host->Selected());
            if (record->deletionRequested)
            {
                // Original native input/draw/HUD callers have all returned.
                // Retained record still keeps the appended label alive until
                // the original fixed-menu destructor has completed.
                pauses.erase(host->owner);
                pauseDeleteHook.unsafe_thiscall<void*>(host->owner, record->deleteFlags);
            }
            else if (!host->valid && record->context.Alive() && record->context.entry < record->context.base->count)
            {
                auto root = record->context.base;
                NativeWrite(&root->rows[record->context.entry].enabled, uint8_t(0));
                if (root->selected == record->context.entry)
                {
                    for (uint8_t i = 0; i < root->count; ++i)
                    {
                        if (root->rows[i].enabled && root->rows[i].visible)
                        {
                            FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(Game().pauseSelect)(root, static_cast<char>(i));
                            break;
                        }
                    }
                }
            }
        }
    }

    inline void ModalFinished()
    {
        auto snapshot = std::move(titles);
        titles.clear();
        for (const auto& [owner, record] : snapshot)
        {
            record->host->Retire();
            record->host->Close();
            if (record->context.deletionRequested)
            {
                // The supported final callsite already deletes before font
                // unload. Do not reorder an unrecognized escape path's native
                // destruction after its borrowed font resources were released.
                retiredTitles.push_back(record);
                Diagnose(Diagnostic::OwnerRetired, "unexpected modal escape; native storage retained without late destruction");
            }
        }
    }

    inline void ResetHooks()
    {
        Unwind::Reset();
        pauseDispatchHook.reset();
        fontDrawHook.reset();
        pauseReadHook.reset();
        pauseDrawHook.reset();
        pauseInputHook.reset();
        pauseDeleteHook.reset();
        pauseCenterHook.reset();
        titleUpdateHook.reset();
        titleInputHook.reset();
        titleDeleteHook.reset();
        titleCtorHook.reset();
    }

    inline bool Init()
    {
        if (nativeReady)
            return true;
        // Native borrowed strings and callbacks require process lifetime.
        // Pin the bridge before any hook or retained native pointer.
        // Dynamic host unload is not supported.
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Init), &ownModule);
        for (const auto& setting : FixSettings().GetDescription().settings)
        {
            if (setting.HasValue() && setting.apply == "restart")
                restartBaseline.emplace(std::make_pair(ownModule, setting.id), FixSettings().Get(setting.id));
        }
        Unwind::titleFrame = ServiceTitle;
        Unwind::worldFrame = ServiceWorld;
        Unwind::modalFinished = ModalFinished;
        auto attach = [](SafetyHookInline& hook, uintptr_t address, void* callback)
        {
            hook = safetyhook::create_inline(reinterpret_cast<void*>(address), callback);
        };
        attach(titleCtorHook, Game().titleConstruct, reinterpret_cast<void*>(&TitleConstruct));
        attach(titleDeleteHook, Game().titleDelete, reinterpret_cast<void*>(&TitleDelete));
        attach(titleInputHook, Game().titleInput, reinterpret_cast<void*>(&TitleInput));
        attach(titleUpdateHook, Game().titleUpdate, reinterpret_cast<void*>(&TitleUpdate));
        attach(pauseCenterHook, Game().pauseCenter, reinterpret_cast<void*>(&PauseCenter));
        attach(pauseDeleteHook, Game().pauseDelete, reinterpret_cast<void*>(&PauseDelete));
        attach(pauseInputHook, Game().pauseInput, reinterpret_cast<void*>(&PauseInput));
        attach(pauseDrawHook, Game().pauseDraw, reinterpret_cast<void*>(&PauseDraw));
        attach(pauseReadHook, Game().pauseRead, reinterpret_cast<void*>(&ReadPauseInput));
        attach(fontDrawHook, Game().fontDraw, reinterpret_cast<void*>(&FontDraw));
        attach(pauseDispatchHook, Game().pauseDispatch, reinterpret_cast<void*>(&DispatchPause));
        Unwind::Install();
        nativeReady = true;
        return true;
    }
}

namespace THPS2Updates
{
    inline std::shared_ptr<THPS2Menu::Host> Current()
    {
        for (const auto& [owner, record] : THPS2Menu::titles)
        {
            const auto& host = record->host;
            if (host->open && host->valid && !host->busy && !host->view.details && THPS2Menu::Host::Snapshot(host).Current())
                return host;
        }
        for (const auto& [owner, record] : THPS2Menu::pauses)
        {
            const auto& host = record->host;
            if (host->open && host->valid && !host->busy && !host->view.details && THPS2Menu::Host::Snapshot(host).Current())
                return host;
        }
        return nullptr;
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        if (!THPS2Menu::nativeReady)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TonyHawksProSkater2.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return Current() != nullptr; };
        config.format = [](const std::string&, bool hub)
        {
            auto host = Current();
            return host ? asi_settings::text::Widen(host->model.Text(hub ? "update.availablehub" : "update.availablesite")) : std::wstring{};
        };
        config.show = [](const std::wstring& text)
        {
            auto host = Current();
            if (!host)
                return;
            host->view.returnPhysical = host->view.physical;
            host->view.returnSelected = host->Selected();
            host->view.detailText = asi_settings::text::Narrow(text);
            host->view.details = true;
            host->view.physical = 0;
            host->Refresh();
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        THPS2Menu::Frame = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
namespace THPS2DeferredClose
{
    inline SafetyHookInline wndProcHook, loopHook;
    inline SafetyHookMid initialExitCheckHook, frameExitCheckHook;
    inline int(__cdecl* launch)(int, int) = nullptr;
    inline void(__cdecl* titlePump)() = nullptr;
    inline unsigned launchDepth = 0, loopDepth = 0;
    inline bool pending = false;
    inline HWND window = nullptr;
    inline std::function<void()> previousTitle;

    inline LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        if (msg == WM_CLOSE && (launchDepth || pending))
        {
            pending = true;
            window = hwnd;
            if (loopDepth) gameExit = 8;
            return 0;
        }
        const auto result = wndProcHook.stdcall<LRESULT>(hwnd, msg, wp, lp);
        // Native shutdown destroys the window and stops the frame timer while
        // its modal loop is still on the stack. Do not resume that dead loop.
        if (msg == WM_CLOSE) ExitProcess(0);
        return result;
    }
    inline int __cdecl Launch(int level, int mode)
    {
        ++launchDepth;
        const int result = launch(level, mode);
        --launchDepth;
        if (pending && !launchDepth) startScreen = 0;
        return result;
    }
    inline int __cdecl Loop()
    {
        ++loopDepth;
        const auto result = loopHook.ccall<int>();
        --loopDepth;
        return result;
    }
    inline void TitleFrame()
    {
        if (previousTitle) previousTitle();
        if (!pending || launchDepth || !IsWindow(window)) return;
        pending = false;
        if (!PostMessageW(window, WM_CLOSE, 0, 0)) pending = true;
    }
    inline void __cdecl TitlePump()
    {
        if (pending && !launchDepth && IsWindow(window))
        {
            pending = false;
            if (!PostMessageW(window, WM_CLOSE, 0, 0)) pending = true;
        }
        titlePump();
    }
    inline void Install()
    {
        auto wnd = hook::pattern("53 8B 5C 24 10 56 8B 74 24 10 57 33 FF 81 FE 00 01 00 00");
        auto call = hook::pattern("8B 0D ? ? ? ? 50 51 E8 ? ? ? ? BF ? ? ? ? 83 C9 FF 33 C0 83 C4 10 F2 AE");
        auto initialCheck = hook::pattern("A1 ? ? ? ? 85 C0 75 44 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ?");
        auto frameCheck = hook::pattern("A1 ? ? ? ? 85 C0 75 1D E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ?");
        auto loop = hook::pattern("56 E8 ? ? ? ? BE ? ? ? ? 8B 0E 85 C9 74 ? 6A FF 6A FF 6A 00 6A 5E");
        auto start = hook::pattern("8B 15 ? ? ? ? 52 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? B9 01 00 00 00 83 C4 08");
        auto frame = hook::pattern("83 C4 08 C7 44 24 08 FF FF FF FF A1 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A 10");
        gameExit.SetAddress(*initialCheck.get_first<int*>(1));
        startScreen.SetAddress(*start.get_first<int*>(2));
        launch = reinterpret_cast<decltype(launch)>(injector::GetBranchDestination(call.get_first(8)).as_int());
        titlePump = reinterpret_cast<decltype(titlePump)>(injector::GetBranchDestination(frame.get_first(21)).as_int());
        wndProcHook = safetyhook::create_inline(wnd.get_first(), WndProc);
        loopHook = safetyhook::create_inline(loop.get_first(), Loop);
        auto requestExit = [](SafetyHookContext&) { if (pending) gameExit = 8; };
        initialExitCheckHook = safetyhook::create_mid(initialCheck.get_first(), requestExit);
        frameExitCheckHook = safetyhook::create_mid(frameCheck.get_first(), requestExit);
        previousTitle = THPS2Menu::Unwind::titleFrame;
        THPS2Menu::Unwind::titleFrame = TitleFrame;
        injector::MakeCALL(frame.get_first(21), TitlePump, true);
        injector::MakeCALL(call.get_first(8), Launch, true);
    }
}
}

export void InstallMenuWheel()
{
    hook::pattern pattern("");
    //Main menu wheel fix
    auto sub_4F0A70 = [](int16_t a1) -> int32_t
    {
        return (int32_t)(cos((float)(a1 & 0xFFF) * 0.00024414062f * 6.2831855f) * (4096.0f / (Screen.Height43 / 480.0f)));
    };
    pattern = hook::pattern("E8 ? ? ? ? 53 89 44 24 1C 89");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0A70), true); //0x457939
    pattern = hook::pattern("E8 ? ? ? ? 0F AF 44 24 20 C1");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0A70), true); //0x457992

    auto sub_4F0AA0 = [](int16_t a1) -> int32_t
    {
        return (int32_t)(sin((float)(a1 & 0xFFF) * 0.00024414062f * 6.2831855f) * (4096.0f / (Screen.Height43 / 480.0f)));
    };
    pattern = hook::pattern("E8 ? ? ? ? 8B F0 53 0F AF");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0AA0), true); //0x457947
    pattern = hook::pattern("E8 ? ? ? ? 8B 5D 0C 89 44");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0AA0), true); //0x45795D
}

export void InstallMenuLayout()
{
    hook::pattern pattern("");
    //menu backgrounds
    pattern = hook::pattern("A1 ? ? ? ? 81 EC 98"); //4D4BA0
    auto sub_4D4BA0 = pattern.get_first(0);
    dword_29D6FF0.SetAddress(*pattern.get_first<uint32_t*>(1));
    struct MenuHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = dword_29D6FF0;
            *(float*)(regs.esp + 0x8) += Screen.fHudOffset;
            *(float*)(regs.esp + 0xC) += Screen.fHudOffsetY;
        }
    }; injector::MakeInline<MenuHook1>(sub_4D4BA0);

    constexpr std::array widthPatterns
    {
        "A1 ? ? ? ? F6 C1 08",
        "A1 ? ? ? ? 89 84 24 ? ? ? ? DF AC 24 ? ? ? ? D8 4A 34",
        "A1 ? ? ? ? 89 44 24 28",
        "A1 ? ? ? ? 89 84 24 ? ? ? ? DF AC 24 ? ? ? ? D8 4A 6C D8 0D ? ? ? ? D9 5A 6C 8B 0D ? ? ? ? 89 4C 24 68",
        "A1 ? ? ? ? 89 44 24 50",
        "A1 ? ? ? ? 89 44 24 70",
        "A1 ? ? ? ? 89 84 24 ? ? ? ? DF AC 24 ? ? ? ? D8 4A 6C D8 0D ? ? ? ? D9 5A 6C 8B 0D ? ? ? ? 89 8C 24 ? ? ? ?"
    };
    constexpr std::array heightPatterns
    {
        "8B 0D ? ? ? ? 89 4C 24 38",
        "8B 0D ? ? ? ? 89 4C 24 48",
        "8B 0D ? ? ? ? 89 4C 24 58",
        "8B 0D ? ? ? ? 89 4C 24 68",
        "8B 0D ? ? ? ? 89 4C 24 40",
        "8B 0D ? ? ? ? 89 4C 24 60",
        "8B 0D ? ? ? ? 89 8C 24 ? ? ? ? DF AC 24 ? ? ? ? D8 4A 54",
        "8B 0D ? ? ? ? 89 8C 24 ? ? ? ? DF AC 24 ? ? ? ? D8 4A 70"
    };
    const auto begin = reinterpret_cast<uintptr_t>(sub_4D4BA0);
    std::array<int32_t**, widthPatterns.size()> widthSites{};
    std::array<int32_t**, heightPatterns.size()> heightSites{};
    for (size_t i = 0; i < widthPatterns.size(); ++i)
        widthSites[i] = hook::range_pattern(begin, begin + 0x414, widthPatterns[i]).get_first<int32_t*>(1);
    for (size_t i = 0; i < heightPatterns.size(); ++i)
        heightSites[i] = hook::range_pattern(begin, begin + 0x414, heightPatterns[i]).get_first<int32_t*>(2);
    for (auto site : widthSites)
        injector::WriteMemory<int32_t*>(site, &Screen.Width43, true);
    for (auto site : heightSites)
        injector::WriteMemory<int32_t*>(site, &Screen.Height43, true);

//some menu things
const std::array menuSites
{
    hook::pattern("C7 41 28 00 00 80 BF C1 EA 07 3B C3 66 89 51 22").get_first<uint8_t>(),
    hook::pattern("C7 41 28 00 00 80 BF F7 E2 C1 EA 07 83 C5 10 66 89 51 22").get_first<uint8_t>(),
    hook::pattern("C7 41 28 00 00 80 BF F7 E2 C1 EA 07 66 89 51 22").get_first<uint8_t>()
};
constexpr std::array<std::array<const char*, 4>, 3> menuWidthPatterns
{{
    {{ "0F AF 05 ? ? ? ? 0F BF EA", "0F AF 05 ? ? ? ? C1 E8 09", "0F AF 3D ? ? ? ?", "0F AF 05 ? ? ? ? 0F BF 51 22" }},
    {{ "0F AF 15 ? ? ? ? C1 EA 09", "0F AF 05 ? ? ? ? 0F BF 54 24 14", "0F AF 05 ? ? ? ? 0F BF 54 24 10", "0F AF 05 ? ? ? ? 0F BF 54 24 44" }},
    {{ "0F AF 15 ? ? ? ? C1 EA 09", "0F AF 05 ? ? ? ? 0F BF 51 12", "0F AF 05 ? ? ? ? 0F BF 51 1A", "0F AF 05 ? ? ? ? 0F BF 51 22" }}
}};
constexpr std::array<std::array<const char*, 4>, 3> menuHeightPatterns
{{
    {{ "0F AF 15 ? ? ? ? B8 ? ? ? ? F7 E2 0F BF 41 10", "0F AF 2D ? ? ? ?", "0F AF 15 ? ? ? ? B8 ? ? ? ? F7 E2 0F BF 41 20", "0F AF 15 ? ? ? ? B8 ? ? ? ? 89 71 2C" }},
    {{ "0F AF 15 ? ? ? ? F7 E2", "0F AF 15 ? ? ? ? B8 ? ? ? ? F7 E2 0F BF C7", "0F AF 15 ? ? ? ? B8 ? ? ? ? F7 E2 0F BF C3", "0F AF 15 ? ? ? ? B8 ? ? ? ? C7 41 28 ? ? ? ?" }},
    {{ "0F AF 15 ? ? ? ? F7 E2", "0F AF 15 ? ? ? ? B8 ? ? ? ? 5B", "0F AF 15 ? ? ? ? B8 ? ? ? ? 5F", "0F AF 15 ? ? ? ? B8 ? ? ? ? C7 41 28 ? ? ? ?" }}
}};
struct MenuHook2
{
    void operator()(injector::reg_pack& regs)
    {
        const auto offset = static_cast<int16_t>(Screen.fHudOffsetReal + THPS2Hud::Offset());
        for (auto vertex : { 0x08, 0x10, 0x18, 0x20 })
        {
            auto* x = reinterpret_cast<int16_t*>(regs.ecx + vertex);
            *x = static_cast<int16_t>(*x + offset);
        }
        // The y of the last vertex is stored after this point
        for (auto vertex : { 0x0A, 0x12, 0x1A })
        {
            auto* y = reinterpret_cast<int16_t*>(regs.ecx + vertex);
            *y = static_cast<int16_t>(*y + Screen.fHudOffsetRealY);
        }
        *reinterpret_cast<float*>(regs.ecx + 0x28) = -1.0f;
    }
};

static std::vector<SafetyHookMid> lastVertexHooks;
if (Screen.fHudOffsetRealY > 0.0f)
    lastVertexHooks.resize(menuSites.size());
for (size_t i = 0; i < menuSites.size(); ++i)
{
    const auto site = reinterpret_cast<uintptr_t>(menuSites[i]);
    std::array<int32_t**, 4> widths{}, heights{};
    for (size_t j = 0; j < widths.size(); ++j)
    {
        widths[j] = hook::range_pattern(site - 0x170, site + 0x20, menuWidthPatterns[i][j]).get_first<int32_t*>(3);
        heights[j] = hook::range_pattern(site - 0x170, site + 0x20, menuHeightPatterns[i][j]).get_first<int32_t*>(3);
    }
    injector::MakeInline<MenuHook2>(site, site + 7);
    for (auto operand : widths)
        injector::WriteMemory<int32_t*>(operand, &Screen.Width43, true);
    for (auto operand : heights)
        injector::WriteMemory<int32_t*>(operand, &Screen.Height43, true);

    if (Screen.fHudOffsetRealY > 0.0f)
    {
        auto store = hook::range_pattern(site, site + 0x20, "66 89 51 22"); //mov [ecx+22h], dx
        lastVertexHooks[i] = safetyhook::create_mid(store.get_first(), [](SafetyHookContext& regs)
        {
            regs.edx = static_cast<uint16_t>(static_cast<int16_t>(regs.edx) + static_cast<int16_t>(Screen.fHudOffsetRealY));
        });
    }
}
THPS2Hud::Install(Screen.fWidth, Screen.fHeight, static_cast<float>(Screen.Width43), static_cast<float>(Screen.Height43), &FixSettings().String("MAIN.HudAspectRatioConstraint"));
THPS2Frame::Install(Screen.fWidth, Screen.fHeight, static_cast<float>(Screen.Width43), static_cast<float>(Screen.Height43), &Screen.Width43, &Screen.Height43);
}
