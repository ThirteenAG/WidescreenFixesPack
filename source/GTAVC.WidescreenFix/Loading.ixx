module;

#include <stdafx.h>
#include "common.h"

export module Loading;

import Skeleton;
import Entity;
import Camera;

export bool gbNoIslandLoading = false;
export bool gbNoInteriorLoading = false;

export bool bSkipNextPlayerCoords = false;
export bool bSkipNextPlayerHeading = false;
export bool bSkipNextFade = false;
export bool bSkipNextLoadScene = false;

class CPtrNode
{
public:
    void* item;
    CPtrNode* prev;
    CPtrNode* next;
};

class CPtrList
{
public:
    CPtrNode* first;
};

export enum eLevelName
{
    LEVEL_IGNORE = -1,
    LEVEL_GENERIC = 0,
    LEVEL_BEACH,
    LEVEL_MAINLAND,

    NUM_LEVELS
};

export enum eAreaName
{
    AREA_MAIN_MAP,
    AREA_HOTEL,
    AREA_MANSION,
    AREA_BANK,
    AREA_MALL,
    AREA_STRIP_CLUB,
    AREA_LAWYERS,
    AREA_COFFEE_SHOP,
    AREA_CONCERT_HALL,
    AREA_STUDIO,
    AREA_RIFLE_RANGE,
    AREA_BIKER_BAR,
    AREA_POLICE_STATION,
    AREA_EVERYWHERE,
    AREA_DIRT,
    AREA_BLOOD,
    AREA_OVALRING,
    AREA_MALIBU_CLUB,
    AREA_PRINT_WORKS
};

export namespace CGame
{
    GameRef<eLevelName> currLevel([]() -> eLevelName*
    {
        auto pattern = find_pattern("3B 05 ? ? ? ? 75 ? 6A");
        if (!pattern.empty())
            return *pattern.get_first<eLevelName*>(2);
        return nullptr;
    });

    GameRef<eAreaName> currArea([]() -> eAreaName*
    {
        auto pattern = find_pattern("83 3D ? ? ? ? ? 74 ? 83 3D ? ? ? ? ? 7E ? E8 ? ? ? ? 84 C0");
        if (!pattern.empty())
            return *pattern.get_first<eAreaName*>(2);
        return nullptr;
    });

    GameRef<bool> playingIntro([]() -> bool*
    {
        auto pattern = find_pattern("80 3D ? ? ? ? ? 74 ? 6A 00 E8 ? ? ? ? 30 D2");
        if (!pattern.empty())
            return *pattern.get_first<bool*>(2);
        return nullptr;
    });
}

using CBuilding = CEntity;

struct CBuildingPool
{
    CBuilding* m_entries;
    uint8_t* m_flags;
    int32_t m_size;
    int32_t m_allocPtr;
};

namespace CPools
{
    CBuildingPool** pp_pBuildingPool = nullptr;
}

namespace CStreaming
{
    CEntity** pIslandLODmainlandEntity = nullptr;
    CEntity** pIslandLODbeachEntity = nullptr;

    void (__cdecl* RequestModel)(int modelId, int flags) = nullptr;
}

namespace CWorld
{
    CPtrList* ms_bigBuildingsList = nullptr;
}

namespace CRenderer
{
    char (__cdecl* ShouldModelBeStreamed)(CEntity* ent, RwV3d* campos) = nullptr;

    injector::hook_back<void(__cdecl*)(CPtrList*)> hbScanBigBuildingList;
    void __cdecl ScanBigBuildingList(CPtrList* list)
    {
        hbScanBigBuildingList.fun(&CWorld::ms_bigBuildingsList[LEVEL_BEACH]);
        hbScanBigBuildingList.fun(&CWorld::ms_bigBuildingsList[LEVEL_MAINLAND]);
    }
}

namespace CStreaming
{
    enum StreamFlags
    {
        STREAMFLAGS_DONT_REMOVE = 0x01,
        STREAMFLAGS_SCRIPTOWNED = 0x02,
        STREAMFLAGS_DEPENDENCY = 0x04,
        STREAMFLAGS_PRIORITY = 0x08,
        STREAMFLAGS_NOFADE = 0x10,
        STREAMFLAGS_20 = 0x20,

        STREAMFLAGS_CANT_REMOVE = STREAMFLAGS_DONT_REMOVE | STREAMFLAGS_SCRIPTOWNED,
        STREAMFLAGS_KEEP_IN_MEMORY = STREAMFLAGS_DONT_REMOVE | STREAMFLAGS_SCRIPTOWNED | STREAMFLAGS_DEPENDENCY,
    };

