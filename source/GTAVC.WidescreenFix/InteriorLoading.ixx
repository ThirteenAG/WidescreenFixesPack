module;

#include <stdafx.h>
#include "common.h"

export module InteriorLoading;

import Skeleton;
import Entity;
import Loading;
import ModelInfo;
import Camera;
import CutsceneMgr;

std::array<CEntity*, 20> ms_multiInteriorList{};
int32_t** ppScriptParams = nullptr;
uint8_t** ppScriptSpace = nullptr;

constexpr auto scarinterior1 = 2954;
constexpr auto scarinterblood1 = 3021;
constexpr auto biker_bar_exterior = 1818;
constexpr auto lawcutmapa = 4789;
constexpr auto lawcutmapb = 4787;
constexpr auto lawcutmapc = 4788;
constexpr auto man_twr_stairs = 2524;
constexpr auto man_scrface_walls = 2482;
constexpr auto man_chandeliers = 2466;
constexpr auto man_hall_wall1 = 2470;
constexpr auto man_doorway2 = 2549;
constexpr auto bnk_ext_int = 4579;
constexpr auto bnk_maindr1 = 4557;
constexpr auto bnk_maindr2 = 4560;
constexpr auto cop_fake_ext01 = 3744;
constexpr auto cop_fake_ext02 = 3745;
constexpr auto cop_fake_ext03 = 3755;
constexpr auto cop_pillar_shad = 3756;
constexpr auto wshbuildws29 = 2837;
constexpr auto wshsthroad11 = 2969;
constexpr auto od_alleys1_01_dy = 3105;
constexpr auto od_acne = 3107;
constexpr auto wshsthroad02 = 2826;
constexpr auto wshsthroad12 = 2970;
constexpr auto man_hall_floor = 2467;
constexpr auto man_hall_pillars = 2468;
constexpr auto man_hall_ref = 2469;
constexpr auto bnk_main_rework = 4549;
constexpr auto pw_printworks = 1484;
constexpr auto hvoodext = 1718;

CEntity* (__cdecl* FindPlayerPed)() = nullptr;

CVector GetPositionToTrack()
{
    if (!FindPlayerPed || CCutsceneMgr::IsRunning())
    {
        return TheCamera->GetCoords();
    }
    else
    {
        return FindPlayerPed()->GetCoords();
    }
}

uint8_t GetModelRealInterior(int modelIndex, int interior)
{
    if (interior == AREA_BIKER_BAR)
    {
        if (modelIndex == scarinterior1 || modelIndex == scarinterblood1)
        {
            return AREA_BIKER_BAR;
        }

        if (modelIndex != biker_bar_exterior)
        {
            return AREA_MAIN_MAP;
        }
    }
    else if (interior == AREA_LAWYERS)
    {
        if (modelIndex != lawcutmapb && modelIndex != lawcutmapc && modelIndex != lawcutmapa)
        {
            return AREA_MAIN_MAP;
        }
    }
    else if (interior == AREA_MANSION)
    {
        if (modelIndex == man_twr_stairs || modelIndex == man_scrface_walls || (modelIndex >= man_chandeliers && modelIndex <= man_hall_wall1))
        {
            return AREA_MAIN_MAP;
        }
    }
    else if (interior == AREA_MAIN_MAP)
    {
        if (modelIndex == man_doorway2 || modelIndex == bnk_ext_int)
        {
            return 50;
        }
    }
    else if (interior == AREA_BANK)
    {
        if (modelIndex != bnk_maindr1 && modelIndex != bnk_maindr2)
        {
            return AREA_MAIN_MAP;
        }
    }
    else if (interior == AREA_POLICE_STATION)
    {
        if (modelIndex != cop_fake_ext01 && modelIndex != cop_fake_ext02 && modelIndex != cop_fake_ext03 && modelIndex != cop_pillar_shad)
        {
            return AREA_MAIN_MAP;
        }

        return 50;
    }
    else if (interior == AREA_PRINT_WORKS)
    {
        return AREA_MAIN_MAP;
    }
    else if (interior == AREA_CONCERT_HALL || interior == AREA_STUDIO)
    {
        return AREA_MAIN_MAP;
    }

    return interior;
}

