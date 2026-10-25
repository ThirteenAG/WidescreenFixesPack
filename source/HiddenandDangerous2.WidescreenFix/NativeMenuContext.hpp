#pragma once
#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"
#include <array>
#include <vector>

namespace HD2Menu
{
    struct Context
    {
        inline static uintptr_t graphics = 0;
        void* world = nullptr;
        void* manager = nullptr;
        void* options = nullptr;
        void* optionsController = nullptr;
        void* asset = nullptr;
        void* group = nullptr;
        void* entry = nullptr;
        std::array<void*, 6> buttons{};
        bool pause = false;
        uint32_t rootIndex = 0;
        const char* refusal = "native world/state identity deferred";

        template<class T> static bool Read(void* object, size_t offset, T& value)
        {
            auto start = reinterpret_cast<uintptr_t>(object);
            if (!start || offset > UINTPTR_MAX - start)
                return false;
            auto address = reinterpret_cast<void*>(start + offset);
            if (!Contracts::Readable(address, sizeof(T)))
                return false;
            std::memcpy(&value, address, sizeof(T));
            return true;
        }

        static bool Table(void* object, uintptr_t original)
        {
            uintptr_t table = 0;
            return Read(object, 0, table) && table == ABI::game + original - 0x400000;
        }

        static bool Children(void* object, void**& begin, size_t& count)
        {
            void** end = nullptr;
            if (!Read(object, 8, begin) || !Read(object, 12, end))
                return false;
            auto first = reinterpret_cast<uintptr_t>(begin);
            auto last = reinterpret_cast<uintptr_t>(end);
            if (last < first || (last - first) % sizeof(void*) || last - first > 64 * sizeof(void*))
                return false;
            count = (last - first) / sizeof(void*);
            return !count || Contracts::Readable(begin, last - first);
        }

        static void* Top()
        {
            // Native deque stores window pointers. The tail cursor points to
            // the next unused slot; the previous block has1024 pointer slots.
            uint32_t count = 0;
            uintptr_t first = 0, tail = 0, map = 0;
            auto base = reinterpret_cast<void*>(ABI::game);
            if (!Read(base, 0x4AE4E0, count) || !count || count > 64
                || !Read(base, 0x4AE4C8, first) || !Read(base, 0x4AE4D0, tail)
                || !Read(base, 0x4AE4D4, map) || !first || !tail || !map)
                return nullptr;
            uintptr_t slot = 0;
            if (tail > first && tail - first <= 4096 && (tail - first) % sizeof(void*) == 0)
                slot = tail - sizeof(void*);
            else if (tail == first && map >= sizeof(void*))
            {
                uintptr_t block = 0;
                if (!Read(reinterpret_cast<void*>(map - sizeof(void*)), 0, block)
                    || !block || block > UINTPTR_MAX - 4092)
                    return nullptr;
                slot = block + 4092;
            }
            void* top = nullptr;
            return slot && Read(reinterpret_cast<void*>(slot), 0, top) ? top : nullptr;
        }

        static bool Stacked(void* window)
        {
            uint32_t count = 0;
            uintptr_t first = 0, tail = 0, map = 0;
            auto base = reinterpret_cast<void*>(ABI::game);
            if (!window || !Read(base, 0x4AE4E0, count) || !count || count > 64
                || !Read(base, 0x4AE4C8, first) || !Read(base, 0x4AE4D0, tail)
                || !Read(base, 0x4AE4D4, map) || !first || !tail || !map) return false;
            while (count--)
            {
                if (tail == first)
                {
                    if (map < sizeof(void*)) return false;
                    map -= sizeof(void*);
                    if (!Read(reinterpret_cast<void*>(map), 0, first) || !first || first > UINTPTR_MAX - 4096) return false;
                    tail = first + 4096;
                }
                if (tail <= first || tail - first > 4096 || (tail - first) % sizeof(void*)) return false;
                tail -= sizeof(void*);
                void* candidate = nullptr;
                if (!Read(reinterpret_cast<void*>(tail), 0, candidate)) return false;
                if (candidate == window) return true;
            }
            return false;
        }

        static bool Stack(std::vector<void*>& result)
        {
            result.clear();
            uint32_t count = 0;
            uintptr_t first = 0, tail = 0, map = 0;
            auto base = reinterpret_cast<void*>(ABI::game);
            if (!Read(base, 0x4AE4E0, count) || !count || count > 64
                || !Read(base, 0x4AE4C8, first) || !Read(base, 0x4AE4D0, tail)
                || !Read(base, 0x4AE4D4, map) || !first || !tail || !map) return false;
            result.reserve(count);
            while (count--)
            {
                if (tail == first)
                {
                    if (map < sizeof(void*)) return false;
                    map -= sizeof(void*);
                    if (!Read(reinterpret_cast<void*>(map), 0, first) || !first || first > UINTPTR_MAX - 4096) return false;
                    tail = first + 4096;
                }
                if (tail <= first || tail - first > 4096 || (tail - first) % sizeof(void*)) return false;
                tail -= sizeof(void*);
                void* candidate = nullptr;
                if (!Read(reinterpret_cast<void*>(tail), 0, candidate)) return false;
                result.push_back(candidate);
            }
            return true;
        }