    SafetyHookInline shRequestBigBuildings = {};
    void __cdecl RequestBigBuildings(eLevelName level)
    {
        auto ms_pBuildingPool = *CPools::pp_pBuildingPool;
        const int size = ms_pBuildingPool->m_size;

        if (size > 0)
        {
            for (int i = size - 1; i >= 0; --i)
            {
                CBuilding* b = (ms_pBuildingPool->m_flags[i] & 0x80) != 0 ? nullptr : &ms_pBuildingPool->m_entries[i];

                if (b && b->bIsBIGBuilding && (((b != *pIslandLODbeachEntity) && (b != *pIslandLODmainlandEntity)) || (b->m_level == level)))
                    if (!b->bStreamBIGBuilding)
                        RequestModel(b->GetModelIndex(), STREAMFLAGS_DONT_REMOVE);
            }
        }

        //RequestIslands(level);
    }

    constexpr auto IslandLOD1 = 2544;
    constexpr auto IslandLOD2 = 2545;
    constexpr auto IslandLODmainland = 2600;
    constexpr auto IslandLODbeach = 2634;

    export void RequestBigBuildings2(eLevelName level, RwV3d& pos)
    {
        auto ms_pBuildingPool = *CPools::pp_pBuildingPool;
        const int size = ms_pBuildingPool->m_size;

        if (size > 0)
        {
            for (int i = size - 1; i >= 0; --i)
            {
                CBuilding* b = (ms_pBuildingPool->m_flags[i] & 0x80) != 0 ? nullptr : &ms_pBuildingPool->m_entries[i];

                if (b && b->bIsBIGBuilding && (((b != *pIslandLODbeachEntity) && (b != *pIslandLODmainlandEntity)) || (b->m_level == level)))
                    if (b->bStreamBIGBuilding)
                    {
                        if (CRenderer::ShouldModelBeStreamed(b, &pos))
                            RequestModel(b->GetModelIndex(), 0);
                    }
                    else
                        RequestModel(b->GetModelIndex(), STREAMFLAGS_DONT_REMOVE);

                if (b && (b->m_modelIndex == IslandLOD1 || b->m_modelIndex == IslandLOD2 || b->m_modelIndex == IslandLODmainland || b->m_modelIndex == IslandLODbeach))
                {
                    if (CGame::currArea == AREA_MAIN_MAP)
                        b->bIsVisible = 0;
                    else
                        b->bIsVisible = 1;
                }

                if (gbNoInteriorLoading)
                {
                    constexpr auto man_twr_stairs = 2524;

                    if (b && b->m_modelIndex == man_twr_stairs)
                    {
                        if (b->m_modelIndex == man_twr_stairs && TheCamera->GetCoords().z > 20.0f && CGame::currArea == AREA_MAIN_MAP)
                            b->bIsVisible = 0;
                        else
                            b->bIsVisible = 1;
                    }
                }
            }
        }

        //RequestIslands(level);
    }

    void FixIslandLODs()
    {
        auto ms_pBuildingPool = *CPools::pp_pBuildingPool;
        const int size = ms_pBuildingPool->m_size;

        if (size > 0)
        {
            for (int i = size - 1; i >= 0; --i)
            {
                CBuilding* b = (ms_pBuildingPool->m_flags[i] & 0x80) != 0 ? nullptr : &ms_pBuildingPool->m_entries[i];

                if (b && (b->m_modelIndex == IslandLOD1 || b->m_modelIndex == IslandLOD2 || b->m_modelIndex == IslandLODmainland || b->m_modelIndex == IslandLODbeach))
                {
                    if (CGame::currArea == AREA_MAIN_MAP)
                        b->bIsVisible = 0;
                    else
                        b->bIsVisible = 1;
                }

                if (gbNoInteriorLoading)
                {
                    constexpr auto man_twr_stairs = 2524;

                    if (b && b->m_modelIndex == man_twr_stairs)
                    {
                        if (b->m_modelIndex == man_twr_stairs && TheCamera->GetCoords().z > 20.0f && CGame::currArea == AREA_MAIN_MAP)
                            b->bIsVisible = 0;
                        else
                            b->bIsVisible = 1;
                    }
                }
            }
        }
    }
}

void __fastcall CMenuManager__MessageScreenStub(void*, void*, char*, uint8_t)
{

}

void __cdecl CTimer__SuspendStub()
{

}

void __cdecl CTimer__ResumeStub()
{

}

namespace CRenderer
{
    SafetyHookInline shIsEntityCullZoneVisible = {};
    bool __cdecl IsEntityCullZoneVisible(void* entity)
    {
        return true;
    }
}

class Loading
{
public:
    Loading()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            gbNoIslandLoading = iniReader.ReadInteger("MAIN", "NoIslandLoading", 1) != 0;