int GetMultiInteriorSlot(int modelIndex)
{
    switch (modelIndex)
    {
        case man_twr_stairs:  return 0;
        case man_scrface_walls:  return 1;
        case wshbuildws29:  return 2;
        case wshsthroad11:  return 3;
        case od_alleys1_01_dy:  return 4;
        case od_acne:  return 5;
        case wshsthroad02:  return 6;
        case wshsthroad12:  return 7;
        case man_chandeliers:  return 8;
        case man_hall_floor:  return 9;
        case man_hall_pillars:  return 10;
        case man_hall_ref:  return 11;
        case man_hall_wall1:  return 12;

        default:
            return -1;
    }
}

SafetyHookInline shHasPreRenderEffects = {};
bool __fastcall HasPreRenderEffects(CEntity* entity, void* edx)
{
    int slot = GetMultiInteriorSlot(entity->m_modelIndex);
    if (slot != -1)
        ms_multiInteriorList[slot] = entity;
    return shHasPreRenderEffects.unsafe_fastcall<bool>(entity, edx);
}

SafetyHookInline shLoadCol = {};
bool __cdecl LoadCol(int32_t slot, uint8_t* buffer, int32_t bufsize)
{
    auto ret = shLoadCol.unsafe_ccall<bool>(slot, buffer, bufsize);

    auto ms_modelInfoPtrs = *CModelInfo::pp_modelInfoPtrs;
    auto bnk_main_rework_info = (CSimpleModelInfo*)ms_modelInfoPtrs[bnk_main_rework];

    if (bnk_main_rework_info && bnk_main_rework_info->m_colModel)
    {
        CColModel* model = bnk_main_rework_info->m_colModel;

        if (model->numTriangles > 104)
        {
            model->triangles[103].a = 0;
            model->triangles[103].b = 0;
            model->triangles[103].c = 0;
            model->triangles[104].a = 0;
            model->triangles[104].b = 0;
            model->triangles[104].c = 0;
        }
    }

    auto pw_printworks_info = (CSimpleModelInfo*)ms_modelInfoPtrs[bnk_main_rework];

    if (pw_printworks_info && pw_printworks_info->m_colModel)
    {
        CColModel* model = pw_printworks_info->m_colModel;

        if (model->numTriangles > 470)
        {
            model->triangles[427].a = 0;
            model->triangles[427].b = 0;
            model->triangles[427].c = 0;
            model->triangles[428].a = 0;
            model->triangles[428].b = 0;
            model->triangles[428].c = 0;
        }
    }

    auto hvoodext_info = (CSimpleModelInfo*)ms_modelInfoPtrs[hvoodext];

    if (hvoodext_info && hvoodext_info->m_colModel)
    {
        CColModel* model = hvoodext_info->m_colModel;

        if (model->numBoxes > 13)
        {
            model->boxes[13].piece = 0;
            model->boxes[13].surface = 0;
        }
    }

    return ret;
}

bool CheckCoordInPolygon(CVector& pos, int nodes, CVector2D* corners)
{
    int j = nodes - 1;
    bool parity = false;

    for (int i = 0; i < nodes; i++)
    {
        if (((corners[i].y < pos.y && corners[j].y >= pos.y) || (corners[j].y < pos.y && corners[i].y >= pos.y)) && (corners[i].x <= pos.x || corners[j].x <= pos.x))
        {
            parity ^= (corners[i].x + (pos.y - corners[i].y) / (corners[j].y - corners[i].y) * (corners[j].x - corners[i].x) < pos.x);
        }

        j = i;
    }

    return parity;
}

void ProcessMultiInteriorEntities()
{
    for (int i = 0; i < (int)ms_multiInteriorList.size(); i++)
    {
        CEntity* entity = ms_multiInteriorList[i];

        if (entity)
        {
            const CVector localPosition = GetPositionToTrack();

            if (entity->m_modelIndex == 2524 || entity->m_modelIndex == 2482 || (entity->m_modelIndex >= 2466 && entity->m_modelIndex <= 2470))
            {
                if (CGame::currArea == 2)
                    entity->m_area = 2;
                else
                    entity->m_area = 0;
            }
            else if (entity->m_modelIndex == 2837 || entity->m_modelIndex == 2969 || entity->m_modelIndex == 3105 || entity->m_modelIndex == 3107 ||
                entity->m_modelIndex == 2826 || entity->m_modelIndex == 2970)
            {

                if (CGame::currArea == 5)
                    entity->m_area = 5;
                else
                    entity->m_area = 0;
            }
        }
    }
}

