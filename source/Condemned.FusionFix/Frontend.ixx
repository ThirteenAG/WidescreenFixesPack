module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include "Shared/RWGTA/FixMenu.hpp"
#include <utility>
#include <atomic>
#include <limits>
#include <memory>
#include <safetyhook.hpp>

export module Frontend;

import ComVars;

// Disconnected native foundation for installed GameClient0ac9798c... .
// No Init, hooks, registration, module pin or static native side effect.
// Full host must supply module/font readiness and complete traversal leases.
export namespace CondemnedMenuNative
{
    static_assert(sizeof(void*) == 4, "Condemned GUI is Win32");
    struct Font
    {
        char face[32]{};
        uint32_t height{};
        uint32_t flags{};
    };
    struct Create
    {
        uint32_t command{};
        const char* help{};
        void* handler{};
        uint32_t parameter1{}, parameter2{};
        RECT rect{};
        uint8_t effect{};
        uint8_t reserved[3]{};
        float transition{0.25f};
        float scale1{1.2f}, scale2{1.2f};
    };
    static_assert(sizeof(Font) == 40 && sizeof(Create) == 52);
    static_assert(offsetof(Create, rect) == 20 && offsetof(Create, effect) == 36 &&
        offsetof(Create, transition) == 40);

    // GameClient.dll functions, globals and class tables as module offsets, found by pattern once.
    struct Offsets
    {
        // Functions
        uint32_t sourceCommand, sourceKey, sourceEscape, sourceLeft, sourceRight, sourceFocused, updated, postUpdated;
        uint32_t rendered, keyPressed, keyReleased, characterReceived, mousePressed, mouseReleased, commandPressed;
        uint32_t commandReleased, removed, screenTerminated, screenDestroyed, screenDeleted, managerTerminated;
        uint32_t managerDeleted, managerDestroyed, interfaceTerminated, interfaceDeleted, interfaceDestroyed;
        uint32_t shellDestroyed, shellTerminated, textDeleted, allocate, free, textConstruct, textCreate, textSet;
        uint32_t textFontSize, stringBuild, textBounds, controlSelect, stringSet, textScale, controlEnable;
        // Globals and class tables
        uint32_t interfaceManager, provider, overlayFirst, overlaySecond, connection, gameState, textTable, interfaceTable;
        uint32_t managerTable, screenTable;
    };

    inline const Offsets& Rva()
    {
        static const Offsets offsets = []()
        {
            const auto module = GetModuleHandleW(L"GameClient.dll");
            const auto base = reinterpret_cast<uintptr_t>(module);
            auto at = [&](const char* signature)
            {
                return uint32_t(reinterpret_cast<uintptr_t>(hook::module_pattern(module, signature).get_first()) - base);
            };
            auto operand = [&](const char* signature, ptrdiff_t offset)
            {
                return uint32_t(*hook::module_pattern(module, signature).get_first<uintptr_t>(offset) - base);
            };
            Offsets o{};
            o.sourceCommand = at("8B 54 24 04 8D 42 C9 83 F8 04 77 4F");
            o.sourceKey = at("53 56 57 8B 7C 24 10 8D 47 F3 83 F8 1B");
            o.sourceEscape = at("56 8B F1 8B 4E 0C 8B 01 FF 50 44 84 C0");
            o.sourceLeft = at("56 8B B1 C8 00 00 00 32 C0 85 F6 57 66 8B B9 10 01 00 00 75 04 33 D2 EB 0B 8B 91 CC 00 00 00 2B D6 C1 FA 02 0F B7 F7 3B F2 73 2B 8B 89 C8 00 00 00 8D 0C B1 8B 09 85 C9 74 1C 8B 11 53 FF 52 4C");
            o.sourceRight = at("56 8B B1 C8 00 00 00 32 C0 85 F6 57 66 8B B9 10 01 00 00 75 04 33 D2 EB 0B 8B 91 CC 00 00 00 2B D6 C1 FA 02 0F B7 F7 3B F2 73 2B 8B 89 C8 00 00 00 8D 0C B1 8B 09 85 C9 74 1C 8B 11 53 FF 52 50");
            o.sourceFocused = at("53 8B 5C 24 08 84 DB 56 57 8B 3D ? ? ? ? 8B F1 0F 84 ? ? ? ? 8B CF E8 ? ? ? ? 8A 87 95 00 00 00");
            o.updated = at("55 8B EC 83 E4 F8 83 EC 1C A1 ? ? ? ? 55");
            o.postUpdated = at("56 8B F1 8A 86 A1 4E 00 00 84 C0 74 25");
            o.rendered = at("83 EC 08 56 8B F1 8B 86 40 01 00 00");
            o.keyPressed = at("56 8B F1 8A 86 48 39 00 00 84 C0 75 06 32 C0 5E C2 08 00 8B 0D");
            o.keyReleased = at("56 8B F1 8A 86 48 39 00 00 84 C0 75 06 32 C0 5E C2 04 00");
            o.characterReceived = at("8B 54 24 04 66 83 FA 20 72 6E 8A 81 04 29 00 00");
            o.mousePressed = at("56 8B F1 8A 86 48 39 00 00 84 C0 0F 84 ? ? ? ? 8A 86 04 29 00 00 84 C0 74 19");
            o.mouseReleased = at("56 8B F1 8A 86 48 39 00 00 84 C0 0F 84 ? ? ? ? 8A 86 04 29 00 00 84 C0 0F 85");
            o.commandPressed = at("83 EC 08 56 8B F1 8A 86 48 39 00 00");
            o.commandReleased = at("57 8B F9 8A 87 48 39 00 00 84 C0 75 06");
            o.removed = at("56 8B F1 8B 4E 0C 83 C1 74 57 74 55");
            o.screenTerminated = at("56 6A 01 8B F1 E8 ? ? ? ? 8D 4E 18");
            o.screenDestroyed = at("53 56 8B F1 8A 46 04 33 DB 3A C3 57");
            o.screenDeleted = at("56 8B F1 C7 06 ? ? ? ? E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 CC CC CC CC CC CC CC CC CC CC CC CC 53 8B 5C 24 08 84 DB 56 57 8B 3D ? ? ? ? 8B F1 0F 84");
            o.managerTerminated = at("53 56 8B F1 8B 46 74 8D 4E 74 57 FF 50 0C");
            o.managerDeleted = at("56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 CC CC 55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 1C 53 56 57 89 65 F0 8B F1 8B 55 08");
            o.managerDestroyed = at("53 56 8B F1 C7 06 ? ? ? ? 8B 86 20 02 00 00");
            o.interfaceTerminated = at("53 56 8B F1 8B 46 0C 33 DB 3B C3 74 2F");
            o.interfaceDeleted = at("56 8B F1 E8 ? ? ? ? F6 44 24 08 01 74 09 56 E8 ? ? ? ? 83 C4 04 8B C6 5E C2 04 00 CC CC A1 ? ? ? ? 81 EC 98 00 00 00");
            o.interfaceDestroyed = at("56 8B F1 8B 86 28 29 00 00 57 33 FF");
            o.shellDestroyed = at("83 EC 08 53 55 56 8B F1 8D 4E 08 C7 06 ? ? ? ?");
            o.shellTerminated = at("51 56 8B F1 8B 0D ? ? ? ? 85 C9 74 27");
            o.textDeleted = at("56 8B F1 57 C7 06 ? ? ? ? FF 15 ? ? ? ? 8D 7E 54 8B CF E8 ? ? ? ? 8D 4F 30 FF 15 ? ? ? ? 8B CE");
            o.allocate = at("E8 ? ? ? ? 84 C0 74 11 8B 0D ? ? ? ? 8B 54 24 04 8B 01 52 FF 50 04");
            o.free = at("E8 ? ? ? ? 84 C0 74 11 8B 0D ? ? ? ? 8B 54 24 04 8B 01 52 FF 50 08");
            o.textConstruct = at("56 8B F1 E8 ? ? ? ? 8D 4E 54 C7 06 ? ? ? ?");
            o.textCreate = at("56 8B F1 8B 4C 24 0C 8B 06 51 8B CE");
            o.textSet = at("83 C1 54 E9 ? ? ? ? CC CC CC CC CC CC CC CC 83 EC 28");
            o.textFontSize = at("8B 44 24 04 85 C0 56 8B F1 75 06 32 C0 5E C2 04 00 85 C0 89 44 24 08 DB 44 24 08 89 86 D0 00 00 00");
            o.stringBuild = at("81 EC 84 00 00 00 56 8B F1 8B 46 44");
            o.textBounds = at("8B 44 24 04 50 83 C1 54 E8 ? ? ? ?");
            o.controlSelect = at("8A 44 24 04 38 41 40 74 0B 88 41 40");
            o.stringSet = at("53 8B 5C 24 08 85 DB 56 8B F1 75 1C");
            o.textScale = at("83 EC 08 56 8B F1 D9 46 28 8B 4C 24 10");
            o.controlEnable = at("8A 44 24 04 84 C0 88 41 41 75 13 8A 41 40");
            o.interfaceManager = operand("8B 0D ? ? ? ? 50 8D 54 24 34 52", 2);
            ::interfaceManager.SetAddress(base + o.interfaceManager);
            o.provider = operand("8B 0D ? ? ? ? 8B 96 F0 00 00 00", 2);
            ::provider.SetAddress(base + o.provider);
            o.overlayFirst = operand("8B 0D ? ? ? ? 85 C9 74 0E 83 FB 68", 2);
            ::overlayFirst.SetAddress(base + o.overlayFirst);
            o.overlaySecond = operand("8B 0D ? ? ? ? 85 C9 74 07 6A 01", 2);
            ::overlaySecond.SetAddress(base + o.overlaySecond);
            o.connection = operand("8B 35 ? ? ? ? 8B B6 B8 00 00 00", 2);
            ::connection.SetAddress(base + o.connection);
            o.gameState = operand("89 3D ? ? ? ? 39 BE 38 23 00 00", 2);
            ::gameState.SetAddress(base + o.gameState);
            o.textTable = operand("C7 06 ? ? ? ? E8 ? ? ? ? 33 C0 89 86 D0 00 00 00", 2);
            o.interfaceTable = operand("C7 06 ? ? ? ? 74 19 8B 0D ? ? ? ?", 2);
            o.managerTable = operand("C7 06 ? ? ? ? 8B 86 20 02 00 00", 2);
            o.screenTable = operand("C7 06 ? ? ? ? 88 8E A8 01 00 00", 2);
            return o;
        }();
        return offsets;
    }

    template<class T> inline std::optional<T> Read(const void* p, size_t offset)
    {
        if (!p) return {};
        T value{};
        std::memcpy(&value, reinterpret_cast<const uint8_t*>(p) + offset, sizeof(value));
        return value;
    }
    class Calls
    {
        uintptr_t module{};
        template<class F> F Function(uint32_t rva) const { return reinterpret_cast<F>(module + rva); }
    public:
        bool Bind(HMODULE image)
        {
            module = reinterpret_cast<uintptr_t>(image);
            return module != 0;
        }
        bool Row(const void* row) const
        {
            return module && row && Read<uintptr_t>(row, 0) == module + Rva().textTable;
        }
        // Allocator and deleting destructor are both native. A null allocation
        // is rejected before calling the native constructor/Create methods.
        void* MakeText(const wchar_t* text, const Font& font, const Create& params) const
        {
            if (!module || !text || !font.height || !font.face[0] || !std::memchr(font.face, 0, sizeof(font.face))) return nullptr;
            void* row = Function<void* (__cdecl*)(size_t)>(Rva().allocate)(220);
            if (!row) return nullptr;
            Function<void* (__thiscall*)(void*)>(Rva().textConstruct)(row);
            if (!Function<bool (__thiscall*)(void*, const wchar_t*, const Font*, const Create*)>(Rva().textCreate)
                (row, text, &font, &params))
            {
                Destroy(row); return nullptr;
            }
            return row;
        }
        void Destroy(void* row) const
        {
            if (Row(row)) Function<void* (__thiscall*)(void*, uint32_t)>(Rva().textDeleted)(row, 1);
        }
        // Literal wchar text is copied into the engine's owned std::wstring.
        // The caller's buffer is never retained by this setter.
        bool SetText(void* row, const wchar_t* text) const
        {
            if (!Row(row) || !text) return false;
            Function<void (__thiscall*)(void*, const wchar_t*)>(Rva().textSet)(row, text);
            return true;
        }
        bool FontSize(void* row, uint32_t height) const
        {
            return Row(row) && height && Function<bool (__thiscall*)(void*, uint32_t)>(Rva().textFontSize)(row, height);
        }
        // Native font/provider readiness is a required future host precondition;
        // dimension calls must run outside all native row/input/render traversals.
        bool BuildString(void* row) const
        {
            if (!Row(row)) return false;
            Function<int (__thiscall*)(void*)>(Rva().stringBuild)(reinterpret_cast<uint8_t*>(row) + 84);
            return Read<uintptr_t>(row, 124).value_or(0) != 0;
        }
        std::optional<RECT> Bounds(void* row) const
        {
            if (!Row(row) || !Read<uintptr_t>(row, 124).value_or(0)) return {};
            RECT box{};
            if (!Function<bool (__thiscall*)(void*, RECT*)>(Rva().textBounds)(row, &box)) return {};
            if (box.right < box.left || box.bottom < box.top) return {};
            return box;
        }
        std::optional<std::wstring_view> Text(void* row) const
        {
            if (!Row(row)) return {};
            const auto length = Read<uint32_t>(row, 152).value_or(UINT32_MAX);
            const auto capacity = Read<uint32_t>(row, 156).value_or(0);
            if (length > 4096 || capacity < length) return {};
            const auto data = capacity < 8 ? reinterpret_cast<const wchar_t*>(reinterpret_cast<const uint8_t*>(row) + 136) :
                Read<const wchar_t*>(row, 136).value_or(nullptr);
            if (!data || data[length]) return {};
            return std::wstring_view(data, length);
        }
    };
}

