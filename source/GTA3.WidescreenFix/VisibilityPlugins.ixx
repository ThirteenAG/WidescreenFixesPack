module;

#include <stdafx.h>
#include "AlphaEntityList.hpp"

export module VisibilityPlugins;

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
        auto inserts = hook::pattern("53 B9 ? ? ? ? 83 EC 10 8B 44 24 18 89 44 24 08 8B 44 24 1C 89 44 24 0C 8B 44 24 04 33 44 24 04 31 DB 89 44 24 04 8D 44 24 08 50 E8");
        if (init.size() != 1)
            return;

        auto initAddress = init.get_first<uint8_t>();
        // The store following Init targets the entity list's head distance.
        const auto listAddress = *reinterpret_cast<uintptr_t*>(initAddress + 22) - offsetof(Node, item.distance);
        uint8_t* insertCall = nullptr;
        for (size_t i = 0; i < inserts.size(); ++i)
        {
            auto entry = inserts.get(i).get<uint8_t>();
            if (*reinterpret_cast<uintptr_t*>(entry + 2) == listAddress)
            {
                if (insertCall)
                    return;
                insertCall = entry + 44;
            }
        }
        if (!insertCall)
            return;

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
