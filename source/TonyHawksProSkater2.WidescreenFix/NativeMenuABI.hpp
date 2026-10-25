#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

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
