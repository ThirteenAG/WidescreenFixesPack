module;

#include "stdafx.h"

export module ExtremeDrawDistance;

namespace ExtremeDistance
{
    uintptr_t pGetRegionBounds = 0;
    uintptr_t pBuildRegionList = 0;
    uintptr_t pCreateDistrictTable = 0;
    uintptr_t pAllocate = 0;
    uintptr_t pMeshDistanceOperand = 0;
    uintptr_t pLegoDistanceOperand = 0;
    uintptr_t pMeshDistanceImmediate = 0;
    uintptr_t pLegoDistanceImmediate = 0;
    uintptr_t pLinearDistancesOperand = 0;
    uintptr_t pSquaredDistancesOperand = 0;
    uintptr_t pEntitySquaredDistancesOperand = 0;
    uintptr_t pLodSquaredDistancesOperand = 0;
    uintptr_t pRegionDistanceOperand = 0;
    uintptr_t pActiveMemoryMap = 0;
    uintptr_t pStreamingDistance = 0;
    uintptr_t pOriginalDistances = 0;
    uintptr_t pOriginalSquaredDistances = 0;
    uintptr_t pStreamingBufferSize = 0;
    uintptr_t pMemoryMaps = 0;
    uintptr_t pMemoryMapCount = 0;
    uintptr_t pMissionLookupContinue = 0;
    uintptr_t pMissionSortContinue = 0;
    uintptr_t pMissionFrameSortContinue = 0;

    void ResolveRuntimePointers()
    {
        auto pattern = hook::pattern("8B 49 64 E9 ? ? ? ? CC CC CC CC CC CC CC CC 64 A1 00 00 00 00"); //0x52E770
        pGetRegionBounds = reinterpret_cast<uintptr_t>(pattern.get_first());
        pattern = hook::pattern("51 8B 4C 24 0C 55 33 ED 3B CD 0F 84"); //0x536FA0
        pBuildRegionList = reinterpret_cast<uintptr_t>(pattern.get_first());
        pattern = hook::pattern("83 EC 0C 8B 4C 24 2C 8B 54 24 28 8B 44 24 14 51 8B 4C 24 28 52 8B 54 24 28 51 8B 4C 24 28 52 51 6A 48"); //0x5E4020
        pCreateDistrictTable = reinterpret_cast<uintptr_t>(pattern.get_first());
        pattern = hook::pattern("8B 44 24 04 8B 4C 24 08 56 50 51 FF 15"); //0x58EC30
        pAllocate = reinterpret_cast<uintptr_t>(pattern.get_first());
        pattern = hook::pattern("D8 1D ? ? ? ? 89 44 24 1C C7 44 24 14 00 00 27 43"); //0x53CEFC + 2
        pMeshDistanceOperand = reinterpret_cast<uintptr_t>(pattern.get_first(2));
        pattern = hook::pattern("D8 1D ? ? ? ? C7 44 24 10 00 00 27 43 DF E0"); //0x53D377 + 2
        pLegoDistanceOperand = reinterpret_cast<uintptr_t>(pattern.get_first(2));
        pattern = hook::pattern("C7 44 24 14 00 00 27 43 DF E0 F6 C4 05 8D 44 24 1C"); //0x53CF06 + 4
        pMeshDistanceImmediate = reinterpret_cast<uintptr_t>(pattern.get_first(4));
        pattern = hook::pattern("C7 44 24 10 00 00 27 43 DF E0 F6 C4 05 8D 44 24 14"); //0x53D37D + 4
        pLegoDistanceImmediate = reinterpret_cast<uintptr_t>(pattern.get_first(4));
        pattern = hook::pattern("D9 04 8D ? ? ? ? D9 44 24 20 D8 25"); //0x53D1AF + 3
        pLinearDistancesOperand = reinterpret_cast<uintptr_t>(pattern.get_first(3));
        pattern = hook::pattern("D9 04 8D ? ? ? ? C3 CC CC 83 EC 08 8B CE E8"); //0x51E8B6 + 3
        pSquaredDistancesOperand = reinterpret_cast<uintptr_t>(pattern.get_first(3));
        pattern = hook::pattern("D9 04 85 ? ? ? ? 0F B6 4E 2E D9 1C 24 89 4C 24 04"); //0x51E8E3 + 3
        pEntitySquaredDistancesOperand = reinterpret_cast<uintptr_t>(pattern.get_first(3));
        pattern = hook::pattern("D9 04 8D ? ? ? ? 8A 4D 10 D9 5C 24 1C C0 E9 04"); //0x53A2E4 + 3
        pLodSquaredDistancesOperand = reinterpret_cast<uintptr_t>(pattern.get_first(3));
        pattern = hook::pattern("D8 05 ? ? ? ? D9 FA D9 54 24 10 D8 1D"); //0x5470B6 + 2
        pRegionDistanceOperand = reinterpret_cast<uintptr_t>(pattern.get_first(2));
        pattern = hook::pattern("A1 ? ? ? ? 8D 1C C6 8D 44 18 01 8B 1D"); //0x55898F + 1
        pActiveMemoryMap = *pattern.get_first<uintptr_t>(1);
        pattern = hook::pattern("A3 ? ? ? ? C2 04 00 CC CC CC CC 83 EC 68 56"); //0x5146B4 + 1
        pStreamingDistance = *pattern.get_first<uintptr_t>(1);
        pattern = hook::pattern("D9 04 95 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 41"); //0x4CB45E + 3
        pOriginalDistances = *pattern.get_first<uintptr_t>(3);
        pattern = hook::pattern("D9 04 8D ? ? ? ? C3 CC CC 83 EC 08 8B CE E8"); //0x51E8B6 + 3
        pOriginalSquaredDistances = *pattern.get_first<uintptr_t>(3);
        pattern = hook::pattern("A3 ? ? ? ? B8 39 8E E3 38 F7 64 24 44 C1 EA 05"); //0x537872 + 1
        pStreamingBufferSize = *pattern.get_first<uintptr_t>(1);
        pattern = hook::pattern("8B 0D ? ? ? ? 8D 44 10 01 C1 E0 04 03 C1 C3"); //0x50DFCB + 2
        pMemoryMaps = *pattern.get_first<uintptr_t>(2);
        pattern = hook::pattern("A1 ? ? ? ? 33 FF 85 C0 7E ? 8B 1D"); //0x51EDEA + 1
        pMemoryMapCount = *pattern.get_first<uintptr_t>(1);
        pattern = hook::pattern("8B 92 C0 00 00 00 39 95 84 00 00 00 8A 95 80 00 00 00"); //0x5E14A2
        pMissionLookupContinue = reinterpret_cast<uintptr_t>(pattern.get_first());
        pattern = hook::pattern("8B D0 2B D1 68 ? ? ? ? C1 FA 02 52 50 51 E8 ? ? ? ? 83 C4 10"); //0x5DBCDA
        pMissionSortContinue = reinterpret_cast<uintptr_t>(pattern.get_first());
        pattern = hook::pattern("8B D0 2B D1 68 ? ? ? ? C1 FA 02 52 50 51 E8 ? ? ? ? 8B 46 3C"); //0x5E1AC1
        pMissionFrameSortContinue = reinterpret_cast<uintptr_t>(pattern.get_first());
    }