export namespace CTimeCycle
{
    void (__cdecl* StartExtraColour)(int32_t c, bool fade) = nullptr;
    void (__cdecl* StopExtraColour)(bool fade) = nullptr;
}

export namespace CStreaming
{
    void (__cdecl* RemoveBuildingsNotInArea)(int32_t area) = nullptr;
    void (__cdecl* LoadScene)(const CVector* pos) = nullptr;
}

void ProcessInteriorSwitching()
{
    static CVector2D polyMall[] = {
        CVector2D(408.0f, 1215.8f), CVector2D(420.0f, 1215.8f), CVector2D(420.0f, 1240.0f), CVector2D(437.1f, 1256.8f), CVector2D(461.5f, 1256.8f),
        CVector2D(478.8f, 1239.4f), CVector2D(478.8f, 1214.8f), CVector2D(461.8f, 1197.6f), CVector2D(461.4f, 1153.9f), CVector2D(478.0f, 1135.8f),
        CVector2D(478.0f, 1112.5f), CVector2D(458.0f, 1092.5f), CVector2D(458.0f, 1051.0f), CVector2D(479.6f, 1037.2f), CVector2D(478.9f, 1012.9f),
        CVector2D(462.4f, 996.0f), CVector2D(437.8f, 996.0f), CVector2D(420.3f, 1013.0f), CVector2D(420.3f, 1036.2f), CVector2D(409.5f, 1036.2f),
        CVector2D(409.5f, 1012.0f), CVector2D(391.2f, 994.5f), CVector2D(367.2f, 994.5f),
        CVector2D(350.0f, 1012.0f), CVector2D(350.0f, 1036.0f),
        CVector2D(360.5f, 1046.0f), CVector2D(360.0f, 1094.0f), CVector2D(366.0f, 1094.7f),
        CVector2D(349.5f, 1111.6f), CVector2D(349.5f, 1136.0f), CVector2D(366.5f, 1153.9f), CVector2D(366.2f, 1258.2f),
        CVector2D(391.0f, 1256.6f), CVector2D(408.5f, 1239.8f)
    };

    static CVector2D polyMansionFirst[] = {
        CVector2D(-409.47f, -557.07f), CVector2D(-398.13f, -556.88f), CVector2D(-396.85f, -555.05f), CVector2D(-360.18f, -555.14f), CVector2D(-333.76f, -557.12f),
        CVector2D(-327.80f, -571.49f), CVector2D(-315.17f, -571.60f), CVector2D(-315.25f, -584.23f), CVector2D(-327.88f, -584.23f), CVector2D(-338.28f, -581.17f),
        CVector2D(-360.21f, -583.77f), CVector2D(-370.69f, -600.37f), CVector2D(-386.35f, -600.25f), CVector2D(-396.82f, -583.77f), CVector2D(-398.12f, -567.59f),
        CVector2D(-409.52f, -567.34f)
    };

    static CVector2D polyMansionTop[] = {
        CVector2D(-331.52f, -578.04f), CVector2D(-328.69f, -577.10f), CVector2D(-327.90f, -571.56f), CVector2D(-315.38f, -571.49f),
        CVector2D(-315.39f, -584.34f), CVector2D(-331.64f, -584.24f)
    };

    static CVector2D polyMansionBottom[] = {
        CVector2D(-351.81f, -580.84f), CVector2D(-327.94f, -584.18f), CVector2D(-315.37f, -584.05f),
        CVector2D(-315.13f, -571.73f), CVector2D(-328.79f, -567.29f), CVector2D(-338.24f, -567.45f), CVector2D(-349.77f, -576.88f)
    };

    static CVector2D polyApartment[] = {
        CVector2D(27.61f, -1332.87f), CVector2D(27.74f, -1329.19f), CVector2D(21.89f, -1328.30f),
        CVector2D(22.30f, -1323.10f), CVector2D(30.09f, -1323.59f), CVector2D(31.83f, -1327.59f), CVector2D(31.34f, -1332.91f),
    };

    static CVector2D polyShootingRange[] = {
        CVector2D(-644.87f, 1299.60f), CVector2D(-686.12f, 1299.51f), CVector2D(-685.93f, 1265.20f), CVector2D(-669.45f, 1263.25f), CVector2D(-658.48f, 1259.68f),
        CVector2D(-662.85f, 1243.74f), CVector2D(-666.36f, 1239.10f), CVector2D(-694.83f, 1237.39f), CVector2D(-695.37f, 1227.69f), CVector2D(-669.83f, 1227.21f),
        CVector2D(-669.76f, 1219.39f), CVector2D(-661.72f, 1219.45f), CVector2D(-657.26f, 1227.02f), CVector2D(-652.77f, 1239.45f),
        CVector2D(-651.63f, 1253.93f), CVector2D(-644.80f, 1253.99f), CVector2D(-644.83f, 1259.57f)
    };

    static CVector2D polyHotel[] = {
        CVector2D(235.58f, -1253.47f), CVector2D(228.84f, -1275.01f), CVector2D(227.50f, -1290.49f),
        CVector2D(224.49f, -1303.20f), CVector2D(198.33f, -1297.43f), CVector2D(208.88f, -1247.63f)
    };

    static CVector2D polyHotelTop[] = {
        CVector2D(236.48f, -1253.75f), CVector2D(225.57f, -1303.16f), CVector2D(198.33f, -1297.43f), CVector2D(208.88f, -1247.63f)
    };

    static CVector2D polyStripClub[] = {
        CVector2D(95.68f, -1465.15f), CVector2D(94.48f, -1458.55f), CVector2D(88.20f, -1447.76f), CVector2D(72.60f, -1439.76f), CVector2D(67.45f, -1441.58f),
        CVector2D(63.18f, -1443.86f), CVector2D(59.05f, -1447.66f), CVector2D(63.27f, -1453.86f), CVector2D(62.12f, -1453.25f), CVector2D(63.05f, -1454.85f),
        CVector2D(65.62f, -1459.29f), CVector2D(76.50f, -1470.99f), CVector2D(92.01f, -1466.28f)
    };

    static CVector2D polyMalibu[] = {
        CVector2D(493.42f, -74.04f), CVector2D(485.45f, -81.95f), CVector2D(479.05f, -83.45f), CVector2D(467.09f, -79.76f), CVector2D(462.72f, -75.42f),
        CVector2D(460.17f, -61.38f), CVector2D(457.98f, -61.43f), CVector2D(452.17f, -55.75f), CVector2D(457.94f, -49.90f), CVector2D(463.60f, -45.23f),
        CVector2D(473.97f, -49.67f), CVector2D(475.72f, -47.63f), CVector2D(486.90f, -51.46f), CVector2D(490.97f, -55.56f), CVector2D(492.96f, -62.70f),
        CVector2D(492.67f, -68.82f)
    };

    CVector pos = GetPositionToTrack();
    int selectedInterior = AREA_MAIN_MAP;

    if (345.0f < pos.x && pos.x < 490.0f && 990.0f < pos.y && pos.y < 1270.0f)
    {
        if (15.0f < pos.z && pos.z < 31.0f && CheckCoordInPolygon(pos, (sizeof(polyMall) / sizeof(polyMall[0])), polyMall))
        {
            selectedInterior = AREA_MALL;
        }
    }
    else if (-455.0f < pos.x && pos.x < -310.0f && -610.0f < pos.y && pos.y < -535.0f)
    {
        if (15.0f < pos.z && pos.z < 32.0f && CheckCoordInPolygon(pos, (sizeof(polyMansionFirst) / sizeof(polyMansionFirst[0])), polyMansionFirst))
        {
            selectedInterior = AREA_MANSION;
        }
        else if (32.0f < pos.z && pos.z < 40.0f && CheckCoordInPolygon(pos, (sizeof(polyMansionTop) / sizeof(polyMansionTop[0])), polyMansionTop))
        {
            selectedInterior = AREA_MANSION;
        }
        else if (10.0f < pos.z && pos.z < 15.0f && CheckCoordInPolygon(pos, (sizeof(polyMansionBottom) / sizeof(polyMansionBottom[0])), polyMansionBottom))
        {
            selectedInterior = AREA_MANSION;
        }
    }
    else if (20.0f < pos.x && pos.x < 35.0f && -1335.0f < pos.y && pos.y < -1320.0f)
    {
        if (12.0f < pos.z && pos.z < 15.0f && CheckCoordInPolygon(pos, (sizeof(polyApartment) / sizeof(polyApartment[0])), polyApartment))
        {
            selectedInterior = AREA_BIKER_BAR;
        }
    }
    else if (-700.0f < pos.x && pos.x < -640.0f && 1205.0f < pos.y && pos.y < 1305.0f)
    {
        if (10.0f < pos.z && pos.z < 19.0f && CheckCoordInPolygon(pos, (sizeof(polyShootingRange) / sizeof(polyShootingRange[0])), polyShootingRange))
        {
            selectedInterior = AREA_RIFLE_RANGE;
        }
    }
    else if (195.0f < pos.x && pos.x < 240.0f && -1305.0f < pos.y && pos.y < -1245.0f)
    {
        if (10.0f < pos.z && pos.z < 18.0f && CheckCoordInPolygon(pos, (sizeof(polyHotel) / sizeof(polyHotel[0])), polyHotel))
        {
            selectedInterior = AREA_HOTEL;
        }
        else if (18.0f < pos.z && pos.z < 28.0f && CheckCoordInPolygon(pos, (sizeof(polyHotelTop) / sizeof(polyHotelTop[0])), polyHotelTop))
        {
            selectedInterior = AREA_HOTEL;
        }
    }
    else if (55.0f < pos.x && pos.x < 100.0f && -1470.0f < pos.y && pos.y < -1435.0f)
    {
        if (9.0f < pos.z && pos.z < 14.0f && CheckCoordInPolygon(pos, (sizeof(polyStripClub) / sizeof(polyStripClub[0])), polyStripClub))
        {
            selectedInterior = AREA_STRIP_CLUB;
        }
    }
    else if (450.0f < pos.x && pos.x < 495.0f && -82.0f < pos.y && pos.y < -48.0f)
    {
        if (9.0f < pos.z && pos.z < 17.1f && CheckCoordInPolygon(pos, (sizeof(polyMalibu) / sizeof(polyMalibu[0])), polyMalibu))
        {
            selectedInterior = AREA_MALIBU_CLUB;
        }
    }
    else if (-1580.0f < pos.x && pos.x < -1210.0f && 1400.0f < pos.y && pos.y < 1600.0f && 290.0f < pos.z && pos.z < 340.0f)
    {
        selectedInterior = AREA_DIRT;
    }
    else if (-1580.0f < pos.x && pos.x < -1190.0f && 1120.0f < pos.y && pos.y < 1380.0f && 230.0f < pos.z && pos.z < 340.0f)
    {
        selectedInterior = AREA_OVALRING;
    }
    else if (-1580.0f < pos.x && pos.x < -1190.0f && 880.0f < pos.y && pos.y < 1120.0f && 230.0f < pos.z && pos.z < 340.0f)
    {
        selectedInterior = AREA_BLOOD;
    }

    if (CGame::currArea != selectedInterior)
    {
        CGame::currArea = (eAreaName)selectedInterior;
        ProcessMultiInteriorEntities();
        CStreaming::RemoveBuildingsNotInArea(CGame::currArea);
        CStreaming::LoadScene(&pos);
    }
}

