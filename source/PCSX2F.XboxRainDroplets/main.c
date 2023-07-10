#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"
#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/injector.h"
#include "../../includes/pcsx2/patterns.h"
#include "../../includes/pcsx2/mips.h"

int CompatibleCRCList[] = { 
    0x4F32A11F, 0xB3AD1EA4, // vcs
    0x7EA439F5, //0xD693D4CF, // lcs
};

char XboxRainDropletsData[255] = "XBOXRAINDROPLETSDATA";

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

enum eParticleVCS
{
    SPARK,
    SPARK_SMALL,
    WATER_SPARK,
    WHEEL_DIRT,
    SAND,
    WHEEL_WATER,
    BLOOD,
    BLOOD_SMALL,
    BLOOD_SPURT,
    DEBRIS,
    DEBRIS2,
    FLYERS,
    WATER,
    FLAME,
    FIREBALL,
    GUNFLASH,
    GUNFLASH_NOANIM,
    GUNSMOKE,
    GUNSMOKE2,
    CIGARETTE_SMOKE,
    SMOKE,
    SMOKE_SLOWMOTION,
    DRY_ICE,
    TEARGAS,
    GARAGEPAINT_SPRAY,
    SHARD,
    SPLASH,
    CARFLAME,
    STEAM,
    STEAM2,
    STEAM_NY,
    STEAM_NY_SLOWMOTION,
    GROUND_STEAM,
    ENGINE_STEAM,
    RAINDROP,
    RAINDROP_SMALL,
    RAIN_SPLASH,
    RAIN_SPLASH_BIGGROW,
    RAIN_SPLASHUP,
    WATERSPRAY,
    EXPLOSION_MEDIUM,
    EXPLOSION_LARGE,
    EXPLOSION_MFAST,
    EXPLOSION_LFAST,
    CAR_SPLASH,
    BOAT_SPLASH,
    BOAT_THRUSTJET,
    WATER_HYDRANT,
    WATER_CANNON,
    EXTINGUISH_STEAM,
    PED_SPLASH,
    PEDFOOT_DUST,
    CAR_DUST,
    HELI_DUST,
    HELI_ATTACK,
    ENGINE_SMOKE,
    ENGINE_SMOKE2,
    CARFLAME_SMOKE,
    FIREBALL_SMOKE,
    PAINT_SMOKE,
    TREE_LEAVES,
    CARCOLLISION_DUST,
    CAR_DEBRIS,
    BIRD_DEBRIS,
    HELI_DEBRIS,
    EXHAUST_FUMES,
    RUBBER_SMOKE,
    BURNINGRUBBER_SMOKE,
    BULLETHIT_SMOKE,
    GUNSHELL_FIRST,
    GUNSHELL,
    GUNSHELL_BUMP1,
    GUNSHELL_BUMP2,
    ROCKET_SMOKE,
    THROWN_FLAME,
    SWIM_SPLASH,
    SWIM_WAKE,
    SWIM_WAKE2,
    HELI_WATER_DROP,
    BALLOON_EXP,
    AUDIENCE_FLASH,
    TEST,
    BIRD_FRONT,
    SHIP_SIDE,
    BEASTIE,
    RAINDROP_2D,
    FERRY_CHIM_SMOKE,
    MULTIPLAYER_HIT,
    HYDRANT_STEAM,
    FLOOR_HIT
};