// Read-only, disconnected state layer. Exact offsets come from the installed
// interfaceManager/manager keyboard, switching, modal and source-render machine bodies.
// It does not establish a deletion lease or publish/retire any native control.
export namespace CondemnedMenuNative
{
    struct View
    {
        uintptr_t module{};
        void* interfaceManager{}, *manager{}, *screen{};
        uint32_t screenId{};
        uintptr_t originalVtable{};
        static std::optional<View> Get(HMODULE image, uintptr_t copiedVtable = 0)
        {
            View out{}; out.module = reinterpret_cast<uintptr_t>(image);
            if (!out.module) return {};
            out.interfaceManager = Read<void*>((Rva(), ::interfaceManager.get_ptr()), 0).value_or(nullptr);
            if (!out.interfaceManager || Read<uintptr_t>(out.interfaceManager, 0) != out.module + Rva().interfaceTable ||
                !Read<uint8_t>(out.interfaceManager, 14664).value_or(0)) return {};
            out.manager = reinterpret_cast<uint8_t*>(out.interfaceManager) + 15816;
            if (Read<uintptr_t>(out.manager, 0) != out.module + Rva().managerTable) return {};
            out.screen = Read<void*>(out.manager, 96).value_or(nullptr);
            out.screenId = Read<uint32_t>(out.manager, 88).value_or(UINT32_MAX);
            if (!out.screen || Read<uint32_t>(out.screen, 16) != out.screenId ||
                Read<void*>(out.screen, 12) != out.manager) return {};
            out.originalVtable = out.module + Rva().screenTable;
            const auto table = Read<uintptr_t>(out.screen, 0).value_or(0);
            if (table != out.originalVtable && (!copiedVtable || table != copiedVtable)) return {};
            const auto begin = Read<uintptr_t>(out.manager, 104).value_or(0);
            const auto end = Read<uintptr_t>(out.manager, 108).value_or(0);
            if (!begin || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64) return {};
            bool member = false;
            for (auto at = begin; at < end; at += 4)
                if (Read<void*>(reinterpret_cast<void*>(at), 0) == out.screen) member = true;
            if (!member) return {};
            return out;
        }
        bool Same(const View& other) const
        {
            return module == other.module && interfaceManager == other.interfaceManager && manager == other.manager &&
                screen == other.screen && screenId == other.screenId;
        }
        bool ScreenMode() const { return Read<uint32_t>(interfaceManager, 8) == 6; }
        bool Covered() const
        {
            // CMessageBox embedded at interfaceManager+9100, active byte+1400.
            if (Read<uint8_t>(interfaceManager, 10500).value_or(1)) return true;
            // These two overlay providers suppress native interfaceManager input first.
            Rva();
            for (auto address : { ::overlayFirst.get_ptr(), ::overlaySecond.get_ptr() })
            {
                const auto overlay = Read<void*>(address, 0).value_or(nullptr);
                if (overlay && Read<uint8_t>(overlay, 1048).value_or(1)) return true;
            }
            // Both native menu managers receive input before the screen manager.
            for (auto offset : {1444u, 4920u})
                if (Read<void*>(interfaceManager, offset + 8).value_or(nullptr)) return true;
            return false;
        }
        std::optional<uint16_t> Selected() const { return Read<uint16_t>(screen, 272); }
        void* SelectedRow() const
        {
            const auto index = Selected();
            const auto begin = Read<uintptr_t>(screen, 200).value_or(0);
            const auto end = Read<uintptr_t>(screen, 204).value_or(0);
            if (!index || !begin || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64 ||
                *index >= (end - begin) / 4) return nullptr;
            return Read<void*>(reinterpret_cast<void*>(begin + 4 * *index), 0).value_or(nullptr);
        }
        // Existing native Escape checks this same connection field and game-state
        // predicate. This is context only; no world or UI state is modified.
        bool PauseContext() const
        {
            const auto connection = Read<void*>((Rva(), ::connection.get_ptr()), 0).value_or(nullptr);
            const auto game = Read<void*>((Rva(), ::gameState.get_ptr()), 0).value_or(nullptr);
            const auto state = Read<uint32_t>(game, 4);
            const auto mode = Read<uint32_t>(connection, 48);
            return mode && *mode != 0 && (*mode == 1 || (state && *state != 4 && *state != 5));
        }
    };
}

// Disconnected extension for the saved native TextCtrl scale and Base enable
// methods. Caller owns the complete traversal lease and must revalidate its
// source after each native method; disabling can invoke native selection logic.
export namespace CondemnedMenuNative
{
    class ControlCalls
    {
        uintptr_t module{};
    public:
        bool Bind(const Calls& calls, const View& view, void* originalRow)
        {
            module = 0;
            if (!calls.Row(originalRow) || Read<uintptr_t>(originalRow, 0) != view.module + Rva().textTable) return false;
            module = view.module;
            return true;
        }
        bool Scale(const Calls& calls, void* row, float x, float y) const
        {
            if (!module || !calls.Row(row) || Read<uintptr_t>(row, 0) != module + Rva().textTable ||
                !std::isfinite(x) || !std::isfinite(y) || x <= 0 || y <= 0) return false;
            float scale[2]{x, y};
            reinterpret_cast<void (__thiscall*)(void*, const float*)>(module + Rva().textScale)(row, scale);
            return true;
        }
        bool Enable(const Calls& calls, void* row, bool enabled) const
        {
            if (!module || !calls.Row(row) || Read<uintptr_t>(row, 0) != module + Rva().textTable) return false;
            reinterpret_cast<char (__thiscall*)(void*, char)>(module + Rva().controlEnable)(row, enabled ? 1 : 0);
            return true;
        }
    };
}

// Disconnected readiness snapshot. The provider's methods are reached only by
// guarded GameClient wrappers; this does not invent an engine-provider ABI.
// Probe is permitted only under the future host's complete traversal lease.
export namespace CondemnedMenuNative
{
    struct Ready
    {
        View view;
        void* sourceRow{};
        void* provider{};
        uintptr_t providerTable{};
        Font font;
        float scaleX{}, scaleY{};
        RECT sourceRect{};
        uint32_t selectedColor{}, enabledColor{}, disabledColor{};
        static bool Provider(void* object, uintptr_t table)
        {
            if (!object || !table || Read<uintptr_t>(object, 0) != table) return false;
            // StringBuild uses provider slots 16/20, Bounds slot 36: the provider is ready once they are set.
            for (auto offset : {16u, 20u, 36u})
                if (!Read<uintptr_t>(reinterpret_cast<void*>(table), offset).value_or(0)) return false;
            return true;
        }
        static std::optional<Ready> Capture(const Calls& calls, const View& source, void* row)
        {
            if (!source.ScreenMode() || source.Covered() || !calls.Row(row)) return {};
            const auto begin = Read<uintptr_t>(source.screen, 200).value_or(0);
            const auto end = Read<uintptr_t>(source.screen, 204).value_or(0);
            if (!begin || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64) return {};
            bool member{};
            for (auto at = begin; at < end; at += 4)
                if (Read<void*>(reinterpret_cast<void*>(at), 0) == row) member = true;
            if (!member || !Read<uint8_t>(row, 20).value_or(0) || !Read<uintptr_t>(row, 124).value_or(0)) return {};
            Ready out{}; out.view = source; out.sourceRow = row;
            out.provider = Read<void*>((Rva(), ::provider.get_ptr()), 0).value_or(nullptr);
            out.providerTable = Read<uintptr_t>(out.provider, 0).value_or(0);
            if (!Provider(out.provider, out.providerTable)) return {};
            const auto font = Read<Font>(row, 84);
            const auto baseHeight = Read<uint32_t>(row, 208);
            const auto x = Read<float>(row, 40), y = Read<float>(row, 44);
            const auto rect = Read<RECT>(row, 24);
            if (!font || !baseHeight || !*baseHeight || *baseHeight > 256 ||
                !font->face[0] || !std::memchr(font->face, 0, sizeof(font->face)) ||
                !x || !y || !std::isfinite(*x) || !std::isfinite(*y) || *x <= 0 || *y <= 0 ||
                !rect || rect->right <= rect->left || rect->bottom <= rect->top) return {};
            out.font = *font; out.font.height = *baseHeight;
            out.scaleX = *x; out.scaleY = *y; out.sourceRect = *rect;
            out.selectedColor = Read<uint32_t>(row, 68).value_or(0);
            out.enabledColor = Read<uint32_t>(row, 72).value_or(0);
            out.disabledColor = Read<uint32_t>(row, 76).value_or(0);
            return out;
        }
        bool SameProvider() const
        {
            return Read<void*>((Rva(), ::provider.get_ptr()), 0) == provider &&
                Provider(provider, providerTable);
        }
        template<class Current> bool Probe(const Calls& calls, Current current) const
        {
            if (!current() || !SameProvider() || !calls.Row(sourceRow)) return false;
            const auto text = calls.Text(sourceRow);
            if (!text || text->empty()) return false;
            const auto bounds = calls.Bounds(sourceRow);
            // Actual native dimensions are physical pixels. Source readiness
            // does not claim a whole-value or accented-glyph rendering pass.
            return current() && SameProvider() && bounds &&
                static_cast<int64_t>(bounds->right) - bounds->left > 0 &&
                static_cast<int64_t>(bounds->bottom) - bounds->top >= 8;
        }
    };
}

export namespace CondemnedMenuNative
{
    class Bindings
    {
        uintptr_t module{};
    public:
        Calls calls;
        ControlCalls controls;
        bool Bind(HMODULE image)
        {
            if (!calls.Bind(image)) return false;
            module = reinterpret_cast<uintptr_t>(image);
            return true;
        }
        uintptr_t Base() const { return module; }
        template<class Function> Function At(uint32_t rva) const { return reinterpret_cast<Function>(module + rva); }
        void* Allocate(size_t bytes) const
        {
            return module && bytes && bytes <= 4096 ? At<void* (__cdecl*)(size_t)>(Rva().allocate)(bytes) : nullptr;
        }
        void Free(void* memory) const { if (module && memory) At<void (__cdecl*)(void*)>(Rva().free)(memory); }
        void Select(void* row, bool selected) const
        {
            // Native09410 invokes this common BaseControl selector for every
            // original subclass too; it is not restricted to the TextCtrl type.
            if (module && row)
                At<char (__thiscall*)(void*, char)>(Rva().controlSelect)(row, selected ? 1 : 0);
        }
        void StringText(void* string, const wchar_t* text) const
        {
            if (module && string && text) At<void (__thiscall*)(void*, const wchar_t*)>(Rva().stringSet)(string, text);
        }
        std::optional<std::wstring> StringText(void* string) const
        {
            const auto length = Read<uint32_t>(string, 68).value_or(UINT32_MAX);
            const auto capacity = Read<uint32_t>(string, 72).value_or(0);
            if (length > 4096 || capacity < length) return {};
            const auto data = capacity < 8 ? reinterpret_cast<const wchar_t*>(reinterpret_cast<uint8_t*>(string) + 52) :
                Read<const wchar_t*>(string, 52).value_or(nullptr);
            if (!data || data[length]) return {};
            return std::wstring(data, length);
        }
    };
}

