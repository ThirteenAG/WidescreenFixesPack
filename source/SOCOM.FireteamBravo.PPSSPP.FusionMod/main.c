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
#define MODULE_NAME "SOCOM.FireteamBravo.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionMod/SOCOM.FireteamBravo.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo.PPSSPP.FusionMod/SOCOM.FireteamBravo.PPSSPP.FusionMod.ini"

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

int (*sub_8809B54)(struct InputData* input, float a2);
int sub_8809B54_Hook(struct InputData* input, float a2)
{
    int res = sub_8809B54(input, a2);

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

int* dword_8C26428;
void sub_883B9A0(int a1, int a2, int a3, int a4, int a5)
{
    *dword_8C26428 = 2;
}

uintptr_t ptr_890F6D0;
int gSkipThisDialogueOnce = 0;
int gSkipThisDialogue = 0;
int (*sub_88577C8)(int* a1, struct InputData input);
int sub_88577C8Hook(int* a1, struct InputData input)
{
    gSkipThisDialogue = 0;
    int v7 = a1[206];

    if (!gSkipThisDialogueOnce && v7 == ptr_890F6D0)
    {
        gSkipThisDialogue = 1;
        gSkipThisDialogueOnce = 1;
    }

    return sub_88577C8(a1, input);
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int OnModuleStart() 
{
    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    UnthrottleEmuDuringLoading = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1);

    if (SkipIntro)
    {
        uintptr_t ptr_883BC3C = pattern.get(0, "02 00 11 34 ? ? ? ? 00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 03 00 04 34", -8);
        dword_8C26428 = (int*)GetAbsoluteAddress(ptr_883BC3C, 0, 4);

        uintptr_t ptr_883BB24 = pattern.get(0, "20 00 BF 8F 08 00 E0 03 30 00 BD 27 ? ? ? ? 20 00 BF 8F 08 00 E0 03 30 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 20 00 B0 AF", -8);
        injector.MakeJAL(ptr_883BB24, (uintptr_t)sub_883B9A0);
        uintptr_t ptr_883BBD0 = pattern.get(0, "20 00 B0 8F 24 00 BF 8F 08 00 E0 03 30 00 BD 27 ? ? ? ? 28 01 B0 AF", -24);
        injector.MakeJAL(ptr_883BBD0, (uintptr_t)sub_883B9A0);

        int* addr = &gSkipThisDialogue;
        uintptr_t ptr_88577F8 = pattern.get(0, "2C 00 A7 A3 38 00 B0 AF", -0);
        MakeInlineWrapper(ptr_88577F8,
            lui(a3, HIWORD(addr)),
            ori(a3, a3, LOWORD(addr)),
            lw(a3, a3, 0),
            sb(a3, sp, 0x2C),
            li(a3, 0)
        );

        uintptr_t ptr_8856EFC = pattern.get(0, "74 00 18 E6 ? ? ? ? 00 00 04 AE ? ? ? ? ? ? ? ? 68 00 04 AE 7C 00 13 26", -4);
        uintptr_t* unk_8C58ADC = (uintptr_t*)GetAbsoluteAddress(ptr_8856EFC, 0, 8);

        uintptr_t ptr_88577C8 = pattern.get(0, "00 00 06 34 00 00 07 34 2E 00 A6 A3", -4);
        sub_88577C8 = (int (*)(int*, struct InputData))ptr_88577C8;
        injector.WriteMemory32(&unk_8C58ADC[33], (uint32_t)sub_88577C8Hook); //0x08C58B60

        ptr_890F6D0 = pattern.get(0, "18 00 B1 AF 25 88 80 00 02 00 04 34", -4);
    }
    
    if (DualAnalogPatch)
    {
        // Grab right stick input and process it similarly to how left stick is processed
        uintptr_t ptr_8809724 = pattern.get(0, "10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 10 00 BF AF ? ? ? ? 25 28 00 00 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? ? ? ? ? 1C 00 B3 AF", -8);
        sub_8809B54 = (int (*)(struct InputData*, float))injector.MakeJAL(ptr_8809724, (uintptr_t)sub_8809B54_Hook);

        //CameraControlsLeftRight
        uintptr_t ptr_88885DC = pattern.get(0, "01 00 06 34 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF", -4);
        injector.MakeJAL(ptr_88885DC, (uintptr_t)RightStickControls);
        //CameraControlsUpDown
        uintptr_t ptr_8888ADC = pattern.get(0, "01 00 06 34 80 BF 04 3C", -4);
        injector.MakeJAL(ptr_8888ADC, (uintptr_t)RightStickControls);
        
        // Enable strafe without button
        uintptr_t ptr_8888674 = pattern.get(0, "00 00 80 44 25 28 00 00 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 14 00 B1 AF ? ? ? ? ? ? ? ? 10 00 B0 AF 25 80 80 00 20 00 A4 24 78 00 85 8C", -4);
        injector.MakeNOP(ptr_8888674);
        uintptr_t ptr_8888508 = pattern.get(0, "00 00 80 44 01 00 05 34 ? ? ? ? 25 30 00 00 10 00 B0 8F 14 00 B1 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 25 28 80 00", -4);
        injector.MakeNOP(ptr_8888508);
        uintptr_t ptr_89DDF9C = pattern.get(0, "86 B6 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 84 03 44 26", -0);
        injector.MakeNOP(ptr_89DDF9C);
        uintptr_t ptr_89DDF6C = pattern.get(0, "1D 00 04 34 ? ? ? ? 25 28 20 02", -4);
        injector.MakeNOP(ptr_89DDF6C);

        // Enable strafe in free look mode
        uintptr_t ptr_89DE278 = pattern.get(0, "04 00 04 26 ? ? ? ? 00 00 00 00 ? ? ? ? 82 D6 00 46", -4);
        injector.WriteInstr(ptr_89DE278, b(11));
        //uintptr_t ptr_89DE240 = pattern.get(0, "25 20 60 02 ? ? ? ? 00 00 00 00 ? ? ? ? 02 C6 00 46", -4);
        //injector.WriteInstr(ptr_89DE240, b(11));
    }

    if (UnthrottleEmuDuringLoading)
    {
        uintptr_t ptr_8A9C224 = pattern.get(0, "80 4F 04 3C 00 A0 84 44", -0);
        MakeInlineWrapper(ptr_8A9C224,
            lui(a0, 0x4F80),
            jal((intptr_t)UnthrottleEmuEnable),
            nop()
        );
        
        uintptr_t ptr_8A9C368 = pattern.get(0, "25 20 00 00 25 10 00 00 10 00 B4 C7", -4);
        injector.MakeJAL(ptr_8A9C368, (uintptr_t)sceKernelExitDeleteThreadHook);
        uintptr_t ptr_8A9C17C = pattern.get(0, "10 00 B0 8F 14 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 00 00 BF AF ? ? ? ? 00 00 00 00 ? ? ? ? BB 44 04 3C", -24);
        injector.MakeJAL(ptr_8A9C17C, (uintptr_t)sceKernelTerminateDeleteThreadHook);
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