        bool Alive() const
        {
            void* currentWorld = nullptr;
            void* state = nullptr;
            auto base = reinterpret_cast<void*>(ABI::game);
            if (!Read(base, 0x4AED14, currentWorld) || currentWorld != world
                || !Read(world, 20, state) || state != manager
                || !Table(manager, pause ? 0x815750 : 0x815E30))
                return false;
            void** begin = nullptr;
            void** end = nullptr;
            if (!Read(manager, 324, begin) || !Read(manager, 328, end)) return false;
            auto first = reinterpret_cast<uintptr_t>(begin);
            auto last = reinterpret_cast<uintptr_t>(end);
            if (!first || last < first || (last - first) % sizeof(void*)
                || last - first > 64 * sizeof(void*) || rootIndex >= (last - first) / sizeof(void*)
                || !Contracts::Readable(begin, last - first) || begin[rootIndex] != options
                || !Table(options, 0x815540) || !Table(group, 0x814430))
                return false;
            void* currentAsset = nullptr;
            void* currentController = nullptr;
            uint32_t id = 0;
            if (!Read(options, 116, currentAsset) || currentAsset != asset
                || !Read(manager, pause ? 492 : 464, currentController) || currentController != optionsController
                || !Read(options, 96, id) || (id >> 26) != rootIndex)
                return false;
            void** children = nullptr;
            size_t count = 0;
            if (!Children(options, children, count)) return false;
            bool containsGroup = false;
            for (size_t i = 0; i < count; ++i)
                containsGroup |= children[i] == group;
            if (!containsGroup || !Children(group, children, count) || count < buttons.size()) return false;
            if (entry && (count != 7 || children[5] != entry || children[6] != buttons[5]
                || !Table(entry, 0x814CB0))) return false;
            for (size_t i = 0; i < buttons.size(); ++i)
                if (children[entry && i == 5 ? 6 : i] != buttons[i] || !Table(buttons[i], 0x814CB0)) return false;
            return true;
        }

        bool Discover()
        {
            *this = {};
            auto base = reinterpret_cast<void*>(ABI::game);
            if (!ABI::ready || !Read(base, 0x4AED14, world) || !Read(world, 20, manager)) return false;
            pause = Table(manager, 0x815750);
            if (!pause && !Table(manager, 0x815E30)) return false;
            refusal = "native Options controller/registry deferred";
            //61CF60 records byte9 in the separate Options controller. Actual
            // callers load that controller from title+464 or pause+492; it is
            // not manager byte9. Window slot0 belongs to confirmation.
            uint8_t index = 0;
            void** windows = nullptr;
            void** end = nullptr;
            if (!Read(manager, pause ? 492 : 464, optionsController)
                || !Read(optionsController, 9, index) || !index || !Read(manager, 324, windows)
                || !Read(manager, 328, end)) return false;
            auto first = reinterpret_cast<uintptr_t>(windows);
            auto last = reinterpret_cast<uintptr_t>(end);
            if (!first || last < first || (last - first) % sizeof(void*)
                || last - first > 64 * sizeof(void*) || index >= (last - first) / sizeof(void*)
                || !Contracts::Readable(windows, last - first)) return false;
            rootIndex = index;
            options = windows[index];
            uintptr_t assetTable = 0;
            refusal = "native Options root/top/asset contract unavailable";
            if (!Table(options, 0x815540) || Top() != options || !Read(options, 116, asset)
                || !graphics || !Read(asset, 0, assetTable) || assetTable != graphics + 0x9BA50) return false;
            auto groupFrame = ABI::FindFrame(asset, "GRoptions", 65569);
            refusal = "native GRoptions frame/group unavailable";
            void** children = nullptr;
            size_t count = 0;
            if (!groupFrame || !Children(options, children, count)) return false;
            for (size_t i = 0; i < count; ++i)
            {
                void* frame = nullptr;
                if (Table(children[i], 0x814430) && Read(children[i], 80, frame) && frame == groupFrame)
                {
                    if (group) return false;
                    group = children[i];
                }
            }
            refusal = "native Options six-control layout unavailable";
            if (!group || !Children(group, children, count) || count != buttons.size()) return false;
            for (size_t i = 0; i < buttons.size(); ++i) buttons[i] = children[i];
            refusal = "native Options ownership changed during discovery";
            return Alive() && Top() == options;
        }
    };
}
