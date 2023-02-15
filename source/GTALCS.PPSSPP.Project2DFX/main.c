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
#endif

int OnModuleStart() {
    sceKernelDelayThread(100000); //added cuz game gets stuck on a loadscreen between islands otherwise, idk why

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 1);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 1000);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 1000.0f);

    if (RenderLodLights)
    {
        uintptr_t ptr_1F6098 = pattern.get(0, "B0 FF BD 27 50 00 A2 8F FF 00 4A 31", 0);
        CCoronas__RegisterCorona = (void*)ptr_1F6098;
        uintptr_t ptr_15700 = pattern.get(0, "00 29 05 00 21 30 05 00 C0 28 05 00 21 30 C5 00 80 28 05 00 21 28 C5 00 21 20 A4 00 AC 01 84 84 10 00 05 34", -12);
        uintptr_t TheCamera = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_15700 + 0)) << 16 | (uint32_t)(*(uint16_t*)(ptr_15700 + 4)));
        pCamPos = (CVector*)(TheCamera + 0xA70); //0xA70 at 0xEB598
        uintptr_t ptr_38358 = pattern.get(0, "0C 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 80 3F 04 3C 00 60 84 44", 4);
        CurrentTimeHoursOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_38358 + 0)) << 16) + *(int16_t*)(ptr_38358 + 4);
        CurrentTimeMinutesOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_38358 + 8)) << 16) + *(int16_t*)(ptr_38358 + 12);
        uintptr_t ptr_2723C = pattern.get(0, "34 00 B0 AE 00 60 84 44", -8);
        CTimer__m_snTimeInMillisecondsPauseModeOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_2723C + 0)) << 16) + *(int16_t*)(ptr_2723C + 4);
        uintptr_t ptr_15E4E4 = pattern.get(0, "48 42 04 3C 00 68 84 44 C3 63 0D 46", -8);
        CTimer__ms_fTimeStepOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_15E4E4 + 0)) << 16) + *(int16_t*)(ptr_15E4E4 + 4);

        SceUID block_id = 0;
        const int corona_struct_size = 112;
        uintptr_t aCoronas = injector.AllocMemBlock(corona_struct_size * CoronaLimit, &block_id);

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
        uintptr_t ptr_1C0E78 = pattern.get(0, "FF 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 60 02 ? ? ? ? 00 00 00 00", 28);

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

        // Coronas Render
        injector.MakeJAL(ptr_1C0E78, (intptr_t)RegisterLODLights);
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