// Disconnected physical text fitting. Caller owns the native/module/font lease
// and supplies an exact current-owner check after every native boundary.
export namespace CondemnedMenuNative
{
    template<class Fits> inline std::optional<std::vector<std::wstring>> WrapHelp(
        std::wstring_view text, size_t maximumLines, Fits fits)
    {
        if (text.size() > 4096 || text.find(L'\0') != std::wstring_view::npos) return {};
        std::vector<std::wstring> lines;
        std::wstring line;
        size_t at{};
        while (at < text.size())
        {
            const auto begin = text.find_first_not_of(L" \t\r\n", at);
            if (begin == std::wstring_view::npos) break;
            auto end = text.find_first_of(L" \t\r\n", begin);
            if (end == std::wstring_view::npos) end = text.size();
            const auto word = text.substr(begin, end - begin);
            auto candidate = line;
            if (!candidate.empty()) candidate += L' ';
            candidate += word;
            const auto accepted = fits(candidate);
            if (!accepted) return {};
            if (*accepted) line = std::move(candidate);
            else
            {
                if (line.empty() || lines.size() + 1 >= maximumLines) return {};
                lines.push_back(std::move(line));
                line = word;
                const auto wordFits = fits(line);
                if (!wordFits || !*wordFits) return {};
            }
            at = end;
        }
        if (!line.empty()) lines.push_back(std::move(line));
        if (lines.size() > maximumLines) return {};
        return lines;
    }

    inline std::optional<std::wstring> Wide(std::string_view text)
    {
        if (text.size() > 4096 || text.find('\0') != std::string_view::npos) return {};
        if (text.empty()) return std::wstring{};
        const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0) return {};
        std::wstring out(static_cast<size_t>(count), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), out.data(), count) != count) return {};
        return out;
    }
    inline size_t PrefixBoundary(std::wstring_view text, size_t count)
    {
        count = (std::min)(count, text.size());
        if (count && count < text.size() && text[count - 1] >= 0xD800 && text[count - 1] <= 0xDBFF &&
            text[count] >= 0xDC00 && text[count] <= 0xDFFF) --count;
        return count;
    }
    struct Fitted { std::wstring text; uint32_t fontHeight{}; RECT bounds{}; };
    template<class Current>
    inline std::optional<Fitted> Fit(const Calls& calls, void* row, std::wstring_view text,
        uint32_t initialHeight, uint32_t minimumHeight, int pixelWidth, int pixelHeight,
        bool fullValue, Current&& current, std::wstring_view requiredTail = {})
    {
        if (!initialHeight || initialHeight > 256 || !minimumHeight || minimumHeight > initialHeight || pixelWidth <= 0 ||
            pixelHeight < 8 || text.size() > 4096 || text.find(L'\0') != std::wstring_view::npos) return {};
        // Multiline text cannot silently escape a single native menu row.
        if (text.find_first_of(L"\r\n") != std::wstring_view::npos) return {};
        auto measure = [&](std::wstring_view value, uint32_t height) -> std::optional<Fitted>
        {
            if (!current()) return {};
            std::wstring copy(value);
            if (!calls.SetText(row, copy.c_str()) || !current()) return {};
            if (!calls.FontSize(row, height) || !current()) return {};
            if (!calls.BuildString(row) || !current()) return {};
            const auto box = calls.Bounds(row);
            if (!current() || !box) return {};
            const int64_t width = static_cast<int64_t>(box->right) - box->left;
            const int64_t measuredHeight = static_cast<int64_t>(box->bottom) - box->top;
            if (width > pixelWidth || measuredHeight > pixelHeight || (!value.empty() && measuredHeight < 8)) return {};
            return Fitted{std::move(copy), height, *box};
        };
        for (auto height = initialHeight; ; --height)
        {
            if (auto fitted = measure(text, height)) return fitted;
            if (!current() || height == minimumHeight) break;
        }
        if (fullValue || !current()) return {}; // Explicit unavailable, never partial value.
        if (!requiredTail.empty() && !text.ends_with(requiredTail)) return {};
        const auto prefix = text.substr(0, text.size() - requiredTail.size());
        const std::wstring suffix = std::wstring(L"...") + std::wstring(requiredTail);
        // At most13 metric candidates for a4096-unit label. Every accepted
        // candidate is measured; no monotonic-width assumption is needed for
        // safety (unusual kerning can only make the result more conservative).
        auto best = measure(suffix, minimumHeight);
        if (!best || !current()) return {};
        size_t low = 0, high = prefix.size();
        for (size_t attempt = 0; low < high && attempt < 13; ++attempt)
        {
            const auto midpoint = low + (high - low + 1) / 2;
            const auto count = PrefixBoundary(prefix, midpoint);
            std::wstring shortened(prefix.substr(0, count)); shortened += suffix;
            if (auto fitted = measure(shortened, minimumHeight))
            {
                best = std::move(fitted);
                low = midpoint;
            }
            else high = midpoint - 1;
            if (!current()) return {};
        }
        // Leave the native object's text/font consistent with the chosen result.
        return measure(best->text, best->fontHeight);
    }
}

// Disconnected logical preparation only. No native allocation, publication,
// callbacks or hooks. A future host must commit a Prepared view only after its
// exact native owner and traversal lease are revalidated.
export namespace CondemnedMenuNative
{
    enum class ItemKind { Link, ModelRow, Previous, Next, Back };
    struct Item
    {
        ItemKind kind{ItemKind::Back};
        size_t destination{};
        RWGTAMenu::Row row{};
        std::string label, value;
        bool enabled{true};
        std::string description, requiredTail;
    };
    struct Location
    {
        std::optional<size_t> page;
        size_t first{};
        bool operator==(const Location&) const = default;
    };
    struct Prepared
    {
        uint64_t revision{};
        Location location;
        bool resetsPageState{};
        std::string title;
        std::vector<Item> items;
    };
    class Session
    {
        RWGTAMenu::Model model;
        uint64_t revision{1};
        bool pause{};
        size_t foreignDepth{};
        Location shown;
        struct ForeignScope
        {
            Session& owner;
            explicit ForeignScope(Session& value) : owner(value) { ++owner.foreignDepth; }
            ~ForeignScope() { --owner.foreignDepth; }
        };
        template<class Current> bool Alive(uint64_t token, Current& current) const
        {
            return token == revision && model.IsOpen() && current();
        }
    public:
        static constexpr size_t pageRows = 5;
        uint64_t Revision() const { return revision; }
        bool IsOpen() const { return model.IsOpen(); }
        bool RestartRequired() const { return model.RestartRequired(); }
        bool InForeignCall() const { return foreignDepth != 0; }
        std::string Text(const std::string& key) const { return model.Text(key); }
        bool NeedsRefresh() const { return RWGTAMenu::Model::Changed().load(std::memory_order_acquire); }
        void BeginRefresh() { RWGTAMenu::Model::Changed().exchange(false, std::memory_order_acq_rel); }
        const Location& Shown() const { return shown; }
        // The native host admits Back only after capacity/selected-owner checks.
        // Logical cancellation can occur during a plugin callback; native controls
        // must remain intact until every enclosing engine traversal has returned.
        void Cancel()
        {
            ++revision;
            model.Close();
        }
        template<class Current> bool Open(HMODULE host, const std::string& language, bool inPause, Current current)
        {
            Cancel();
            const auto token = revision;
            pause = inPause;
            shown = {};
            { ForeignScope scope(*this); model.OpenPlugins(host, language, pageRows, {}, true); }
            if (!Alive(token, current))
            {
                if (token == revision) Cancel();
                return false;
            }
            return true;
        }
        template<class Current> std::optional<Prepared> Prepare(Location location, Current current)
        {
            const auto token = revision;
            if (!Alive(token, current)) return {};
            const bool entering = location.page && location.page != shown.page;
            auto refuse = [&]() -> std::optional<Prepared>
            {
                // PageShown resets preset confirmation. If preparation fails
                // after that logical change, require deferred native return,
                // rather than keep an old view with a mismatched confirmation.
                if (entering && token == revision) Cancel();
                return {};
            };
            Prepared result{token, location, entering, {}, {}};
            if (!location.page)
            {
                result.title = model.Text("menu.title");
                const auto& pages = model.Pages();
                if (location.first > pages.size()) return refuse();
                const auto end = (std::min)(pages.size(), location.first + pageRows);
                for (size_t i = location.first; i < end; ++i)
                    result.items.push_back({ItemKind::Link, i, {}, pages[i].title, {}, true});
                // Preserve numbered page suffixes if a long sibling name is
                // shortened. This is generated UI identity, never setting value.
                for (auto& item : result.items)
                {
                    const auto at = item.label.rfind(" (");
                    if (at != std::string::npos && item.label.ends_with(')')) item.requiredTail = item.label.substr(at);
                }
                if (location.first)
                    result.items.push_back({ItemKind::Previous, location.first > pageRows ? location.first - pageRows : 0,
                        {}, model.Text("menu.previous"), {}, true});
                if (end < pages.size())
                    result.items.push_back({ItemKind::Next, end, {}, model.Text("menu.next"), {}, true});
            }
            else
            {
                const auto page = *location.page;
                if (page >= model.Pages().size()) return refuse();
                result.title = model.Pages()[page].title;
                if (location.page != shown.page) model.PageShown(page);
                std::vector<const RWGTAMenu::Row*> visible;
                { ForeignScope scope(*this); visible = model.VisibleRows(page); }
                if (!Alive(token, current)) return refuse();
                // Copy every row before the first subsequent plugin call. A
                // read can close/reopen the model and invalidate all page rows.
                std::vector<RWGTAMenu::Row> rows;
                rows.reserve(visible.size());
                for (auto row : visible) rows.push_back(*row);
                if (location.first > rows.size()) return refuse();
                const auto end = (std::min)(rows.size(), location.first + pageRows);
                for (size_t i = location.first; i < end; ++i)
                {
                    const auto& row = rows[i];
                    Item item{ItemKind::ModelRow, 0, row, {}, {}, false};
                    item.label = model.Label(row);
                    item.description = model.Help(row);
                    bool enabled{};
                    { ForeignScope scope(*this); enabled = model.Enabled(row, pause); }
                    if (!Alive(token, current)) return refuse();
                    item.enabled = enabled;
                    { ForeignScope scope(*this); item.value = enabled ? model.Value(row) : model.Text("menu.unavailable"); }
                    if (!Alive(token, current)) return refuse();
                    result.items.push_back(std::move(item));
                }
                if (location.first)
                    result.items.push_back({ItemKind::Previous, location.first > pageRows ? location.first - pageRows : 0,
                        {}, model.Text("menu.previous"), {}, true});
                if (end < rows.size())
                    result.items.push_back({ItemKind::Next, end, {}, model.Text("menu.next"), {}, true});
            }
            result.items.push_back({ItemKind::Back, 0, {}, model.Text("menu.back"), {}, true});
            if (!Alive(token, current)) return refuse();
            return result;
        }
        // Host calls Reject after fitting/publication refusal, before deferred
        // native Back. The old widgets then cannot dispatch stale Model actions.
        void Reject(const Prepared& candidate)
        {
            if (candidate.revision == revision && candidate.resetsPageState) Cancel();
        }
        // Called only after native geometry/text/readback has succeeded. A failed
        // root preparation preserves navigation; failed page-entry reset
        // cancels logically and requires the host's safe deferred native return.
        template<class Current> bool Commit(Prepared& candidate, Current current)
        {
            if (!Alive(candidate.revision, current)) return false;
            shown = candidate.location;
            candidate.revision = ++revision; // former physical pool actions are stale
            return true;
        }
        template<class Current> bool Change(const Prepared& displayed, size_t index, int direction, Current current)
        {
            if (!Alive(displayed.revision, current) || displayed.location != shown ||
                index >= displayed.items.size()) return false;
            const auto token = revision;
            const auto item = displayed.items[index];
            if (item.kind != ItemKind::ModelRow || !item.enabled || direction < -1 || direction > 1) return false;
            if (direction && item.row.kind != RWGTAMenu::RowKind::Setting && item.row.kind != RWGTAMenu::RowKind::Preset)
                return false;
            bool enabled{};
            { ForeignScope scope(*this); enabled = model.Enabled(item.row, pause); }
            if (!Alive(token, current) || !enabled) return false;
            bool changed{};
            { ForeignScope scope(*this); changed = model.Change(item.row, direction, pause); }
            return Alive(token, current) && changed;
        }
    };
}

// Disconnected, allocation-free callback storage. Admission still requires the
// future host's exact sender/current-selection/modal checks. No engine calls,
// strings, plugin API calls or native ownership changes occur here.
export namespace CondemnedMenuNative
{
    enum class CommandKind : uint8_t { Open, Accept, Left, Right, Back };
    struct Request
    {
        CommandKind kind{};
        uint64_t owner{}, pool{};
        uint16_t selected{};
        uintptr_t sender{};
    };
    class Pending
    {
        std::array<Request, 32> entries{};
        size_t first{}, count{};
    public:
        bool Full() const noexcept { return count == entries.size(); }
        bool Empty() const noexcept { return count == 0; }
        // A caller consumes input only on true. Capacity is checked before any
        // logical Back cancellation, so a rejected key cannot retire a session.
        bool Push(Request request) noexcept
        {
            if (Full()) return false;
            entries[(first + count) % entries.size()] = request;
            ++count;
            return true;
        }
        std::optional<Request> Pop() noexcept
        {
            if (!count) return {};
            auto request = entries[first];
            first = (first + 1) % entries.size();
            --count;
            return request;
        }
        void Clear() noexcept { first = count = 0; }
    };
}

