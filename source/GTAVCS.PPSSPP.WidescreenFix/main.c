#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <systemctrl.h>

#include "includes/log.h"
#include "includes/injector.h"
#include "includes/patterns.h"
#include "includes/inireader.h"
#include "includes/gvm.h"
#define gv(...) _get_addr_for_game_version(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#include "includes/mips.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.ini"

PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);

SceKernelModuleInfo ModuleInfo;

enum GtaPad {
  PAD_LX = 1,
  PAD_LY = 2,
  PAD_RX = 3,
  PAD_RY = 4,
};

short cameraX(short *pad) {
  return pad[PAD_RX];
}

short cameraY(short *pad) {
  return pad[PAD_RY];
}

short aimX(short *pad) {
  return pad[PAD_LX] ? pad[PAD_LX] : pad[PAD_RX];
}

short aimY(short *pad) {
  return pad[PAD_LY] ? pad[PAD_LY] : pad[PAD_RY];
}

//void lambda(void* regs)
//{
//    __asm__ volatile (
//        "nop\n\tnop\n\tnop\n\t"
//        );
//}

int OnModuleStart(SceKernelModuleInfo* mod) {
    logger.Write(LOG_PATH, "Hello...\n");
    logger.WriteF(LOG_PATH, "%x\n", 0);

    //logger.WriteF(LOG_PATH, "%x %x\n", ptr + 0x24 + mod->text_addr, (intptr_t)lambda);
    //injector.MakeInline(ptr + 0x24, (intptr_t)lambda);

    injector.base_addr = mod->text_addr;
    pattern.base_addr = mod->text_addr;
    inireader.SetIniPath(INI_PATH);

    /*
    enum GameVersion
    {
        ULUS10160,
        ULES00502,
    };

    if (strcmp((char*)(mod->text_addr + 0x36F8D8), "GTA3") == 0) {
        gvm.init(ULUS10160);
    }
    else if (strcmp((char*)(mod->text_addr + 0x36FCB8), "GTA3") == 0) {
        gvm.init(ULES00502);
    }

    // --> gv(0x123, 0x456)
    */

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first(mod->text_addr, mod->text_size, "00 00 00 00 ? ? ? ? 00 00 00 00 ? ? 04 3C 25 28 00 00", -4);
        injector.MakeNOP(ptr);
    }

    if (DualAnalogPatch)
    {
        // Implement right analog stick
        uintptr_t ptr = pattern.get_first(mod->text_addr, mod->text_size, "06 00 00 10 03 00 A7 A3", 0);
        injector.WriteInstr(ptr + 0x24, 
            sh(a1, sp, 0)
        );
        injector.WriteInstr(ptr + 0x1C,
            lhu(a1, sp, 0xE)
        );

        // Redirect camera movement
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "36 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 16 00 40 10", 0);
        injector.MakeNOP(ptr + 0x00);
        injector.WriteMemory8(ptr + 0x10 + 0x2, 0x00);  // beqz -> b
        injector.MakeJAL(ptr + 0x8C, (intptr_t)cameraX);
        injector.MakeNOP(ptr + 0x108);
        injector.WriteMemory8(ptr + 0x118 + 0x2, 0x00); // beqz -> b
        injector.MakeJAL(ptr + 0x144, (intptr_t)cameraY);

        // Redirect gun aim movement
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "40 00 80 04 ? ? ? ? 3E 00 80 10", 0);
        injector.MakeJAL(ptr + 0x50, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x7C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x8C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x158, (intptr_t)aimY);
        injector.MakeJAL(ptr + 0x1BC, (intptr_t)aimY);

        // Allow using L trigger when walking
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "0E 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 08 00 80 10 ? ? ? ? ? ? ? ? 03 00 80 04", 0);
        injector.MakeNOP(ptr + 0x10);
        injector.MakeNOP(ptr + 0x9C);

        // Force L trigger value in the L+camera movement function
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "0A 00 C7 84", 0);
        injector.WriteInstr(ptr + 0x00,
            li(a3, -0x1)
        );
    }

    if (true)
    {

    }

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
}

int module_thread(SceSize args, void* argp)
{
    /*
    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
    while (1)
    {
        sceCtrlPeekBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_SQUARE)
        {
            if (pad.Buttons & PSP_CTRL_CIRCLE)
            {
                sceKernelDelayThread(1000000 / 10);
                //hook_new_select_table();

                //LOG_Message("Hello world!\n");

            }

        }

        sceKernelDelayThread(1000);
    }
    sceKernelExitDeleteThread(0);
    */
    return 0;
}

int module_start(SceSize args, void* argp) {
    if (sceIoDevctl("kemulator:", 0x00000003, NULL, 0, NULL, 0) == 0) {
        SceUID modules[10];
        int count = 0;
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
                    ModuleInfo = info;
                    OnModuleStart(&ModuleInfo);
                    SceUID thid = sceKernelCreateThread(MODULE_NAME, module_thread, 0, 0x10000, 0, NULL);
                    if (thid >= 0)
                        sceKernelStartThread(thid, 0, argp);
                }
            }
        }
    }
    return 0;
}