#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <systemctrl.h>
#include <math.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#define MODULE_NAME_INTERNAL "FireTeamBravo3"
#define MODULE_NAME_INTERNAL2 "SocomTacticsLoader"
#define MODULE_NAME "FireteamBravo3.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionMod/SOCOM.FireteamBravo3.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/SOCOM.FireteamBravo3.PPSSPP.FusionMod/SOCOM.FireteamBravo3.PPSSPP.FusionMod.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
_Static_assert(sizeof(MODULE_NAME) - 1 < 28, "MODULE_NAME can't have more than 28 characters");
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

int bCameraMode = 0;
int bCameraRelated = 0;
float gRSx = 0.0f;
float gRSy = 0.0f;
float gLSx = 0.0f;
float gLSy = 0.0f;
int sceCtrlPeekBufferPositiveHook(SceCtrlData* pad_data, int count)
{
    int r = sceCtrlPeekBufferPositive(pad_data, count);
    gRSx = (float)((float)pad_data->Rsrv[0] * 0.0078125f) - 1.0f;
    gRSy = (float)((float)pad_data->Rsrv[1] * 0.0078125f) - 1.0f;

    gLSx = (float)((float)pad_data->Lx * 0.0078125f) - 1.0f;
    gLSy = (float)((float)pad_data->Ly * 0.0078125f) - 1.0f;

    if (bCameraMode)
    {
        pad_data->Lx = pad_data->Rsrv[0];
        pad_data->Ly = pad_data->Rsrv[1];
    }

    return r;
}

void GetStickData(int a1, float* outX, float* outY, int a4, int a5)
{
    float v6 = gRSx;
    float v7 = -gRSy;

    if (!bCameraRelated)
    {
        int v5 = *(uintptr_t*)(*(uintptr_t*)a1 + 104);
        float v6 = *(float*)(v5 + 280);
        float v7 = -*(float*)(v5 + 284);
    }

    float v8 = fabsf(v7) * 20.0f;
    float v9 = fabsf(v6) * 20.0f;

    // Clamp values between 0 and 19
    int v10 = (int)v8;
    v10 = (v10 < 0) ? 0 : (v10 > 19 ? 19 : v10);

    int v11 = (int)v9;
    v11 = (v11 < 0) ? 0 : (v11 > 19 ? 19 : v11);

    // Clamp values between 0 and 19
    int v14 = v10 + 1;
    v14 = (v14 < 0) ? 0 : (v14 > 19 ? 19 : v14);

    int v15 = v11 + 1;
    v15 = (v15 < 0) ? 0 : (v15 > 19 ? 19 : v15);

    // Clamp values between 0.0 and 1.0
    float v16 = v8 - (float)v10;
    v16 = (v16 < 0.0) ? 0.0 : (v16 > 1.0 ? 1.0 : v16);

    float v17 = v8 - (float)v10;
    v17 = (v17 < 0.0) ? 0.0 : (v17 > 1.0 ? 1.0 : v17);

    int v18 = 4 * v10;

    *outX = (float)(*(float*)(a5 + v18) * (float)(1.0 - v16)) + (float)(*(float*)(a5 + 4 * v14) * v16);
    *outY = (float)(*(float*)(a4 + 4 * v11) * (float)(1.0 - v17)) + (float)(*(float*)(a4 + 4 * v15) * v17);

    if (v7 < 0.0f)
        *outX = -*outX;
    if (v6 < 0.0f)
        *outY = -*outY;
}

void GetStickData2(int a1, float* outX, float* outY, int a4, int a5)
{
    bCameraRelated = 1;
    GetStickData(a1, outX, outY, a4, a5);
    bCameraRelated = 0;
}

int (*sub_89DB34C)(int, int, double);
int sub_89DB34CHook(int a1, int a2, double a3)
{
    bCameraRelated = 1;
    int r = sub_89DB34C(a1, a2, a3);
    bCameraRelated = 0;
    bCameraMode = *(int*)(*(int*)(*(int*)(*(int*)(a1 + 4) + 60) + 660) + 1212);
    return r;
}

int sub_89DB34CHook2(int a1, int a2, double a3)
{
    bCameraRelated = 0;
    int r = sub_89DB34C(a1, a2, a3);
    bCameraRelated = 0;
    bCameraMode = *(int*)(*(int*)(*(int*)(*(int*)(a1 + 4) + 60) + 660) + 1212);
    return r;
}

void (*sub_891A188)(int a1, float* a2);
void sub_891A188Hook(int a1, float* a2)
{
    sub_891A188(a1, a2);

    //a2[0] = a2[0];
    //a2[1] = gRSy;
    a2[2] = gLSx;
    a2[3] = -gRSx;
}

int UnthrottleEmuDuringLoading = 0;

int sceKernelSetEventFlagHook(SceUID evid, u32 bits)
{
    UnthrottleEmuEnable();
    return sceKernelSetEventFlag(evid, bits);
}