// Disconnected host bookkeeping, not hook installation. The future wrappers
// must enter every proved whole engine scope, including owning deletion callers.
// Zero counters alone do not prove an arbitrary callback is a service boundary.
export namespace CondemnedMenuNative
{
    enum class Phase : size_t { Update, Input, Render, Transition, Lifecycle, Foreign, Service, Count };
    class Traversals
    {
        std::atomic<DWORD> thread{};
        std::array<uint32_t, static_cast<size_t>(Phase::Count)> depths{};
        bool afterUpdate{};
    public:
        class Scope
        {
            Traversals* owner{};
            size_t index{};
        public:
            Scope() = default;
            Scope(Traversals& value, size_t at) : owner(&value), index(at) { ++owner->depths[index]; }
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;
            Scope(Scope&& other) noexcept : owner(std::exchange(other.owner, nullptr)), index(other.index) {}
            ~Scope() { if (owner) --owner->depths[index]; }
            explicit operator bool() const { return owner != nullptr; }
        };
        // Called by the exact engine-facing Update wrapper, never by input or
        // discovery callbacks. Other threads must delegate stock behavior only.
        bool BindUpdateThread()
        {
            const auto now = GetCurrentThreadId();
            DWORD empty{};
            thread.compare_exchange_strong(empty, now, std::memory_order_acq_rel);
            return thread.load(std::memory_order_acquire) == now;
        }
        bool Here() const
        {
            const auto bound = thread.load(std::memory_order_acquire);
            return bound && bound == GetCurrentThreadId();
        }
        Scope Enter(Phase phase)
        {
            const auto index = static_cast<size_t>(phase);
            if (!Here() || index >= depths.size() || depths[index] == UINT32_MAX) return {};
            if (phase == Phase::Update) afterUpdate = false;
            return Scope(*this, index);
        }
        bool Quiet() const
        {
            return Here() && std::all_of(depths.begin(), depths.end(), [](uint32_t value) { return value == 0; });
        }
        // Invoked only after original whole10051150 and its Update Scope return.
        // If an enclosing broker/render/lifecycle is still active, its return
        // cannot be replaced with an assumed leaf/broker service point.
        bool MarkCompleteUpdate()
        {
            if (!Quiet()) return false;
            afterUpdate = true;
            return true;
        }
        Scope Service()
        {
            if (!Quiet() || !afterUpdate) return {};
            afterUpdate = false;
            return Enter(Phase::Service);
        }
    };
    struct LogicalOwner
    {
        uint64_t identity{}, pool{};
        Session settings;
        Pending pending;
        bool returnPending{};
        // admission supplied by the host includes exact native owner, selected
        // widget, current pool and modal/focus gates; this function does not
        // pretend a cached raw pointer is a native lifetime lease.
        bool Queue(Request request, bool admitted)
        {
            if (!admitted || request.owner != identity || request.pool != pool || !pending.Push(request)) return false;
            if (request.kind == CommandKind::Back && settings.InForeignCall())
            {
                returnPending = true;
                settings.Cancel(); // logical only; shared API burst stops now
            }
            return true;
        }
        void Invalidate()
        {
            settings.Cancel();
            returnPending = true;
            ++pool;
            pending.Clear();
        }
    };
}

export namespace CondemnedMenuNative
{
    struct Array
    {
        void** begin{}, **end{}, **capacity{};
        size_t Size() const { return begin && end >= begin ? static_cast<size_t>(end - begin) : 0; }
        static std::optional<Array> Get(void* screen)
        {
            Array out{Read<void**>(screen, 200).value_or(nullptr), Read<void**>(screen, 204).value_or(nullptr),
                Read<void**>(screen, 208).value_or(nullptr)};
            const auto b = reinterpret_cast<uintptr_t>(out.begin), e = reinterpret_cast<uintptr_t>(out.end),
                c = reinterpret_cast<uintptr_t>(out.capacity);
            if (!b || e < b || c < e || (e - b) % 4 || (c - b) % 4 || (c - b) / 4 > 64) return {};
            return out;
        }
        bool Same(const Array& other) const { return begin == other.begin && end == other.end && capacity == other.capacity; }
        void Put(void* screen) const
        {
            // Fields are changed only at the complete verified service boundary.
            std::memcpy(reinterpret_cast<uint8_t*>(screen) + 200, this, sizeof(*this));
        }
    };
    static_assert(sizeof(Array) == 12);
    struct Pool
    {
        static constexpr size_t maxItems = 8; //5 root links+Previous+Next+Back
        static constexpr size_t helpLines = 5;
        static constexpr size_t fixedRows = 2 + helpLines;
        std::array<void*, fixedRows + 2 * maxItems> nodes{};
        size_t count{};
        Array array;
        Prepared presented;
        std::array<uint16_t, maxItems> indices{};
        std::string description;
        std::string footer;
        bool described{};
        // No destructor calls into the game. Explicit cleanup requires the full
        // traversal/resource lease and the host's authorized deletion scope.
        void Destroy(const Bindings& api)
        {
            for (auto& node : nodes) { if (node) api.calls.Destroy(node); node = nullptr; }
            if (array.begin) api.Free(array.begin);
            array = {}; count = 0;
        }
        bool Contains(const void* object) const
        {
            return object && std::find(nodes.begin(), nodes.end(), object) != nodes.end();
        }
        std::optional<size_t> Item(uint16_t physical, void* selected) const
        {
            for (size_t i = 0; i < presented.items.size(); ++i)
                if (indices[i] == physical && nodes[physical] == selected) return i;
            return {};
        }
    };
    inline std::optional<RECT> Client(const View& view)
    {
        const auto rect = Read<RECT>(view.screen, 220);
        if (!rect || rect->right <= rect->left || rect->bottom <= rect->top ||
            static_cast<int64_t>(rect->right) - rect->left > 8192 ||
            static_cast<int64_t>(rect->bottom) - rect->top > 8192) return {};
        // The remaining width is the photograph, outside the paper list.
        auto content = *rect;
        content.right = content.left + (content.right - content.left) * 2 / 5;
        return content;
    }
    template<class Current> inline void* TextNode(const Bindings& api, const Ready& ready, Pool& pool,
        const RECT& rectangle, void* handler, uint32_t command, uint32_t item, uint32_t generation, bool selectable,
        Current& current)
    {
        if (!current() || pool.count == pool.nodes.size()) return nullptr;
        Create params{}; params.command = command; params.handler = handler;
        params.parameter1 = item; params.parameter2 = generation; params.rect = rectangle;
        const auto row = api.calls.MakeText(L" ", ready.font, params);
        if (!row) return nullptr;
        pool.nodes[pool.count++] = row; // retain even if the owner changed in Create
        if (!current() || !api.controls.Scale(api.calls, row, ready.scaleX, ready.scaleY) || !current()) return nullptr;
        for (const auto& color : {std::pair<size_t, uint32_t>{68, ready.selectedColor}, {72, ready.enabledColor},
            {76, selectable ? ready.disabledColor : ready.enabledColor}})
            std::memcpy(reinterpret_cast<uint8_t*>(row) + color.first, &color.second, sizeof(color.second));
        if (!api.controls.Enable(api.calls, row, selectable) || !current() ||
            Read<uint8_t>(row, 65).value_or(2) != static_cast<uint8_t>(selectable) ||
            Read<float>(row, 40) != ready.scaleX || Read<float>(row, 44) != ready.scaleY) return nullptr;
        return row;
    }
    template<class Current> inline bool FitControl(const Bindings& api, const Ready& ready, void* row,
        const std::string& utf8, int logicalWidth, int logicalHeight, bool whole, Current& current, const std::string& requiredTail = {})
    {
        const auto text = Wide(utf8), tail = Wide(requiredTail);
        if (!text || !tail || !current()) return false;
        if (text->empty())
        {
            if (!api.calls.SetText(row, L"") || !current()) return false;
            const auto copied = api.calls.Text(row);
            return copied && copied->empty() && current();
        }
        const auto minHeight = static_cast<uint32_t>((std::max)(1.0, std::ceil(8.0 / ready.scaleY)));
        if (minHeight > ready.font.height) return false;
        const auto fitted = Fit(api.calls, row, *text, ready.font.height, minHeight,
            static_cast<int>(logicalWidth * ready.scaleX), static_cast<int>(logicalHeight * ready.scaleY), whole, current, *tail);
        if (!fitted || !current()) return false;
        const auto copied = api.calls.Text(row);
        return copied && *copied == fitted->text && current();
    }
    template<class Current> inline bool FitHelp(const Bindings& api, const Ready& ready, Pool& pool,
        const std::string& utf8, int width, int height, Current& current)
    {
        const auto text = Wide(utf8);
        if (!text || !current()) return false;
        const auto minimum = static_cast<uint32_t>((std::max)(1.0, std::ceil(8.0 / ready.scaleY)));
        if (minimum > ready.font.height) return false;
        for (auto font = ready.font.height; ; --font)
        {
            auto fits = [&](const std::wstring& line) -> std::optional<bool>
            {
                auto node = pool.nodes[1];
                if (!current() || !api.calls.SetText(node, line.c_str()) || !current() ||
                    !api.calls.FontSize(node, font) || !current() || !api.calls.BuildString(node) || !current()) return {};
                const auto bounds = api.calls.Bounds(node);
                if (!bounds || !current()) return {};
                return bounds->right - bounds->left <= width * ready.scaleX &&
                    bounds->bottom - bounds->top <= height * ready.scaleY;
            };
            const auto lines = WrapHelp(*text, Pool::helpLines, fits);
            if (!current()) return false;
            if (lines)
            {
                for (size_t i = 0; i < Pool::helpLines; ++i)
                {
                    auto node = pool.nodes[1 + i];
                    const auto line = i < lines->size() ? (*lines)[i].c_str() : L"";
                    if (!api.calls.SetText(node, line) || !current() || !api.calls.FontSize(node, font) || !current()) return false;
                    if (*line)
                    {
                        if (!api.calls.BuildString(node) || !current()) return false;
                        const auto bounds = api.calls.Bounds(node);
                        if (!bounds || !current() || bounds->right - bounds->left > width * ready.scaleX ||
                            bounds->bottom - bounds->top > height * ready.scaleY) return false;
                    }
                    const auto copied = api.calls.Text(node);
                    if (!copied || *copied != line || !current()) return false;
                }
                return true;
            }
            if (font == minimum) return false;
        }
    }
    template<class Current> inline bool PreparePool(const Bindings& api, const Ready& ready,
        Pool& out, Prepared prepared, const std::string& unavailable, uint32_t firstCommand, uint32_t generation,
        Current current)
    {
        const auto client = Client(ready.view);
        const auto count = prepared.items.size();
        if (!client || !count || count > Pool::maxItems || out.count || !current()) return false;
        const auto width = client->right - client->left;
        const auto rowHeight = (client->bottom - client->top) / static_cast<LONG>(count + Pool::fixedRows);
        if (width < 64 || rowHeight < 1 || rowHeight * ready.scaleY < 8) return false;
        const auto title = TextNode(api, ready, out, {client->left, client->top, client->right, client->top + rowHeight},
            nullptr, 0, 0, generation, false, current);
        if (!title || !FitControl(api, ready, title, prepared.title, width, rowHeight, false, current)) return false;
        for (size_t i = 1; i < Pool::fixedRows; ++i)
        {
            const auto top = client->top + rowHeight * static_cast<LONG>(count + i);
            const auto node = TextNode(api, ready, out,
                {client->left, top, client->right, top + rowHeight}, nullptr, 0, 0, generation, false, current);
            if (!node || !FitControl(api, ready, node, "", width, rowHeight, true, current)) return false;
        }
        for (size_t i = 0; i < count; ++i)
        {
            auto& item = prepared.items[i];
            const auto top = client->top + rowHeight * static_cast<LONG>(i + 1);
            const bool hasValue = !item.value.empty();
            const auto labelWidth = hasValue ? width * 3 / 5 - 8 : width;
            const auto valueLeft = hasValue ? labelWidth + 8 : width;
            const auto valueWidth = width - valueLeft;
            const RECT labelRect{client->left, top, client->left + labelWidth, top + rowHeight};
            const RECT valueRect{client->left + valueLeft, top, client->right, top + rowHeight};
            out.indices[i] = static_cast<uint16_t>(out.count);
            const auto label = TextNode(api, ready, out, labelRect, ready.view.screen, firstCommand,
                static_cast<uint32_t>(i), generation, item.enabled, current);
            const auto value = TextNode(api, ready, out, valueRect, nullptr, 0, 0, generation, false, current);
            if (!label || !value || !FitControl(api, ready, label, item.label, labelWidth, rowHeight, false, current, item.requiredTail)) return false;
            if (!FitControl(api, ready, value, item.value, (std::max)(1L, valueWidth), rowHeight, true, current))
            {
                if (!current()) return false;
                item.enabled = false;
                if (!api.controls.Enable(api.calls, label, false) || !current() ||
                    Read<uint8_t>(label, 65).value_or(2) != 0 ||
                    !FitControl(api, ready, value, unavailable, (std::max)(1L, valueWidth), rowHeight, true, current)) return false;
            }
        }
        out.array.begin = static_cast<void**>(api.Allocate(out.count * sizeof(void*)));
        if (!out.array.begin || !current()) return false;
        std::memcpy(out.array.begin, out.nodes.data(), out.count * sizeof(void*));
        out.array.end = out.array.capacity = out.array.begin + out.count;
        out.presented = std::move(prepared);
        return current();
    }
}

