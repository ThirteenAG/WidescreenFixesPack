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

#define MODULE_NAME_INTERNAL "CTW"
#define MODULE_NAME "GTACTW.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionMod/GTACTW.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionMod/GTACTW.PPSSPP.FusionMod.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);
#endif

int OnModuleStart() {
    int RadioInAllVehicles = inireader.ReadInteger("MAIN", "RadioInAllVehicles", 1);

    if (RadioInAllVehicles)
    {
        uintptr_t ptr = pattern.get_first("2B 10 02 00 25 10 00 00 00 00 BF 8F 08 00 E0 03 10 00 BD 27", 8);
        MakeInlineWrapper(ptr,
            lw(ra, sp, 0),
            li(v0, 0x1)
            );

        ptr = pattern.get_first("01 00 A5 24 2A 20 85 00", 8);
        injector.MakeNOP(ptr);
        injector.MakeNOP(ptr + 16);
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