    struct RadarRegionList
    {
        uintptr_t regions[30];
        int32_t count;
    };

    SafetyHookInline shBuildRegionList;
    void __cdecl BuildRegionList(RadarRegionList* list, uintptr_t region, const float* position, float radius, int loadedOnly)
    {
        // The native result array doubles as its visited set. Once full, further
        // regions are never recorded, so adjacent unrecorded regions recurse forever.
        // All callers own thirty-entry arrays; stop traversal at that capacity.
        if (!list || !region || list->count >= static_cast<int32_t>(std::size(list->regions)))
            return;
        shBuildRegionList.ccall<void>(list, region, position, radius, loadedOnly);
    }

    void __cdecl BuildRadarRegionList(RadarRegionList* list, uintptr_t region, const float* position, float radius, int loadedOnly)
    {
        reinterpret_cast<void(__cdecl*)(RadarRegionList*, uintptr_t, const float*, float, int)>(pBuildRegionList)
            (list, region, position, radius, loadedOnly);
        struct Candidate { uintptr_t region; float distance; };
        std::array<Candidate, 30> candidates{};
        size_t count = 0;
        for (int32_t i = 0; i < list->count; ++i)
        {
            auto current = list->regions[i];
            if (!current)
                continue;
            auto flags = *reinterpret_cast<uint32_t*>(current + 0x50);
            if (!(flags & 0x10) || (flags & (0x100 | 0x40000)))
                continue;
            float minimum[3], maximum[3];
            reinterpret_cast<void(__thiscall*)(uintptr_t, float*, float*)>(pGetRegionBounds)(current, minimum, maximum);
            float distance = 0.0f;
            for (size_t axis = 0; axis < 3; ++axis)
            {
                auto delta = std::max({ minimum[axis] - position[axis], 0.0f, position[axis] - maximum[axis] });
                distance += delta * delta;
            }
            candidates[count++] = { current, distance };
        }
        // PrepareRegions and RenderMeshes share twelve inline material/extent slots.
        // Keep native ordering unless the expanded resident set exceeds that limit.
        if (count > 12)
            std::stable_sort(candidates.begin(), candidates.begin() + count,
                [](const auto& a, const auto& b) { return a.distance < b.distance; });
        list->count = static_cast<int32_t>(std::min(count, size_t(12)));
        for (int32_t i = 0; i < list->count; ++i)
            list->regions[i] = candidates[i].region;
    }