export namespace CondemnedMenuNative
{
    struct SourceCallbacks
    {
        uintptr_t command{}, key{}, escape{}, left{}, right{}, focus{};
    };
    struct Owner
    {
        Ready ready;
        Array original, entryArray;
        std::array<uintptr_t, 34> shadow{}; //RTTI[-1] plus all33 native slots
        std::wstring caption;
        void* entry{};
        uint16_t entryIndex{}, originalSelected{};
        LogicalOwner logical;
        std::unique_ptr<Pool> active, staging, retiring;
        bool retired{}, headerHidden{}, published{};
        uint32_t entryCommand{}, rowCommand{};
        uintptr_t Table() const { return reinterpret_cast<uintptr_t>(shadow.data() + 1); }
        bool Registered() const
        {
            const auto& view = ready.view;
            if (Read<void*>((Rva(), ::interfaceManager.get_ptr()), 0) != view.interfaceManager ||
                !Read<uint8_t>(view.interfaceManager, 14664).value_or(0) ||
                Read<uintptr_t>(view.interfaceManager, 0) != view.module + Rva().interfaceTable ||
                Read<uintptr_t>(view.manager, 0) != view.module + Rva().managerTable ||
                Read<void*>(view.screen, 12) != view.manager || Read<uint32_t>(view.screen, 16) != view.screenId) return false;
            const auto table = Read<uintptr_t>(view.screen, 0).value_or(0);
            if (table != view.originalVtable && table != Table()) return false;
            const auto begin = Read<uintptr_t>(view.manager, 104).value_or(0), end = Read<uintptr_t>(view.manager, 108).value_or(0);
            if (!begin || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64) return false;
            for (auto at = begin; at < end; at += 4)
                if (Read<void*>(reinterpret_cast<void*>(at), 0) == view.screen) return true;
            return false;
        }
        bool Current(bool allowCovered = false) const
        {
            if (retired || !Registered() || !ready.SameProvider()) return false;
            const auto view = View::Get(reinterpret_cast<HMODULE>(ready.view.module), Table());
            return view && ready.view.Same(*view) && view->ScreenMode() &&
                (allowCovered || (!view->Covered() && !Read<void*>(view->screen, 216).value_or(nullptr)));
        }
        bool Private() const
        {
            const auto array = Array::Get(ready.view.screen);
            return active && array && array->Same(active->array);
        }
        bool OwnedNode(void* node) const
        {
            return node && (node == entry || (active && active->Contains(node)) ||
                (staging && staging->Contains(node)) || (retiring && retiring->Contains(node)));
        }
        std::optional<size_t> SelectedItem() const
        {
            if (!Current() || !Private()) return {};
            const auto physical = ready.view.Selected();
            if (!physical) return {};
            const auto selected = ready.view.SelectedRow();
            if (!Read<uint8_t>(selected, 20).value_or(0) || !Read<uint8_t>(selected, 65).value_or(0)) return {};
            return active->Item(*physical, selected);
        }
        void Cancel()
        {
            retired = true;
            logical.Invalidate();
        }
        template<class Current> bool PrepareEntry(Bindings& api, uint64_t identity, uint32_t command,
            uint32_t rowsCommand, const std::string& label, const SourceCallbacks& callbacks, Current current)
        {
            if (!callbacks.command || !callbacks.key || !callbacks.escape || !callbacks.left || !callbacks.right || !callbacks.focus) return false;
            const auto array = Array::Get(ready.view.screen);
            const auto text = Wide(label);
            const auto oldCaption = api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24);
            const auto client = Client(ready.view);
            if (!array || array->Size() >= 63 || !text || !oldCaption || !client || !current()) return false;
            original = *array; caption = *oldCaption;
            originalSelected = ready.view.Selected().value_or(UINT16_MAX);
            const auto nativeBack = reinterpret_cast<uint8_t*>(ready.view.manager) + 116;
            LONG lastBottom = client->top, footer = client->bottom;
            entryIndex = static_cast<uint16_t>(original.Size());
            for (size_t i = 0; i < original.Size(); ++i)
            {
                const auto rect = Read<RECT>(original.begin[i], 24);
                if (!rect || rect->bottom < rect->top) return false;
                if (original.begin[i] == nativeBack)
                {
                    footer = (std::min)(footer, rect->top);
                    entryIndex = static_cast<uint16_t>(i);
                }
                else lastBottom = (std::max)(lastBottom, rect->bottom);
            }
            const auto gap = static_cast<int64_t>(footer) - lastBottom;
            const auto margin = static_cast<LONG>((std::max)(1.0, std::ceil(2.0 / ready.scaleY)));
            const auto height = static_cast<LONG>((std::max)(static_cast<double>(ready.font.height), std::ceil(8.0 / ready.scaleY)));
            if (gap < static_cast<int64_t>(height) + 2 * margin) return false;
            Create params{}; params.command = command; params.handler = ready.view.screen;
            params.parameter2 = static_cast<uint32_t>(identity);
            params.rect = {client->left, lastBottom + margin, client->right, lastBottom + margin + height};
            entry = api.calls.MakeText(text->c_str(), ready.font, params);
            if (!entry || !current() || !api.controls.Scale(api.calls, entry, ready.scaleX, ready.scaleY) || !current() ||
                !FitControl(api, ready, entry, label, client->right - client->left, height, true, current)) return false;
            for (const auto& color : {std::pair<size_t, uint32_t>{68, ready.selectedColor}, {72, ready.enabledColor}, {76, ready.disabledColor}})
                std::memcpy(reinterpret_cast<uint8_t*>(entry) + color.first, &color.second, 4);
            std::memcpy(shadow.data(), reinterpret_cast<void*>(ready.view.originalVtable - 4), shadow.size() * 4);
            shadow[2] = callbacks.command; shadow[6] = callbacks.escape; shadow[11] = callbacks.key;
            shadow[29] = callbacks.left; shadow[30] = callbacks.right; shadow[9] = callbacks.focus;
            entryArray.begin = static_cast<void**>(api.Allocate((original.Size() + 1) * 4));
            if (!entryArray.begin || !current()) return false;
            for (size_t i = 0, j = 0; i <= original.Size(); ++i)
                entryArray.begin[i] = i == entryIndex ? entry : original.begin[j++];
            entryArray.end = entryArray.capacity = entryArray.begin + original.Size() + 1;
            entryCommand = command; rowCommand = rowsCommand; logical.identity = identity; logical.pool = 1;
            return current();
        }
        bool PublishEntry(const Bindings& api)
        {
            const auto present = Array::Get(ready.view.screen);
            if (!Current() || !present || !present->Same(original) || !entryArray.begin) return false;
            entryArray.Put(ready.view.screen);
            const auto pointer = Table(); std::memcpy(ready.view.screen, &pointer, 4);
            for (auto offset : {272u, 274u, 278u})
            {
                auto index = Read<uint16_t>(ready.view.screen, offset).value_or(UINT16_MAX);
                if (index != UINT16_MAX && index >= entryIndex) ++index;
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &index, 2);
            }
            published = true;
            return Current() && Read<uint8_t>(entry, 20).value_or(0) && api.calls.Row(entry);
        }
        bool PublishPool(const Bindings& api)
        {
            if (!staging || !Current() || !logical.settings.IsOpen()) return false;
            const auto token = logical.settings.Revision();
            const auto before = Array::Get(ready.view.screen);
            if (!before || !(before->Same(entryArray) || (active && before->Same(active->array)))) return false;
            std::optional<size_t> preferred;
            const auto oldSelected = SelectedItem();
            if (oldSelected && active->presented.location == staging->presented.location)
            {
                const auto& old = active->presented.items[*oldSelected];
                for (size_t i = 0; i < staging->presented.items.size(); ++i)
                {
                    const auto& next = staging->presented.items[i];
                    const bool same = old.kind == next.kind && (old.kind == ItemKind::ModelRow ?
                        old.row.kind == next.row.kind && old.row.plugin == next.row.plugin && old.row.setting == next.row.setting :
                        old.destination == next.destination);
                    if (same && next.enabled) { preferred = i; break; }
                }
            }
            // Track suppression before native setter; a forced callback can cancel
            // construction and must still restore this exact source caption.
            headerHidden = true;
            api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24, L"");
            if (!Current() || token != logical.settings.Revision() || !HasArray(*before)) return false;
            retiring = std::move(active);
            active = std::move(staging);
            active->array.Put(ready.view.screen);
            uint16_t selected = preferred ? active->indices[*preferred] : UINT16_MAX;
            if (!preferred)
                for (size_t i = 0; i < active->presented.items.size(); ++i)
                    if (active->presented.items[i].enabled) { selected = active->indices[i]; break; }
            if (selected == UINT16_MAX) return false; //Back is always enabled
            for (auto offset : {272u, 274u, 278u})
            {
                const auto value = offset == 272 ? selected : UINT16_MAX;
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &value, 2);
            }
            const auto pool = active.get();
            const auto selectedNode = pool->nodes[selected];
            api.Select(selectedNode, true);
            if (!Current() || token != logical.settings.Revision() || active.get() != pool || !Private()) return false;
            if (Read<uint8_t>(selectedNode, 64).value_or(0) != 1) return false;
            if (!Current() || token != logical.settings.Revision() ||
                !logical.settings.Commit(active->presented, [&]() { return Current() && Private(); })) return false;
            ++logical.pool;
            return Current() && Private();
        }
        // These methods are used only after all original engine traversals return.
        // Resources are destroyed BEFORE related native interface/provider teardown.
        bool HasArray(const Array& expected) const
        {
            const auto present = Array::Get(ready.view.screen);
            return Registered() && present && present->Same(expected);
        }
        void ReturnToSource(const Bindings& api)
        {
            if (!Registered()) return;
            const auto array = Array::Get(ready.view.screen);
            if (!array || !(array->Same(entryArray) || (active && array->Same(active->array)))) return;
            const auto selected = Read<uint16_t>(ready.view.screen, 272).value_or(UINT16_MAX);
            if (selected < array->Size())
            {
                api.Select(array->begin[selected], false);
                if (!HasArray(*array)) return;
            }
            entryArray.Put(ready.view.screen);
            for (auto offset : {272u, 274u, 278u})
            {
                const uint16_t index = offset == 272 ? entryIndex : UINT16_MAX;
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &index, 2);
            }
            if (headerHidden && ready.SameProvider())
            {
                api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24, caption.c_str());
                if (!HasArray(entryArray)) return;
                headerHidden = false;
            }
            if (!HasArray(entryArray) || entryIndex >= entryArray.Size() || entryArray.begin[entryIndex] != entry) return;
            api.Select(entry, true);
        }
        void Restore(const Bindings& api)
        {
            if (!Registered()) { published = false; return; }
            const auto array = Array::Get(ready.view.screen);
            const bool owned = array && (array->Same(entryArray) || (active && array->Same(active->array)));
            std::array<uint16_t, 3> indices{originalSelected, UINT16_MAX, UINT16_MAX};
            if (owned)
            {
                // Preserve current stock selection/hover when removing just the
                // inserted entry. Private rows map back to the prior stock row.
                if (array->Same(entryArray))
                {
                    for (size_t i = 0; i < indices.size(); ++i)
                    {
                        const auto index = Read<uint16_t>(ready.view.screen, std::array<size_t, 3>{272, 274, 278}[i]).value_or(UINT16_MAX);
                        indices[i] = index == UINT16_MAX || index == entryIndex ?
                            (i == 0 ? originalSelected : UINT16_MAX) :
                            static_cast<uint16_t>(index > entryIndex ? index - 1 : index);
                    }
                }
                const auto selected = Read<uint16_t>(ready.view.screen, 272).value_or(UINT16_MAX);
                if (selected < array->Size())
                {
                    api.Select(array->begin[selected], false);
                    if (!HasArray(*array)) return;
                }
                original.Put(ready.view.screen);
                for (size_t i = 0; i < indices.size(); ++i)
                {
                    if (indices[i] >= original.Size()) indices[i] = UINT16_MAX;
                    std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + std::array<size_t, 3>{272, 274, 278}[i], &indices[i], 2);
                }
            }
            if (!Registered()) return;
            if (Read<uintptr_t>(ready.view.screen, 0) == Table())
                std::memcpy(ready.view.screen, &ready.view.originalVtable, 4);
            if (headerHidden && ready.SameProvider())
            {
                api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24, caption.c_str());
                if (!Registered() || (owned && !HasArray(original))) return;
                headerHidden = false;
            }
            // A foreign array is never evidence that cached original rows live.
            if (owned && HasArray(original) && indices[0] < original.Size())
                api.Select(original.begin[indices[0]], true);
            published = false;
        }
        bool PoolReferenced(const Pool& pool) const
        {
            const auto array = Array::Get(ready.view.screen);
            if (!array) return true;
            if (pool.array.begin && array->begin == pool.array.begin) return true;
            for (auto at = array->begin; at < array->end; ++at) if (pool.Contains(*at)) return true;
            return false;
        }
        bool DestroyPools(const Bindings& api)
        {
            for (auto pool : {active.get(), staging.get(), retiring.get()})
                if (pool && PoolReferenced(*pool)) return false;
            for (auto pool : {active.get(), staging.get(), retiring.get()}) if (pool) pool->Destroy(api);
            active.reset(); staging.reset(); retiring.reset();
            return true;
        }
        bool Referenced() const
        {
            if (Read<uintptr_t>(ready.view.screen, 0) == Table()) return true;
            const auto array = Array::Get(ready.view.screen);
            if (!array) return true; // unknown native vector is not detachment
            if (array->begin == entryArray.begin || (active && array->begin == active->array.begin) ||
                (staging && array->begin == staging->array.begin) ||
                (retiring && array->begin == retiring->array.begin)) return true;
            for (auto at = array->begin; at < array->end; ++at) if (OwnedNode(*at)) return true;
            return false;
        }
        bool DetachForTeardown()
        {
            // Only a queued highest related native owning operation authorizes
            // removal from a foreign vector. No native call/allocator/free occurs
            // here; all unrelated rows and the vector's native capacity survive.
            if (!ready.view.screen || Read<void*>(ready.view.screen, 12) != ready.view.manager ||
                Read<uint32_t>(ready.view.screen, 16) != ready.view.screenId) return false;
            const auto array = Array::Get(ready.view.screen);
            if (!array) return false;
            const bool ownedBuffer = array->begin == entryArray.begin ||
                (active && array->begin == active->array.begin) ||
                (staging && array->begin == staging->array.begin) ||
                (retiring && array->begin == retiring->array.begin);
            if (ownedBuffer)
            {
                // Never leave the engine pointing at a plugin pointer-buffer
                // that Dispose is about to free. Original storage was retained
                // through all hooked source deletion operations.
                if (!original.begin) return false;
                for (auto at = original.begin; at < original.end; ++at)
                    if (!Read<uint8_t>(*at, 20).value_or(0)) return false;
                original.Put(ready.view.screen);
                for (auto offset : {272u, 274u, 278u})
                {
                    const uint16_t index = offset == 272 && originalSelected < original.Size() ? originalSelected : UINT16_MAX;
                    std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &index, 2);
                }
                if (Read<uintptr_t>(ready.view.screen, 0) == Table())
                    std::memcpy(ready.view.screen, &ready.view.originalVtable, 4);
                return !Referenced();
            }
            std::array<void*, 3> selected{};
            constexpr std::array<size_t, 3> offsets{272, 274, 278};
            for (size_t i = 0; i < selected.size(); ++i)
            {
                const auto index = Read<uint16_t>(ready.view.screen, offsets[i]).value_or(UINT16_MAX);
                if (index < array->Size()) selected[i] = array->begin[index];
            }
            auto write = array->begin;
            for (auto at = array->begin; at < array->end; ++at) if (!OwnedNode(*at)) *write++ = *at;
            const Array detached{array->begin, write, array->capacity};
            detached.Put(ready.view.screen);
            for (size_t i = 0; i < selected.size(); ++i)
            {
                uint16_t index = UINT16_MAX;
                for (auto at = detached.begin; at < detached.end; ++at)
                    if (*at == selected[i]) index = static_cast<uint16_t>(at - detached.begin);
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offsets[i], &index, 2);
            }
            if (Read<uintptr_t>(ready.view.screen, 0) == Table())
                std::memcpy(ready.view.screen, &ready.view.originalVtable, 4);
            return !Referenced();
        }
        bool Dispose(const Bindings& api, bool forced = false)
        {
            Restore(api);
            if (Referenced() && !(forced && DetachForTeardown())) return false;
            if (!DestroyPools(api)) return false;
            if (entry) api.calls.Destroy(entry);
            entry = nullptr;
            if (entryArray.begin) api.Free(entryArray.begin);
            entryArray = {};
            return true;
        }
    };
}

