module;

#include <stdafx.h>
#include <cstddef>
#include <memory>
#include <new>
#include <vector>

export module VisibilityPlugins;

namespace AlphaEntityList
{
    struct Info
    {
        void* entity;
        float distance;
    };

    struct Node
    {
        Info item;
        Node* prev;
        Node* next;
    };

    struct List
    {
        Node head, tail, freeHead, freeTail;
        Node* links;
    };

    static_assert(sizeof(Node) == 16 && sizeof(List) == 68);
    static_assert(offsetof(List, freeHead) == 32 && offsetof(List, links) == 64);

    class Growth
    {
        std::vector<std::unique_ptr<Node[]>> chunks;

    public:
        // Call only after the native initializer has detached the old nodes.
        void Reset() noexcept { chunks.clear(); }

        bool EnsureFreeNode(List& list) noexcept
        {
            if (list.freeHead.next != &list.freeTail)
                return true;

            // Never relocate the game's allocation: the active sorted list
            // contains pointers to its nodes, and Shutdown owns that allocation.
            constexpr size_t count = 256;
            try
            {
                auto nodes = std::make_unique<Node[]>(count);
                auto first = nodes.get();
                chunks.push_back(std::move(nodes));
                for (size_t i = 0; i < count; ++i)
                {
                    first[i].prev = i ? &first[i - 1] : &list.freeHead;
                    first[i].next = i + 1 < count ? &first[i + 1] : &list.freeTail;
                }
                list.freeHead.next = first;
                list.freeTail.prev = &first[count - 1];
                return true;
            }
            catch (const std::bad_alloc&)
            {
                // Preserve the native failure path if memory is exhausted.
                return false;
            }
        }
    };
}

namespace
{
    using namespace AlphaEntityList;
    Growth alphaEntityGrowth;
    void (__thiscall* InitListNative)(List*, unsigned int) = nullptr;
    Node* (__thiscall* InsertSortedNative)(List*, const Info&) = nullptr;

    void __fastcall InitEntityList(List* list, void*, unsigned int count)
    {
        // Respect larger limits installed by other mods.
        InitListNative(list, std::max(count, 450u));
        alphaEntityGrowth.Reset();
    }

    Node* __fastcall InsertEntity(List* list, void*, const Info& info)
    {
        alphaEntityGrowth.EnsureFreeNode(*list);
        return InsertSortedNative(list, info);
    }

    void InitAlphaEntityGrowth()
    {
        auto init = hook::pattern("68 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? C7 05");
        auto initAddress = init.get_first<uint8_t>();
        // The store following Init targets the entity list's head distance.
        const auto listAddress = *reinterpret_cast<uintptr_t*>(initAddress + 22) - offsetof(Node, item.distance);
        char signature[256]{};
        sprintf_s(signature, "53 B9 %02X %02X %02X %02X 83 EC 10 8B 44 24 18 89 44 24 08 8B 44 24 1C 89 44 24 0C 8B 44 24 04 33 44 24 04 31 DB 89 44 24 04 8D 44 24 08 50 E8",
            unsigned(listAddress & 255), unsigned((listAddress >> 8) & 255), unsigned((listAddress >> 16) & 255), unsigned(listAddress >> 24));
        auto insertCall = hook::pattern(signature).get_first<uint8_t>(44);

        auto initCall = initAddress + 15;
        InitListNative = reinterpret_cast<decltype(InitListNative)>(injector::GetBranchDestination(initCall).as_int());
        InsertSortedNative = reinterpret_cast<decltype(InsertSortedNative)>(injector::GetBranchDestination(insertCall).as_int());
        injector::MakeCALL(initCall, InitEntityList, true);
        injector::MakeCALL(insertCall, InsertEntity, true);
    }
}

class VisibilityPlugins
{
public:
    VisibilityPlugins()
    {
        WFP::onGameInitEvent() += []()
        {
            // Timed window overlays and fading entities share this list.
            // Even 450 nodes can fill at wider FOVs; grow only when needed.
            InitAlphaEntityGrowth();
        };
    }
} VisibilityPlugins;