            if (gbNoIslandLoading)
            {
                auto pattern = hook::pattern("05 ? ? ? ? 8D 9C 24");
                CWorld::ms_bigBuildingsList = *pattern.get_first<CPtrList*>(1);

                pattern = hook::pattern("E8 ? ? ? ? 84 C0 59 59 74 ? 6A ? EB ? 8D 40");
                CRenderer::ShouldModelBeStreamed = (decltype(CRenderer::ShouldModelBeStreamed))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("A3 ? ? ? ? EB ? 8D 40 ? 39 FE");
                auto arr = *pattern.get_first<CEntity**>(1);
                CStreaming::pIslandLODmainlandEntity = &arr[0];
                CStreaming::pIslandLODbeachEntity = &arr[1];

                //CStreaming::RequestBigBuildings
                pattern = hook::pattern("E8 ? ? ? ? 59 59 83 ED");
                CStreaming::RequestModel = (decltype(CStreaming::RequestModel))injector::GetBranchDestination(pattern.get_first()).as_int();
                pattern = hook::pattern("53 56 8B 35 ? ? ? ? 57 89 F0 55 8B 58 ? 8B 7C 24 ? 83 EB ? 72");
                CPools::pp_pBuildingPool = *pattern.get_first<CBuildingPool**>(4);
                CStreaming::shRequestBigBuildings = safetyhook::create_inline(pattern.get_first(), CStreaming::RequestBigBuildings);

                //LoadScene
                pattern = hook::pattern("0F 84 ? ? ? ? 8B 35 ? ? ? ? 89 F1 8B 41 ? 89 44 24 ? 83 6C 24 ? ? 0F 82 ? ? ? ? 8B 54 24 ? ? ? ? 89 44 24 ? C1 64 24 ? ? 8B 4C 24 ? ? ? ? 89 44 24 ? 89 C0");
                injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true);

                pattern = hook::pattern("0F 8C ? ? ? ? FF 74 24 ? E8 ? ? ? ? 8B 35");
                injector::MakeNOP(pattern.get_first(), 6);

                pattern = hook::pattern("0F 82 ? ? ? ? 89 EF ? ? ? 89 44 24");
                injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true);

                pattern = hook::pattern("75 ? 83 3D ? ? ? ? ? 74 ? A1 ? ? ? ? 6A ? EB ? 83 7C 24");
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);

                pattern = hook::pattern("75 ? 83 3D ? ? ? ? ? 74 ? A1 ? ? ? ? 6A ? 50 E8 ? ? ? ? 59 59 8B 1D");
                injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);

                pattern = hook::pattern("0F 82 ? ? ? ? 89 EA");
                injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true);

                static eLevelName level = LEVEL_IGNORE;
                static auto pos = CVector{ 0.0f, 0.0f, 0.0f };

                pattern = find_pattern("59 89 44 24 ? 68 ? ? ? ? E8 ? ? ? ? 8B 2D");
                static auto LoadSceneHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    level = (eLevelName)regs.eax;
                    pos = **reinterpret_cast<RwV3d**>(regs.esp + 0x88);
                });

                pattern = find_pattern("FF 74 24 ? E8 ? ? ? ? 59 6A ? E8");
                static auto LoadSceneHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CStreaming::RequestBigBuildings2(level, pos);
                    CStreaming::RequestBigBuildings2(LEVEL_GENERIC, pos);
                });

                //CStreaming::RemoveBuildingsNotInArea
                pattern = find_pattern("8B 7D ? 83 EF");
                static auto RemoveBuildingsNotInAreaHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CStreaming::FixIslandLODs();
                });

                //CStreaming::RemoveIslandsNotUsed
                pattern = hook::pattern("75 ? 8B 1D ? ? ? ? 85 DB 0F 84");
                injector::MakeNOP(pattern.get_first(), 2);

                pattern = hook::pattern("75 ? 8B 1D ? ? ? ? 85 DB 74 ? 8A 43");
                injector::MakeNOP(pattern.get_first(), 2);

                //CStreaming::RemoveUnusedBigBuildings
                pattern = hook::pattern("0F 84 ? ? ? ? 8B 35 ? ? ? ? 89 F5");
                injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true);

                pattern = hook::pattern("0F 8C ? ? ? ? FF 74 24 ? E8 ? ? ? ? 59 83 C4");
                injector::MakeNOP(pattern.get_first(), 6);

                //CStreaming::LoadBigBuildingsWhenNeeded
                pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 75 ? BB");
                injector::MakeRET(injector::GetBranchDestination(pattern.get_first()).as_int());

                //CRenderer::ScanWorld
                pattern = hook::pattern("E8 ? ? ? ? 59 68 ? ? ? ? E8 ? ? ? ? 59 E9");
                CRenderer::hbScanBigBuildingList.fun = injector::MakeCALL(pattern.get_first(), CRenderer::ScanBigBuildingList).get();

                //CColStore::EnsureCollisionIsInMemory
                pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 6A ? E8 ? ? ? ? 59 E8 ? ? ? ? 45");
                injector::MakeCALL(pattern.get_first(), CMenuManager__MessageScreenStub);

                pattern = hook::pattern("E8 ? ? ? ? 6A ? E8 ? ? ? ? 59 E8 ? ? ? ? 45");
                injector::MakeCALL(pattern.get_first(), CTimer__SuspendStub);

                pattern = hook::pattern("E8 ? ? ? ? 45 83 C7 ? 83 C6");
                injector::MakeCALL(pattern.get_first(), CTimer__ResumeStub);
            }
        };
    }
} Loading;