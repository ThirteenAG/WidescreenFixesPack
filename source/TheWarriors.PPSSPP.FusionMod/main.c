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

#define MODULE_NAME_INTERNAL "WARR"
#define MODULE_NAME "TheWarriors.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionMod/TheWarriors.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionMod/TheWarriors.PPSSPP.FusionMod.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);
#endif

int OnModuleStart() 
{
    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);
    
    char szForceAspectRatio[100];
    char* ForceAspectRatio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", szForceAspectRatio, sizeof(szForceAspectRatio));

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first("10 00 A5 27 ? ? ? ? ? ? ? ? ? ? ? ? 21 28 00 00", 0);
        injector.MakeNOP(ptr + 12);
        injector.MakeNOP(ptr + 28);
        injector.MakeNOP(ptr + 44);
    }
    
    if (DualAnalogPatch)
    {
        uintptr_t ptr = pattern.get_first("21 10 51 00 ? ? ? ? 1C 00 50 A4", 4);
    
        MakeInlineWrapper(ptr + 0x00,
            sh(s0, v0, 0x1C),
            lbu(a2, sp, 0x1B),
            lbu(a1, sp, 0x1A)
        );

        injector.MakeNOP(ptr + 0x04);
        injector.WriteInstr(ptr + 0x08, b(21));
        injector.MakeNOP(ptr + 0x0C);
    
        injector.WriteInstr(ptr + 0x6C, sb(a2, s1, 0x1B));
        injector.WriteInstr(ptr + 0x8C, sb(a1, s1, 0x1A));
    
        sceKernelIcacheInvalidateRange((const void*)ptr, 0x90);
    }
    
    if (Enable60FPS)
    {
        uintptr_t ptr = pattern.get_first("02 00 42 2C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 00 00 A2 8C", 4);
        injector.MakeNOP(ptr);
    }

    if (strlen(ForceAspectRatio) && ForceAspectRatio[0] != '0')
    {
        // Default is 512/320 for some reason
        int x = 512;
        int y = 320;
        float fAspectRatio = (float)x / (float)y;
        
        if (strcmp(ForceAspectRatio, "auto") != 0)
        {
            char* ch;
            ch = strtok(ForceAspectRatio, ":");
            if (ch)
            {
                x = str2int(ch, 10);
                ch = strtok(NULL, ":");
                if (ch)
                {
                    y = str2int(ch, 10);
                    fAspectRatio = (float)x / (float)y;
                }
            }
        }
        else
        {
            fAspectRatio = 16.0f / 9.0f;
        }

        uintptr_t ptr_28C = pattern.get(0, "94 18 C1 E7 03 03 01 46", 4);
        MakeInlineWrapper(ptr_28C,
            lui(t9, HIWORD(fAspectRatio)),
            ori(t9, t9, LOWORD(fAspectRatio)),
            mtc1(t9, f12)
        );

        float fHudScale = fAspectRatio;
        uintptr_t ptr_B90 = pattern.get(0, "02 00 02 46 00 00 C3 8F", -4);
        MakeInlineWrapper(ptr_B90,
            lui(t9, HIWORD(fHudScale)),
            ori(t9, t9, LOWORD(fHudScale)),
            mtc1(t9, f1)
        );
    }

    //{
    //    float fHudSize = 1.5f;
    //    uintptr_t ptr_2FC = pattern.get(0, "CD 08 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 05 00 63 34", 8);
    //    MakeInlineWrapper(ptr_2FC,
    //        lui(v0, HIWORD(fHudSize)),
    //        ori(v0, v0, LOWORD(fHudSize)),
    //        mtc1(v0, f4)
    //    );
    //}

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
                    inireader.SetIniPath(INI_PATH); //crashes on sceIoClose for some reason
                    inireader.SetIniPath(INI_PATH); //but works if called twice (!?)
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