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

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int OnModuleStart() {
    sceKernelDelayThread(100000);

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 1);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 1000);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 1000.0f);

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