module;

#include <stdafx.h>
#include "common.h"

export module Loading;

import Skeleton;
import Entity;

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

enum eLevelName
{
    LEVEL_IGNORE = -1,
    LEVEL_GENERIC = 0,
    LEVEL_INDUSTRIAL,
    LEVEL_COMMERCIAL,
    LEVEL_SUBURBAN,
    NUM_LEVELS
};

namespace CGame
{
    GameRef<eLevelName> currLevel([]() -> eLevelName*
    {
        auto pattern = find_pattern("3B 05 ? ? ? ? 75 ? 6A");
        if (!pattern.empty())
            return *pattern.get_first<eLevelName*>(2);
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

const char* (__cdecl* GetLevelSplashScreen)(eLevelName levelName) = nullptr;
void (__cdecl* LoadSplash)(const char* name) = nullptr;

namespace CFileLoader
{
    void (__cdecl* LoadCollisionFromDatFile)(unsigned int gameLevel) = nullptr;
}

namespace CCollision
{
    GameRef<eLevelName> ms_collisionInMemory([]() -> eLevelName*
    {
        auto pattern = find_pattern("8B 0D ? ? ? ? 39 C1 74 ? 53");
        if (!pattern.empty())
            return *pattern.get_first<eLevelName*>(2);
        return nullptr;
    });

    bool bAlreadyLoaded = false;
    SafetyHookInline shSortOutCollisionAfterLoad = {};
    void SortOutCollisionAfterLoad(void)
    {
        if (ms_collisionInMemory == CGame::currLevel)
            return;
        if (CGame::currLevel != LEVEL_GENERIC)
        {
            if (bAlreadyLoaded)
            {
                ms_collisionInMemory = CGame::currLevel;
                return;
            }
            bAlreadyLoaded = true;
            CFileLoader::LoadCollisionFromDatFile(LEVEL_INDUSTRIAL);
            CFileLoader::LoadCollisionFromDatFile(LEVEL_COMMERCIAL);
            CFileLoader::LoadCollisionFromDatFile(LEVEL_SUBURBAN);

            if (!CGame::playingIntro)
                LoadSplash(GetLevelSplashScreen(CGame::currLevel));
        }
        ms_collisionInMemory = CGame::currLevel;
    }
}

void (__cdecl* DeleteIsland)(CEntity* entity) = nullptr;

namespace CStreaming
{
    CEntity** pIslandLODindustEntity = nullptr;
    CEntity** pIslandLODcomIndEntity = nullptr;
    CEntity** pIslandLODcomSubEntity = nullptr;
    CEntity** pIslandLODsubIndEntity = nullptr;
    CEntity** pIslandLODsubComEntity = nullptr;

    void (__cdecl* RequestModel)(int modelId, int flags) = nullptr;

    SafetyHookInline shRemoveIslandsNotUsed = {};
    void RemoveIslandsNotUsed(eLevelName level)
    {
        DeleteIsland(*pIslandLODindustEntity);
        DeleteIsland(*pIslandLODcomIndEntity);
        DeleteIsland(*pIslandLODcomSubEntity);
        DeleteIsland(*pIslandLODsubIndEntity);
        DeleteIsland(*pIslandLODsubComEntity);
    }
}

namespace CWorld
{
    CPtrList* ms_bigBuildingsList = nullptr;
}

namespace CRenderer
{
    injector::hook_back<void(__cdecl*)(CPtrList*)> hbScanBigBuildingList;
    void __cdecl ScanBigBuildingList(CPtrList* list)
    {
        hbScanBigBuildingList.fun(&CWorld::ms_bigBuildingsList[LEVEL_INDUSTRIAL]);
        hbScanBigBuildingList.fun(&CWorld::ms_bigBuildingsList[LEVEL_COMMERCIAL]);
        hbScanBigBuildingList.fun(&CWorld::ms_bigBuildingsList[LEVEL_SUBURBAN]);
    }
}

namespace CStreaming
{
    GameRef<bool> ms_hasLoadedLODs([]() -> bool*
    {
        auto pattern = find_pattern("C6 05 ? ? ? ? ? 83 0D ? ? ? ? ? 83 0D");
        if (!pattern.empty())
            return *pattern.get_first<bool*>(2);
        return nullptr;
    });

    SafetyHookInline shRequestBigBuildings = {};
    void __cdecl RequestBigBuildings(eLevelName level)
    {
        constexpr int BIGBUILDINGFLAGS = 9;

        auto ms_pBuildingPool = *CPools::pp_pBuildingPool;
        const int size = ms_pBuildingPool->m_size;

        if (size > 0)
        {
            for (int i = size - 1; i >= 0; --i)
            {
                CBuilding* b = (ms_pBuildingPool->m_flags[i] & 0x80) != 0 ? nullptr : &ms_pBuildingPool->m_entries[i];

                if (b && b->bIsBIGBuilding && (((b != *pIslandLODindustEntity) && (b != *pIslandLODcomIndEntity) && (b != *pIslandLODcomSubEntity) && (b != *pIslandLODsubIndEntity) && (b != *pIslandLODsubComEntity)) || (b->m_level == level)))
                    RequestModel(b->GetModelIndex(), BIGBUILDINGFLAGS);
            }
        }

        //RequestIslands(level);
        ms_hasLoadedLODs = false;
    }
}

void __fastcall cDMAudio__SetEffectsFadeVolStub(void*, void*, uint8_t)
{

}

void __cdecl CPad__StopPadsShakingStub()
{

}

void __cdecl CCollision__LoadCollisionScreenStub(eLevelName)
{

}

void __fastcall cDMAudio__ServiceStub(void*, void*)
{

}

void __cdecl CStreaming__RemoveIslandsNotUsedStub(eLevelName)
{

}

void __cdecl CStreaming__RemoveBigBuildingsStub(eLevelName)
{

}

void __cdecl CModelInfo__RemoveColModelsFromOtherLevelsStub(eLevelName levelName)
{

}

void __cdecl CFileLoader__LoadCollisionFromDatFileStub(unsigned int)
{

}

const char* __cdecl GetLevelSplashScreenStubStub(eLevelName)
{
    return "";
}

void __cdecl LoadSplashStub(const char*)
{

}

void __cdecl CStreaming__RemoveUnusedBigBuildingsStub(eLevelName)
{

}

void __cdecl CStreaming__RemoveUnusedBuildingsStub(eLevelName)
{

}

void __cdecl CStreaming__RequestBigBuildingsStub(eLevelName)
{

}

void __cdecl CStreaming__HaveAllBigBuildingsLoadedStub(eLevelName)
{

}

void __cdecl CStreaming__LoadAllRequestedModelsStub(bool)
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
            static bool gbNoIslandLoading = iniReader.ReadInteger("MAIN", "NoIslandLoading", 1) != 0;
            static bool gbDisableZoneCull = iniReader.ReadInteger("MAIN", "DisableZoneCull", 1) != 0;

            if (gbNoIslandLoading)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 59 50 E8 ? ? ? ? C6 05");
                GetLevelSplashScreen = (decltype(GetLevelSplashScreen))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? 0F B6 1D ? ? ? ? 59 8D 4C 24");
                LoadSplash = (decltype(LoadSplash))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 A3 ? ? ? ? E8");
                CFileLoader::LoadCollisionFromDatFile = (decltype(CFileLoader::LoadCollisionFromDatFile))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? A1 ? ? ? ? 59 EB ? 83 F8");
                DeleteIsland = (decltype(DeleteIsland))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = hook::pattern("05 ? ? ? ? 50 E8 ? ? ? ? 59 68");
                CWorld::ms_bigBuildingsList = *pattern.get_first<CPtrList*>(1);

                pattern = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 EB");
                auto arr = *pattern.get_first<CEntity**>(1);
                CStreaming::pIslandLODindustEntity = &arr[0];
                CStreaming::pIslandLODcomIndEntity = &arr[1];
                CStreaming::pIslandLODcomSubEntity = &arr[2];
                CStreaming::pIslandLODsubIndEntity = &arr[3];
                CStreaming::pIslandLODsubComEntity = &arr[4];

                //CCollision::LoadCollisionWhenINeedIt
                pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 15");
                injector::MakeCALL(pattern.get_first(0), cDMAudio__SetEffectsFadeVolStub);
                injector::MakeCALL(pattern.get_first(5), CPad__StopPadsShakingStub);
                injector::MakeCALL(pattern.get_first(16), CCollision__LoadCollisionScreenStub);
                injector::MakeCALL(pattern.get_first(27), cDMAudio__ServiceStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59");
                injector::MakeCALL(pattern.get_first(0), CStreaming__RemoveIslandsNotUsedStub);
                injector::MakeCALL(pattern.get_first(8), CStreaming__RemoveIslandsNotUsedStub);
                injector::MakeCALL(pattern.get_first(16), CStreaming__RemoveIslandsNotUsedStub);
                injector::MakeCALL(pattern.get_first(24), CStreaming__RemoveBigBuildingsStub);
                injector::MakeCALL(pattern.get_first(32), CStreaming__RemoveBigBuildingsStub);
                injector::MakeCALL(pattern.get_first(40), CStreaming__RemoveBigBuildingsStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 E8 ? ? ? ? 6A ? 6A ? E8 ? ? ? ? A1");
                injector::MakeCALL(pattern.get_first(0), CModelInfo__RemoveColModelsFromOtherLevelsStub);

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 A3 ? ? ? ? E8");
                injector::MakeCALL(pattern.get_first(0), CFileLoader__LoadCollisionFromDatFileStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 50 E8 ? ? ? ? 59 A1 ? ? ? ? 50");
                injector::MakeCALL(pattern.get_first(0), GetLevelSplashScreenStubStub);
                injector::MakeCALL(pattern.get_first(7), LoadSplashStub);

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 6A");
                injector::MakeCALL(pattern.get_first(), CStreaming__RemoveUnusedBigBuildingsStub);

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 6A ? E8 ? ? ? ? A1");
                injector::MakeCALL(pattern.get_first(), CStreaming__RemoveUnusedBuildingsStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A ? E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 6A ? 6A ? E8 ? ? ? ? 59 59 E8");
                injector::MakeCALL(pattern.get_first(), CStreaming__RequestBigBuildingsStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A ? 6A ? E8 ? ? ? ? 59 59 E8 ? ? ? ? B9");
                injector::MakeCALL(pattern.get_first(), CStreaming__HaveAllBigBuildingsLoadedStub);

                pattern = hook::pattern("B9 ? ? ? ? 6A ? E8 ? ? ? ? 83 C4 ? 5D 5F");
                injector::MakeCALL(pattern.get_first(7), cDMAudio__SetEffectsFadeVolStub);

                //CCollision::SortOutCollisionAfterLoad
                pattern = hook::pattern("E8 ? ? ? ? 8D 04 24 50 E8 ? ? ? ? 59 80 3D");
                CCollision::shSortOutCollisionAfterLoad = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CCollision::SortOutCollisionAfterLoad);

                //CRecordDataForChase::StartChaseScene
                pattern = hook::pattern("E8 ? ? ? ? 6A ? E8 ? ? ? ? 59 6A ? 6A");
                injector::MakeNOP(pattern.get_first(), 5);

                //COMMAND_LOAD_COLLISION
                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59 E8 ? ? ? ? 30 C0");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 E8 ? ? ? ? 30 C0 83 C4");
                injector::MakeNOP(pattern.get_first(), 5);

                //COMMAND_LOAD_COLLISION_WITH_SCREEN
                pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 35");
                injector::MakeCALL(pattern.get_first(), cDMAudio__SetEffectsFadeVolStub);

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 35");
                injector::MakeCALL(pattern.get_first(), CPad__StopPadsShakingStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 35");
                injector::MakeCALL(pattern.get_first(), CCollision__LoadCollisionScreenStub);

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 8B 35 ? ? ? ? 6A ? 56");
                injector::MakeCALL(pattern.get_first(), cDMAudio__ServiceStub);

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? A1");
                injector::MakeCALL(pattern.get_first(), CStreaming__RemoveUnusedBigBuildingsStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? A1");
                injector::MakeCALL(pattern.get_first(), CStreaming__RemoveUnusedBuildingsStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 59 B9");
                injector::MakeCALL(pattern.get_first(), CStreaming__RequestBigBuildingsStub);

                pattern = hook::pattern("E8 ? ? ? ? 59 B9 ? ? ? ? 6A ? E8 ? ? ? ? E8");
                injector::MakeCALL(pattern.get_first(), CStreaming__LoadAllRequestedModelsStub);

                pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 30 C0 81 C4 ? ? ? ? 5D 5F 5E 5B C2 ? ? 8B 44 24");
                injector::MakeCALL(pattern.get_first(), cDMAudio__SetEffectsFadeVolStub);

                //CStreaming::RequestBigBuildings
                pattern = hook::pattern("E8 ? ? ? ? 59 59 B9 ? ? ? ? 01 E9");
                CStreaming::RequestModel = (decltype(CStreaming::RequestModel))injector::GetBranchDestination(pattern.get_first()).as_int();
                pattern = hook::pattern("53 56 8B 35 ? ? ? ? 57 89 F0 55 8B 58 ? 8B 7C 24 ? 83 EB ? 72");
                CPools::pp_pBuildingPool = *pattern.get_first<CBuildingPool**>(4);
                CStreaming::shRequestBigBuildings = safetyhook::create_inline(pattern.get_first(), CStreaming::RequestBigBuildings);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A 01 6A 01 E8 ? ? ? ? 59 59 E8 ? ? ? ? 68");
                CStreaming::shRemoveIslandsNotUsed = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CStreaming::RemoveIslandsNotUsed);

                //CStreaming::RequestIslands
                pattern = hook::pattern("E8 ? ? ? ? C6 05 ? ? ? ? ? 59 5D 5F");
                injector::MakeRET(injector::GetBranchDestination(pattern.get_first()).as_int());

                //CStreaming::RemoveUnusedBigBuildings
                pattern = hook::pattern("E8 ? ? ? ? 59 83 FB ? 74 ? 6A ? E8 ? ? ? ? 59 83 FB ? 74 ? 6A ? E8 ? ? ? ? 59 53");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 83 FB ? 74 ? 6A ? E8 ? ? ? ? 59 53");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 53 E8 ? ? ? ? 59 5B C3");
                injector::MakeNOP(pattern.get_first(), 5);

                //CModelInfo::RemoveColModelsFromOtherLevels
                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 6A ? 6A ? A3");
                injector::MakeRET(injector::GetBranchDestination(pattern.get_first()).as_int());

                //CRenderer::ScanWorld
                pattern = hook::pattern("E8 ? ? ? ? 59 68 ? ? ? ? E8 ? ? ? ? 59 E9");
                CRenderer::hbScanBigBuildingList.fun = injector::MakeCALL(pattern.get_first(), CRenderer::ScanBigBuildingList).get();

                //RestoreForStartLoad
                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 59 6A ? E8 ? ? ? ? A1");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 59 6A ? E8 ? ? ? ? A1");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A ? E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 6A ? 6A ? E8 ? ? ? ? 59 59 53");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 6A ? 6A ? E8 ? ? ? ? 59 59 53");
                injector::MakeNOP(pattern.get_first(), 5);

                pattern = hook::pattern("E8 ? ? ? ? 59 6A ? 6A ? E8 ? ? ? ? 59 59 53");
                injector::MakeNOP(pattern.get_first(), 5);
            }

            if (gbDisableZoneCull)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 84 C0 59 74 ? 8A 83");
                CRenderer::shIsEntityCullZoneVisible = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CRenderer::IsEntityCullZoneVisible);

                pattern = hook::pattern("75 ? 55 E8 ? ? ? ? 83 F8 ? 59 75 ? A1");
                injector::MakeNOP(pattern.get_first(), 2);
            }
        };
    }
} Loading;