#pragma once
#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"

namespace THPS3Menu
{
    inline constexpr uint32_t RootID = Checksum("wfp_thps3_advanced");
    inline constexpr uint32_t EntryID = Checksum("wfp_thps3_advanced_entry");
    inline constexpr uint32_t ParentID = Checksum("contain1");
    inline constexpr uint32_t TitleID = Checksum("options_main_menu");
    inline constexpr uint32_t PauseID = Checksum("game_menu");

    inline bool Object(const void* value, uint32_t table, size_t size)
    {
        return Readable(value, size) && Read<uint32_t>(value) == table;
    }
    inline bool ScreenObject(const void* value, uint32_t id)
    {
        if (id == Checksum("main_menu"))
            return Object(value, 0x58DF14, 128);
        if (id == Checksum("game_menu"))
            return Object(value, 0x58DCB4, 192);
        return false;
    }
    inline void* Manager()
    {
        auto* value = Read<void*>(reinterpret_cast<void*>(0x8F0D00));
        return Object(value, 0x58EFC8, 344) ? value : nullptr;
    }
    inline void* Lookup(void* manager, uint32_t id)
    {
        if (!Object(manager, 0x58EFC8, 344)) return nullptr;
        // The native lookup writes its own cached entry/index, as stock callers
        // do. Its RET8 contract is not the factory's RET12 command interface.
        return reinterpret_cast<void*(__thiscall*)(void*, uint32_t, uint32_t)>(0x4D23B0)(manager, id, 0);
    }
    inline void* SelectedChild(void* parent)
    {
        if (!Object(parent, 0x58EF00, 704) || Read<uint8_t>(parent, 700)) return nullptr;
        const auto index = Read<int32_t>(parent, 420);
        if (index < 0 || index > 1024) return nullptr;
        // Container420 is an index; a vertical menu's field420 is a child ID.
        return reinterpret_cast<void*(__thiscall*)(void*, int32_t, uint32_t*)>(0x4CD710)(parent, index, nullptr);
    }

    struct Context
    {
        void* frontend = nullptr;
        void* screen = nullptr;
        void* manager = nullptr;
        void* parent = nullptr;
        void* base = nullptr;
        uint32_t screenID = 0, baseID = 0;
        bool pause = false;

        static Context Discover(const char** failure = nullptr)
        {
            Context result;
            auto refuse = [&](const char* reason) { if (failure) *failure = reason; return Context{}; };
            if (failure) *failure = nullptr;
            result.frontend = Read<void*>(reinterpret_cast<void*>(0x8E1FB0));
            if (!Readable(result.frontend, 76)) return refuse("frontend unavailable");
            if (Read<uint32_t>(result.frontend, 60)) return refuse("frontend transition");
            result.screen = Read<void*>(result.frontend, 68);
            result.screenID = Read<uint32_t>(result.frontend, 64);
            if (!ScreenObject(result.screen, result.screenID)) return refuse("screen identity");
            if (!Read<uint8_t>(result.screen, 4)) return refuse("screen inactive");
            result.manager = Manager();
            if (!result.manager) return refuse("manager identity");
            result.parent = Lookup(result.manager, ParentID);
            if (!Object(result.parent, 0x58EF00, 704) || !Read<uint8_t>(result.parent, 92)
                || !Read<uint8_t>(result.parent, 93) || !Read<uint8_t>(result.parent, 94)) return refuse("parent identity/show/active/focus");
            result.base = SelectedChild(result.parent);
            if (!Object(result.base, 0x58F11C, 448) || !Read<uint8_t>(result.base, 92)
                || !Read<uint8_t>(result.base, 93)) return refuse("selected child identity/show/active");
            result.baseID = Read<uint32_t>(result.base, 52);
            if (result.baseID != TitleID && result.baseID != PauseID) return refuse("selected child is outside Options/pause");
            if (Lookup(result.manager, result.baseID) != result.base || Read<void*>(result.base, 4) != result.parent) return refuse("selected child ownership");
            result.pause = result.baseID == PauseID;
            return result;
        }
        bool Alive() const
        {
            return frontend && Read<void*>(reinterpret_cast<void*>(0x8E1FB0)) == frontend
                && Readable(frontend, 76) && !Read<uint32_t>(frontend, 60)
                && Read<void*>(frontend, 68) == screen && Read<uint32_t>(frontend, 64) == screenID
                && ScreenObject(screen, screenID) && Read<uint8_t>(screen, 4)
                && Manager() == manager && Lookup(manager, ParentID) == parent
                && Object(parent, 0x58EF00, 704) && Read<uint8_t>(parent, 92)
                && Read<uint8_t>(parent, 93) && Read<uint8_t>(parent, 94)
                && Lookup(manager, baseID) == base && Object(base, 0x58F11C, 448)
                && Read<void*>(base, 4) == parent;
        }
        bool StockCurrent() const
        {
            return Alive() && SelectedChild(parent) == base && Read<uint8_t>(base, 92) && Read<uint8_t>(base, 93);
        }
        bool PrivateCurrent(void* root, bool requireShown = true) const
        {
            return Alive() && Lookup(manager, RootID) == root && Object(root, 0x58F11C, 448)
                && Read<void*>(root, 4) == parent && SelectedChild(parent) == root
                && (!requireShown || (Read<uint8_t>(root, 92) && Read<uint8_t>(root, 93)));
        }
    };
}
