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

#define MODULE_NAME_INTERNAL "APP_APPLICATION_NAME"
#define MODULE_NAME_INTERNAL2 "SocomPSPLoader"
#define MODULE_NAME "SOCOM.FireteamBravo2.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionMod/SOCOM.FireteamBravo2.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo2.PPSSPP.FusionMod/SOCOM.FireteamBravo2.PPSSPP.FusionMod.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
#endif

enum
{
    EMULATOR_DEVCTL__TOGGLE_FASTFORWARD = 0x30,
    EMULATOR_DEVCTL__GET_ASPECT_RATIO,
    EMULATOR_DEVCTL__GET_SCALE
};

void UnthrottleEmuEnable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)1, 0, NULL, 0);
}

void UnthrottleEmuDisable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)0, 0, NULL, 0);
}

struct InputData
{
    SceCtrlData ctrlData;
    int buttonsData;
    char field_14;
    char field_15;
    char field_16;
    char field_17;
    float field_18;
    float field_1C;
    float LSx;
    float LSy;
    float fDeadZone;
    float fSensitivity;
    float field_30;
    //...
};

float LeftStickControls(struct InputData* a1, int axis, char isCamera)
{
    float value = 0.0f;
    if (isCamera == 0)
    {
        if (axis == 0)
            value = a1->LSx;
        else
            value = a1->LSy;
    }
    else
    {
        if (axis == 0)
            value = a1->field_18;
        else
            value = a1->field_1C;
    }

    if (axis == 0)
        return value;

    return -value;
}

float gRSx = 0.0f;
float gRSy = 0.0f;
float RightStickControls(struct InputData* a1, int axis, char isCamera)
{
    float value = 0.0f;
    if (isCamera == 0)
    {
        if (axis == 0)
            value = gRSx;
        else
            value = gRSy;
    }
    else
    {
        if (axis == 0)
            value = gRSx; //a1->field_18;
        else
            value = gRSy; //a1->field_1C;
    }

    if (axis == 0)
        return value;

    return -value;
}

int (*sub_881630C)(struct InputData* input, float a2);
int sub_881630C_Hook(struct InputData* input, float a2)
{
    int res = sub_881630C(input, a2);

    {
        int vRx, vRy;
        float RxValue, RyValue;
        unsigned int Buttons = input->ctrlData.Buttons;
        //sceCtrlPeekBufferPositive(&input->ctrlData, 1);
        unsigned char RxInput = input->ctrlData.Rsrv[0];
        int RSx = RxInput - 128;
        int bHasInput = (input->ctrlData.Buttons ^ Buttons) != 0;
        int RSy = input->ctrlData.Rsrv[1] - 128;
        if (RSx < -127)
            RSx = -127;
        if (RSy < -127)
            RSy = -127;
        if (RSx < -(int)input->fDeadZone || (int)input->fDeadZone < RSx)
        {
            bHasInput = 1;
            if (RSx <= 0)
                vRx = RSx + (int)input->fDeadZone;
            else
                vRx = RSx - (int)input->fDeadZone;
            RxValue = (float)vRx;
        }
        else
        {
            RxValue = 0.0f;
        }
        if (RSy < -(int)input->fDeadZone || (int)input->fDeadZone < RSy)
        {
            bHasInput = 1;
            if (RSy <= 0)
                vRy = RSy + (int)input->fDeadZone;
            else
                vRy = RSy - (int)input->fDeadZone;
            RyValue = (float)vRy;
        }
        else
        {
            RyValue = 0.0f;
        }
        float fSensitivity = input->fSensitivity;
        gRSx = fSensitivity * RxValue;
        gRSy = fSensitivity * RyValue;
    }

    return res;
}

int UnthrottleEmuDuringLoading = 0;

int sceKernelExitDeleteThreadHook(int status)
{
    UnthrottleEmuDisable();
    return sceKernelExitDeleteThread(status);
}

int sceKernelTerminateDeleteThreadHook(SceUID thid)
{
    UnthrottleEmuDisable();
    return sceKernelTerminateDeleteThread(thid);
}

