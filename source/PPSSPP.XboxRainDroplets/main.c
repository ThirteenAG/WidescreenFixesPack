#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <systemctrl.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "PPSSPP.XboxRainDroplets"
#define LOG_PATH "ms0:/PSP/PLUGINS/PPSSPP.XboxRainDroplets/PPSSPP.XboxRainDroplets.log"
#define INI_PATH "ms0:/PSP/PLUGINS/PPSSPP.XboxRainDroplets/PPSSPP.XboxRainDroplets.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
#endif

#ifndef __INTELLISENSE__
#define align16 __attribute__((aligned(16)))
#else
#define align16
#endif

char align16 sync_buf[255] = "XBOXRAINDROPLETSDATA";

enum eParticle
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

void CParticle__AddParticleHook(uint32_t type, uint32_t vecPos)
{
    struct XRData* data = (struct XRData*)sync_buf;
    RwV3d* point = (RwV3d*)vecPos;

    if (type == WATER_SPARK || type == WHEEL_WATER || type == WATER || type == WATER_HYDRANT ||
        type == WATERSPRAY || type == BOAT_THRUSTJET || type == WATER_CANNON ||
        type == BLOOD || type == BLOOD_SMALL || type == BLOOD_SPURT)
    {
        data->FillScreenMoving_Vec_x = point->x;
        data->FillScreenMoving_Vec_y = point->y;
        data->FillScreenMoving_Vec_z = point->z;

        data->FillScreenMoving_amount = 1;
        if (type == BLOOD || type == BLOOD_SMALL || type == BLOOD_SPURT)
            data->FillScreenMoving_isBlood = 1;
        else
            data->FillScreenMoving_isBlood = 0;
    }
    else if ( type == SPLASH || type == CAR_SPLASH || type == BOAT_SPLASH ||
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

intptr_t dword_08BC9100 = 0;
intptr_t dword_08BB3C38 = 0;
intptr_t dword_08BB456C = 0;
intptr_t dword_08BB4570 = 0;
intptr_t dword_08BB345C = 0;
intptr_t dword_08BB194C = 0;
intptr_t TheCamera = 0;
void GameLoopStuff()
{
    if (sync_buf[0] != 'X')
    {
        struct XRData* data = (struct XRData*)sync_buf;
        uint8_t gMenuActivated = *(uint8_t*)(dword_08BC9100 + 0x20);
        data->ms_enabled = gMenuActivated == 0;

        float CWeather_Rain = *(float*)(injector.GetGP() + dword_08BB3C38);
        uint8_t CCullZones_CamNoRain = (*(uint32_t*)(injector.GetGP() + dword_08BB456C) & 8) != 0;
        uint8_t CCullZones_PlayerNoRain = (*(uint32_t*)(injector.GetGP() + dword_08BB4570) & 8) != 0;
        uint8_t CCutsceneMgr__ms_running = *(uint8_t*)((*(uint32_t*)(injector.GetGP() + dword_08BB345C) + 0x13));
        uint32_t CGame__currArea = *(uint32_t*)(injector.GetGP() + dword_08BB194C);

        if (CGame__currArea != 0 || CCullZones_CamNoRain || CCullZones_PlayerNoRain || CCutsceneMgr__ms_running)
            data->ms_rainIntensity = 0.0f;
        else
            data->ms_rainIntensity = CWeather_Rain;

        uint32_t RslCamera = *(uint32_t*)(TheCamera + 0x7BC);
        if (RslCamera)
        {
            uint32_t Node = RslCameraGetNode(RslCamera);
            struct RwMatrix* pCamMatrix = (struct RwMatrix*)(sub_8A1A5D4(Node));

            data->p_right_x = (uint32_t)&pCamMatrix->right.x;
            data->p_right_y = (uint32_t)&pCamMatrix->right.y;
            data->p_right_z = (uint32_t)&pCamMatrix->right.z;
            data->p_up_x = (uint32_t)&pCamMatrix->up.x;
            data->p_up_y = (uint32_t)&pCamMatrix->up.y;
            data->p_up_z = (uint32_t)&pCamMatrix->up.z;
            data->p_at_x = (uint32_t)&pCamMatrix->at.x;
            data->p_at_y = (uint32_t)&pCamMatrix->at.y;
            data->p_at_z = (uint32_t)&pCamMatrix->at.z;
            data->p_pos_x = (uint32_t)&pCamMatrix->pos.x;
            data->p_pos_y = (uint32_t)&pCamMatrix->pos.y;
            data->p_pos_z = (uint32_t)&pCamMatrix->pos.z;
        }
    }
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int OnModuleStart() {
    //vcs
    uintptr_t ptr_8937A50 = pattern.get(0, "1C 00 BF AF ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 01 00 15 34", 4);
    if (ptr_8937A50)
    {
        injector.MakeJAL(ptr_8937A50, (intptr_t)GameLoopStuff);

        uintptr_t ptr_6710 = pattern.get(0, "00 00 B0 AF 08 00 BF AF ? ? ? ? 25 80 80 00 01 00 04 34 ? ? ? ? ? ? ? ? 25 20 20 02 ? ? ? ? ? ? ? ? ? ? ? ? 25 20 20 02 ? ? ? ? 00 00 00 00", -8);
        dword_08BC9100 = GetAbsoluteAddress(ptr_6710, 0, 4);
        dword_08BB3C38 = *(int16_t*)pattern.get(0, "A3 3C 04 3C 0A D7 84 34 00 68 84 44 3E 60 0D 46 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 20 02", -4);
        dword_08BB456C = *(int16_t*)pattern.get(0, "80 00 44 30 ? ? ? ? 00 00 00 00 ? ? ? ? 01 00 10 34", -4);
        dword_08BB4570 = *(int16_t*)pattern.get(0, "00 00 00 00 20 00 B0 27 ? ? ? ? 25 20 00 02 00 00 00 DA", -20);
        dword_08BB345C = *(int16_t*)pattern.get(0, "00 00 00 00 ? ? ? ? 1C 00 85 8C 02 00 A5 38", -20);
        dword_08BB194C = *(int16_t*)pattern.get(0, "02 00 04 34 ? ? ? ? 01 00 04 34 30 00 A7 93", -4);
        uintptr_t ptr_1F0C = pattern.get(0, "00 00 B0 AF 04 00 B1 AF 0C 00 B3 AF 10 00 B4 AF 14 00 B5 AF 18 00 B6 AF 1C 00 BF AF", -8);
        TheCamera = GetAbsoluteAddress(ptr_1F0C, 0, 4);

        uintptr_t ptr_FC4B4 = pattern.get(0, "C0 29 10 00 80 38 10 00", 0);
        MakeInlineWrapper(ptr_FC4B4,
            move(s7, a0),
            move(k1, a1),
            move(a0, s0),
            move(a1, s1),
            jal((intptr_t)CParticle__AddParticleHook),
            nop(),
            move(a0, s7),
            move(a1, k1),
            sll(a1, s0, 7)
        );
    }

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
}

int module_start(SceSize args, void* argp) {
    if (sceIoDevctl("kemulator:", 0x00000003, NULL, 0, NULL, 0) == 0) {
        SceUID modules[10];
        int count = 0;
        int result = 0;
        if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) >= 0) {
            int i;
            SceKernelModuleInfo info;
            for (i = 0; i < count; ++i) {
                info.size = sizeof(SceKernelModuleInfo);
                if (sceKernelQueryModuleInfo(modules[i], &info) < 0) {
                    continue;
                }

                if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
                {
                    injector.SetGameBaseAddress(info.text_addr, info.text_size);
                    pattern.SetGameBaseAddress(info.text_addr, info.text_size);
                    inireader.SetIniPath(INI_PATH);
                    logger.SetPath(LOG_PATH);
                    result = 1;
                }
                else if (strcmp(info.name, MODULE_NAME) == 0)
                {
                    injector.SetModuleBaseAddress(info.text_addr, info.text_size);
                }
            }

            if (result)
                OnModuleStart();
        }
    }
    return 0;
}