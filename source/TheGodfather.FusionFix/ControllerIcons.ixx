module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "FloatingPoint.hpp"
#include <wrl/client.h>
#include <stdexcept>

export module ControllerIcons;

import NativeMenuABI;

import Window;

import Input;
import Paths;
import ComVars;

namespace GodfatherIcons
{
    using Microsoft::WRL::ComPtr;
    std::atomic<int> style{};
    SafetyHookInline labelHook, glyphHook, actionHook, parseHook, textHook, quadHook;
    SafetyHookMid conversionHook;
    IDirect3DDevice9* textureDevice{};
    std::array<ComPtr<IDirect3DTexture9>, 3> atlases;
    void*(__thiscall* actionName)(void*, void*, int, int, int, int){};
    IDirect3DTexture9* Atlas();
    void* SymbolFont(void* context);

    struct Glyph
    {
        uint16_t character, width, height, advance;
        int16_t x, y;
        uint16_t left, top, right, bottom;
    };
    static_assert(sizeof(Glyph) == 20);

    constexpr const char* tokens[]{ "[b=ffStart]", "[b=ffSelect]", "[b=ffLT]", "[b=ffLB]",
        "[b=ffRT]", "[b=ffRB]", "[b=ffLeft]", "[b=ffRight]", "[b=ffUp]", "[b=ffDown]",
        "[b=ffX]", "[b=ffB]", "[b=ffY]", "[b=ffA]", "[b=ffLS]", "[b=ffRS]" };
    constexpr unsigned firstGlyph = 128;
    thread_local int parsedGlyph = -1;
    thread_local void* parsedContext{};
    thread_local bool privateGlyph = false;
    export thread_local bool flushing = false;

    struct DrawState
    {
        ComPtr<IDirect3DBaseTexture9> original;
        bool replaced = false;
    };
    thread_local DrawState drawing;

    const char* Binding(int category, int binding)
    {
        if (category != 2)
            return nullptr;
        constexpr unsigned buttons[]{ 13, 11, 10, 12, 3, 5, 2, 4, 1, 0, 14, 15 };
        if (binding >= 0 && size_t(binding) < std::size(buttons))
            return tokens[buttons[binding]];
        constexpr unsigned directions[]{ 8, 9, 7, 6 };
        if (binding >= 128 && binding < 132)
            return tokens[directions[binding - 128]];
        // StandardGamepad exposes X/Y for the left stick and Z/RotZ for the right.
        if (binding == 144 || binding == 145 || (binding >= 152 && binding <= 155))
            return tokens[14];
        if (binding == 146 || binding == 149 || binding == 156 || binding == 157 || binding == 162 || binding == 163)
            return tokens[15];
        return nullptr;
    }

    void* __fastcall Label(void* self, void*, void* output, int binding, int category, int flag)
    {
        auto result = labelHook.unsafe_thiscall<void*>(self, output, binding, category, flag);
        auto context = GodfatherTextSlot.is_initialized() ? GodfatherTextSlot.get() : nullptr;
        if (const auto text = Binding(category, binding); text && result && GodfatherPaths::assign && SymbolFont(context) && Atlas())
            GodfatherPaths::assign(result, text, std::strlen(text));
        return result;
    }

    bool KeyboardActive(const void* input)
    {
        if (!input)
            return false;
        const auto bytes = static_cast<const uint8_t*>(input);
        return bytes[56] && bytes[988] >= 5;
    }

    void* __fastcall Action(void* self, void*, void* output, int action, int context, int category, int fallback)
    {
        // The native formatter otherwise prefers any attached pad, even after keyboard input.
        if (category != 2 && KeyboardActive(self))
            return actionName(self, output, action, context, category, fallback);
        return actionHook.unsafe_thiscall<void*>(self, output, action, context, category, fallback);
    }