enum eParticleLCS
{
    LCS_SPARK,
    LCS_SPARK_SMALL,
    LCS_WATER_SPARK,
    LCS_WHEEL_DIRT,
    LCS_SAND,
    LCS_WHEEL_WATER,
    LCS_BLOOD,
    LCS_BLOOD_SMALL,
    LCS_BLOOD_SPURT,
    LCS_DEBRIS,
    LCS_DEBRIS2,
    LCS_FLYERS,
    LCS_WATER,
    LCS_FLAME,
    LCS_FIREBALL,
    LCS_GUNFLASH,
    LCS_GUNFLASH_NOANIM,
    LCS_GUNSMOKE,
    LCS_GUNSMOKE2,
    LCS_CIGARETTE_SMOKE,
    LCS_SMOKE,
    LCS_SMOKE_SLOWMOTION,
    LCS_DRY_ICE,
    LCS_TEARGAS,
    LCS_GARAGEPAINT_SPRAY,
    LCS_SHARD,
    LCS_SPLASH,
    LCS_CARFLAME,
    LCS_STEAM,
    LCS_STEAM2,
    LCS_STEAM_NY,
    LCS_STEAM_NY_SLOWMOTION,
    LCS_GROUND_STEAM,
    LCS_ENGINE_STEAM,
    LCS_RAINDROP,
    LCS_RAINDROP_SMALL,
    LCS_RAIN_SPLASH,
    LCS_RAIN_SPLASH_BIGGROW,
    LCS_RAIN_SPLASHUP,
    LCS_WATERSPRAY,
    LCS_EXPLOSION_MEDIUM,
    LCS_EXPLOSION_LARGE,
    LCS_EXPLOSION_MFAST,
    LCS_EXPLOSION_LFAST,
    LCS_CAR_SPLASH,
    LCS_BOAT_SPLASH,
    LCS_BOAT_THRUSTJET,
    LCS_WATER_HYDRANT,
    LCS_WATER_CANNON,
    LCS_EXTINGUISH_STEAM,
    LCS_PED_SPLASH,
    LCS_PEDFOOT_DUST,
    LCS_CAR_DUST,
    LCS_HELI_DUST,
    LCS_HELI_ATTACK,
    LCS_ENGINE_SMOKE,
    LCS_ENGINE_SMOKE2,
    LCS_CARFLAME_SMOKE,
    LCS_FIREBALL_SMOKE,
    LCS_PAINT_SMOKE,
    LCS_TREE_LEAVES,
    LCS_CARCOLLISION_DUST,
    LCS_CAR_DEBRIS,
    LCS_BIRD_DEBRIS,
    LCS_HELI_DEBRIS,
    LCS_EXHAUST_FUMES,
    LCS_RUBBER_SMOKE,
    LCS_BURNINGRUBBER_SMOKE,
    LCS_BULLETHIT_SMOKE,
    LCS_GUNSHELL_FIRST,
    LCS_GUNSHELL,
    LCS_GUNSHELL_BUMP1,
    LCS_GUNSHELL_BUMP2,
    LCS_ROCKET_SMOKE,
    LCS_TEST,
    LCS_BIRD_FRONT,
    LCS_SHIP_SIDE,
    LCS_BEASTIE,
    LCS_RAINDROP_2D,
    LCS_FERRY_CHIM_SMOKE,
    LCS_MULTIPLAYER_HIT,
    LCS_HYDRANT_STEAM,
};

typedef struct RwV3d
{
    float x;
    float y;
    float z;
} RwV3d;

typedef struct RwMatrix
{
    RwV3d    right;
    uint32_t flags;
    RwV3d    up;
    uint32_t pad1;
    RwV3d    at;
    uint32_t pad2;
    RwV3d    pos;
    uint32_t pad3;
} RwMatrix;

#pragma pack(push, 1)
struct XRData {
    uint32_t p_enabled;
    uint32_t ms_enabled;

    float ms_rainIntensity;
    uint32_t p_rainIntensity;

    float ms_right_x;
    float ms_right_y;
    float ms_right_z;
    float ms_up_x;
    float ms_up_y;
    float ms_up_z;
    float ms_at_x;
    float ms_at_y;
    float ms_at_z;
    float ms_pos_x;
    float ms_pos_y;
    float ms_pos_z;

    uint32_t p_right_x;
    uint32_t p_right_y;
    uint32_t p_right_z;
    uint32_t p_up_x;
    uint32_t p_up_y;
    uint32_t p_up_z;
    uint32_t p_at_x;
    uint32_t p_at_y;
    uint32_t p_at_z;
    uint32_t p_pos_x;
    uint32_t p_pos_y;
    uint32_t p_pos_z;

    float RegisterSplash_Vec_x;
    float RegisterSplash_Vec_y;
    float RegisterSplash_Vec_z;
    float RegisterSplash_distance;
    int32_t RegisterSplash_duration;
    float RegisterSplash_removaldistance;

    int32_t FillScreen_amount;

    float FillScreenMoving_Vec_x;
    float FillScreenMoving_Vec_y;
    float FillScreenMoving_Vec_z;
    float FillScreenMoving_amount;
    int32_t FillScreenMoving_isBlood;

#ifdef __cplusplus
    bool Enabled(uint64_t ptr)
    {
        if (p_enabled)
            return *(uint32_t*)(ptr + p_enabled);
        else
            return ms_enabled != 0;
    }

    float GetRainIntensity(uint64_t ptr)
    {
        if (p_rainIntensity)
            return *(float*)(ptr + p_rainIntensity);
        else
            return ms_rainIntensity != 0;
    }