int sceKernelClearEventFlagHook(SceUID evid, u32 bits)
{
    UnthrottleEmuDisable();
    return sceKernelClearEventFlag(evid, bits);
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int SkipIntro = 0;
int DualAnalogPatch = 0;
float fFOVFactor = 0.0f;

uintptr_t dword_8C877CC;
float (*sub_88851DC)(int a1);
float sub_88851DC_hook(int a1)
{
    return -(gRSy) * sub_88851DC(a1);
}
float (*sub_888A12C)(int a1, float a2, float* a3, float* a4);
float sub_888A12C_hook(int a1, float a2, float* a3, float* a4)
{
    float fRSx = gRSx / 10.0f;
    float fRSy = gRSy / 10.0f;

    if (*(uint8_t*)(*(uint32_t*)dword_8C877CC + 49)) // y inv
    {
        fRSy = -fRSy;
    }

    float sens = sub_88851DC(a1);

    float y = *(float*)(a1 + 208) + -(fRSy * sens);


    if (y > 0.921875f)
        y = 0.921875f;
    else if (y < -1.3f)
        y = -1.3f;

    float f26 = *(float*)(a1 + 212) + -(fRSx * sens);

    asm volatile ("lw $a1,  %[x]" ::[x] "m" (f26));
    asm volatile ("mtc1 $a1, $f26");

    return y;
}

float sub_8A79B24(float* a1, float a2)
{
    float result = a1[3];
    float diff1 = a1[4] - result;
    float diff2 = a1[2] - result;
    a1[4] = result;
    float prod1 = a1[1] * diff1;
    float prod2 = *a1 * diff2;
    if (fabsf(prod1) < fabsf(prod2))
    {
        result += a2 * (prod1 + prod2);
        a1[3] = result;
    }
    return result;
}

void (*sub_88871B4)(float* a1, float a2);
void sub_88871B4_hook(float* a1, float a2)
{
    int i = 0;
    float* v4 = a1;
    do
    {
        if (i == 2) // smoothing for default cam
            a1[198] = 1.0f;
        else
            a1[198] = sub_8A79B24(v4, a2);
        ++i;
        v4 += 6;
        ++a1;
    } while (i < 33);
}

float* flt_8CCA2C0;
void sub_888717C_hook(int a1, float* a2)
{
    if (fFOVFactor)
    {
        float* defCamData = &flt_8CCA2C0[33 * 0];
        defCamData[1] = 22.0f * fFOVFactor;
    }

    int v4 = 0;
    float* v5 = a2;
    do
    {
        ++v4;
        *(float*)(a1 + 8) = *v5;
        a1 += 24;
        ++v5;
    } while (v4 < 33);
}

int OnModuleStart() 
{
    SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    UnthrottleEmuDuringLoading = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1);
    fFOVFactor = inireader.ReadFloat("MAIN", "FOVFactor", 0.0f);
    if (fFOVFactor < 0.0f)
        fFOVFactor = 0.0f;
    if (fFOVFactor > 2.5f)
        fFOVFactor = 2.5f;

    if (SkipIntro)
    {
        //"HealthWarningScreen"
        uintptr_t ptr_88950F4 = pattern.get(0, "18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? ? ? ? ? 18 00 04 8E", -8);
        injector.MakeNOP(ptr_88950F4);
        //"SCEScreen"
        uintptr_t ptr_8895108 = pattern.get(0, "18 00 04 8E 01 00 06 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00", -12);
        injector.MakeNOP(ptr_8895108);
        //"MovieScreen"
        uintptr_t ptr_889511C = pattern.get(0, "00 00 00 00 ? ? ? ? 18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 80 3F 04 3C", -12);
        injector.MakeNOP(ptr_889511C);
        //"USNScreen"
        uintptr_t ptr_889513C = pattern.get(0, "18 00 04 8E ? ? ? ? 01 00 06 34 ? ? ? ? ? ? ? ? 80 3F 04 3C", -8);
        injector.MakeNOP(ptr_889513C);
    }
    
    if (DualAnalogPatch)
    {
        // Grab Right Stick data
        uintptr_t ptr_8A7A6F8 = pattern.get(0, "01 00 05 34 04 00 A5 8F 10 00 A4 30", -4);
        injector.MakeJAL(ptr_8A7A6F8, (uintptr_t)sceCtrlPeekBufferPositiveHook);

        uintptr_t ptr_8A7A834 = pattern.get(0, "08 00 A4 93 00 00 4C E6", -4);
        injector.WriteInstr(ptr_8A7A834, b(5));

        // Movement
        uintptr_t ptr_89D9978 = pattern.get(0, "25 28 A0 03 25 20 00 02 06 A3 00 46", -4);
        sub_89DB34C = (int(*)(int, int, double))injector.MakeJAL(ptr_89D9978, (uintptr_t)sub_89DB34CHook2);
        uintptr_t ptr_89DBB7C = pattern.get(0, "25 28 A0 03 04 00 05 8E", -4);

        // Look
        sub_89DB34C = (int(*)(int,int,double))injector.MakeJAL(ptr_89DBB7C, (uintptr_t)sub_89DB34CHook);
        uintptr_t ptr_89DAA64 = pattern.get(0, "25 28 A0 03 ? ? ? ? 00 00 00 00 04 00 04 8E", -4);
        sub_89DB34C = (int(*)(int, int, double))injector.MakeJAL(ptr_89DAA64, (uintptr_t)sub_89DB34CHook);

        // Movement
        uintptr_t ptr_89DB5BC = pattern.get(0, "25 28 00 02 03 00 15 34", -4);
        sub_891A188 = (void(*)(int, float*))injector.MakeJAL(ptr_89DB5BC, (uintptr_t)sub_891A188Hook);
        uintptr_t ptr_89DB5D4 = pattern.get(0, "25 28 00 02 00 00 15 34", -4);
        sub_891A188 = (void(*)(int, float*))injector.MakeJAL(ptr_89DB5D4, (uintptr_t)sub_891A188Hook);

        // DPadUP camera mode
        uintptr_t ptr_89DBD4C = pattern.get(0, "25 40 40 00 ? ? ? ? 25 20 40 02", -4);
        injector.MakeJAL(ptr_89DBD4C, (uintptr_t)GetStickData2);

        // Custom camera movement
        uintptr_t ptr_88877D4 = pattern.get(0, "00 00 BA E7 25 20 00 02", -4);
        sub_888A12C = (float(*)(int, float, float*, float*))injector.MakeJAL(ptr_88877D4 + 12, (uintptr_t)sub_888A12C_hook);

        uintptr_t ptr_88877F0 = pattern.get(0, "00 00 BA C7 01 00 04 34", -0);
        injector.WriteInstr(ptr_88877F0, swc1(f26, sp, 0));

        // Cam smoothing disable
        uintptr_t ptr_8887758 = pattern.get(0, "06 A3 00 46 38 04 11 26", -4);
        sub_88871B4 = (void (*)(float*, float))injector.MakeJAL(ptr_8887758, (uintptr_t)sub_88871B4_hook);

        // FOV
        uintptr_t ptr_888773C = pattern.get(0, "20 01 16 26", -8);
        flt_8CCA2C0 = (float*)GetAbsoluteAddress(ptr_888773C, 0, 4);

        uintptr_t ptr_888774C = pattern.get(0, "25 20 C0 02 25 20 C0 02 ? ? ? ? 06 A3 00 46", -4);
        injector.MakeJAL(ptr_888774C, (uintptr_t)sub_888717C_hook);

        // Binoculars cam RSy support
        uintptr_t ptr_89DAA84 = pattern.get(0, "87 65 00 46", -4);
        sub_88851DC = (float(*)(int))injector.MakeJAL(ptr_89DAA84, (uintptr_t)sub_88851DC_hook);
        injector.MakeNOP(ptr_89DAA84 + 8);

        uintptr_t ptr_89DAA90 = pattern.get(0, "31 00 84 90 ? ? ? ? 02 A5 00 46", -8);
        dword_8C877CC = (uintptr_t)GetAbsoluteAddress(ptr_89DAA90, 0, 4);
    }

    if (UnthrottleEmuDuringLoading)
    {
        uintptr_t ptr_881AB4C = pattern.get(0, "01 00 05 34 ? ? ? ? 25 20 40 00 ? ? ? ? ? ? ? ? ? ? ? ? 25 28 00 00", -4);
        injector.MakeJAL(ptr_881AB4C, (uintptr_t)sceKernelSetEventFlagHook);
        uintptr_t ptr_881AC38 = pattern.get(0, "01 00 05 34 ? ? ? ? 25 20 40 00 00 00 BF 8F", -4);
        injector.MakeJAL(ptr_881AC38, (uintptr_t)sceKernelSetEventFlagHook);
        uintptr_t ptr_881AD1C = pattern.get(0, "25 28 00 00 ? ? ? ? 25 20 40 00 ? ? ? ? ? ? ? ? ? ? ? ? 01 00 14 34", -4);
        injector.MakeJAL(ptr_881AD1C, (uintptr_t)sceKernelClearEventFlagHook);
    }

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
}

void SetModuleGP()
{
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

            if (strcmp(info.name, MODULE_NAME) == 0)
            {
                injector.SetGP((void*)info.text_addr);
                injector.SetModuleBaseAddress(info.text_addr, info.text_size);
            }
        }
    }
}

int sceKernelStartModuleHook(SceUID modid, SceSize argsize, void* argp, int* status, SceKernelSMOption* option)
{
    int r = sceKernelStartModule(modid, argsize, argp, status, option);
    SceKernelModuleInfo info;
    info.size = sizeof(SceKernelModuleInfo);
    if (sceKernelQueryModuleInfo(modid, &info) >= 0)
    {
        if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
        {
            SetModuleGP();
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
    uintptr_t ptr_880438C = pattern.get(0, "25 40 00 00 ? ? ? ? 28 00 A2 AF", -4);
    injector.MakeJAL(ptr_880438C, (uintptr_t)sceKernelStartModuleHook);
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