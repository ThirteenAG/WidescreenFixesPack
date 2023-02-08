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

#define MODULE_NAME_INTERNAL "MC3"
#define MODULE_NAME "MidnightClubLARemix.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionMod/MidnightClubLARemix.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/MidnightClubLARemix.PPSSPP.FusionMod/MidnightClubLARemix.PPSSPP.FusionMod.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
#endif

int OnModuleStart() 
{
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);

    if (DualAnalogPatch)
    {
        uintptr_t ptr = pattern.get_first("0D 00 A6 93 02 00 A5 A3 03 00 A6 A3 02 00 A6 93", 8);
    
        MakeInlineWrapper(ptr,
            lbu(a1, sp, 12),
            lbu(a2, sp, 13),
            sb(a1, sp, 2),
            sb(a2, sp, 3),
            lbu(a2, sp, 2),
            lbu(a1, sp, 3),
        
            lbu(s4, sp, 12+2),
            lbu(s5, sp, 13+2),
            sb(s4, sp, 2+2),
            sb(s5, sp, 3+2),
            lbu(s5, sp, 2+2),
            lbu(s4, sp, 3+2)
        );

        ptr = pattern.get_first("10 00 06 A2 11 00 05 A2", 0);
        MakeInlineWrapper(ptr,
            sb(a2, s0, 0x10),
            sb(a1, s0, 0x11),
            sb(s5, s0, 0x10+2),
            sb(s4, s0, 0x11+2)
        );

        ptr = pattern.get(0, "02 00 A4 A3 03 00 A4 A3", 0);
        MakeInlineWrapper(ptr,
            sb(a0, sp,2),
            sb(a0, sp,3),
            lbu(a2, sp,2),
            lbu(a1, sp,3),
        
            sh(a0, sp, 2+2),
            sh(a0, sp, 3+2),
            lhu(a2, sp, 2+2),
            lhu(a1, sp, 3+2)
        );
        
        ptr = pattern.get(1, "02 00 A4 A3 03 00 A4 A3", 0);
        MakeInlineWrapper(ptr,
            sb(a0, sp, 2),
            sb(a0, sp, 3),
            lbu(a2, sp, 2),
            lbu(a1, sp, 3),
        
            sh(a0, sp, 2 + 2),
            sh(a0, sp, 3 + 2),
            lhu(a2, sp, 2 + 2),
            lhu(a1, sp, 3 + 2)
        );

        ptr = pattern.get_first("00 00 A0 A0 01 00 A7 A0 02 00 A6 A0", 0);
        MakeInlineWrapper(ptr,

            sb(zero, a1, 0),
            lui(s3, 0x0),
            ori(s3, s3, 0xA),
            bne(a0, s3, 3), //-->
            lui(s3, 0x0),
            ori(s3, s3, 0x80),
            sb(s3, a1, 0)
            //jmp  <--
        );
    }

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
}

int module_start(SceSize args, void* argp) 
{
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