class CRunningScript
{
    enum
    {
        MAX_STACK_DEPTH = 6,
        NUM_LOCAL_VARS = 16,
        NUM_TIMERS = 2
    };

    enum
    {
        ANDOR_NONE = 0,
        ANDS_1 = 1,
        ANDS_2,
        ANDS_3,
        ANDS_4,
        ANDS_5,
        ANDS_6,
        ANDS_7,
        ANDS_8,
        ORS_1 = 21,
        ORS_2,
        ORS_3,
        ORS_4,
        ORS_5,
        ORS_6,
        ORS_7,
        ORS_8
    };

public:
    CRunningScript* next;
    CRunningScript* prev;
    char m_abScriptName[8];
    uint32_t m_nIp;
    uint32_t m_anStack[MAX_STACK_DEPTH];
    uint16_t m_nStackPointer;
    int32_t m_anLocalVariables[NUM_LOCAL_VARS + NUM_TIMERS];
    bool m_bIsActive;
    bool m_bCondResult;
    bool m_bIsMissionScript;
    bool m_bSkipWakeTime;
    uint32_t m_nWakeTime;
    uint16_t m_nAndOrState;
    bool m_bNotFlag;
    bool m_bDeatharrestEnabled;
    bool m_bDeatharrestExecuted;
    bool m_bMissionFlag;
};