    RwV3d GetRight(uint64_t ptr)
    {
        return RwV3d((p_right_x ? *(float*)(ptr + p_right_x) : ms_right_x),
            (p_right_y ? *(float*)(ptr + p_right_y) : ms_right_y),
            (p_right_z ? *(float*)(ptr + p_right_z) : ms_right_z));
    }

    RwV3d GetUp(uint64_t ptr)
    {
        return RwV3d((p_up_x ? *(float*)(ptr + p_up_x) : ms_up_x),
            (p_up_y ? *(float*)(ptr + p_up_y) : ms_up_y),
            (p_up_z ? *(float*)(ptr + p_up_z) : ms_up_z));
    }

    RwV3d GetAt(uint64_t ptr)
    {
        return RwV3d((p_at_x ? *(float*)(ptr + p_at_x) : ms_at_x),
            (p_at_y ? *(float*)(ptr + p_at_y) : ms_at_y),
            (p_at_z ? *(float*)(ptr + p_at_z) : ms_at_z));
    }

    RwV3d GetPos(uint64_t ptr)
    {
        return RwV3d((p_pos_x ? *(float*)(ptr + p_pos_x) : ms_pos_x),
            (p_pos_y ? *(float*)(ptr + p_pos_y) : ms_pos_y),
            (p_pos_z ? *(float*)(ptr + p_pos_z) : ms_pos_z));
    }
#endif
};
#pragma pack(pop)

uint32_t RslCameraGetNode(uint32_t a1)
{
    return *(uint32_t*)(a1 + 4);
}

uint32_t sub_8A1A5D4(uint32_t a1)
{
    return a1 + 16;
}

intptr_t TheCamera = 0;
intptr_t dword_729480 = 0;
intptr_t dword_486DE0 = 0;
intptr_t dword_4CD144 = 0;
intptr_t dword_4CD680 = 0;
intptr_t dword_4CD684 = 0;
intptr_t dword_48F820 = 0;
intptr_t dword_489F7C = 0;
void GameLoopStuffVCS()
{
    if (XboxRainDropletsData[0] != 'X')
    {
        struct XRData* data = (struct XRData*)XboxRainDropletsData;
        uint8_t gMenuActivated = *(uint8_t*)(dword_729480 + 0x40);
        uint32_t splashActive = *(uint8_t*)(dword_486DE0);
        data->ms_enabled = (gMenuActivated == 0) && !splashActive;

        float CWeather_Rain = *(float*)(dword_4CD144);
        uint8_t CCullZones_CamNoRain = (*(uint32_t*)(dword_4CD680) & 8) != 0;
        uint8_t CCullZones_PlayerNoRain = (*(uint32_t*)(dword_4CD684) & 8) != 0;
        uint8_t CCutsceneMgr__ms_running = *(uint8_t*)((*(uint32_t*)(dword_48F820) + 0x20));
        uint32_t CGame__currArea = *(uint32_t*)(dword_489F7C);
        
        if (CGame__currArea != 0 || CCullZones_CamNoRain || CCullZones_PlayerNoRain || CCutsceneMgr__ms_running)
            data->ms_rainIntensity = 0.0f;
        else
            data->ms_rainIntensity = CWeather_Rain;
        
        uint32_t RslCamera = *(uint32_t*)(TheCamera + 0x7F4);
        if (RslCamera)
        {
            uint32_t Node = RslCameraGetNode(RslCamera);
            struct RwMatrix* pCamMatrix = (struct RwMatrix*)(sub_8A1A5D4(Node));
        
            data->ms_right_x = pCamMatrix->right.x;
            data->ms_right_y = pCamMatrix->right.y;
            data->ms_right_z = pCamMatrix->right.z;
            data->ms_up_x = pCamMatrix->up.x;
            data->ms_up_y = pCamMatrix->up.y;
            data->ms_up_z = pCamMatrix->up.z;
            data->ms_at_x = pCamMatrix->at.x;
            data->ms_at_y = pCamMatrix->at.y;
            data->ms_at_z = pCamMatrix->at.z;
            data->ms_pos_x = pCamMatrix->pos.x;
            data->ms_pos_y = pCamMatrix->pos.y;
            data->ms_pos_z = pCamMatrix->pos.z;
        }
    }
}

