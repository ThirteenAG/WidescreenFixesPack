#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <systemctrl.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#define MODULE_NAME_INTERNAL "SplinterCellPSP"
#define MODULE_NAME_INTERNAL2 "PSPLoader"
#define MODULE_NAME "SplinterCellEssentials.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionMod/SplinterCellEssentials.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/SplinterCellEssentials.PPSSPP.FusionMod/SplinterCellEssentials.PPSSPP.FusionMod.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);
#endif

#define DEADZONE 0.3f

static float getAxis(float axis, float deadzone, float speed) {
    if (fabsf(axis) >= deadzone) {
        if (axis < 0.0f) {
            return speed * ((axis + deadzone) / (1.0f - deadzone));
        }
        else {
            return speed * ((axis - deadzone) / (1.0f - deadzone));
        }
    }

    return 0.0f;
}

void __0fGUInputKDirectAxis6JEInputKeyfTCPatched(int _this, int axis, int unk) {
    int param = *(uint32_t*)(_this) + 0x130;
    int arg1 = _this + *(uint16_t*)(param);
    int arg2 = *(uint32_t*)(*(uint32_t*)(_this + 0x12A0) + 0x34);

    float* (*__0fGUInputMFindAxisNameP6GAActorPCcK)(int arg1, int arg2, const char* name);
    __0fGUInputMFindAxisNameP6GAActorPCcK = (void*)(0x80000000 | *(uint32_t*)(param + 4));
    if (__0fGUInputMFindAxisNameP6GAActorPCcK) {
        SceCtrlData pad;
        sceCtrlPeekBufferPositive(&pad, 1);

        if (axis == 0xE0) {
            float Lx = ((float)pad.Lx - 128.0f) / 128.0f;
            float Rx = ((float)pad.Rsrv[0] - 128.0f) / 128.0f;

            float* aStrafe = __0fGUInputMFindAxisNameP6GAActorPCcK(arg1, arg2, "aStrafe");
            float* aTurn = __0fGUInputMFindAxisNameP6GAActorPCcK(arg1, arg2, "aTurn");

            if (aStrafe)
                *aStrafe = getAxis(Lx, DEADZONE, 1.0f);
            if (aTurn)
                *aTurn = getAxis(Rx, DEADZONE, 1.0f);
        }
        else if (axis == 0xE1) {
            float Ly = -(((float)pad.Ly - 128.0f) / 128.0f);
            float Ry = -(((float)pad.Rsrv[1] - 128.0f) / 128.0f);

            float* aForward = __0fGUInputMFindAxisNameP6GAActorPCcK(arg1, arg2, "aForward");
            float* aLookUp = __0fGUInputMFindAxisNameP6GAActorPCcK(arg1, arg2, "aLookUp");

            if (aForward)
                *aForward = getAxis(Ly, DEADZONE, 1.0f);
            if (aLookUp)
                *aLookUp = getAxis(Ry, DEADZONE, -1.0f);
        }
    }
}

int PSPLoaderHandler()
{
    //int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 0);

    //if (SkipIntro)
    {
        uintptr_t ptr = pattern.get(0, "25 28 00 00 25 30 00 00 25 38 00 00", 12);
        injector.MakeNOP(ptr);
        //ptr = pattern.get(1, "25 28 00 00 25 30 00 00 25 38 00 00", 12);
        //injector.MakeNOP(ptr); //game gets stuck if no intros are played
    }

    return 0;
}

int OnModuleStart() 
{
    uintptr_t ptr = pattern.get_first("B0 FF BD 27 B0 03 8C C4", 0);

    if (!ptr)
        return 0;

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);

    if (DualAnalogPatch)
    {
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

        uintptr_t ptr = pattern.get_first("B0 FF BD 27 B0 03 8C C4", 0);
        injector.WriteInstr(ptr + 0x00, jr(ra));
        injector.WriteInstr(ptr + 0x04, li(v0, 0));

        ptr = pattern.get_first("70 FF BD 27 68 00 B0 AF", 0);
        injector.MakeJMP(ptr, (uintptr_t)__0fGUInputKDirectAxis6JEInputKeyfTCPatched);
    }

    if (Enable60FPS)
    {
        float f = 1.0f / 75.0f;
        uintptr_t ptr = pattern.get_first("08 3D 05 3C 89 88 A5 34", 0);
        injector.MakeInlineLUIORI(ptr, f);

        ptr = pattern.get_first("50 00 BE C7 54 00 B0 8F 58 00 B1 8F", 0);
        MakeInlineWrapper(ptr,
            lui(s4, 0x880),
            sw(s4, s4, 0x3900),
            lw(s1, sp, 0x50),
            lui(s2, 0x3D09),
            addiu(s2, s2, -0x7777),
            bne(s1, s2, 3), //-->
            lui(s3, HIWORD(f)),
            ori(s3, s3, LOWORD(f)),
            mtc1(s3, f30),
            lwc1(f30, sp, 0x50) // <--
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

                if (strcmp(info.name, MODULE_NAME_INTERNAL2) == 0)
                {
                    injector.SetGameBaseAddress(info.text_addr, info.text_size);
                    pattern.SetGameBaseAddress(info.text_addr, info.text_size);
                    //inireader.SetIniPath(INI_PATH);
                    //logger.SetPath(LOG_PATH);
                    return PSPLoaderHandler();
                }
                else if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
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