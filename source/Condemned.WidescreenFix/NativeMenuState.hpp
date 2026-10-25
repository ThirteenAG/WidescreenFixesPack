#pragma once
#include "NativeMenuABI.hpp"

// Read-only, disconnected state layer. Exact offsets come from the installed
// interfaceManager/manager keyboard, switching, modal and source-render machine bodies.
// It does not establish a deletion lease or publish/retire any native control.
namespace CondemnedMenuNative
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
            out.interfaceManager = Read<void*>(reinterpret_cast<void*>(out.module + 0x16F388), 0).value_or(nullptr);
            if (!Readable(out.interfaceManager, 16900) || Read<uintptr_t>(out.interfaceManager, 0) != out.module + 0x142594 ||
                !Read<uint8_t>(out.interfaceManager, 14664).value_or(0)) return {};
            out.manager = reinterpret_cast<uint8_t*>(out.interfaceManager) + 15816;
            if (Read<uintptr_t>(out.manager, 0) != out.module + 0x147F94) return {};
            out.screen = Read<void*>(out.manager, 96).value_or(nullptr);
            out.screenId = Read<uint32_t>(out.manager, 88).value_or(UINT32_MAX);
            if (!Readable(out.screen, 440) || Read<uint32_t>(out.screen, 16) != out.screenId ||
                Read<void*>(out.screen, 12) != out.manager) return {};
            out.originalVtable = out.module + 0x14814C;
            const auto table = Read<uintptr_t>(out.screen, 0).value_or(0);
            if (table != out.originalVtable && (!copiedVtable || table != copiedVtable)) return {};
            const auto begin = Read<uintptr_t>(out.manager, 104).value_or(0);
            const auto end = Read<uintptr_t>(out.manager, 108).value_or(0);
            if (!begin || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64 ||
                !Readable(reinterpret_cast<void*>(begin), end - begin)) return {};
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
            for (auto rva : {0x16F248u, 0x16F24Cu})
            {
                const auto overlay = Read<void*>(reinterpret_cast<void*>(module + rva), 0).value_or(nullptr);
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
                *index >= (end - begin) / 4 || !Readable(reinterpret_cast<void*>(begin), end - begin)) return nullptr;
            return Read<void*>(reinterpret_cast<void*>(begin + 4 * *index), 0).value_or(nullptr);
        }
        // Existing native Escape checks this same connection field and game-state
        // predicate. This is context only; no world or UI state is modified.
        bool PauseContext() const
        {
            const auto connection = Read<void*>(reinterpret_cast<void*>(module + 0x16FE24), 0).value_or(nullptr);
            const auto game = Read<void*>(reinterpret_cast<void*>(module + 0x1689C0), 0).value_or(nullptr);
            const auto state = Read<uint32_t>(game, 4);
            const auto mode = Read<uint32_t>(connection, 48);
            return mode && *mode != 0 && (*mode == 1 || (state && *state != 4 && *state != 5));
        }
    };
}