    float fRequestedDrawDistanceScale = 1.0f;
    float fDrawDistance = 167.0f;
    float fRenderRegionDistanceSquared = 27889.0f;
    std::array<float, 16> DrawDistances, DrawDistancesSquared;

    struct ResourceMemoryMap
    {
        uint32_t header[4];
        struct Partition
        {
            int32_t id, type, slotSize, slotCount;
        } partitions[8];
    };
    static_assert(sizeof(ResourceMemoryMap) == 0x90);

    constexpr int32_t nStreamingCapacityScale = 2;
    bool bStreamingCapacityIncreased = false;
    std::array<void*, 16> RegionAllocators = {};
    std::array<uintptr_t, 16> DistrictMissionTables = {};

    SafetyHookInline shCreateMissionManager;
    uintptr_t __fastcall CreateMissionManager(uintptr_t manager, void*)
    {
        auto result = shCreateMissionManager.thiscall<uintptr_t>(manager);
        // The game owns a single mission manager. Keep its layout, replacing the
        // inline eight-entry array with a pointer to storage sized for our slots.
        DistrictMissionTables.fill(0);
        *reinterpret_cast<uintptr_t**>(manager + 0x18) = DistrictMissionTables.data();
        return result;
    }

    int32_t __fastcall AddDistrictMissionTable(uintptr_t manager, void*, void* source, uintptr_t region)
    {
        auto tables = *reinterpret_cast<uintptr_t**>(manager + 0x18);
        auto& count = *reinterpret_cast<int32_t*>(manager + 0x38);
        reinterpret_cast<void(__cdecl*)(uintptr_t*, void*, int, int, int, int, int, int)>(pCreateDistrictTable)
            (&tables[count], source, 0, 0, 0, 0, 0, 0);
        *reinterpret_cast<uintptr_t*>(tables[count] + 0x44) = region;
        return ++count;
    }

    int32_t __fastcall RemoveDistrictMissionTable(uintptr_t manager, void*, uintptr_t region)
    {
        auto tables = *reinterpret_cast<uintptr_t**>(manager + 0x18);
        auto& count = *reinterpret_cast<int32_t*>(manager + 0x38);
        for (int32_t i = 0; i < count; ++i)
        {
            if (*reinterpret_cast<uintptr_t*>(tables[i] + 0x44) == region)
            {
                tables[i] = tables[count - 1];
                return --count;
            }
        }
        return count;
    }

    uintptr_t __fastcall GetDistrictMissionTable(uintptr_t manager, void*, int32_t index)
    {
        return (*reinterpret_cast<uintptr_t**>(manager + 0x18))[index];
    }