class InteriorLoading
{
public:
    InteriorLoading()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            gbNoInteriorLoading = iniReader.ReadInteger("MAIN", "NoInteriorLoading", 1) != 0;

            if (gbNoInteriorLoading)
            {
                auto pattern = find_pattern("8B 04 95 ? ? ? ? 83 C0 ? 50 8B 04 8D ? ? ? ? 83 C0 ? 50 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 5B");
                CModelInfo::pp_modelInfoPtrs = (CBaseModelInfo***)(pattern.get_first(3));

                pattern = find_pattern("E8 ? ? ? ? 59 59 30 C0 81 C4 ? ? ? ? 5D 5F 5E 5B C2 ? ? 8D 45 ? 89 E9 6A ? 50 E8 ? ? ? ? 83 3D ? ? ? ? ? 74 ? 6A");
                CTimeCycle::StartExtraColour = (decltype(CTimeCycle::StartExtraColour))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = find_pattern("E8 ? ? ? ? 59 30 C0 81 C4 ? ? ? ? 5D 5F 5E 5B C2 ? ? 8D 45 ? 89 E9 6A ? 50 E8 ? ? ? ? 8B 35");
                CTimeCycle::StopExtraColour = (decltype(CTimeCycle::StopExtraColour))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = find_pattern("E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? E8");
                CStreaming::RemoveBuildingsNotInArea = (decltype(CStreaming::RemoveBuildingsNotInArea))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = find_pattern("E8 ? ? ? ? A1 ? ? ? ? 59 50 68");
                CStreaming::LoadScene = (decltype(CStreaming::LoadScene))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = find_pattern("66 A1 ? ? ? ? 66 89 83");
                ppScriptParams = pattern.get_first<int32_t*>(2);

                pattern = find_pattern("B8 ? ? ? ? BF ? ? ? ? 01 E8");
                ppScriptSpace = pattern.get_first<uint8_t*>(1);

                pattern = find_pattern("E8 ? ? ? ? 0F B6 0D ? ? ? ? 8A 58");
                FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

                pattern = find_pattern("88 46 ? 8A 47");
                injector::MakeNOP(pattern.get_first(), 3, true);
                static auto LoadObjectInstanceHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    const uint32_t mi = *(uint32_t*)(regs.esp + 0x58);
                    const uint8_t al = static_cast<uint8_t>(regs.eax & 0xFF);
                    *reinterpret_cast<uint8_t*>(regs.esi + 0x5F) = GetModelRealInterior(mi, al);
                });

                pattern = find_pattern("88 43 ? E9 ? ? ? ? 89 C0");
                injector::MakeNOP(pattern.get_first(), 3, true);
                static auto LoadObjectInstanceHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    const uint32_t mi = *(uint32_t*)(regs.esp + 0x58);
                    const uint8_t al = static_cast<uint8_t>(regs.eax & 0xFF);
                    *reinterpret_cast<uint8_t*>(regs.ebx + 0x5F) = GetModelRealInterior(mi, al);
                });

                pattern = find_pattern("E8 ? ? ? ? 24 ? 8A 53");
                shHasPreRenderEffects = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), HasPreRenderEffects);

                pattern = find_pattern("E8 ? ? ? ? 83 C4 ? 84 C0 0F 85 ? ? ? ? 8D 85");
                shLoadCol = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), LoadCol);

                pattern = find_pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8");
                static auto CGameProcessHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    ProcessInteriorSwitching();

                    static bool bOnce = false;
                    if (!bOnce)
                    {
                        bOnce = true;
                        CVector pos = TheCamera->GetCoords();
                        ProcessMultiInteriorEntities();
                        CStreaming::RemoveBuildingsNotInArea(CGame::currArea);
                        CStreaming::LoadScene(&pos);
                    }
                });

                pattern = find_pattern("E8 ? ? ? ? C6 05 ? ? ? ? ? 59 EB");
                static auto AfterInitialiseWhenRestartingHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CVector pos = TheCamera->GetCoords();
                    ProcessMultiInteriorEntities();
                    CStreaming::RemoveBuildingsNotInArea(CGame::currArea);
                    CStreaming::LoadScene(&pos);
                });

                static auto loc_452309 = (uintptr_t)hook::pattern("30 C0 81 C4 88 00 00 00 5D 5F 5E 5B C2 04 00 8D 45 ? 89 E9 6A 01 50 E8 ? ? ? ? 66 A1 ? ? ? ? 50 E8 ? ? ? ? 59 30 C0 81 C4 88 00 00 00 5D 5F 5E 5B C2 04 00 8D 45").get_first();
                pattern = find_pattern("8B 1D ? ? ? ? 69 DB 70 01 00 00 81 C3 ? ? ? ? 83 3D");
                static auto COMMAND_SET_PLAYER_CONTROL = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CRunningScript* script = (CRunningScript*)regs.ebp;

                    if (iequals(script->m_abScriptName, "shit"))
                    {
                        auto ScriptParams = *ppScriptParams;
                        auto player = ScriptParams[0];
                        auto control = ScriptParams[1];

                        if (!control)
                        {
                            bSkipNextPlayerCoords = true;
                            bSkipNextPlayerHeading = true;
                            bSkipNextFade = true;
                            bSkipNextLoadScene = true;
                            return_to(loc_452309);
                        }
                        else
                        {
                            bSkipNextPlayerCoords = false;
                            bSkipNextPlayerHeading = false;
                            bSkipNextFade = false;
                            bSkipNextLoadScene = false;
                        }
                    }
                });

                static auto loc_44EDC4 = (uintptr_t)hook::pattern("30 C0 81 C4 ? ? ? ? 5D 5F 5E 5B C2 ? ? 8D 46 ? 89 F1 6A ? 50 E8 ? ? ? ? A1 ? ? ? ? 6B C0 ? 8B 04 C5 ? ? ? ? 80 B8 ? ? ? ? ? 74 ? 8B 88 ? ? ? ? 85 C9 74 ? 83 C1 ? FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35 ? ? ? ? FF 35 ? ? ? ? E8").get_first();
                pattern = find_pattern("D9 05 ? ? ? ? D9 5C 24 ? D9 05 ? ? ? ? D9 5C 24 ? D9 05 ? ? ? ? D9 5C 24 ? D9 44 24 ? D8 1D ? ? ? ? DF E0 8B 1D");
                static auto COMMAND_SET_PLAYER_COORDINATES = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (bSkipNextPlayerCoords)
                    {
                        return_to(loc_44EDC4);
                    }
                });

                static auto loc_4544B7 = (uintptr_t)hook::pattern("30 C0 81 C4 68 01 00 00 5F 5E 5B C2 04 00 8D 46 ? 89 F1 6A 01 50 E8 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 80 B8").get_first();
                pattern = find_pattern("8B 35 ? ? ? ? 69 F6 70 01 00 00 81 C6 ? ? ? ? 8B 06");
                static auto COMMAND_SET_PLAYER_HEADING = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (bSkipNextPlayerHeading)
                    {
                        return_to(loc_4544B7);
                    }
                });

                static auto loc_454135 = (uintptr_t)hook::pattern("30 C0 81 C4 68 01 00 00 5F 5E 5B C2 04 00 B9 ? ? ? ? E8 ? ? ? ? 84 C0").get_first();
                pattern = find_pattern("DB 05 ? ? ? ? 66 A1 ? ? ? ? B9 ? ? ? ? 50 50");
                static auto COMMAND_DO_FADE = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (bSkipNextFade)
                    {
                        return_to(loc_454135);
                    }
                });

                static auto loc_45AA37 = (uintptr_t)hook::pattern("30 C0 81 C4 08 02 00 00 5D 5F 5E 5B C2 04 00 8D 45 ? 89 E9 6A 03 50 E8 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? A1").get_first();
                pattern = find_pattern("D9 05 ? ? ? ? D9 5C 24 ? D9 05 ? ? ? ? D9 9C 24 ? ? ? ? D9 05 ? ? ? ? D9 9C 24 ? ? ? ? E8");
                static auto COMMAND_LOAD_SCENE = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (bSkipNextLoadScene)
                    {
                        return_to(loc_45AA37);
                    }
                });
            }
        };
    }
} InteriorLoading;