    IDirect3DTexture9* Atlas()
    {
        auto device = GodfatherWindow::device;
        if (!device || GetCurrentThreadId() != GodfatherWindow::renderThread)
            return nullptr;
        if (textureDevice != device)
        {
            for (auto& atlas : atlases)
                atlas.Reset();
            textureDevice = device;
        }
        const int selected = style.load(std::memory_order_relaxed);
        const size_t index = selected == 2 ? 1 : selected == 3 ? 2 : 0;
        auto& atlas = atlases[index];
        if (!atlas)
        {
            HMODULE module{};
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&Atlas), &module))
                return nullptr;
            const auto resource = FindResourceW(module, MAKEINTRESOURCEW(5100 + index), MAKEINTRESOURCEW(10));
            if (!resource || SizeofResource(module, resource) != 128 + 256 * 256 * 4)
                return nullptr;
            const auto data = static_cast<const uint8_t*>(LockResource(LoadResource(module, resource)));
            if (!data)
                return nullptr;
            const auto header = reinterpret_cast<const uint32_t*>(data);
            if (header[0] != 0x20534444 || header[1] != 124 || header[3] != 256 || header[4] != 256
                || header[19] != 32 || header[20] != 0x41 || header[21] || header[22] != 32
                || header[23] != 0xFF0000 || header[24] != 0xFF00 || header[25] != 0xFF || header[26] != 0xFF000000)
                return nullptr;
            Godfather::FloatingPointScope floatingPoint;
            ComPtr<IDirect3DTexture9> created;
            if (FAILED(device->CreateTexture(256, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                created.GetAddressOf(), nullptr)))
                return nullptr;
            D3DLOCKED_RECT pixels{};
            if (FAILED(created->LockRect(0, &pixels, nullptr, 0)))
                return nullptr;
            const bool valid = pixels.pBits && pixels.Pitch >= 256 * 4;
            if (valid)
                for (size_t row = 0; row < 256; ++row)
                    std::memcpy(static_cast<uint8_t*>(pixels.pBits) + row * pixels.Pitch, data + 128 + row * 256 * 4, 256 * 4);
            const auto unlocked = created->UnlockRect(0);
            if (!valid || FAILED(unlocked))
                return nullptr;
            atlas = std::move(created);
        }
        return atlas.Get();
    }

    void* SymbolFont(void* context)
    {
        if (!context)
            return nullptr;
        return *reinterpret_cast<void**>(static_cast<uint8_t*>(context) + 32);
    }

    bool Remap(Glyph& glyph, unsigned character, unsigned width, unsigned height)
    {
        if (character >= 16 || !width || !height || width > 4095 || height > 4095
            || !glyph.height || glyph.height > 512 || glyph.width > 512 || glyph.advance > 1024)
            return false;
        const unsigned column = character % 4, row = character / 4;
        const unsigned spacing = glyph.advance > glyph.width ? glyph.advance - glyph.width : 1;
        glyph.width = glyph.height;
        glyph.advance = uint16_t(glyph.width + spacing);
        glyph.x = 0;
        // Native glyph UVs use sixteenth-pixel units and the original raster dimensions.
        glyph.left = uint16_t(column * width * 4);
        glyph.right = uint16_t((column + 1) * width * 4);
        glyph.top = uint16_t(row * height * 4);
        glyph.bottom = uint16_t((row + 1) * height * 4);
        return true;
    }

    bool __fastcall FindGlyph(void* self, void*, uint8_t character, Glyph** output)
    {
        const bool replacement = character >= firstGlyph && character < firstGlyph + std::size(tokens)
            && parsedGlyph == character && self == SymbolFont(parsedContext) && Atlas();
        const auto cell = replacement ? character - firstGlyph : character;
        const bool found = glyphHook.unsafe_thiscall<bool>(self, uint8_t(cell), output);
        privateGlyph = false;
        if (!found || !output || !*output || !replacement)
            return found;
        auto texture = *reinterpret_cast<void**>(static_cast<uint8_t*>(self) + 1060);
        if (!texture)
            return found;
        auto raster = *static_cast<void**>(texture);
        if (!raster)
            return found;
        const auto dimensions = reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(raster) + 12);
        thread_local std::array<Glyph, 16> copies;
        auto& copy = copies[cell];
        copy = **output;
        if (Remap(copy, cell, dimensions[0], dimensions[1]))
        {
            *output = &copy;
            privateGlyph = true;
        }
        return found;
    }

    bool ObserveTexture()
    {
        auto device = GodfatherWindow::device;
        if (!device || GetCurrentThreadId() != GodfatherWindow::renderThread)
            return false;
        ComPtr<IDirect3DBaseTexture9> bound;
        if (FAILED(device->GetTexture(0, bound.GetAddressOf())))
            return false;
        if (drawing.replaced)
            for (const auto& atlas : atlases)
                if (bound.Get() == atlas.Get())
                    return bool(drawing.original);
        drawing.original = std::move(bound);
        drawing.replaced = false;
        return bool(drawing.original);
    }

    int Token(const char* text, int* consumed)
    {
        if (!text || !consumed || text[0] != '[' || _strnicmp(text, "[b=ff", 5))
            return -1;
        for (unsigned cell = 0; cell < std::size(tokens); ++cell)
        {
            const auto length = std::strlen(tokens[cell]);
            if (!_strnicmp(text, tokens[cell], length))
            {
                *consumed = int(length - 1);
                return int(firstGlyph + cell);
            }
        }
        return -1;
    }

    int __fastcall Parse(void* self, void*, const char* text, int* consumed, int* glyph)
    {
        parsedContext = self;
        parsedGlyph = consumed && glyph ? Token(text, consumed) : -1;
        if (parsedGlyph >= 0 && (!SymbolFont(self) || !Atlas()))
            parsedGlyph = -1;
        if (parsedGlyph >= 0)
        {
            *glyph = parsedGlyph;
            return 0;
        }
        return parseHook.unsafe_thiscall<int>(self, text, consumed, glyph);
    }

    void PreserveToken(SafetyHookContext& registers)
    {
        // The PC formatter must not reinterpret a physical button as another action.
        if (registers.eax >= firstGlyph && registers.eax < firstGlyph + std::size(tokens) && parsedGlyph == registers.eax)
            registers.eax = 24;
    }

    bool Flush(void* context)
    {
        if (!GodfatherRenderSlot.is_initialized() || !context)
            return false;
        auto api = static_cast<uint8_t*>(GodfatherRenderSlot.get());
        if (!api)
            return false;
        const auto count = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(context) + 60);
        const auto length = *count;
        if (length)
        {
            auto vertices = *reinterpret_cast<void**>(static_cast<uint8_t*>(context) + 56);
            if (length > 585 || !vertices)
                return false;
            const auto draw = *reinterpret_cast<void(__cdecl**)(int, void*, int)>(api + 48);
            if (!draw)
                return false;
            struct Scope
            {
                bool previous = flushing;
                Scope() { flushing = true; }
                ~Scope() { flushing = previous; }
            } scope;
            draw(3, vertices, length);
            *count = 0;
        }
        return true;
    }

    void* __fastcall Quad(void* self, void*, float* rectangle, int alignment, float* uv, bool normal)
    {
        const bool replacement = !normal && privateGlyph;
        if (ObserveTexture() && replacement != drawing.replaced)
        {
            auto texture = replacement ? Atlas() : drawing.original.Get();
            if (texture && Flush(self) && SUCCEEDED(GodfatherWindow::device->SetTexture(0, texture)))
                drawing.replaced = replacement;
        }
        return quadHook.unsafe_thiscall<void*>(self, rectangle, alignment, uv, normal);
    }

    void* __fastcall Text(void* self, void*, void* font, float* rectangle, float scale, const char* text,
        int horizontal, int vertical, bool shadow)
    {
        struct Scope
        {
            DrawState previous = std::move(drawing);
            int glyph = parsedGlyph;
            void* context = parsedContext;
            bool replacement = privateGlyph;
            Scope()
            {
                drawing = {};
                parsedGlyph = -1;
                parsedContext = nullptr;
                privateGlyph = false;
            }
            ~Scope()
            {
                if (drawing.replaced && drawing.original && GodfatherWindow::device)
                    GodfatherWindow::device->SetTexture(0, drawing.original.Get());
                drawing = std::move(previous);
                parsedGlyph = glyph;
                parsedContext = context;
                privateGlyph = replacement;
            }
        } scope;
        return textHook.unsafe_thiscall<void*>(self, font, rectangle, scale, text, horizontal, vertical, shadow);
    }

    export void Init()
    {
        auto labels = hook::pattern("51 8B 44 24 14 8B 89 98 09 00 00 8B 54 24 10 56 8B 74 24 0C 50 8B 44 24 14 51 8B 0D ? ? ? ? 52 50 56");
        auto glyphs = hook::pattern("8B 49 24 32 C0 85 C9 74 14 0F B6 54 24 04 8B 0C 91 85 C9 74 08 8B 44 24 08 89 08 B0 01 C2 08 00");
        auto bind = hook::pattern("8B 4D 00 8B 15 ? ? ? ? 51 6A 01 FF 52 20 83 C4 08 84 DB");
        auto root = hook::pattern("89 7C 24 18 A3 ? ? ? ? 8B 4C 24 20 68 00 01 00 00 C6 44 24 1C 01");
        auto actions = hook::pattern("53 55 33 DB 56 89 44 24 48 57 8B F1 89 5C 24 10 C7 44 24 48 0F 00 00 00 "
            "89 5C 24 44 88 5C 24 34 8B 86 60 08 00 00 3B C3 8B 7C 24 68 8B 6C 24 64 89 5C 24 58 7C 4B");
        auto names = hook::pattern("6A FF 6A 02 57 55 8D 44 24 24 50 8B CE E8 ? ? ? ? 6A FF 53 50 8D 4C 24 3C");
        auto parser = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 18 64 89 25 00 00 00 00 83 EC 18 53 "
            "8B 5C 24 30 55 56 8B F1 8B 4C 24 34 C7 03 00 00 00 00 C7 00 FF FF FF FF 8A 01");
        auto text = hook::pattern("B8 A8 40 00 00 E8 ? ? ? ? A1 ? ? ? ? 89 84 24 A4 40 00 00 8B 84 24 B0 40 00 00 D9 40 04");
        auto quads = hook::pattern("83 EC 0C 53 55 8B E9 8B 45 2C 0F B7 55 3E 89 44 24 08 66 8B 45 3C 0F B7 C8 56 8D 71 06 3B F2");
        auto convert = hook::pattern("8B 04 85 ? ? ? ? 83 F8 20 8B 0D ? ? ? ? 8B 5C 24 38");
        if (GodfatherMenu::layoutShift) // Chinese executable
        {
            bind = hook::pattern("8B 4D 00 8B 15 ? ? ? ? 51 6A 01 FF 52 20 8B 6C 24 24 83 C4 08 84 DB");
            actions = hook::pattern("53 55 33 DB 56 89 44 24 48 57 8B F1 89 5C 24 10 C7 44 24 48 0F 00 00 00 89 5C 24 44 88 5C 24 34 8B 86 64 08 00 00 3B C3 8B 7C 24 68 8B 6C 24 64 89 5C 24 58 7C 4B");
            parser = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 18 64 89 25 00 00 00 00 83 EC 20 53 8B 5C 24 38 55 56 8B F1 8B 4C 24 3C C7 03 00 00 00 00 C7 00 FF FF FF FF 8A 01");
            text = hook::pattern("B8 A4 40 00 00 E8 ? ? ? ? A1 ? ? ? ? 89 84 24 A0 40 00 00 8B 84 24 AC 40 00 00 D9 40 04");
        }
        GodfatherTextSlot.SetAddress(*root.get_first<void**>(5));
        GodfatherRenderSlot.SetAddress(*bind.get_first<void**>(5));
        const auto action = actions.get_first<uint8_t>(-29);
        actionName = reinterpret_cast<decltype(actionName)>(injector::GetBranchDestination(names.get_first<uint8_t>(13)).as_int());
        glyphHook = safetyhook::create_inline(glyphs.get_first(), FindGlyph);
        parseHook = safetyhook::create_inline(parser.get_first(), Parse);
        textHook = safetyhook::create_inline(text.get_first(), Text);
        quadHook = safetyhook::create_inline(quads.get_first(), Quad);
        conversionHook = safetyhook::create_mid(convert.get_first(), PreserveToken);
        labelHook = safetyhook::create_inline(labels.get_first(), Label);
        actionHook = safetyhook::create_inline(action, Action);
        auto& settings = FixSettings();
        style.store(settings.Int("MAIN.GamepadIcons"), std::memory_order_relaxed);
        settings.OnChange("MAIN.GamepadIcons", []()
        {
            style.store(FixSettings().Int("MAIN.GamepadIcons"), std::memory_order_relaxed);
        });
    }
}