export namespace CondemnedMenuNative
{
    // Terms run before nondeleting/scalar destructors. Interface/manager dtors
    // do NOT replace a queued Term: their machine bodies do not call Term.
    enum class LifeOp : uint8_t
    {
        ShellTerm, InterfaceTerm, ManagerTerm, ScreenTerm, RemoveAll,
        ShellDtor, InterfaceScalar, InterfaceDtor, ManagerScalar, ManagerDtor,
        ScreenScalar, ScreenDtor, TextScalar
    };
    struct DeleteJob { LifeOp operation{}; void* receiver{}; uint32_t flags{}; };
    class Lifecycle
    {
        std::array<DeleteJob, 64> pending{};
        size_t count{};
        std::optional<DeleteJob> authorized;
        bool cleaning{};
        static bool TermCovers(LifeOp parent, LifeOp child)
        {
            if (parent == LifeOp::ShellTerm)
                return child == LifeOp::InterfaceTerm || TermCovers(LifeOp::InterfaceTerm, child);
            if (parent == LifeOp::InterfaceTerm)
                return child == LifeOp::ManagerTerm || TermCovers(LifeOp::ManagerTerm, child);
            if (parent == LifeOp::ManagerTerm)
                return child == LifeOp::ScreenTerm || child == LifeOp::ScreenScalar ||
                    child == LifeOp::ScreenDtor || child == LifeOp::RemoveAll;
            return parent == LifeOp::ScreenTerm && child == LifeOp::RemoveAll;
        }
        static bool Covers(LifeOp parent, LifeOp child)
        {
            if (parent == child || TermCovers(parent, child)) return true;
            if (parent == LifeOp::ShellDtor)
                return child == LifeOp::InterfaceTerm || TermCovers(LifeOp::InterfaceTerm, child) ||
                    child == LifeOp::InterfaceDtor || child == LifeOp::ManagerDtor;
            if (parent == LifeOp::InterfaceScalar || parent == LifeOp::InterfaceDtor)
                return child == LifeOp::InterfaceDtor || child == LifeOp::ManagerDtor;
            if (parent == LifeOp::ManagerScalar) return child == LifeOp::ManagerDtor;
            if (parent == LifeOp::ScreenScalar) return child == LifeOp::ScreenDtor || child == LifeOp::RemoveAll;
            return parent == LifeOp::ScreenDtor && child == LifeOp::RemoveAll;
        }
        // Pending physical destruction is broader than authorized child calls:
        // an enclosing destructor makes later member scalar deletion stale even
        // when it invokes the member's nondeleting destructor, not its scalar.
        static bool Supersedes(LifeOp parent, LifeOp child)
        {
            if (Covers(parent, child)) return true;
            if (parent == LifeOp::ShellDtor)
                return child == LifeOp::InterfaceScalar || child == LifeOp::ManagerScalar;
            return (parent == LifeOp::InterfaceScalar || parent == LifeOp::InterfaceDtor) &&
                child == LifeOp::ManagerScalar;
        }
        static unsigned Rank(LifeOp operation)
        {
            // Preserve required Terms first. Destructors that do not own screen
            // deletion must wait requested child destruction before member free.
            if (operation <= LifeOp::RemoveAll) return static_cast<unsigned>(operation);
            if (operation == LifeOp::ScreenScalar || operation == LifeOp::ScreenDtor) return 5;
            if (operation == LifeOp::ManagerScalar || operation == LifeOp::ManagerDtor) return 6;
            if (operation == LifeOp::InterfaceScalar || operation == LifeOp::InterfaceDtor) return 7;
            return 8;
        }
    public:
        static bool Related(const Owner& source, LifeOp operation, void* object)
        {
            switch (operation)
            {
                case LifeOp::ShellTerm: case LifeOp::ShellDtor:
                    return object == reinterpret_cast<uint8_t*>(source.ready.view.interfaceManager) - 8;
                case LifeOp::InterfaceTerm: case LifeOp::InterfaceDtor: case LifeOp::InterfaceScalar:
                    return object == source.ready.view.interfaceManager;
                case LifeOp::ManagerTerm: case LifeOp::ManagerDtor: case LifeOp::ManagerScalar:
                    return object == source.ready.view.manager;
                case LifeOp::TextScalar: return source.OwnedNode(object);
                default: return object == source.ready.view.screen;
            }
        }
        bool Authorized(const Owner& source, LifeOp operation, void* object) const
        {
            if (!Related(source, operation, object)) return false;
            if (cleaning && operation == LifeOp::TextScalar) return true;
            return authorized && authorized->operation != operation && Covers(authorized->operation, operation);
        }
        bool Executing(DeleteJob job) const
        {
            return authorized && authorized->operation == job.operation && authorized->receiver == job.receiver;
        }
        bool Queue(Owner& source, DeleteJob job)
        {
            if (!Related(source, job.operation, job.receiver)) return false;
            // Logical cancellation precedes source loss. The queue is fixed-size;
            // no allocation or engine API occurs in a lifecycle callback.
            source.Cancel();
            if (job.operation == LifeOp::TextScalar) return true;
            // Owned text requests retire the whole pool; Dispose deletes each
            // exact owned text once before any parent/provider shutdown.
            for (size_t i = 0; i < count; ++i)
            {
                if (pending[i].operation == job.operation && pending[i].receiver == job.receiver)
                { pending[i].flags |= job.flags; return true; }
                if (Supersedes(pending[i].operation, job.operation)) return true;
            }
            for (size_t i = 0; i < count; )
            {
                if (Supersedes(job.operation, pending[i].operation)) pending[i] = pending[--count];
                else ++i;
            }
            // At most12 non-text operations exist for this singleton owner.
            // Text operations never occupy slots, so64 cannot fill normally.
            if (count == pending.size()) return false;
            pending[count++] = job;
            return true;
        }
        bool Empty() const { return count == 0; }
        template<class Cleanup> void CleanOwned(Cleanup cleanup)
        {
            const auto previous = cleaning;
            cleaning = true;
            try { cleanup(); }
            catch (...) { cleaning = previous; throw; }
            cleaning = previous;
        }
        template<class Dispose, class Original> bool Collect(Owner& source, Dispose dispose, Original original)
        {
            // Caller is AFTER original wholeUpdate with ALL native scopes zero.
            // This authorization covers only exact owned text cleanup, not a
            // reentrant higher owner operation triggered by a native destructor.
            cleaning = true;
            bool detached{};
            try { detached = dispose(count != 0); }
            catch (...) { cleaning = false; throw; }
            cleaning = false;
            if (!detached) return false; // retain jobs/storage until exact detach
            while (count)
            {
                size_t first = 0;
                for (size_t i = 1; i < count; ++i)
                    if (Rank(pending[i].operation) < Rank(pending[first].operation)) first = i;
                const auto job = pending[first];
                pending[first] = pending[--count];
                // Remove parent-covered pending descendants before native entry.
                for (size_t i = 0; i < count; )
                {
                    if (Supersedes(job.operation, pending[i].operation)) pending[i] = pending[--count];
                    else ++i;
                }
                authorized = job;
                try { original(job); }
                catch (...) { authorized.reset(); throw; }
                authorized.reset();
            }
            (void)source;
            return true;
        }
    };
}

