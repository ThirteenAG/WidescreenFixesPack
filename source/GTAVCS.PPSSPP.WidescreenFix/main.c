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

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.ini"

PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);

SceKernelModuleInfo ModuleInfo;

#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a);

enum GtaPad {
  PAD_LX = 1,
  PAD_LY = 2,
  PAD_RX = 3,
  PAD_RY = 4,
};

#define MAKE_SYSCALL(a, n) _sw(0x0000000C | ((n) << 6), a);
#define MAKE_JAL(a, n) _sw(0x0C000000 | ((n) >> 2), a);

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

static int PatchVCS(u32 addr, u32 text_addr) {
  // Implement right analog stick
  if (_lw(addr + 0x00) == 0x10000006 && _lw(addr + 0x04) == 0xA3A70003) {
    _sw(0xA7A50000 | (_lh(addr + 0x1C)), addr + 0x24);      // sh $a1, X($sp)
    _sw(0x97A50000 | (_lh(addr - 0xC) + 0x2), addr + 0x1C); // lhu $a1, X($sp)
    return 1;
  }

  // Redirect camera movement
  if (_lw(addr + 0x00) == 0x14800036 && _lw(addr + 0x10) == 0x10400016) {
    _sw(0x00000000, addr + 0x00);
    _sw(0x10000016, addr + 0x10);
    MAKE_JAL(addr + 0x8C, (intptr_t)cameraX);
    _sw(0x00000000, addr + 0x108);
    _sw(0x10000002, addr + 0x118);
    MAKE_JAL(addr + 0x144, (intptr_t)cameraY);
    return 1;
  }

  // Redirect gun aim movement
  if (_lw(addr + 0x00) == 0x04800040 && _lw(addr + 0x08) == 0x1080003E) {
      MAKE_JAL(addr + 0x50, (intptr_t)aimX);
      MAKE_JAL(addr + 0x7C, (intptr_t)aimX);
      MAKE_JAL(addr + 0x8C, (intptr_t)aimX);
      MAKE_JAL(addr + 0x158, (intptr_t)aimY);
      MAKE_JAL(addr + 0x1BC, (intptr_t)aimY);
    return 1;
  }

  // Allow using L trigger when walking
  if (_lw(addr + 0x00) == 0x1480000E && _lw(addr + 0x10) == 0x10800008 &&
      _lw(addr + 0x1C) == 0x04800003) {
    _sw(0, addr + 0x10);
    _sw(0, addr + 0x9C);
    return 1;
  }

  // Force L trigger value in the L+camera movement function
  if (_lw(addr + 0x00) == 0x84C7000A) {
    _sw(0x2407FFFF, addr + 0x00);
    return 1;
  }

  return 0;
}

static int PatchLCS(u32 addr, u32 text_addr) {
  // Implement right analog stick
  if (_lw(addr + 0x00) == 0x10000006 && _lw(addr + 0x04) == 0xA3A70013) {
    _sw(0xA7A50000 | (_lh(addr + 0x1C)), addr + 0x24);      // sh $a1, X($sp)
    _sw(0x97A50000 | (_lh(addr - 0xC) + 0x2), addr + 0x1C); // lhu $a1, X($sp)
    return 1;
  }

  // Redirect camera movement
  if (_lw(addr + 0x00) == 0x14800034 && _lw(addr + 0x10) == 0x10400014) {
    _sw(0x00000000, addr + 0x00);
    _sw(0x10000014, addr + 0x10);
    MAKE_JAL(addr + 0x84, (intptr_t)cameraX);
    _sw(0x00000000, addr + 0x100);
    _sw(0x10000002, addr + 0x110);
    MAKE_JAL(addr + 0x13C, (intptr_t)cameraY);
    return 1;
  }

  // Redirect gun aim movement
  if (_lw(addr + 0x00) == 0x04800036 && _lw(addr + 0x08) == 0x10800034) {
    MAKE_JAL(addr + 0x3C, (intptr_t)aimX);
    MAKE_JAL(addr + 0x68, (intptr_t)aimX);
    MAKE_JAL(addr + 0x78, (intptr_t)aimX);
    MAKE_JAL(addr + 0x130, (intptr_t)aimY);
    MAKE_JAL(addr + 0x198, (intptr_t)aimY);
    return 1;
  }

  // Allow using L trigger when walking
  if (_lw(addr + 0x00) == 0x14A0000E && _lw(addr + 0x10) == 0x10A00008 &&
      _lw(addr + 0x1C) == 0x04A00003) {
    _sw(0, addr + 0x10);
    _sw(0, addr + 0x74);
    return 1;
  }

  // Force L trigger value in the L+camera movement function
  if (_lw(addr + 0x00) == 0x850A000A) {
    _sw(0x240AFFFF, addr + 0x00);
    return 1;
  }

  return 0;
}

int OnModuleStart(SceKernelModuleInfo* mod) {
    logger.Write(LOG_PATH, "Hello...\n");
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

        /// ////////////////////////////////////////////

        u32 text_addr = mod->text_addr;
        int gta_version = -1;

        u32 i;
        for (i = 0; i < mod->text_size; i += 4) {
            u32 addr = text_addr + i;

            if ((gta_version == -1 || gta_version == 0) && PatchVCS(addr, text_addr)) {
                gta_version = 0;
                continue;
            }

            if ((gta_version == -1 || gta_version == 1) && PatchLCS(addr, text_addr)) {
                gta_version = 1;
                continue;
            }
        }
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