    void ExpandDistrictMissionTables()
    {
        {
            auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 6C 53 55 8B E9"); //0x5DA010
            shCreateMissionManager = safetyhook::create_inline(pattern.get_first(), CreateMissionManager);
        }
        {
            auto pattern = hook::pattern("8B 44 24 04 56 6A 00 6A 00 6A 00 6A 00 8B F1 8B 4E 38"); //0x5D1EE0
            static auto hookAddDistrictMissionTable = safetyhook::create_inline(pattern.get_first(), AddDistrictMissionTable);
        }
        {
            auto pattern = hook::pattern("56 8B 71 38 33 C0 85 F6 7E ? 57 8B 7C 24 0C 53"); //0x5BB080
            static auto hookRemoveDistrictMissionTable = safetyhook::create_inline(pattern.get_first(), RemoveDistrictMissionTable);
        }
        {
            auto pattern = hook::pattern("8B 44 24 04 8B 44 81 18 C2 04 00 CC CC CC CC CC"); //0x5BB4E0
            static auto hookGetDistrictMissionTable = safetyhook::create_inline(pattern.get_first(), GetDistrictMissionTable);
        }
        // Mission counts, vector/numeric properties and entity linking iterate it.
        // Change LEA [manager+18h] to MOV [manager+18h], preserving instruction size.
        {
            auto pattern = hook::pattern("8D 7B 18 8B 0F 8B 41 44 8B 54 24 14 3B 90 C0 00 00 00"); //0x5D1F31
            injector::WriteMemory(pattern.get_first(), uint8_t(0x8B), true);
        }
        {
            auto pattern = hook::pattern("8D 5D 18 90 8B 0B 8B 41 44 8B 54 24 30"); //0x5DA71C, 0x5DA7FC
            pattern.count(2);
            for (size_t i = 0; i < 2; ++i)
                injector::WriteMemory(pattern.get(i).get<void>(), uint8_t(0x8B), true);
        }
        {
            auto pattern = hook::pattern("8D 73 18 8B 0E 8B 41 44 8B 54 24 14 3B 90 C0 00 00 00"); //0x5DBEE3
            injector::WriteMemory(pattern.get_first(), uint8_t(0x8B), true);
        }
        {
            auto pattern = hook::pattern("8D 6B 18 8D 9B 00 00 00 00 8B 7D 00 85 FF 74"); //0x5DD637
            injector::WriteMemory(pattern.get_first(), uint8_t(0x8B), true);
        }

        {
            auto pattern = hook::pattern("8B 44 85 18 8B 50 44 8B 92 C0 00 00 00 39 95 84 00 00 00"); //0x5E149B
            static auto MissionLookupHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.eax = (*reinterpret_cast<uintptr_t**>(regs.ebp + 0x18))[regs.eax];
                regs.edx = *reinterpret_cast<uintptr_t*>(regs.eax + 0x44);
                regs.eip = pMissionLookupContinue;
            });
        }

        {
            auto pattern = hook::pattern("8D 44 81 18 83 C1 18 8B D0 2B D1 68"); //0x5DBCD3
            static auto MissionSortHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.ecx = *reinterpret_cast<uintptr_t*>(regs.ecx + 0x18);
                regs.eax = regs.ecx + regs.eax * sizeof(uintptr_t);
                regs.eip = pMissionSortContinue;
            });
        }

        {
            auto pattern = hook::pattern("8D 44 86 18 8D 4E 18 8B D0 2B D1 68"); //0x5E1ABA
            static auto MissionFrameSortHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.ecx = *reinterpret_cast<uintptr_t*>(regs.esi + 0x18);
                regs.eax = regs.ecx + regs.eax * sizeof(uintptr_t);
                regs.eip = pMissionFrameSortContinue;
            });
        }
    }

    // Cache used by terrain::find_region and the physics broad phase. The native
    // nine entries silently discard further loaded districts, leaving the player
    // without a region when travelling into one of them.
    struct LoadedRegion
    {
        float minimum[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
        float maximum[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        uint16_t allocationIndex = 0xFFFF;
        uint16_t padding = 0;
    };
    static_assert(sizeof(LoadedRegion) == 0x1C);
    std::array<LoadedRegion, RegionAllocators.size() + 1> LoadedRegions;
    uintptr_t* pRegionPool = nullptr;

    int32_t __cdecl CountLoadedRegions()
    {
        return static_cast<int32_t>(std::count_if(LoadedRegions.begin(), LoadedRegions.end(),
                                    [](const auto& region) { return region.allocationIndex != 0xFFFF; }));
    }

    uintptr_t __cdecl GetLoadedRegion(int32_t index)
    {
        if (index < 0)
            return 0;
        for (const auto& region : LoadedRegions)
        {
            if (region.allocationIndex != 0xFFFF && index-- == 0)
                return *pRegionPool + 0x134 * region.allocationIndex;
        }
        return 0;
    }

    void ExpandLoadedRegionCache()
    {
        {
            auto pattern = hook::pattern("8B 0D ? ? ? ? 03 F2 6B F6 1C 0F B7 86"); //0x519A40 + 2
            pRegionPool = *reinterpret_cast<uintptr_t**>(pattern.get_first(2));
        }
        {
            auto pattern = hook::pattern("B9 FF FF 00 00 33 C0 66 39 0D ? ? ? ? 74"); //0x5199B0
            static auto countLoadedRegionsHook = safetyhook::create_inline(pattern.get_first(), CountLoadedRegions);
        }
        {
            auto pattern = hook::pattern("8B 54 24 04 56 33 F6 85 D2 7E ? B8"); //0x519A20
            static auto getLoadedRegionHook = safetyhook::create_inline(pattern.get_first(), GetLoadedRegion);
        }
        {
            auto pattern = hook::pattern("B8 ? ? ? ? 0F B7 10 3B D1 74 ? 83 C0 1C 46"); //0x519A93 + 1
            injector::WriteMemory(pattern.get_first(1), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + 0x18, true);
        }
        {
            auto pattern = hook::pattern("3D ? ? ? ? 7C ? 5E 83 C4 0C C3 8B C6 6B C0 1C"); //0x519AA3 + 1
            injector::WriteMemory(pattern.get_first(1), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + sizeof(LoadedRegions) + 0x18, true);
        }
        {
            auto pattern = hook::pattern("8D 88 ? ? ? ? C7 44 24 04 FF FF 7F 7F 8B 54 24 04"); //0x519AB4 + 2
            injector::WriteMemory(pattern.get_first(2), reinterpret_cast<uintptr_t>(LoadedRegions.data()), true);
        }
        {
            auto pattern = hook::pattern("66 C7 80 ? ? ? ? FF FF 89 51 08 C7 44 24 04 FF FF 7F FF"); //0x519AE3 + 3
            injector::WriteMemory(pattern.get_first(3), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + 0x18, true);
        }
        {
            auto pattern = hook::pattern("8D 80 ? ? ? ? 89 08 C7 44 24 08 FF FF 7F FF"); //0x519AFB + 2
            injector::WriteMemory(pattern.get_first(2), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + 0xC, true);
        }
        {
            auto pattern = hook::pattern("BE ? ? ? ? EB ? 8D A4 24 00 00 00 00 66 8B 7E 10"); //0x52E9F2 + 1
            injector::WriteMemory(pattern.get_first(1), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + 0x8, true);
        }
        {
            auto pattern = hook::pattern("81 FE ? ? ? ? 0F 8C ? ? ? ? 5F 5E 5D 5B 81 C4 A0 00 00 00 C3 81 EC C0 00 00 00"); //0x52EB49 + 2
            injector::WriteMemory(pattern.get_first(2), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + sizeof(LoadedRegions) + 0x8, true);
        }
        {
            auto pattern = hook::pattern("B8 ? ? ? ? 8D 6F 20 8B F7 8D 5F 30 83 C7 10"); //0x53B92E + 1
            injector::WriteMemory(pattern.get_first(1), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + 0x18, true);
        }
        {
            auto pattern = hook::pattern("3D ? ? ? ? 89 44 24 14 0F 8C ? ? ? ? A1"); //0x53BA94 + 1
            injector::WriteMemory(pattern.get_first(1), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + sizeof(LoadedRegions) + 0x18, true);
        }
        {
            auto pattern = hook::pattern("BD ? ? ? ? 90 66 8B 5D 00 66 81 FB FF FF 74"); //0x5401CA + 1
            injector::WriteMemory(pattern.get_first(1), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + 0x18, true);
        }
        {
            auto pattern = hook::pattern("81 FD ? ? ? ? 7C ? 5F 5E 5D 5B C3 8B 4E 64"); //0x540201 + 2
            injector::WriteMemory(pattern.get_first(2), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + sizeof(LoadedRegions) + 0x18, true);
        }
        {
            auto pattern = hook::pattern("81 C7 ? ? ? ? 8D 57 0C 52 57 E8"); //0x540214 + 2
            injector::WriteMemory(pattern.get_first(2), reinterpret_cast<uintptr_t>(LoadedRegions.data()), true);
        }
        {
            auto pattern = hook::pattern("BE ? ? ? ? EB ? 8D A4 24 00 00 00 00 8D 64 24 00"); //0x565CBE + 1
            injector::WriteMemory(pattern.get_first(1), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + 0x8, true);
        }
        {
            auto pattern = hook::pattern("81 FE ? ? ? ? 0F 8C ? ? ? ? 5F 5E 5D 5B 81 C4 A0 00 00 00 C3 56"); //0x565E19 + 2
            injector::WriteMemory(pattern.get_first(2), reinterpret_cast<uintptr_t>(LoadedRegions.data()) + sizeof(LoadedRegions) + 0x8, true);
        }
        // Native query callers own fifteen-entry vectors; the neighbour builder
        // owns thirty-two scratch entries and a matching bounds buffer.
        {
            auto pattern = hook::pattern("8B 4B 3C 89 04 8B FF 43 3C 83 C6 1C 81 FE"); //0x52EB3D
            static auto continuation = reinterpret_cast<uintptr_t>(pattern.get_first(9));
            static auto BoxRegionCapacityHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (*reinterpret_cast<uint32_t*>(regs.ebx + 0x3C) >= 15)
                    regs.eip = continuation;
            });
        }
        {
            auto pattern = hook::pattern("8B 53 3C 89 0C 93 FF 43 3C 83 C6 1C 81 FE"); //0x565E0D
            static auto continuation = reinterpret_cast<uintptr_t>(pattern.get_first(9));
            static auto SphereRegionCapacityHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (*reinterpret_cast<uint32_t*>(regs.ebx + 0x3C) >= 15)
                    regs.eip = continuation;
            });
        }
        {
            auto pattern = hook::pattern("89 44 94 34 FF 84 24 B4 00 00 00 8D 4C 24 28 51"); //0x53BA03
            static auto continuation = reinterpret_cast<uintptr_t>(pattern.get_first(120));
            static auto NeighbourRegionCapacityHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (regs.edx >= 32)
                    regs.eip = continuation;
            });
        }
    }

    struct ShadowRegionList
    {
        uintptr_t vtable;
        int32_t count = 0;
        std::array<uintptr_t, RegionAllocators.size() + 1> regions = {};
    };
    static_assert(offsetof(ShadowRegionList, regions) == 8);

    int __fastcall VisitShadowRegion(ShadowRegionList* list, void*, uintptr_t region)
    {
        if ((*reinterpret_cast<uint32_t*>(region + 0x50) & 0x10) == 0)
            return 0;
        auto end = list->regions.begin() + list->count;
        if (end != list->regions.end() && std::find(list->regions.begin(), end, region) == end)
            list->regions[list->count++] = region;
        return 0;
    }

    uintptr_t ShadowRegionVtable[] = { reinterpret_cast<uintptr_t>(&VisitShadowRegion) };
    thread_local ShadowRegionList* pShadowRegions = nullptr;
    SafetyHookInline shRenderStencilShadows;
    void __fastcall RenderStencilShadows(uintptr_t renderer, void*, uintptr_t camera)
    {
        ShadowRegionList regions{ reinterpret_cast<uintptr_t>(ShadowRegionVtable) };
        auto previous = pShadowRegions;
        pShadowRegions = &regions;
        shRenderStencilShadows.thiscall<void>(renderer, camera);
        pShadowRegions = previous;
    }

    injector::hook_back<int(__thiscall*)(uintptr_t, void*, ShadowRegionList*)> hbCollectShadowRegions;
    int __fastcall CollectShadowRegions(uintptr_t map, void*, void* polygon, ShadowRegionList* nativeList)
    {
        auto result = hbCollectShadowRegions.fun(map, polygon, pShadowRegions);
        // Keep the native loop counter, but never copy into its nine-entry array:
        // the tenth entry would overwrite the visibility polygon immediately after it.
        nativeList->count = pShadowRegions->count;
        return result;
    }

    void ExpandShadowRegionList()
    {
        auto pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC 14 01 00 00 A0"); //0x53D5E0
        shRenderStencilShadows = safetyhook::create_inline(pattern.get_first(), RenderStencilShadows);

        pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 8B CB E8"); //0x53D89C
        hbCollectShadowRegions.fun = injector::MakeCALL(pattern.get_first(), CollectShadowRegions, true).get();

        pattern = hook::pattern("8B BC B4 84 00 00 00 8D 4C 24 60 51 8B 4F 64 8D 54 24 24"); //0x53D900
        static auto nextInstruction = reinterpret_cast<uintptr_t>(pattern.get_first(7)); //0x53D900 + 7
        static auto ShadowRegionReadHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.edi = pShadowRegions->regions[regs.esi];
            regs.eip = nextInstruction;
        });
    }

    void LimitEntityRegionMembership()
    {
        // entity has two inline region pointers and a fixed seven-pointer overflow
        // vector. Larger spatial queries must not overwrite that vector's count.
        auto pattern = hook::pattern("8B 44 24 08 83 F8 01 53 55 8B 6C 24 0C 8B D9"); //0x4F5510
        static auto EntityRegionCountHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto& count = *reinterpret_cast<int32_t*>(regs.esp + 8);
            count = std::min(count, 9);
        });

        // add_me_to_region also has direct callers. If full, skip both the pointer
        // insertion and region::add so the reciprocal membership stays consistent.
        pattern = hook::pattern("8B 46 50 8B 50 1C 89 3C 90 FF 40 1C 56 8B CF"); //0x4F536B
        static auto returnWithoutAdding = reinterpret_cast<uintptr_t>(pattern.get_first(0x14)); //0x4F536B + 0x14
        static auto EntityRegionCapacityHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto extended = *reinterpret_cast<uintptr_t*>(regs.esi + 0x50);
            if (!extended || *reinterpret_cast<uint32_t*>(extended + 0x1C) >= 7)
                regs.eip = returnWithoutAdding;
        });
    }

    void ExpandRegionAllocatorPool()
    {
        // Region::create_proximity_maps acquires one allocator per loaded region.
        // The original eight-pointer table cannot support expanded district slots.
        // Keep all native lifecycle paths, including the inlined release in region
        // teardown, on the same relocated table. The occupancy mask is only 32 bits.
        {
            auto pattern = hook::pattern("8B 15 ? ? ? ? 33 C9 85 D2 56 7E ? A1"); //0x5198A0 + 2
            injector::WriteMemory(*pattern.get_first<uintptr_t>(2), uint32_t(RegionAllocators.size()), true);
        }
        // lego_bitvector_pool also has one entry per loaded region. It does not
        // grow after its first block; size that block before the first allocation.
        {
            auto pattern = hook::pattern("3D ? ? ? ? 89 44 24 10 89 5C 24 14 89 7C 24 18"); //0x5346BC + 1
            injector::WriteMemory(*pattern.get_first<uintptr_t>(1) + 0x10, uint32_t(RegionAllocators.size()), true);
        }
        // Region entity hash tables are checked out from a separate list. Its
        // initializer makes nine tables (eight district slots plus one spare).
        // An empty checkout reads the sentinel's uninitialized value as a table.
        {
            auto pattern = hook::pattern("BB 09 00 00 00 6A 28 E8 ? ? ? ? 8B F0 83 C4 04"); //0x55544B + 1
            injector::WriteMemory(pattern.get_first(1), uint32_t(RegionAllocators.size() + 1), true);
        }
        {
            auto pattern = hook::pattern("8B 04 8D ? ? ? ? C3 CC CC CC A1"); //0x5198D5 + 3
            injector::WriteMemory(pattern.get_first(3), RegionAllocators.data(), true);
        }
        {
            auto pattern = hook::pattern("39 14 8D ? ? ? ? 74 ? 41 3B C8 7C ? C3 B8 01 00 00 00"); //0x5198F0 + 3
            injector::WriteMemory(pattern.get_first(3), RegionAllocators.data(), true);
        }
        {
            auto pattern = hook::pattern("8B 34 BD ? ? ? ? 3B F3 74 ? 8B 46 10 50 E8"); //0x52E710 + 3
            injector::WriteMemory(pattern.get_first(3), RegionAllocators.data(), true);
        }
        {
            auto pattern = hook::pattern("8B 0C BD ? ? ? ? 51 E8 ? ? ? ? 83 C4 08"); //0x52E72A + 3
            injector::WriteMemory(pattern.get_first(3), RegionAllocators.data(), true);
        }
        {
            auto pattern = hook::pattern("89 1C BD ? ? ? ? A1 ? ? ? ? 47 3B F8 7C"); //0x52E73A + 3
            injector::WriteMemory(pattern.get_first(3), RegionAllocators.data(), true);
        }
        {
            auto pattern = hook::pattern("A1 ? ? ? ? 53 33 DB 3B C3 0F 85"); //0x53B860 + 1
            injector::WriteMemory(pattern.get_first(1), RegionAllocators.data(), true);
        }
        {
            auto pattern = hook::pattern("89 34 BD ? ? ? ? 89 6E 0C 74 ? 38 1D"); //0x53B8A6 + 3
            injector::WriteMemory(pattern.get_first(3), RegionAllocators.data(), true);
        }
        {
            auto pattern = hook::pattern("39 04 8D ? ? ? ? 74 ? 41 3B CA 7C ? 5F 89 5E 1C"); //0x5456C7 + 3
            injector::WriteMemory(pattern.get_first(3), RegionAllocators.data(), true);
        }
    }

    uint32_t ExpandStreamingMaps(ResourceMemoryMap* maps, uint32_t count, uint32_t originalSize, int32_t scale)
    {
        scale = std::clamp(scale, 1, 4);
        uint64_t bufferSize = originalSize;
        for (uint32_t i = 0; i < count; ++i)
        {
            auto& map = maps[i];
            // Single large district slots belong to mission layouts, not city
            // streaming. Preserve those layouts and all authored slot sizes.
            if (map.partitions[6].type != 0 || map.partitions[6].slotCount <= 1)
                continue;
            for (auto index : { 5, 6 })
            {
                auto& part = map.partitions[index];
                if (part.type != 0 || part.slotCount <= 0 || part.slotCount > 128 || part.slotSize <= 0 || part.slotSize % 4096)
                    return 0;
                part.slotCount *= scale;
                if (part.slotCount > static_cast<int32_t>(RegionAllocators.size()))
                    return 0;
            }
            uint64_t mapSize = 0;
            for (const auto& part : map.partitions)
            {
                if (part.slotSize < 0 || part.slotCount < 0)
                    return 0;
                mapSize += uint64_t(part.slotSize) * part.slotCount;
            }
            bufferSize = std::max(bufferSize, mapSize);
        }
        // Bound the contiguous allocation in this 32-bit game. The caller only
        // commits the copied maps once allocation succeeds; otherwise use stock.
        if (bufferSize > 512ull * 1024 * 1024 || bufferSize > uint64_t(originalSize) + 256ull * 1024 * 1024)
            return 0;
        return static_cast<uint32_t>(bufferSize);
    }

    void SetDrawDistance(float scale);

    void* __cdecl AllocateStreamingBuffer(uint32_t alignment, uint32_t size)
    {
        auto allocate = reinterpret_cast<void* (__cdecl*)(uint32_t, uint32_t)>(pAllocate);
        auto maps = *reinterpret_cast<ResourceMemoryMap**>(pMemoryMaps);
        auto count = *reinterpret_cast<int32_t*>(pMemoryMapCount);
        bStreamingCapacityIncreased = false;
        if (nStreamingCapacityScale > 1 && maps && count > 0 && count <= 128)
        {
            std::vector<ResourceMemoryMap> expanded(maps, maps + count);
            auto expandedSize = ExpandStreamingMaps(expanded.data(), count, size, nStreamingCapacityScale);
            if (expandedSize > size)
            {
                if (auto buffer = allocate(alignment, expandedSize))
                {
                    std::copy(expanded.begin(), expanded.end(), maps);
                    *reinterpret_cast<uint32_t*>(pStreamingBufferSize) = expandedSize;
                    bStreamingCapacityIncreased = true;
                    SetDrawDistance(fRequestedDrawDistanceScale);
                    return buffer;
                }
            }
        }
        return allocate(alignment, size);
    }

    SafetyHookInline shFindTerrainPacks;
    void __fastcall FindTerrainPacks(void* terrain, void*, const void* position, void* region, void* extraRegions, void* packs)
    {
        auto& distance = *reinterpret_cast<float*>(pStreamingDistance);
        auto original = distance;
        auto maps = *reinterpret_cast<ResourceMemoryMap**>(pMemoryMaps);
        auto count = *reinterpret_cast<int32_t*>(pMemoryMapCount);
        auto active = *reinterpret_cast<int32_t*>(pActiveMemoryMap);
        bool cityLayout = maps && active >= 0 && active < count && maps[active].partitions[6].slotCount > 1;
        // Keep the stock prefetch margin beyond the extended detail range. Smaller
        // script-requested distances remain untouched (interiors/mission transitions).
        if (bStreamingCapacityIncreased && cityLayout && distance >= 600.0f)
            distance = std::min(900.0f, distance + std::max(0.0f, fDrawDistance - 167.0f));
        shFindTerrainPacks.thiscall<void>(terrain, position, region, extraRegions, packs);
        distance = original;
    }

    void SetDrawDistance(float scale)
    {
        if (!std::isfinite(scale))
            scale = 1.0f;
        scale = std::clamp(scale, 1.0f, 10.0f);
        if (scale == 1.0f)
            return;

        // This changes rendering, not pack residency. Above ~3.6x the detail radius
        // exceeds the stock 600-unit streaming search distance; higher scales are experimental.
        // Leave low-detail city meshes available as a fallback for unloaded regions.
        fDrawDistance = 167.0f * scale;
        fRenderRegionDistanceSquared = fDrawDistance * fDrawDistance;
        injector::WriteMemory(pRegionDistanceOperand, &fRenderRegionDistanceSquared, true);
        injector::WriteMemory(pMeshDistanceOperand, &fDrawDistance, true);
        injector::WriteMemory(pLegoDistanceOperand, &fDrawDistance, true);
        injector::WriteMemory(pMeshDistanceImmediate, fDrawDistance, true);
        injector::WriteMemory(pLegoDistanceImmediate, fDrawDistance, true);

        // Rendering uses both linear and squared fade distances. Keep the original
        // tables for assigning authored distance categories when entities are loaded.
        for (size_t i = 0; i < DrawDistances.size(); ++i)
        {
            DrawDistances[i] = reinterpret_cast<float*>(pOriginalDistances)[i] * scale;
            DrawDistancesSquared[i] = reinterpret_cast<float*>(pOriginalSquaredDistances)[i] * scale * scale;
        }
        injector::WriteMemory(pLinearDistancesOperand, DrawDistances.data(), true);
        injector::WriteMemory(pSquaredDistancesOperand, DrawDistancesSquared.data(), true);
        injector::WriteMemory(pEntitySquaredDistancesOperand, DrawDistancesSquared.data(), true);
        injector::WriteMemory(pLodSquaredDistancesOperand, DrawDistancesSquared.data(), true);
    }
}

export void InitExtremeDrawDistance(float scale)
{
    using namespace ExtremeDistance;
    fRequestedDrawDistanceScale = std::clamp(scale, 2.0f, 10.0f);
    ResolveRuntimePointers();
    ExpandRegionAllocatorPool();
    ExpandDistrictMissionTables();
    ExpandLoadedRegionCache();
    LimitEntityRegionMembership();
    ExpandShadowRegionList();
    shBuildRegionList = safetyhook::create_inline(reinterpret_cast<void*>(pBuildRegionList), BuildRegionList);
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B 8C 24 D4 00 00 00 33 C0 83 C4 14"); //0x619729
        injector::MakeCALL(pattern.get_first(), BuildRadarRegionList, true);
    }
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 53 A3 ? ? ? ? 89 1D"); //0x55BAD3
        injector::MakeCALL(pattern.get_first(), AllocateStreamingBuffer, true);
    }
    {
        auto pattern = hook::pattern("55 8B EC 83 E4 F8 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 50"); //0x54EC50
        shFindTerrainPacks = safetyhook::create_inline(pattern.get_first(), FindTerrainPacks);
    }
}
