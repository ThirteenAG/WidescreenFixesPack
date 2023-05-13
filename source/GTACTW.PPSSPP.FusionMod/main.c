#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <systemctrl.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/nanoprintf.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#define MODULE_NAME_INTERNAL "CTW"
#define MODULE_NAME "GTACTW.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionMod/GTACTW.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionMod/GTACTW.PPSSPP.FusionMod.ini"
#define DAT_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionMod/GTACTW.PPSSPP.FusionMod.dat"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_KERNEL, 1, 0); // PSP_MODULE_USER crashes for some reason
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

uint32_t cPed__Vehicle(uint32_t a1)
{
    return **(uint32_t**)(a1 + 0x1CC);
}

int* gPlayers;
int gLocalPlayerId;
int FindPlayerVehicle()
{
    return cPed__Vehicle(gPlayers[gLocalPlayerId]);
}

char IsWidescreen()
{
    return *(char*)((uintptr_t)gPlayers[gLocalPlayerId] + 0xE31);
}

char bCamModeLastState;
char bCamModeCurState;
char bCamResetLastState;
char bCamResetCurState;
char CamMode = 0;
short CamAngle = 0;
int CamZ = 13464;
char refresh_dat = 0;

void WriteDAT()
{
    SceUID dat_uid = sceIoOpen(DAT_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (dat_uid > 0)
    {
        char buffer[100];
        npf_snprintf(buffer, sizeof(buffer), "%d %d %d", CamMode, CamAngle, CamZ);
        logger.Write(buffer);
        sceIoWrite(dat_uid, buffer, strlen(buffer));
        sceIoClose(dat_uid);
    }
}

void ReadDAT()
{
    SceUID dat_uid = sceIoOpen(DAT_PATH, PSP_O_RDONLY, 0777);
    if (dat_uid > 0)
    {
        char buffer[100];
        sceIoRead(dat_uid, buffer, sizeof(buffer));
        char* pch = strtok(buffer, " ");
        CamMode = str2int(pch, 10);
        pch = strtok(NULL, " ");
        CamAngle = str2int(pch, 10);
        pch = strtok(NULL, " ");
        CamZ = str2int(pch, 10);
        sceIoClose(dat_uid);
    }
}

int sub_8933468(short* a1, short a2)
{
    int (*fn_sub_8933468)(short* a1, short a2) = 0x8933468;

    short x = -(a2 + 16384);

    if (FindPlayerVehicle() && !IsWidescreen())
    {
        SceCtrlData pad;
        sceCtrlSetSamplingCycle(0);
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
        sceCtrlPeekBufferPositive(&pad, 1);

        bCamModeCurState = pad.Buttons & PSP_CTRL_RIGHT;
        if (!bCamModeLastState && bCamModeCurState)
        {
            if (CamMode == 1)
                CamMode = 0;
            else
                CamMode = 1;
            WriteDAT();
        }
        bCamModeLastState = bCamModeCurState;

        if (CamMode == 1)
        {
            bCamResetCurState = pad.Buttons & PSP_CTRL_UP;
            if (!bCamResetLastState && bCamResetCurState)
            {
                CamAngle = 0;
                CamZ = 13464;
                refresh_dat = 1;
            }
            bCamResetLastState = bCamResetCurState;

            if (pad.Rsrv[0] <= 118)
            {
                CamAngle += -(pad.Rsrv[0] - 118) * 8;
                if (CamAngle > 0)
                    CamAngle = 0;
                if (CamAngle < -10000)
                    CamAngle = -10000;
                refresh_dat = 1;
            }
            else if (pad.Rsrv[0] >= 138)
            {
                CamAngle -= (pad.Rsrv[0] - 138) * 8;
                if (CamAngle > 0)
                    CamAngle = 0;
                if (CamAngle < -10000)
                    CamAngle = -10000;
                refresh_dat = 1;
            }

            if (pad.Rsrv[1] <= 118)
            {
                CamZ += -(pad.Rsrv[1] - 118) * 8;
                if (CamZ > 120000)
                    CamZ = 120000;
                if (CamZ < 13464)
                    CamZ = 13464;
                refresh_dat = 1;
            }
            else if (pad.Rsrv[1] >= 138)
            {
                CamZ -= (pad.Rsrv[1] - 138) * 8;
                if (CamZ > 120000)
                    CamZ = 120000;
                if (CamZ < 13464)
                    CamZ = 13464;
                refresh_dat = 1;
            } 
            
            if (pad.Rsrv[0] > 118 && pad.Rsrv[0] < 138 && pad.Rsrv[1] > 118 && pad.Rsrv[1] < 138)
            {
                if (refresh_dat == 1)
                {
                    WriteDAT();
                    refresh_dat = 0;
                }
            }

            x = CamAngle;
        }
    }
    return fn_sub_8933468(a1, -16384 - x);
}

int FindPlayerVehicleCheckCam()
{
    if (CamMode == 1 && FindPlayerVehicle() && !IsWidescreen())
        return 1;
    else
        return 0;
}

int SwapRBCircle;
int Check_Button_Action(int a1, int a2)
{
    int (*fn_sub_8891160)(int a1, int a2) = 0x8891160;
    
    if (FindPlayerVehicle())
    {
        //25 SQUARE
        //26 RT
        //27 LT
        //24 CROSS

        switch (a2)
        {
        case 24:
            a2 = 26;
            break;
        case 26:
            a2 = 24;
            break;
        case 25:
            a2 = 27;
            break;
        case 27:
            a2 = 25;
            break;
        case 50:
            a2 = 1;
            break;
        case 1:
            a2 = 50;
            break;
        case 38:
            a2 = 25;
            break;
        default:
            break;
        }
    }
    else
    {
        if (SwapRBCircle)
        {
            switch (a2)
            {
            //case 1:
            //    a2 = 50;
            //    break;
            case 4:
                a2 = 26;
                break;
            //case 10:
            //    a2 = 4;
            //    break;
            case 26:
                a2 = 4;
                break;
            //case 38:
            //    a2 = 1;
            //    break;
            case 50:
                a2 = 1;
                break;
            default:
                break;
            }
        }
    }

    return fn_sub_8891160(a1, a2);
}

int Check_Button_Action2(int a1, int a2)
{
    int (*fn_sub_8890E68)(int a1, int a2) = 0x8890E68;

    if (!FindPlayerVehicle())
    {
        if (SwapRBCircle)
        {
            switch (a2)
            {
            case 4:
                a2 = 10;
                break;
            case 10:
                a2 = 4;
                break;
            default:
                break;
            }
        }
    }

    return fn_sub_8890E68(a1, a2);
}

int OnModuleStart() {
    int SwapDrivingControls = inireader.ReadInteger("MAIN", "SwapDrivingControls", 1);
    SwapRBCircle = inireader.ReadInteger("MAIN", "SwapRBCircle", 1);
    int Enable3rdPersonCamera = inireader.ReadInteger("MAIN", "Enable3rdPersonCamera", 1);
    int RadioInAllVehicles = inireader.ReadInteger("MAIN", "RadioInAllVehicles", 1);
    int UnthrottleEmuDuringLoading = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1);
    int UnlockXinMissionsInReplayBoard = inireader.ReadInteger("MAIN", "UnlockXinMissionsInReplayBoard", 0);

    if (UnthrottleEmuDuringLoading)
    {
        UnthrottleEmuEnable();
        injector.MakeJAL(0x88CFC1C, (intptr_t)UnthrottleEmuDisable);

        MakeInlineWrapperWithNOP(0x089924A0,
            jalr(a2),
            addu(a0, s0, a1),
            jal((intptr_t)UnthrottleEmuEnable),
            nop()
        );

        MakeInlineWrapper(0x089924C0,
            jalr(a2),
            addu(a0, s0, a1),
            jal((intptr_t)UnthrottleEmuDisable),
            nop()
        );
    }

    injector.MakeNOP(0x08B15788); //?

    gPlayers = (int*)0x8C0D160;
    gLocalPlayerId = *(int*)0x8B5B238;

    if (SwapDrivingControls)
    {
        injector.MakeJAL(0x888BDD0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x888BDEC, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x888BE08, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x888BE24, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8891448, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x88915F8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x88D8E34, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x88D8F08, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x896E814, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x896E824, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89C65B0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89C822C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89C8248, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CAE94, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CAEB0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CAEE8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CAF78, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CAFA0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB034, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB05C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB104, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB130, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB67C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB6A8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB6CC, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CB734, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CC198, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CC5A8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CDAC0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CDAD0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE034, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE04C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE05C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE0B4, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE0C8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE0D8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE164, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE1B0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE3BC, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE7F8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE82C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE87C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE92C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE98C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CE9B0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CF974, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD08, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD18, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD28, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD38, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD48, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD58, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD68, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD78, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFD88, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89CFFD8, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D00E4, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D0124, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D01D0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D039C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D03B0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D03C0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D042C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D048C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D0510, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D0694, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D0744, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D0764, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D07A4, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D081C, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89D0830, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x89FC894, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8A2ACD4, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8A2AD10, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8A6DD80, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8A6DD90, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8A6DDA0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8A6DDB0, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8AC7BBC, (uintptr_t)Check_Button_Action);
        injector.MakeJAL(0x8AC7C08, (uintptr_t)Check_Button_Action);

        injector.MakeJAL(0x89D0624, (uintptr_t)Check_Button_Action2);
        injector.MakeJAL(0x89D0724, (uintptr_t)Check_Button_Action2);
    }

    if (Enable3rdPersonCamera)
    {
        ReadDAT();

        injector.MakeNOP(0x88704C8); //disable cinematic cam

        injector.MakeJAL(0x88559F0, (uintptr_t)sub_8933468);

        uintptr_t pCamZ = &CamZ;
        //MakeInlineWrapper(0x88CDC80,
        //    lw(a2, sp, 0x34),
        //    jal((uintptr_t)FindPlayerVehicleCheckCam),
        //    nop(),
        //    beq(v0, zero, 4), //-->
        //    move(a0, a2),
        //    lui(a0, HIWORD(pCamZ)),
        //    ori(a0, a0, LOWORD(pCamZ)),
        //    lw(a0, a0, 0)
        //);

        //MakeInlineWrapper(0x88CCDA8,
        //    jal((uintptr_t)FindPlayerVehicleCheckCam),
        //    nop(),
        //    beq(v0, zero, 4), //-->
        //    addu(a1, a1, a0),
        //    lui(a1, HIWORD(pCamZ)),
        //    ori(a1, a1, LOWORD(pCamZ)),
        //    lw(a1, a1, 0)
        //);

        MakeInlineWrapper(0x088CBFA8,
            lw(a2, sp, 0xB4),
            jal((uintptr_t)FindPlayerVehicleCheckCam),
            nop(),
            beq(v0, zero, 4), //-->
            move(a0, a2),
            lui(a0, HIWORD(pCamZ)),
            ori(a0, a0, LOWORD(pCamZ)),
            lw(a0, a0, 0)
        );
    }

    if (RadioInAllVehicles)
    {
        uintptr_t ptr = pattern.get_first("2B 10 02 00 25 10 00 00 00 00 BF 8F 08 00 E0 03 10 00 BD 27", 8);
        MakeInlineWrapperWithNOP(ptr,
            lw(ra, sp, 0),
            li(v0, 0x1),
            jr(ra),
            addiu(sp, sp, 0x10)
        );

        ptr = pattern.get_first("01 00 A5 24 2A 20 85 00", 8);
        injector.MakeNOP(ptr);
        injector.MakeNOP(ptr + 16);
    }

    if (UnlockXinMissionsInReplayBoard)
    {
        MakeInlineWrapper(0x08A06040,
            li(t5, 0x60),
            li(t6, 0xFF),
            bne(a2, t5, 3), //-->
            nop(),
            sw(t6, a3, 0x1),
            sw(t6, a3, 0x2),
            lw(a3, a3, 0x0)
        );
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