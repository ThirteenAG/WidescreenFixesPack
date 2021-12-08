#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <systemctrl.h>

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.WidescreenFix"

PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);

SceKernelModuleInfo ModuleInfo;

int sceKernelQuerySystemCall(void *function);

#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a);

enum GtaPad {
  PAD_LX = 1,
  PAD_LY = 2,
  PAD_RX = 3,
  PAD_RY = 4,
};

static u32 cameraXStub, cameraYStub, aimXStub, aimYStub;

static u32 MakeSyscallStub(void *function) {
  SceUID block_id = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "", PSP_SMEM_High, 2 * sizeof(u32), NULL);
  u32 stub = (u32)sceKernelGetBlockHeadAddr(block_id);
  _sw(0x03E00008, stub);
  _sw(0x0000000C | (sceKernelQuerySystemCall(function) << 6), stub + 4);
  return stub;
}

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
    MAKE_CALL(addr + 0x8C, cameraXStub);
    _sw(0x00000000, addr + 0x108);
    _sw(0x10000002, addr + 0x118);
    MAKE_CALL(addr + 0x144, cameraYStub);
    return 1;
  }

  // Redirect gun aim movement
  if (_lw(addr + 0x00) == 0x04800040 && _lw(addr + 0x08) == 0x1080003E) {
    MAKE_CALL(addr + 0x50, aimXStub);
    MAKE_CALL(addr + 0x7C, aimXStub);
    MAKE_CALL(addr + 0x8C, aimXStub);
    MAKE_CALL(addr + 0x158, aimYStub);
    MAKE_CALL(addr + 0x1BC, aimYStub);
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
    MAKE_CALL(addr + 0x84, cameraXStub);
    _sw(0x00000000, addr + 0x100);
    _sw(0x10000002, addr + 0x110);
    MAKE_CALL(addr + 0x13C, cameraYStub);
    return 1;
  }

  // Redirect gun aim movement
  if (_lw(addr + 0x00) == 0x04800036 && _lw(addr + 0x08) == 0x10800034) {
    MAKE_CALL(addr + 0x3C, aimXStub);
    MAKE_CALL(addr + 0x68, aimXStub);
    MAKE_CALL(addr + 0x78, aimXStub);
    MAKE_CALL(addr + 0x130, aimYStub);
    MAKE_CALL(addr + 0x198, aimYStub);
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


void module_init()
{


    cameraXStub = MakeSyscallStub(cameraX);
    cameraYStub = MakeSyscallStub(cameraY);
    aimXStub = MakeSyscallStub(aimX);
    aimYStub = MakeSyscallStub(aimY);

    u32 i;
    for (i = 0; i < ModuleInfo.text_size; i += 4) {
        u32 addr = ModuleInfo.text_addr + i;

        PatchVCS(addr, ModuleInfo.text_addr);

        //if ((gta_version == -1 || gta_version == 1) && PatchLCS(addr, ModuleInfo.text_addr)) {
        //    gta_version = 1;
        //    continue;
        //}
    }



    //LOG_Initialise();
    //PSPPatcher_Init(addr);
    //SettingsMgr_Init();
    //
    //Nop(0x132e28); //EU
    //Nop(0x132efc); //US


    //if (SettingsMgr_Get().bUsePuzzleModifierInNormalGame)
    //	Nop(0x153DD4);
    //
    //if (SettingsMgr_Get().bHideFighterFaceInLifebar)
    //	PatchChar(0x2CECE0, 0x00);

    //MakeCall(0xA0DB4, (int)hook_load_hero_model);


    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();
}

int module_thread(SceSize args, void* argp)
{
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
                    module_init();
                    SceUID thid = sceKernelCreateThread(MODULE_NAME, module_thread, 0, 0x10000, 0, NULL);
                    if (thid >= 0)
                        sceKernelStartThread(thid, 0, argp);
                }
            }
        }
    }
    return 0;
}