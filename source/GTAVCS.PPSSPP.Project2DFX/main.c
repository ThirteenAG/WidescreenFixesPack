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

#include "lodl.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.Project2DFX"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/GTAVCS.PPSSPP.Project2DFX.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.Project2DFX/GTAVCS.PPSSPP.Project2DFX.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
#endif

#ifndef __INTELLISENSE__
#define align16 __attribute__((aligned(16)))
#else
#define align16
#endif

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
void (*CSprite__FlushSpriteBuffer)();
int (*CSprite__CalcScreenCoors)(CVector* in, CVector* out, float* outW, float* outH, uint8_t farClip);
void (*CSprite__RenderBufferedOneXLUSprite)();
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

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int OnModuleStart() {
    sceKernelDelayThread(250000);

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 1);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 1000);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 1000.0f);

    int SkyGfx = inireader.ReadInteger("PROJECT2DFX", "SkyGfx", 0);

    uintptr_t ptr_3CF00 = pattern.get(0, "25 20 20 02 30 00 64 26 00 00 80 D8 00 00 41 D8", -16);
    TheCamera = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_3CF00 + 0)) << 16) + *(int16_t*)(ptr_3CF00 + 4);
    pCamPos = (CVector*)(TheCamera + 0x9B0); //0x9B0 at 0x218648
    
    if (RenderLodLights)
    {
        uintptr_t ptr_17D324 = pattern.get(0, "E0 FF BD 27 20 00 A2 8F", 0);
        CCoronas__RegisterCorona = (void*)ptr_17D324;
        CDraw__ms_fNearClipZOffset = *(int16_t*)pattern.get(0, "25 90 A0 00 25 30 80 00 FF 00 13 31", 12);
        CDraw__ms_fFarClipZOffset = *(int16_t*)pattern.get(0, "25 90 A0 00 25 30 80 00 FF 00 13 31", 24);
        CurrentTimeHoursOffset = *(int16_t*)pattern.get(0, "0C 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? 80 3F 04 3C", 4);
        CurrentTimeMinutesOffset = *(int16_t*)pattern.get(0, "0C 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? 80 3F 04 3C", 8);
        CTimer__m_snTimeInMillisecondsPauseModeOffset = *(int16_t*)pattern.get(0, "34 00 B0 AE 00 60 84 44", -4);
        CTimer__ms_fTimeStepOffset = *(int16_t*)pattern.get(0, "48 42 04 3C 00 68 84 44 C3 63 0D 46", -4);

        SceUID block_id = 0;
        const int corona_struct_size = 112;
        uintptr_t aCoronas = injector.AllocMemBlock(corona_struct_size * CoronaLimit, &block_id);

        uintptr_t ptr_17D174 = pattern.get(0, "3C 68 0C 46 21 20 85 00", -4);
        uintptr_t ptr_17D180 = pattern.get(0, "14 00 B1 AF 18 00 BF AF ? ? ? ? 06 6B 00 46", -4);
        uintptr_t ptr_17D22C = pattern.get(0, "38 00 24 2A ? ? ? ? 70 00 10 26", 0);
        uintptr_t ptr_17D2D4 = pattern.get(0, "00 00 05 34 ? ? ? ? C0 31 05 00 00 39 05 00", -4); // count = 3
        uintptr_t ptr_17D2DC = pattern.get(0, "C0 31 05 00 00 39 05 00 23 30 C7 00 21 30 C4 00", -4);
        uintptr_t ptr_17D2FC = pattern.get(0, "38 00 A6 28 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_17D3DC = pattern.get(0, "00 00 07 34 01 00 08 34", -4);
        uintptr_t ptr_17D3EC = pattern.get(0, "00 00 00 00 ? ? ? ? C0 41 07 00", -8);
        uintptr_t ptr_17D424 = pattern.get(0, "38 00 E8 28", 0);
        uintptr_t ptr_17D428 = pattern.get(0, "38 00 04 34 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_17E410 = pattern.get(0, "44 00 A4 AF ? ? ? ? 00 00 17 34", -8);
        uintptr_t ptr_17E42C = pattern.get(0, "EF FF 1E 24", -4);
        uintptr_t ptr_17EAF0 = pattern.get(0, "38 00 E4 2A", 0);
        uintptr_t ptr_17ED08 = pattern.get(0, "38 00 04 2A", 0);
        uintptr_t ptr_17ED94 = pattern.get(2, "94 00 B4 E7 98 00 B6 E7 9C 00 B8 E7", -8); // count = 4
        uintptr_t ptr_17EDD8 = pattern.get(0, "00 00 05 34 BF FF 04 24", -4);
        uintptr_t ptr_17F218 = pattern.get(0, "38 00 24 2A ? ? ? ? 00 60 85 44", 0);
        uintptr_t ptr_17FCB0 = pattern.get(0, "00 00 0F 34 01 00 19 34", -4);
        uintptr_t ptr_17FCBC = pattern.get(0, "38 00 18 34", 0);
        uintptr_t ptr_17FCC0 = pattern.get(0, "C0 C9 0F 00 00 81 0F 00 23 C8 30 03", -8); // count = 2
        uintptr_t ptr_17FCF0 = pattern.get(0, "38 00 F9 29 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_17FD44 = pattern.get(0, "38 00 F9 29 ? ? ? ? C0 C9 0F 00", 0);
        uintptr_t ptr_133F80 = pattern.get(0, "FF 00 04 34 ? ? ? ? ? ? ? ? 00 00 00 00", 16);
        uintptr_t ptr_2FDD50 = pattern.get(0, "A0 42 04 3C 00 70 84 44 3C 60 0E 46", 0);
        uintptr_t ptr_2FDDA0 = pattern.get(0, "A0 42 04 3C 00 68 84 44 3C 60 0D 46", 0);

        injector.WriteInstr(ptr_17D174, lui(s0, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17D2D4, lui(a0, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17D3DC, lui(a2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17E410, lui(s2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17ED94, lui(s5, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17FCB0, lui(t6, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17D180, ori(s0, s0, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17D2DC, ori(a0, a0, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17D3EC, ori(a2, a2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17E42C, ori(s2, s2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17EDD8, ori(s5, s5, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17FCC0, ori(t6, t6, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17D22C, slti(a0, s1, CoronaLimit));
        injector.WriteInstr(ptr_17D2FC, slti(a2, a1, CoronaLimit));
        injector.WriteInstr(ptr_17D424, slti(t0, a3, CoronaLimit));
        injector.WriteInstr(ptr_17D428, li(a0, CoronaLimit));
        injector.WriteInstr(ptr_17EAF0, slti(a0, s7, CoronaLimit));
        injector.WriteInstr(ptr_17ED08, slti(a0, s0, CoronaLimit));
        injector.WriteInstr(ptr_17F218, slti(a0, s1, CoronaLimit));
        injector.WriteInstr(ptr_17FCF0, slti(t9, t7, CoronaLimit));
        injector.WriteInstr(ptr_17FCBC, li(t8, CoronaLimit));
        injector.WriteInstr(ptr_17FD44, slti(t9, t7, CoronaLimit));

        // Coronas Render
        injector.MakeJAL(ptr_133F80, (intptr_t)RegisterLODLights);

        // Heli Height Limit
        injector.MakeInlineLUIORI(ptr_2FDD50, 800.0f);
        injector.MakeInlineLUIORI(ptr_2FDDA0, 800.0f);
    }

    if (SkyGfx)
    {
        fSmallStars = clampf(inireader.ReadFloat("STARS", "SmallestStarsSize", 0.15f), 0.03f, 2.5f);
        fMiddleStars = clampf(inireader.ReadFloat("STARS", "MiddleStarsSize", 0.6f), 0.03f, 2.5f);
        fBiggestStars = clampf(inireader.ReadFloat("STARS", "BiggestStarsSize", 1.2f), 0.03f, 2.5f);
        fBiggestStarsSpawnChance = 1.0f - 0.01f * clampf(inireader.ReadFloat("STARS", "BiggestStarsChance", 20), 0.0f, 100.0f);

        CWeather__CloudCoverage = *(int16_t*)pattern.get(0, "00 60 84 44 3C 70 0F 46", -4);
        CWeather__Foggyness = *(int16_t*)pattern.get(0, "23 20 A4 00 ? ? ? ? 00 60 84 44", -4);

        uintptr_t ptr_8B20938 = pattern.get(0, "02 00 05 3C ? ? ? ? 1A 00 85 00", -4);
        base__Random = (int(*)())ptr_8B20938;

        uintptr_t ptr_8AA82D8 = pattern.get(0, "10 00 B4 E7 14 00 B6 E7 18 00 B0 AF 1C 00 B1 AF 20 00 B2 AF 24 00 B3 AF 28 00 BF AF", -4);
        CSprite__CalcScreenCoors = (int(*)(CVector*, CVector*, float*, float*, uint8_t))ptr_8AA82D8;
        uintptr_t ptr_8AA9B24 = pattern.get(0, "4C 00 B1 AF FF 00 B1 30", -4);
        CSprite__RenderBufferedOneXLUSprite = (void(*)())ptr_8AA9B24;

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
        
        uintptr_t ptr_888D604 = pattern.get(0, "00 F0 84 44 ? ? ? ? 01 00 04 34", -4);
        CSprite__FlushSpriteBuffer = (void(*)())injector.MakeJAL(ptr_888D604, (uintptr_t)CSprite__FlushSpriteBufferHook);        
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