int* dw8CB4F3C = 0;
void sub_8862708(int a1, int a2, int a3, int a4)
{
    *dw8CB4F3C = 2;
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int SkipIntro = 0;
int DualAnalogPatch = 0;

int OnModuleStart() 
{
    //sceKernelDelayThread(100000);

    if (SkipIntro)
    {
        uintptr_t ptr_886266C = pattern.get(0, "08 00 E0 03 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 02 00 06 34", -4);
        dw8CB4F3C = (int*)GetAbsoluteAddress(ptr_886266C, 0, 8);
    
        uintptr_t ptr_88628E8 = pattern.get(0, "25 38 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? 0D 00 06 34", -4);
        injector.MakeJAL(ptr_88628E8, (uintptr_t)sub_8862708);
        uintptr_t ptr_8862A08 = pattern.get(0, "14 00 B0 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 48 01 B0 AF", -24);
        injector.MakeJAL(ptr_8862A08, (uintptr_t)sub_8862708);
    }
    
    if (DualAnalogPatch)
    {
        // Grab right stick input and process it similarly to how left stick is processed
        uintptr_t ptr_8B57BDC = pattern.get(0, "06 B3 00 46 0B 00 44 2E", -4);
        sub_881630C = (int(*)(struct InputData*, float))injector.MakeJAL(ptr_8B57BDC, (uintptr_t)sub_881630C_Hook);
        uintptr_t ptr_8815BCC = pattern.get(0, "10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 10 00 BF AF ? ? ? ? 25 28 00 00 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? 1C 00 B3 AF", -8);
        sub_881630C = (int(*)(struct InputData*, float))injector.MakeJAL(ptr_8815BCC, (uintptr_t)sub_881630C_Hook);
        
        //CameraControlsLeftRight
        uintptr_t ptr_889D880 = pattern.get(0, "01 00 06 34 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF", -4);
        injector.MakeJAL(ptr_889D880, (uintptr_t)RightStickControls);
        //CameraControlsUpDown
        uintptr_t ptr_889DD48 = pattern.get(0, "01 00 06 34 80 BF 04 3C", -4);
        injector.MakeJAL(ptr_889DD48, (uintptr_t)RightStickControls);
        
        // Enable strafe without button
        uintptr_t ptr_889D918 = pattern.get(0, "00 00 80 44 25 28 00 00 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF ? ? ? ? ? ? ? ? 10 00 B0 AF 25 80 80 00 20 00 A4 24 78 00 85 8C", -4);
        uintptr_t ptr_889D7AC = pattern.get(0, "00 00 80 44 01 00 05 34 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 25 28 80 00", -4);
        uintptr_t ptr_8A41C84 = pattern.get(0, "06 B7 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 80 03 44 26", -0);
        uintptr_t ptr_8A41C54 = pattern.get(0, "1D 00 04 34 ? ? ? ? 25 28 20 02", -4);
        injector.MakeNOP(ptr_889D918);
        injector.MakeNOP(ptr_889D7AC);
        injector.MakeNOP(ptr_8A41C84);
        injector.MakeNOP(ptr_8A41C54);
    }

    if (UnthrottleEmuDuringLoading)
    {
        uintptr_t ptr_8B578EC = pattern.get(0, "80 4F 04 3C 00 60 84 44 ? ? ? ? ? ? ? ? 00 D0 80 44", 0);
    	logger.WriteF("ptr_8B578EC: 0x%08X\n", ptr_8B578EC);
        MakeInlineWrapper(ptr_8B578EC,
            lui(a0, 0x4F80),
            jal((intptr_t)UnthrottleEmuEnable),
            nop()
        );
        
        uintptr_t ptr_8B57C98 = pattern.get(0, "25 20 00 00 25 10 00 00 24 00 B4 C7", -4);
        injector.MakeJAL(ptr_8B57C98, (uintptr_t)sceKernelExitDeleteThreadHook);
        uintptr_t ptr_8B57834 = pattern.get(0, "00 00 00 00 ? ? ? ? ? ? ? ? FF FF 04 24 ? ? ? ? 10 00 B0 8F", -12);
        injector.MakeJAL(ptr_8B57834, (uintptr_t)sceKernelTerminateDeleteThreadHook);
    }

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
}

int sceKernelStartModuleHook(SceUID modid, SceSize argsize, void* argp, int* status, SceKernelSMOption* option)
{
    SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    UnthrottleEmuDuringLoading = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1);

    int r = sceKernelStartModule(modid, argsize, argp, status, option);
    SceKernelModuleInfo info;
    info.size = sizeof(SceKernelModuleInfo);
    if (sceKernelQueryModuleInfo(modid, &info) >= 0)
    {
        if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
        {
            injector.SetGameBaseAddress(info.text_addr, info.text_size);
            pattern.SetGameBaseAddress(info.text_addr, info.text_size);
            inireader.SetIniPath(INI_PATH);
            logger.SetPath(LOG_PATH);
            OnModuleStart();
        }
    }
    return r;
}

int PSPLoaderHandler()
{
    uintptr_t ptr_88041F8 = pattern.get(0, "25 40 00 00 34 00 A2 AF", -4);
    injector.MakeJAL(ptr_88041F8, (uintptr_t)sceKernelStartModuleHook);
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
                else if (strcmp(info.name, MODULE_NAME) == 0)
                {
                    injector.SetModuleBaseAddress(info.text_addr, info.text_size);
                }
            }
        }
    }
    return 0;
}