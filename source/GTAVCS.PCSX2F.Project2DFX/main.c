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
}

int main()
{
    return 0;
}
