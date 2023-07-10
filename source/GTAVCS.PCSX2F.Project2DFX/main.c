#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"

//#define NANOPRINTF_IMPLEMENTATION
//#include "../../includes/pcsx2/nanoprintf.h"
//#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/memalloc.h"
#include "../../includes/pcsx2/patterns.h"
#include "../../includes/pcsx2/mips.h"
#include "../../includes/pcsx2/injector.h"
#include "../../includes/pcsx2/inireader.h"

#include "lodl.h"

int CompatibleCRCList[] = { 0x4F32A11F };
char PluginData[MaxIniSize] = { 1 };

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

float* CWeather__Foggyness;
float* CWeather__CloudCoverage;
void (*CSprite__FlushSpriteBuffer)();
int (*CSprite__CalcScreenCoors)(CVector* in, CVector* out, float* outW, float* outH, uint8_t farClip);
void (*CSprite__RenderBufferedOneXLUSprite)();
void CSprite__FlushSpriteBufferHook()
{
    CSprite__FlushSpriteBuffer();

    CVector ScreenPos, WorldPos, WorldStarPos;
    float SZ, SZX, SZY;

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
                    float coverage = MAX(*CWeather__Foggyness, *CWeather__CloudCoverage);
                    int brightness = (1.0f - coverage) * starintens;

                    int a0 = brightness;
                    int a1 = brightness;
                    int a2 = brightness;
                    int a3 = starintens;
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
                    asm volatile ("move $a4, $v0");
                    
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

void init()
{
    inireader.SetIniPath((char*)PluginData + sizeof(uint32_t), *(uint32_t*)PluginData);

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 0);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 900);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 500.0f);

    int SkyGfx = inireader.ReadInteger("PROJECT2DFX", "SkyGfx", 0);

    if (RenderLodLights)
    {
        uintptr_t ptr_27DD10 = pattern.get(0, "FF 00 4A 31 ? ? ? ? ? ? ? ? ? ? ? ? 80 50 0A 00 ? ? ? ? 21 50 42 01", 0);
        CCoronas__RegisterCoronaINT = (void*)ptr_27DD10;
        uintptr_t ptr_2A2ED4 = pattern.get(0, "10 01 B2 FF 08 01 B1 FF 2D 90 40 00 28 01 B5 FF", -4);
        uintptr_t TheCamera = GetAbsoluteAddress(ptr_2A2ED4, 0, 20);
        pCamPos = (CVector*)(TheCamera + 0xA50); //0xA50 at 0x377460
        uintptr_t ptr_228700 = pattern.get(0, "80 3F 01 3C 00 00 81 44 ? ? ? ? ? ? ? 3C ? ? ? 3C ? ? ? 3C ? ? ? ? ? ? ? 3C ? ? ? 3C ? ? ? 3C ? ? ? 3C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 08 00 E0 03", -4);
        CurrentTimeHoursOffset = GetAbsoluteAddress(ptr_228700, 24, 56);
        CurrentTimeMinutesOffset = GetAbsoluteAddress(ptr_228700, 32, 64);
        uintptr_t ptr_1085AC = pattern.get(0, "34 00 44 8C ? ? ? ? ? ? ? ? 2B 10 44 00", 4);
        CTimer__m_snTimeInMillisecondsPauseModeOffset = GetAbsoluteAddress(ptr_1085AC, 0, 4);
        uintptr_t ptr_132678 = pattern.get(0, "02 08 00 46 82 08 02 46 42 08 03 46 00 21 00 46", 0);
        CTimer__ms_fTimeStepOffset = GetAbsoluteAddress(ptr_132678, -20, -8);

        IncreaseCoronasLimit(CoronaLimit);

        // Coronas Render
        uintptr_t ptr_21ED38 = pattern.get(0, "00 00 00 00 48 00 02 3C 48 7A 43 8C", -4);
        injector.MakeJAL(ptr_21ED38, (intptr_t)RegisterLODLights);

        // Heli Height Limit
        uintptr_t ptr_3EA644 = pattern.get(1, "A0 42 01 3C 00 00 81 44 00 00 00 00", 0); // count = 3
        uintptr_t ptr_3EA690 = pattern.get(2, "A0 42 01 3C 00 00 81 44 00 00 00 00", 0); // count = 3
        injector.MakeInlineLUIORI(ptr_3EA644, 800.0f);
        injector.MakeInlineLUIORI(ptr_3EA690, 800.0f);
    }

    if (SkyGfx)
    {
        fSmallStars = clampf(inireader.ReadFloat("STARS", "SmallestStarsSize", 0.15f), 0.03f, 2.5f);
        fMiddleStars = clampf(inireader.ReadFloat("STARS", "MiddleStarsSize", 0.6f), 0.03f, 2.5f);
        fBiggestStars = clampf(inireader.ReadFloat("STARS", "BiggestStarsSize", 1.2f), 0.03f, 2.5f);
        fBiggestStarsSpawnChance = 1.0f - 0.01f * clampf(inireader.ReadFloat("STARS", "BiggestStarsChance", 20), 0.0f, 100.0f);

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

        uintptr_t ptr_18CFC4 = pattern.get(0, "4D 00 02 3C 4D 00 03 3C ? ? ? ? 20 00 A4 27", -0);
        CWeather__CloudCoverage = (float*)GetAbsoluteAddress(ptr_18CFC4, 0, 8);
        CWeather__Foggyness = (float*)GetAbsoluteAddress(ptr_18CFC4, 0, 16);

        uintptr_t ptr_408058 = pattern.get(0, "49 00 08 3C 01 00 03 3C", -0);
        base__Random = (int(*)())ptr_408058;

        uintptr_t ptr_39FDE8 = pattern.get(0, "49 00 02 3C ? ? ? ? ? ? ? ? 00 00 B0 FF ? ? ? ? 08 00 BF FF 52 00 04 3C", -0);
        CSprite__FlushSpriteBuffer = (void(*)())ptr_39FDE8;
        uintptr_t ptr_39FC48 = pattern.get(0, "70 00 02 3C 10 00 B0 FF", -4);
        CSprite__CalcScreenCoors = (int(*)(CVector*, CVector*, float*, float*, uint8_t))ptr_39FC48;
        uintptr_t ptr_39FF48 = pattern.get(0, "80 68 10 46 ? ? ? ? 40 60 0F 46 80 3F 01 3C 00 00 81 44 01 63 0F 46 20 00 A0 AF 41 6B 10 46 68 00 B5 FF", -0);
        CSprite__RenderBufferedOneXLUSprite = (void(*)())ptr_39FF48;
        uintptr_t ptr_18D450 = pattern.get(0, "00 00 00 00 48 00 03 3C 01 00 04 24 ? ? ? ? ? ? ? ? 00 00 45 8C", -4);
        injector.MakeJAL(ptr_18D450, (uintptr_t)CSprite__FlushSpriteBufferHook);
    }
}

int main()
{
    return 0;
}