void CParticle__AddParticleHookVCS(uint32_t type, uint32_t vecPos)
{
    struct XRData* data = (struct XRData*)XboxRainDropletsData;
    RwV3d* point = (RwV3d*)vecPos;
    
    if (type == WATER_SPARK || type == WHEEL_WATER || type == WATER || type == WATER_HYDRANT ||
        type == WATERSPRAY || type == BOAT_THRUSTJET || type == WATER_CANNON ||
        type == BLOOD || type == BLOOD_SMALL || type == BLOOD_SPURT)
    {
        data->FillScreenMoving_Vec_x = point->x;
        data->FillScreenMoving_Vec_y = point->y;
        data->FillScreenMoving_Vec_z = point->z;
    
        data->FillScreenMoving_amount = 1.0f;
        if (type == BLOOD || type == BLOOD_SMALL || type == BLOOD_SPURT)
            data->FillScreenMoving_isBlood = 1;
        else
            data->FillScreenMoving_isBlood = 0;
    }
    else if (type == SPLASH || type == CAR_SPLASH || type == BOAT_SPLASH ||
        type == PED_SPLASH || type == SWIM_SPLASH || type == HELI_WATER_DROP)
    {
        data->RegisterSplash_Vec_x = point->x;
        data->RegisterSplash_Vec_y = point->y;
        data->RegisterSplash_Vec_z = point->z;
        data->RegisterSplash_distance = 10.0f;
        data->RegisterSplash_duration = 300;
        data->RegisterSplash_removaldistance = 50.0f;
    }
}

void CParticle__AddParticleHookLCS(uint32_t type, uint32_t vecPos)
{
    struct XRData* data = (struct XRData*)XboxRainDropletsData;
    RwV3d* point = (RwV3d*)vecPos;

    if (type == LCS_WATER_SPARK || type == LCS_WHEEL_WATER || type == LCS_WATER || type == LCS_WATER_HYDRANT ||
        type == LCS_WATERSPRAY || type == LCS_BOAT_THRUSTJET || type == LCS_WATER_CANNON ||
        type == LCS_BLOOD || type == LCS_BLOOD_SMALL || type == LCS_BLOOD_SPURT)
    {
        data->FillScreenMoving_Vec_x = point->x;
        data->FillScreenMoving_Vec_y = point->y;
        data->FillScreenMoving_Vec_z = point->z;

        data->FillScreenMoving_amount = 1.0f;
        if (type == LCS_BLOOD || type == LCS_BLOOD_SMALL || type == LCS_BLOOD_SPURT)
            data->FillScreenMoving_isBlood = 1;
        else
            data->FillScreenMoving_isBlood = 0;
    }
    else if (type == LCS_SPLASH || type == LCS_CAR_SPLASH || type == LCS_BOAT_SPLASH || type == LCS_PED_SPLASH)
    {
        data->RegisterSplash_Vec_x = point->x;
        data->RegisterSplash_Vec_y = point->y;
        data->RegisterSplash_Vec_z = point->z;
        data->RegisterSplash_distance = 10.0f;
        data->RegisterSplash_duration = 300;
        data->RegisterSplash_removaldistance = 50.0f;
    }
}

void GameLoopStuffLCS()
{
    if (XboxRainDropletsData[0] != 'X')
    {
        struct XRData* data = (struct XRData*)XboxRainDropletsData;
        uint8_t gMenuActivated = *(uint8_t*)(0x63474D);
        data->ms_enabled = (gMenuActivated == 0);

        float CWeather_Rain = *(float*)(0x3D9B94);
        uint8_t CCullZones_CamNoRain = (*(uint32_t*)(0x3DA200) & 8) != 0;
        uint8_t CCullZones_PlayerNoRain = (*(uint32_t*)(0x3DA1FC) & 8) != 0;
        uint8_t CCutsceneMgr__ms_running = *(uint32_t*)(0x3D5AC0);
        uint32_t CGame__currArea = *(uint32_t*)(0x3D8430);

        if (CGame__currArea != 0 || CCullZones_CamNoRain || CCullZones_PlayerNoRain || CCutsceneMgr__ms_running)
            data->ms_rainIntensity = 0.0f;
        else
            data->ms_rainIntensity = CWeather_Rain;

        uint32_t RslCamera = *(uint32_t*)(TheCamera + 0xBA0);
        if (RslCamera)
        {
            uint32_t Node = RslCameraGetNode(RslCamera);
            struct RwMatrix* pCamMatrix = (struct RwMatrix*)(sub_8A1A5D4(Node));

            data->ms_right_x = pCamMatrix->right.x;
            data->ms_right_y = pCamMatrix->right.y;
            data->ms_right_z = pCamMatrix->right.z;
            data->ms_up_x = pCamMatrix->up.x;
            data->ms_up_y = pCamMatrix->up.y;
            data->ms_up_z = pCamMatrix->up.z;
            data->ms_at_x = pCamMatrix->at.x;
            data->ms_at_y = pCamMatrix->at.y;
            data->ms_at_z = pCamMatrix->at.z;
            data->ms_pos_x = pCamMatrix->pos.x;
            data->ms_pos_y = pCamMatrix->pos.y;
            data->ms_pos_z = pCamMatrix->pos.z;
        }
    }
}

