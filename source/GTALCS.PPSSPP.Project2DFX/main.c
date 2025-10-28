#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <systemctrl.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#include "lodl.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTALCS.PPSSPP.Project2DFX"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/GTALCS.PPSSPP.Project2DFX.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.Project2DFX/GTALCS.PPSSPP.Project2DFX.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
_Static_assert(sizeof(MODULE_NAME) - 1 < 28, "MODULE_NAME can't have more than 28 characters");
#endif

uint8_t align16 aCoronas[1024 * 112] = { 0 };  // Static array for 1024 coronas (112 bytes each)

// https://github.com/AndroidModLoader/GTA_StarrySkies/blob/main/main.cpp
#define AMOUNT_OF_STARS 100
#define STAR_SKYBOX_SIDES 5
float StarCoorsX[STAR_SKYBOX_SIDES][AMOUNT_OF_STARS], StarCoorsY[STAR_SKYBOX_SIDES][AMOUNT_OF_STARS], StarSizes[STAR_SKYBOX_SIDES][AMOUNT_OF_STARS];
float fSmallStars, fMiddleStars, fBiggestStars, fBiggestStarsSpawnChance;
CVector PositionsTable[5] =
{
    { 100.0f,  0.0f,   10.0f}, // Left
    {-100.0f,  0.0f,   10.0f}, // Right
    {   0.0f,  100.0f, 10.0f}, // Front
    {   0.0f, -100.0f, 10.0f}, // Back
    {   0.0f,  0.0f,   95.0f}, // Up
};

float clampf(float f, float min, float max)
{
    if (f > max) return max;
    if (f < min) return min;
    return f;
}

int (*base__Random)();
float randf(float min, float max)
{
    return (((float)base__Random()) / (float)RAND_MAX) * (max - min) + min;
}

int16_t CWeather__Foggyness;
int16_t CWeather__CloudCoverage;
int** gpCoronaTexture;
void CSprite__FlushSpriteBufferHook()
{
    CSprite__FlushSpriteBuffer();

    CVector align16 ScreenPos, WorldPos, WorldStarPos;
    volatile float align16 SZ;
    volatile float align16 SZX;
    volatile float align16 SZY;

    for (int side = 0; side < STAR_SKYBOX_SIDES; ++side)
    {
        WorldPos.x = PositionsTable[side].x + GetCamPos()->x;
        WorldPos.y = PositionsTable[side].y + GetCamPos()->y;
        WorldPos.z = PositionsTable[side].z + GetCamPos()->z;
        for (int i = 0; i < AMOUNT_OF_STARS; ++i)
        {
            WorldStarPos = WorldPos;
            SZ = StarSizes[side][i];
            switch (side)
            {
            case 0:
            case 1:
                WorldStarPos.y -= StarCoorsX[side][i];
                WorldStarPos.z += StarCoorsY[side][i];
                break;

            case 2:
            case 3:
                WorldStarPos.x -= StarCoorsX[side][i];
                WorldStarPos.z += StarCoorsY[side][i];
                break;

            default:
                WorldStarPos.x += StarCoorsX[side][i];
                WorldStarPos.y += StarCoorsY[side][i];
                break;
            }

            if (CSprite__CalcScreenCoors(&WorldStarPos, &ScreenPos, &SZX, &SZY, true))
            {
                int starintens = 0;
                if (CurrentTimeHours() < 22 && CurrentTimeHours() > 5)
                    starintens = 0;
                else if (CurrentTimeHours() > 22 || CurrentTimeHours() < 5)
                    starintens = 255;
                else if (CurrentTimeHours() == 22)
                    starintens = 255 * CurrentTimeMinutes() / 60.0f;
                else if (CurrentTimeHours() == 5)
                    starintens = 255 * (60 - CurrentTimeMinutes()) / 60.0f;
                if (starintens != 0) {
                    float coverage = MAX(*(float*)((uintptr_t)injector.GetGP() + CWeather__Foggyness), *(float*)((uintptr_t)injector.GetGP() + CWeather__CloudCoverage));
                    int brightness = (1.0f - coverage) * starintens;

                    brightness = 255;

                    int a0 = brightness;
                    int a1 = brightness;
                    int a2 = brightness;
                    int a3 = 255;
                    int a4 = 255;
                    float f12 = ScreenPos.x;
                    float f13 = ScreenPos.y;
                    float f14 = ScreenPos.z;
                    float f15 = SZX * SZ;
                    float f16 = SZY * SZ;
                    float f17 = 1.0f / ScreenPos.z;

                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (a0));
                    asm volatile ("move $a0, $v0");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (a1));
                    asm volatile ("move $a1, $v0");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (a2));
                    asm volatile ("move $a2, $v0");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (a3));
                    asm volatile ("move $a3, $v0");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (a4));
                    asm volatile ("move $t0, $v0");

                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (f12));
                    asm volatile ("mtc1 $v0, $f12");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (f13));
                    asm volatile ("mtc1 $v0, $f13");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (f14));
                    asm volatile ("mtc1 $v0, $f14");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (f15));
                    asm volatile ("mtc1 $v0, $f15");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (f16));
                    asm volatile ("mtc1 $v0, $f16");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (f17));
                    asm volatile ("mtc1 $v0, $f17");
                    asm volatile ("lw $v0,  %[x]" ::[x] "m" (CSprite__RenderBufferedOneXLUSprite));
                    asm volatile ("jalr $v0");
                }
            }
        }
    }
}