export namespace CondemnedMenuNative
{
    enum Hook : size_t { Update, PostUpdate, Render, KeyDown, KeyUp, Character, MouseDown, MouseUp, CommandOn, CommandOff,
        RemoveAll, ScreenTerm, ScreenDtor, ScreenScalar, ManagerTerm, ManagerScalar, ManagerDtor,
        InterfaceTerm, InterfaceScalar, InterfaceDtor, ShellDtor, ShellTerm, TextScalar, HookCount };
    struct Host
    {
        Bindings api;
        Traversals scopes;
        Lifecycle life;
        std::shared_ptr<Owner> source;
        std::array<SafetyHookInline, HookCount> hooks;
        HMODULE ownModule{};
        uint64_t nextOwner{};
        uint32_t diagnostics{}, written{};
        bool ready{}, acceptDown{}, repeatAccept{};
    };
    // Native vtables retain our callbacks. Supported bridge pins both modules;
    // bookkeeping deliberately survives CRT/static engine teardown. Dynamic
    // plugin unloading is unsupported. Ordinary native Back does not leak pools.
    inline Host* host{};
    inline std::string notice;
    inline constexpr uint32_t entryCommand = 0x57465001, rowCommand = 0x57465002;
    enum Diagnostic : uint32_t { Unsupported = 1, SourceRefused = 2, EntryPublished = 4, ViewRefused = 8,
        PagePublished = 16, OwnerRetired = 32, HookRefused = 64, Exception = 128 };
    inline void FlushLog()
    {
        if (!host) return;
        const auto fresh = host->diagnostics & ~host->written;
        if (!fresh) return;
        for (auto item : { std::pair<uint32_t, const char*>{ Unsupported, "unsupported executable" },
            { SourceRefused, "Options menu not recognised" }, { ViewRefused, "page could not be shown" },
            { OwnerRetired, "menu owner retired" }, { HookRefused, "hooks could not be installed" },
            { Exception, "menu operation failed" } })
        {
            if (fresh & item.first)
                OutputDebugStringA((std::string("Condemned native menu: ") + item.second + "\n").c_str());
        }
        host->written |= fresh;
    }
    inline std::string Language()
    {
        // Auto uses English until the native locale accessor is established.
        const auto& selected = FixSettings().String("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (selected == tag) return selected;
        return "en";
    }
    inline bool Admit(void* receiver, CommandKind kind)
    {
        if (!host || !host->ready || !host->scopes.Here()) return false;
        const auto owner = host->source;
        if (!owner || owner->ready.view.screen != receiver || !owner->Current()) return false;
        const auto selected = owner->ready.view.Selected();
        const auto row = owner->ready.view.SelectedRow();
        if (!selected || !row) return false;
        if (kind == CommandKind::Open)
        {
            if (owner->logical.settings.IsOpen() || owner->Private() || *selected != owner->entryIndex || row != owner->entry) return false;
        }
        else
        {
            const bool discoveryBack = kind == CommandKind::Back && owner->logical.settings.InForeignCall() &&
                owner->logical.settings.IsOpen() && owner->HasArray(owner->entryArray) &&
                *selected == owner->entryIndex && row == owner->entry;
            if (!discoveryBack && (!owner->logical.settings.IsOpen() || !owner->SelectedItem())) return false;
        }
        if (kind == CommandKind::Accept)
        {
            const auto item = owner->SelectedItem();
            if (item && owner->active->presented.items[*item].kind == ItemKind::Back) kind = CommandKind::Back;
        }
        const Request request{kind, owner->logical.identity, owner->logical.pool, *selected, reinterpret_cast<uintptr_t>(row)};
        return owner->logical.Queue(request, true);
    }
    inline int __fastcall SourceCommand(void* self, void*, uint32_t command, uint32_t parameter1, uint32_t parameter2)
    {
        const auto owner = host ? host->source : nullptr;
        auto scope = host ? host->scopes.Enter(Phase::Input) : Traversals::Scope{};
        try
        {
            if (owner && owner->ready.view.screen == self)
            {
                if (command == entryCommand && parameter2 == static_cast<uint32_t>(owner->logical.identity))
                    return Admit(self, CommandKind::Open) ? 1 : 0;
                if (command == rowCommand && parameter2 == static_cast<uint32_t>(owner->logical.pool) &&
                    owner->SelectedItem() == parameter1)
                    return host->repeatAccept ? 1 : (Admit(self, CommandKind::Accept) ? 1 : 0);
            }
        }
        catch (...) { if (host) host->diagnostics |= Exception; return 0; }
        return host->api.At<int (__thiscall*)(void*, uint32_t, uint32_t, uint32_t)>(Rva().sourceCommand)(self, command, parameter1, parameter2);
    }
    inline char __fastcall SourceKey(void* self, void*, uint32_t key, uint32_t repeat)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try
        {
            if (key == VK_RETURN && host->source && host->source->ready.view.screen == self)
            {
                if (host->repeatAccept) return 1;
                const auto kind = host->source->logical.settings.IsOpen() ? CommandKind::Accept : CommandKind::Open;
                if (Admit(self, kind)) return 1;
            }
        }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<char (__thiscall*)(void*, uint32_t, uint32_t)>(Rva().sourceKey)(self, key, repeat);
    }
    inline int __fastcall SourceEscape(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try { if (Admit(self, CommandKind::Back)) return 1; }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<int (__thiscall*)(void*)>(Rva().sourceEscape)(self);
    }
    inline char __fastcall SourceLeft(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try { if (Admit(self, CommandKind::Left)) return 1; }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<char (__thiscall*)(void*)>(Rva().sourceLeft)(self);
    }
    inline char __fastcall SourceRight(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try { if (Admit(self, CommandKind::Right)) return 1; }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<char (__thiscall*)(void*)>(Rva().sourceRight)(self);
    }
    inline int __fastcall SourceFocused(void* self, void*, char focused)
    {
        auto scope = host->scopes.Enter(Phase::Transition);
        const auto owner = host->source;
        if (!focused && host->ready && host->scopes.Here() && owner && owner->ready.view.screen == self)
        {
            // SetCurrent calls this before changing current screen. Close the
            // logical owner now so a foreign preset burst aborts subsequent Sets.
            owner->logical.settings.Cancel();
            owner->logical.returnPending = true;
            owner->logical.pending.Clear();
            ++owner->logical.pool;
        }
        return host->api.At<int (__thiscall*)(void*, char)>(Rva().sourceFocused)(self, focused);
    }
    inline bool Build(const std::shared_ptr<Owner>& owner, Location location)
    {
        const auto identity = owner->logical.identity;
        const auto before = Array::Get(owner->ready.view.screen);
        if (!before) return false;
        auto current = [&]() { return host->source == owner && owner->logical.identity == identity &&
            owner->Current() && owner->HasArray(*before); };
        if (!current()) return false;
        // Clear before foreign reads: notifications arriving during preparation
        // remain set and trigger another safely scoped update afterward.
        owner->logical.settings.BeginRefresh();
        auto prepared = owner->logical.settings.Prepare(location, current);
        if (!prepared || !current()) return false;
        owner->staging = std::make_unique<Pool>();
        if (!PreparePool(host->api, owner->ready, *owner->staging, *prepared,
            owner->logical.settings.Text("menu.unavailable"), rowCommand,
            static_cast<uint32_t>(owner->logical.pool + 1), current) || !current())
        { owner->logical.settings.Reject(*prepared); return false; }
        if (!owner->PublishPool(host->api)) { owner->logical.settings.Reject(*prepared); return false; }
        host->diagnostics |= PagePublished;
        return host->source == owner && owner->logical.identity == identity && owner->Current() && owner->Private();
    }
    inline void Close(const std::shared_ptr<Owner>& owner)
    {
        if (owner->logical.settings.IsOpen()) owner->logical.settings.Cancel();
        owner->logical.pending.Clear();
        if (owner->Registered() && (Read<uint8_t>(owner->ready.view.interfaceManager, 10500).value_or(0) ||
            Read<void*>(owner->ready.view.screen, 216).value_or(nullptr)))
        { owner->logical.returnPending = true; return; }
        owner->logical.returnPending = false;
        ++owner->logical.pool;
        owner->ReturnToSource(host->api);
        host->life.CleanOwned([&]()
        {
            if (!owner->DestroyPools(host->api)) owner->logical.returnPending = true;
        });
    }
    inline void Act(const std::shared_ptr<Owner>& owner, Request request)
    {
        if (host->source != owner || !owner->Current() || request.owner != owner->logical.identity ||
            request.pool != owner->logical.pool || request.selected != owner->ready.view.Selected() ||
            request.sender != reinterpret_cast<uintptr_t>(owner->ready.view.SelectedRow())) return;
        auto current = [&]() { return host->source == owner && owner->Current() &&
            request.pool == owner->logical.pool && request.sender == reinterpret_cast<uintptr_t>(owner->ready.view.SelectedRow()) &&
            (request.kind == CommandKind::Open ? owner->HasArray(owner->entryArray) : owner->SelectedItem().has_value()); };
        if (request.kind == CommandKind::Open)
        {
            if (!owner->logical.settings.Open(host->ownModule, Language(), owner->ready.view.PauseContext(), current) || !current()) return;
            if (!Build(owner, {})) { host->diagnostics |= ViewRefused; Close(owner); }
            return;
        }
        const auto selected = owner->SelectedItem();
        if (!selected || !owner->active) return;
        const auto displayed = owner->active->presented;
        const auto item = displayed.items[*selected];
        auto location = displayed.location;
        if (request.kind == CommandKind::Back || (request.kind == CommandKind::Accept && item.kind == ItemKind::Back))
        {
            if (location.page) location = {};
            else { Close(owner); return; }
        }
        else if (item.kind == ItemKind::ModelRow)
        {
            const int direction = request.kind == CommandKind::Accept ? 0 : request.kind == CommandKind::Left ? -1 : 1;
            owner->logical.settings.Change(displayed, *selected, direction, current);
            if (!current() || !owner->logical.settings.IsOpen()) return;
        }
        else
        {
            if (request.kind != CommandKind::Accept) return;
            if (item.kind == ItemKind::Link) location = {item.destination, 0};
            else if (item.kind == ItemKind::Previous || item.kind == ItemKind::Next) location.first = item.destination;
            else return;
        }
        if (!Build(owner, location)) { host->diagnostics |= ViewRefused; Close(owner); }
    }
    inline void Capture()
    {
        const auto view = View::Get(reinterpret_cast<HMODULE>(host->api.Base()));
        if (!view || !view->ScreenMode() || view->Covered()) return;
        const auto array = Array::Get(view->screen);
        if (!array) return;
        auto owner = std::make_shared<Owner>();
        for (auto at = array->begin; at < array->end; ++at)
        {
            auto ready = Ready::Capture(host->api.calls, *view, *at);
            if (ready) { owner->ready = *ready; break; }
        }
        if (!owner->ready.sourceRow) { host->diagnostics |= SourceRefused; return; }
        auto current = [&]() { const auto now = View::Get(reinterpret_cast<HMODULE>(view->module));
            return !owner->retired && now && now->Same(*view) && now->ScreenMode() && !now->Covered(); };
        host->source = owner; // retain construction before any native callback
        if (!host->api.controls.Bind(host->api.calls, *view, owner->ready.sourceRow) ||
            !owner->ready.Probe(host->api.calls, current) ||
            !owner->PrepareEntry(host->api, ++host->nextOwner, entryCommand, rowCommand,
                RWGTAMenu::PluginText(FixSettings(), Language(), "menu.title"),
                {reinterpret_cast<uintptr_t>(&SourceCommand), reinterpret_cast<uintptr_t>(&SourceKey),
                 reinterpret_cast<uintptr_t>(&SourceEscape), reinterpret_cast<uintptr_t>(&SourceLeft), reinterpret_cast<uintptr_t>(&SourceRight),
                 reinterpret_cast<uintptr_t>(&SourceFocused)}, current) ||
            !current() || !owner->PublishEntry(host->api))
        { owner->Cancel(); host->diagnostics |= SourceRefused; return; }
        host->diagnostics |= EntryPublished;
    }
    inline void Describe(const std::shared_ptr<Owner>& owner)
    {
        const auto selected = owner->SelectedItem();
        const auto client = Client(owner->ready.view);
        if (!selected || !client || !owner->active || owner->active->count < 2) return;
        const auto pool = owner->active.get();
        const auto token = owner->logical.settings.Revision();
        const auto physical = owner->ready.view.Selected();
        const auto& text = pool->presented.items[*selected].description;
        // The footer: the restart warning, else the update notice
        const auto footer = owner->logical.settings.RestartRequired() ? owner->logical.settings.Text("menu.restartwarning") : notice;
        if (pool->described && pool->description == text && pool->footer == footer) return;
        auto current = [&]() { return host->source == owner && owner->Current() && owner->Private() &&
            owner->active.get() == pool && token == owner->logical.settings.Revision() && physical == owner->ready.view.Selected(); };
        const auto height = (client->bottom - client->top) / static_cast<LONG>(pool->presented.items.size() + Pool::fixedRows);
        const auto width = client->right - client->left;
        if (!FitHelp(host->api, owner->ready, *pool, text, width, height, current))
        {
            if (!current()) return;
            for (size_t i = 1; i <= Pool::helpLines; ++i)
                if (!FitControl(host->api, owner->ready, pool->nodes[i],
                    i == 1 ? owner->logical.settings.Text("menu.unavailable") : "", width, height, true, current)) return;
        }
        if (!FitControl(host->api, owner->ready, pool->nodes[Pool::fixedRows - 1], footer, width, height, true, current)) return;
        pool->description = text;
        pool->footer = footer;
        pool->described = true;
    }
    inline uintptr_t LifeOriginal(DeleteJob job)
    {
        const auto slot = job.operation == LifeOp::RemoveAll ? RemoveAll : job.operation == LifeOp::ScreenTerm ? ScreenTerm :
            job.operation == LifeOp::ScreenDtor ? ScreenDtor : job.operation == LifeOp::ScreenScalar ? ScreenScalar :
            job.operation == LifeOp::ManagerTerm ? ManagerTerm : job.operation == LifeOp::ManagerDtor ? ManagerDtor :
            job.operation == LifeOp::ManagerScalar ? ManagerScalar : job.operation == LifeOp::InterfaceTerm ? InterfaceTerm :
            job.operation == LifeOp::InterfaceDtor ? InterfaceDtor : job.operation == LifeOp::InterfaceScalar ? InterfaceScalar :
            job.operation == LifeOp::ShellTerm ? ShellTerm : job.operation == LifeOp::ShellDtor ? ShellDtor : TextScalar;
        auto scope = host->scopes.Enter(Phase::Lifecycle);
        if (job.operation == LifeOp::RemoveAll)
            return host->hooks[slot].thiscall<uintptr_t>(job.receiver, static_cast<char>(job.flags));
        if (job.operation == LifeOp::ScreenScalar || job.operation == LifeOp::ManagerScalar ||
            job.operation == LifeOp::InterfaceScalar || job.operation == LifeOp::TextScalar)
            return host->hooks[slot].thiscall<uintptr_t>(job.receiver, job.flags);
        return host->hooks[slot].thiscall<uintptr_t>(job.receiver);
    }
    inline uintptr_t Destroy(DeleteJob job)
    {
        const auto owner = host->source;
        if (!host->ready || !host->scopes.Here() || !owner || !Lifecycle::Related(*owner, job.operation, job.receiver)) return LifeOriginal(job);
        if (host->life.Executing(job)) return reinterpret_cast<uintptr_t>(job.receiver);
        if (host->life.Authorized(*owner, job.operation, job.receiver)) return LifeOriginal(job);
        const bool quiet = host->scopes.Quiet();
        // Protect preparation too, not only original teardown: Select/StringText
        // may synchronously reenter the whole update before LifeOriginal starts.
        auto collection = quiet ? host->scopes.Enter(Phase::Lifecycle) : Traversals::Scope{};
        if (!host->life.Queue(*owner, job)) return reinterpret_cast<uintptr_t>(job.receiver);
        if (!quiet) return reinterpret_cast<uintptr_t>(job.receiver);
        uintptr_t result = reinterpret_cast<uintptr_t>(job.receiver);
        if (!host->life.Collect(*owner, [&](bool forced) { return owner->Dispose(host->api, forced); }, [&](DeleteJob pending)
        { const auto value = LifeOriginal(pending); if (pending.operation == job.operation && pending.receiver == job.receiver) result = value; }))
        { host->diagnostics |= ViewRefused; return result; }
        if (host->source == owner) host->source.reset();
        host->diagnostics |= OwnerRetired;
        return result;
    }
    inline void Drain()
    {
        auto service = host->scopes.Service();
        if (!service || !host->ready) return;
        DWORD foreground{};
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
        if (foreground != GetCurrentProcessId()) host->acceptDown = host->repeatAccept = false;
        const auto owner = host->source;
        if (owner && owner->retired)
        {
            if (!host->life.Collect(*owner, [&](bool forced) { return owner->Dispose(host->api, forced); }, &LifeOriginal))
            { host->diagnostics |= ViewRefused; FlushLog(); return; }
            if (host->source == owner) host->source.reset();
            host->diagnostics |= OwnerRetired;
        }
        else if (owner)
        {
            if (!owner->Registered()) { owner->Cancel(); return; }
            const auto now = View::Get(reinterpret_cast<HMODULE>(host->api.Base()), owner->Table());
            // Preserve a covered session. No API/native mutations while a modal
            // or another native menu manager temporarily covers this screen.
            if (Read<uint8_t>(owner->ready.view.interfaceManager, 10500).value_or(0) ||
                Read<void*>(owner->ready.view.screen, 216).value_or(nullptr) ||
                (now && now->Covered())) return;
            if (!now || !now->Same(owner->ready.view) || !now->ScreenMode())
            { Close(owner); owner->Cancel(); return; }
            if (owner->logical.returnPending || (owner->logical.settings.IsOpen() && !owner->Private())) Close(owner);
            while (auto request = owner->logical.pending.Pop())
            {
                Act(owner, *request);
                if (host->source != owner || owner->retired || !owner->Current()) break;
            }
            if (owner->logical.settings.IsOpen() && owner->Current() && owner->Private() && owner->logical.settings.NeedsRefresh())
            {
                if (!Build(owner, owner->logical.settings.Shown())) { host->diagnostics |= ViewRefused; Close(owner); }
            }
            if (owner->logical.settings.IsOpen() && owner->Private()) Describe(owner);
            host->life.CleanOwned([&]()
            {
                for (auto pool : {&owner->retiring, &owner->staging})
                {
                    if (!*pool) continue;
                    if (owner->PoolReferenced(**pool)) { owner->logical.returnPending = true; continue; }
                    (*pool)->Destroy(host->api); pool->reset();
                }
            });
        }
        else Capture();
        FlushLog();
    }
    inline uintptr_t __fastcall Updated(void* self, void*)
    {
        uintptr_t result{};
        if (!host->scopes.BindUpdateThread()) return host->hooks[Update].thiscall<uintptr_t>(self);
        { auto scope = host->scopes.Enter(Phase::Update); result = host->hooks[Update].thiscall<uintptr_t>(self); }
        if (host->scopes.MarkCompleteUpdate())
            try { Drain(); } catch (...) { host->diagnostics |= Exception; if (host->source) host->source->logical.returnPending = true; }
        return result;
    }
    inline uintptr_t __fastcall PostUpdated(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Transition);
        const auto result = host->hooks[PostUpdate].thiscall<uintptr_t>(self);
        return result;
    }
    inline uintptr_t __fastcall Rendered(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Render);
        const auto result = host->hooks[Render].thiscall<uintptr_t>(self);
        return result;
    }
    inline char __fastcall KeyPressed(void* self, void*, uint32_t key, uint32_t repeat)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        if (key == VK_RETURN) { host->repeatAccept = host->acceptDown; host->acceptDown = true; }
        const auto result = host->hooks[KeyDown].thiscall<char>(self, key, repeat);
        host->repeatAccept = false;
        return result;
    }
    inline char __fastcall KeyReleased(void* self, void*, uint32_t key)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        if (key == VK_RETURN) { host->acceptDown = false; host->repeatAccept = false; }
        const auto result = host->hooks[KeyUp].thiscall<char>(self, key);
        return result;
    }
    inline char __fastcall CharacterReceived(void* self, void*, uint32_t character)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[Character].thiscall<char>(self, character);
        return result;
    }
    inline uintptr_t __fastcall MousePressed(void* self, void*, int x, int y)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[MouseDown].thiscall<uintptr_t>(self, x, y);
        return result;
    }
    inline uintptr_t __fastcall MouseReleased(void* self, void*, int x, int y)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[MouseUp].thiscall<uintptr_t>(self, x, y);
        return result;
    }
    inline char __fastcall CommandPressed(void* self, void*, uint32_t command)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[CommandOn].thiscall<char>(self, command);
        return result;
    }
    inline char __fastcall CommandReleased(void* self, void*, uint32_t command)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[CommandOff].thiscall<char>(self, command);
        return result;
    }
    inline uintptr_t __fastcall Removed(void* self, void*, char flags)
    {
        return Destroy({LifeOp::RemoveAll, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall ScreenTerminated(void* self, void*)
    {
        return Destroy({LifeOp::ScreenTerm, self, 0});
    }
    inline uintptr_t __fastcall ScreenDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::ScreenDtor, self, 0});
    }
    inline uintptr_t __fastcall ScreenDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::ScreenScalar, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall ManagerTerminated(void* self, void*)
    {
        return Destroy({LifeOp::ManagerTerm, self, 0});
    }
    inline uintptr_t __fastcall ManagerDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::ManagerScalar, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall ManagerDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::ManagerDtor, self, 0});
    }
    inline uintptr_t __fastcall InterfaceTerminated(void* self, void*)
    {
        return Destroy({LifeOp::InterfaceTerm, self, 0});
    }
    inline uintptr_t __fastcall InterfaceDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::InterfaceScalar, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall InterfaceDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::InterfaceDtor, self, 0});
    }
    inline uintptr_t __fastcall ShellDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::ShellDtor, self, 0});
    }
    inline uintptr_t __fastcall ShellTerminated(void* self, void*)
    {
        return Destroy({LifeOp::ShellTerm, self, 0});
    }
    inline uintptr_t __fastcall TextDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::TextScalar, self, static_cast<uint32_t>(flags)});
    }
    inline void InitImpl(HMODULE image)
    {
        if (host) return;
        auto candidate = std::make_unique<Host>();
        HMODULE own{}, game{};
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&InitImpl), &own);
        candidate->ownModule = own;
        if (!candidate->api.Bind(image))
        {
            host = candidate.release(); host->diagnostics |= Unsupported; FlushLog(); return;
        }
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&InitImpl), &own) ||
            !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(image), &game)) return;
        candidate->ownModule = own;
        host = candidate.release();
        host->hooks[0] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().updated), &Updated, safetyhook::InlineHook::StartDisabled);
        host->hooks[1] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().postUpdated), &PostUpdated, safetyhook::InlineHook::StartDisabled);
        host->hooks[2] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().rendered), &Rendered, safetyhook::InlineHook::StartDisabled);
        host->hooks[3] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().keyPressed), &KeyPressed, safetyhook::InlineHook::StartDisabled);
        host->hooks[4] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().keyReleased), &KeyReleased, safetyhook::InlineHook::StartDisabled);
        host->hooks[5] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().characterReceived), &CharacterReceived, safetyhook::InlineHook::StartDisabled);
        host->hooks[6] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().mousePressed), &MousePressed, safetyhook::InlineHook::StartDisabled);
        host->hooks[7] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().mouseReleased), &MouseReleased, safetyhook::InlineHook::StartDisabled);
        host->hooks[8] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().commandPressed), &CommandPressed, safetyhook::InlineHook::StartDisabled);
        host->hooks[9] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().commandReleased), &CommandReleased, safetyhook::InlineHook::StartDisabled);
        host->hooks[10] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().removed), &Removed, safetyhook::InlineHook::StartDisabled);
        host->hooks[11] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().screenTerminated), &ScreenTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[12] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().screenDestroyed), &ScreenDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[13] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().screenDeleted), &ScreenDeleted, safetyhook::InlineHook::StartDisabled);
        host->hooks[14] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().managerTerminated), &ManagerTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[15] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().managerDeleted), &ManagerDeleted, safetyhook::InlineHook::StartDisabled);
        host->hooks[16] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().managerDestroyed), &ManagerDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[17] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().interfaceTerminated), &InterfaceTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[18] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().interfaceDeleted), &InterfaceDeleted, safetyhook::InlineHook::StartDisabled);
        host->hooks[19] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().interfaceDestroyed), &InterfaceDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[20] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().shellDestroyed), &ShellDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[21] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().shellTerminated), &ShellTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[22] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + Rva().textDeleted), &TextDeleted, safetyhook::InlineHook::StartDisabled);
        for (auto& hook : host->hooks) if (!hook)
        { for (auto& installed : host->hooks) installed.reset(); host->diagnostics |= HookRefused; FlushLog(); return; }
        // All trampolines exist before any hook can execute. Before committed
        // readiness, callbacks delegate through these complete originals.
        for (auto& hook : host->hooks) if (!hook.enable())
        { for (auto& installed : host->hooks) installed.reset(); host->diagnostics |= HookRefused; FlushLog(); return; }
        host->ready = true;
    }
    inline void Init(HMODULE image) noexcept
    {
        try { InitImpl(image); }
        catch (...)
        {
            if (!host) return;
            host->ready = false;
            for (auto& hook : host->hooks) hook.reset();
            host->diagnostics |= Exception;
            try { FlushLog(); } catch (...) {}
        }
    }
}