void init()
{
    //vcs
    uintptr_t ptr_21E994 = pattern.get(0, "52 00 02 3C ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 48 00 10 3C", -8);
    if (ptr_21E994)
    {
        injector.MakeJAL(ptr_21E994, (intptr_t)GameLoopStuffVCS);

        uintptr_t ptr_471414 = pattern.get(0, "73 00 10 3C ? ? ? ? 10 00 BF FF", 0);
        dword_729480 = GetAbsoluteAddress(ptr_471414, 0, 16);
        uintptr_t ptr_21D0BC = pattern.get(0, "48 00 02 3C ? ? ? ? ? ? ? ? 48 00 13 3C ? ? ? ? 2D B0 40 00", -0);
        dword_486DE0 = GetAbsoluteAddress(ptr_21D0BC, 0, 8);
        uintptr_t ptr_2B36C0 = pattern.get(0, "4D 00 02 3C CC 3D 01 3C CD CC 21 34 00 08 81 44 ? ? ? ? ? ? ? ? 00 00 B0 FF 2D 30 80 00", -0);
        dword_4CD144 = GetAbsoluteAddress(ptr_2B36C0, 0, 16);
        uintptr_t ptr_40CEE0 = pattern.get(0, "4D 00 04 3C ? ? ? ? 2D 88 A0 00", -0);
        dword_4CD680 = GetAbsoluteAddress(ptr_40CEE0, 0, 16);
        uintptr_t ptr_40CF80 = pattern.get(0, "4D 00 03 3C ? ? ? ? D0 44 42 26", -0);
        dword_4CD684 = GetAbsoluteAddress(ptr_40CF80, 0, 4);
        uintptr_t ptr_1DF68C = pattern.get(0, "49 00 10 3C ? ? ? ? ? ? ? ? 1C 00 43 8C ? ? ? ? 00 00 00 00 ? ? ? ? 1C 00 43 8C ? ? ? ? D0 01 B0 DF", -0);
        dword_48F820 = GetAbsoluteAddress(ptr_1DF68C, 0, 4);
        uintptr_t ptr_113FB8 = pattern.get(0, "49 00 02 3C AA 02 64 94", -0);
        dword_489F7C = GetAbsoluteAddress(ptr_113FB8, 0, 8);
        uintptr_t ptr_1091D4 = pattern.get(0, "6F 00 02 3C 00 00 B0 FF 08 00 B1 FF ? ? ? ? 2D 88 80 00", -0);
        TheCamera = GetAbsoluteAddress(ptr_1091D4, 0, 12);
        
        uintptr_t ptr_1E90C4 = pattern.get(0, "38 01 B5 E7 30 01 B4 E7 ? ? ? ? 50 01 A5 8F", -0);
        MakeInlineWrapper(ptr_1E90C4,
            jal((intptr_t)CParticle__AddParticleHookVCS),
            nop(),
            swc1(f21, sp, 0x138)
        );
    }

    //lcs
    uintptr_t ptr_1F5BEC = pattern.get(0, "2D 20 40 00 63 00 04 3C", -4);
    if (ptr_1F5BEC)
    {
        injector.MakeJAL(ptr_1F5BEC, (intptr_t)GameLoopStuffLCS);

        uintptr_t ptr_101E04 = pattern.get(0, "44 00 02 3C 10 00 B0 FF ? ? ? ? 18 00 B1 FF", -0);
        TheCamera = GetAbsoluteAddress(ptr_101E04, 0, 8);
    
        uintptr_t ptr_1CD600 = pattern.get(0, "28 01 B5 E7 20 01 B4 E7 ? ? ? ? 40 01 A4 8F", -0);
        MakeInlineWrapper(ptr_1CD600,
            jal((intptr_t)CParticle__AddParticleHookLCS),
            nop(),
            swc1(f21, sp, 0x128)
        );
    }
}

int main()
{
    return 0;
}