void RslRenderStateSetHook(int a1, int a2)
{
    RslRenderStateSet(8, 0);
    RslRenderStateSet(12, 1);
    RslRenderStateSet(10, 2);
    RslRenderStateSet(11, 2);
    RslRenderStateSet(1, *gpCoronaTexture[0]);
    RenderLODLightsBuffered();

    RslRenderStateSet(a1, a2);
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int OnModuleStart() {
    sceKernelDelayThread(100000); //added cuz game gets stuck on a loadscreen between islands otherwise, idk why

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 1);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 1000);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 1000.0f);

    int SkyGfx = inireader.ReadInteger("PROJECT2DFX", "SkyGfx", 1);

    uintptr_t ptr_15700 = pattern.get(0, "00 29 05 00 21 30 05 00 C0 28 05 00 21 30 C5 00 80 28 05 00 21 28 C5 00 21 20 A4 00 AC 01 84 84 10 00 05 34", -12);
    TheCamera = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_15700 + 0)) << 16 | (uint32_t)(*(uint16_t*)(ptr_15700 + 4)));
    pCamPos = (CVector*)(TheCamera + 0xA70); //0xA70 at 0xEB598

    uintptr_t ptr_8806D58 = pattern.get(0, "25 28 00 00 09 00 04 34 ? ? ? ? 02 00 05 34 0E 00 04 34", -4);
    RslRenderStateSet = (void(*)(int, int))injector.GetBranchDestination(ptr_8806D58);
    uintptr_t ptr_8836EC8 = pattern.get(0, "00 F0 84 44 ? ? ? ? 01 00 04 34", -4);
    CSprite__FlushSpriteBuffer = (void(*)())injector.GetBranchDestination(ptr_8836EC8);
    uintptr_t ptr_894DA78 = pattern.get(0, "02 00 06 3C ? ? ? ? 1A 00 A6 00", -8);
    base__Random = (int(*)())ptr_894DA78;
    uintptr_t ptr_8A25970 = pattern.get(0, "20 00 B4 E7 24 00 B6 E7 28 00 B0 AF 2C 00 B1 AF 30 00 B2 AF 34 00 B3 AF 38 00 B4 AF 3C 00 BF AF 25 80 E0 00", -4);
    CSprite__CalcScreenCoors = (int(*)(CVector*, CVector*, float*, float*, uint8_t))ptr_8A25970;
    uintptr_t ptr_8A27098 = pattern.get(0, "58 00 B0 AF FF 00 90 30 64 00 B3 AF 00 9C 07 00 00 3F 04 3C 54 00 B4 E7 5C 00 B1 AF 60 00 B2 AF 68 00 B4 AF 6C 00 B5 AF", -4);
    CSprite__RenderBufferedOneXLUSprite = (void(*)())ptr_8A27098;

    if (CoronaLimit)
    {
        if (CoronaLimit > 1024)
            CoronaLimit = 1024;

        //SceUID block_id = 0;
        //const int corona_struct_size = 112;
        //uintptr_t aCoronas = injector.AllocMemBlock(corona_struct_size * CoronaLimit, &block_id);

        uintptr_t ptr_1F5ED0 = pattern.get(0, "24 00 B1 AF 2C 00 B3 AF 3C 60 0D 46", -4);
        uintptr_t ptr_1F5EE4 = pattern.get(0, "3C 60 0D 46 21 28 A4 00", 8);
        uintptr_t ptr_1F5F94 = pattern.get(0, "38 00 04 2A ? ? ? ? 70 00 52 26", 0);
        uintptr_t ptr_1F6048 = pattern.get(0, "00 00 05 34 ? ? ? ? C0 31 05 00 00 39 05 00", -4); // count = 2
        uintptr_t ptr_1F6050 = pattern.get(0, "C0 31 05 00 00 39 05 00 23 30 C7 00 21 30 C4 00", -4);
        uintptr_t ptr_1F6070 = pattern.get(0, "38 00 A6 28 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_1F6150 = pattern.get(0, "00 00 07 34 01 00 08 34", -4);
        uintptr_t ptr_1F6160 = pattern.get(1, "00 00 00 00 ? ? ? ? C0 41 07 00", -8); // count = 2
        uintptr_t ptr_1F6198 = pattern.get(0, "38 00 E8 28", 0);
        uintptr_t ptr_1F619C = pattern.get(0, "38 00 04 34 ? ? ? ? 00 00 00 00 C0 21 07 00", 0);
        uintptr_t ptr_1F71E0 = pattern.get(0, "64 00 B0 AF C0 40 04 3C", -8);
        uintptr_t ptr_1F7208 = pattern.get(0, "06 00 17 34 68 00 A5 AF", -4);
        uintptr_t ptr_1F78E0 = pattern.get(0, "38 00 C4 2B", 0);
        uintptr_t ptr_1F7ADC = pattern.get(0, "38 00 04 2A ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_1F7B70 = pattern.get(1, "B4 00 B4 E7 B8 00 B6 E7 BC 00 B8 E7", -8); // count = 3
        uintptr_t ptr_1F7BB0 = pattern.get(0, "00 00 05 34 BF FF 04 24", -4);
        uintptr_t ptr_1F7BE4 = pattern.get(0, "38 00 A6 28 ? ? ? ? C0 31 05 00", 0);
        uintptr_t ptr_1F7FF4 = pattern.get(0, "38 00 24 2A", 0);
        uintptr_t ptr_1F8A3C = pattern.get(0, "00 00 0F 34 01 00 19 34", -4);
        uintptr_t ptr_1F8A48 = pattern.get(0, "38 00 18 34", 0);
        uintptr_t ptr_1F8A4C = pattern.get(0, "C0 C9 0F 00 00 81 0F 00 23 C8 30 03", -8); // count = 2
        uintptr_t ptr_1F8A7C = pattern.get(0, "38 00 F9 29 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_1F8AD0 = pattern.get(0, "38 00 F9 29 ? ? ? ? C0 C9 0F 00", 0);

        injector.WriteInstr(ptr_1F5ED0, lui(s2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6048, lui(a0, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6150, lui(a2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F71E0, lui(s2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F7B70, lui(s5, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F8A3C, lui(t6, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F5EE4, addiu(s2, s2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6050, addiu(a0, a0, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6160, addiu(a2, a2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F7208, addiu(s2, s2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F7BB0, addiu(s5, s5, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F8A4C, addiu(t6, t6, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F5F94, slti(a0, s0, CoronaLimit));
        injector.WriteInstr(ptr_1F6070, slti(a2, a1, CoronaLimit));
        injector.WriteInstr(ptr_1F6198, slti(t0, a3, CoronaLimit));
        injector.WriteInstr(ptr_1F619C, li(a0, CoronaLimit));
        injector.WriteInstr(ptr_1F78E0, slti(a0, fp, CoronaLimit));
        injector.WriteInstr(ptr_1F7ADC, slti(a0, s0, CoronaLimit));
        injector.WriteInstr(ptr_1F7FF4, slti(a0, s1, CoronaLimit));
        injector.WriteInstr(ptr_1F7BE4, slti(a2, a1, CoronaLimit));
        injector.WriteInstr(ptr_1F8A7C, slti(t9, t7, CoronaLimit));
        injector.WriteInstr(ptr_1F8A48, li(t8, CoronaLimit));
        injector.WriteInstr(ptr_1F8AD0, slti(t9, t7, CoronaLimit));
    }

    if (RenderLodLights)
    {
        uintptr_t ptr_1F6098 = pattern.get(0, "B0 FF BD 27 50 00 A2 8F FF 00 4A 31", 0);
        CCoronas__RegisterCorona = (void*)ptr_1F6098;
        uintptr_t ptr_8A259A8 = pattern.get(0, "10 00 B4 27 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 25 20 80 02", -16);
        CDraw__ms_fNearClipZ = (float*)GetAbsoluteAddress(ptr_8A259A8, 0, 4);
        CDraw__ms_fFarClipZ = (float*)GetAbsoluteAddress(ptr_8A259A8, 8, 12);
        uintptr_t ptr_38358 = pattern.get(0, "0C 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 80 3F 04 3C 00 60 84 44", 4);
        CurrentTimeHoursOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_38358 + 0)) << 16) + *(int16_t*)(ptr_38358 + 4);
        CurrentTimeMinutesOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_38358 + 8)) << 16) + *(int16_t*)(ptr_38358 + 12);
        uintptr_t ptr_2723C = pattern.get(0, "34 00 B0 AE 00 60 84 44", -8);
        CTimer__m_snTimeInMillisecondsPauseModeOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_2723C + 0)) << 16) + *(int16_t*)(ptr_2723C + 4);
        uintptr_t ptr_15E4E4 = pattern.get(0, "48 42 04 3C 00 68 84 44 C3 63 0D 46", -8);
        CTimer__ms_fTimeStepOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_15E4E4 + 0)) << 16) + *(int16_t*)(ptr_15E4E4 + 4);

        // Coronas Render
        //uintptr_t ptr_1C0E78 = pattern.get(0, "FF 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 60 02 ? ? ? ? 00 00 00 00", 28);
        //injector.MakeJAL(ptr_1C0E78, (intptr_t)RegisterLODLights);
        uintptr_t ptr_89FBC40 = pattern.get(0, "01 00 04 34 ? ? ? ? 00 00 A5 8C ? ? ? ? ? ? ? ? ? ? ? ? 9C 00 A4 AF", -12);
        gpCoronaTexture = (int**)GetAbsoluteAddress(ptr_89FBC40, 0, 4);
        uintptr_t ptr_89FB164 = pattern.get(0, "25 28 00 00 0C 00 04 34 ? ? ? ? 01 00 05 34 0A 00 04 34 ? ? ? ? 02 00 05 34 0B 00 04 34 ? ? ? ? 02 00 05 34 00 60 91 44", -4);
        injector.MakeJAL(ptr_89FB164, (uintptr_t)RslRenderStateSetHook);
    }

    if (SkyGfx)
    {
        fSmallStars = clampf(inireader.ReadFloat("STARS", "SmallestStarsSize", 0.15f), 0.03f, 2.5f);
        fMiddleStars = clampf(inireader.ReadFloat("STARS", "MiddleStarsSize", 0.6f), 0.03f, 2.5f);
        fBiggestStars = clampf(inireader.ReadFloat("STARS", "BiggestStarsSize", 1.2f), 0.03f, 2.5f);
        fBiggestStarsSpawnChance = 1.0f - 0.01f * clampf(inireader.ReadFloat("STARS", "BiggestStarsChance", 20), 0.0f, 100.0f);

        CWeather__CloudCoverage = *(int16_t*)pattern.get(0, "00 60 84 44 3C 70 0F 46", -4);
        CWeather__Foggyness = *(int16_t*)pattern.get(0, "23 20 A4 00 ? ? ? ? 00 60 84 44", -4);

        for (int side = 0; side < STAR_SKYBOX_SIDES; ++side)
        {
            for (int i = 0; i < AMOUNT_OF_STARS; ++i)
            {
                StarCoorsX[side][i] = 95.0f * randf(-1.0f, 1.0f);

                // Side=4 is when rendering stars directly ABOVE us
                if (side == 4) StarCoorsY[side][i] = 95.0f * randf(-1.0f, 1.0f);
                else StarCoorsY[side][i] = 95.0f * randf(-0.35f, 1.0f);

                // Smaller chances for a bigger star (this is more life-like)
                if (randf(0.0f, 1.0f) > fBiggestStarsSpawnChance) StarSizes[side][i] = 0.8f * randf(fSmallStars, fBiggestStars);
                else StarSizes[side][i] = 0.8f * randf(fSmallStars, fMiddleStars);
            }
        }

        CSprite__FlushSpriteBuffer = (void(*)())injector.MakeJAL(ptr_8836EC8, (uintptr_t)CSprite__FlushSpriteBufferHook);
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