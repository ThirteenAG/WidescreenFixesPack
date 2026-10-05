#include "../../external/injector/include/psp/runtime.hpp"
using namespace mips_asm;

extern "C" {
#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <systemctrl.h>

#include "../../external/injector/include/psp/log.h"
#include "../../external/injector/include/psp/nanoprintf.h"

#include "../../external/injector/include/psp/patterns.h"
#include "../../external/injector/include/psp/inireader.h"
#include "../../external/injector/include/psp/gvm.h"

#define MODULE_NAME_INTERNAL "CTW"
#define MODULE_NAME "GTACTW.PPSSPP.FusionFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.ini"
#define DAT_PATH "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/GTACTW.PPSSPP.FusionFix.dat"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
static_assert(sizeof(MODULE_NAME) - 1 < 28, "MODULE_NAME can't have more than 28 characters");
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
    int (*fn_sub_8933468)(short* a1, short a2) = reinterpret_cast<decltype(fn_sub_8933468)>(0x8933468);

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
    int (*fn_sub_8891160)(int a1, int a2) = reinterpret_cast<decltype(fn_sub_8891160)>(0x8891160);
    
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
    int (*fn_sub_8890E68)(int a1, int a2) = reinterpret_cast<decltype(fn_sub_8890E68)>(0x8890E68);

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
        injector::MakeCALL(0x88CFC1C, UnthrottleEmuDisable);

        injector::MakeInlineWithNOP(0x089924A0,
            jalr(a2),
            addu(a0, s0, a1),
            jal((intptr_t)UnthrottleEmuEnable),
            nop()
        );

        injector::MakeInline(0x089924C0,
            jalr(a2),
            addu(a0, s0, a1),
            jal((intptr_t)UnthrottleEmuDisable),
            nop()
        );
    }

    injector::MakeNOP(0x08B15788); //?

    gPlayers = (int*)0x8C0D160;
    gLocalPlayerId = *(int*)0x8B5B238;

    if (SwapDrivingControls)
    {
        injector::MakeCALL(0x888BDD0, Check_Button_Action);
        injector::MakeCALL(0x888BDEC, Check_Button_Action);
        injector::MakeCALL(0x888BE08, Check_Button_Action);
        injector::MakeCALL(0x888BE24, Check_Button_Action);
        injector::MakeCALL(0x8891448, Check_Button_Action);
        injector::MakeCALL(0x88915F8, Check_Button_Action);
        injector::MakeCALL(0x88D8E34, Check_Button_Action);
        injector::MakeCALL(0x88D8F08, Check_Button_Action);
        injector::MakeCALL(0x896E814, Check_Button_Action);
        injector::MakeCALL(0x896E824, Check_Button_Action);
        injector::MakeCALL(0x89C65B0, Check_Button_Action);
        injector::MakeCALL(0x89C822C, Check_Button_Action);
        injector::MakeCALL(0x89C8248, Check_Button_Action);
        injector::MakeCALL(0x89CAE94, Check_Button_Action);
        injector::MakeCALL(0x89CAEB0, Check_Button_Action);
        injector::MakeCALL(0x89CAEE8, Check_Button_Action);
        injector::MakeCALL(0x89CAF78, Check_Button_Action);
        injector::MakeCALL(0x89CAFA0, Check_Button_Action);
        injector::MakeCALL(0x89CB034, Check_Button_Action);
        injector::MakeCALL(0x89CB05C, Check_Button_Action);
        injector::MakeCALL(0x89CB104, Check_Button_Action);
        injector::MakeCALL(0x89CB130, Check_Button_Action);
        injector::MakeCALL(0x89CB67C, Check_Button_Action);
        injector::MakeCALL(0x89CB6A8, Check_Button_Action);
        injector::MakeCALL(0x89CB6CC, Check_Button_Action);
        injector::MakeCALL(0x89CB734, Check_Button_Action);
        injector::MakeCALL(0x89CC198, Check_Button_Action);
        injector::MakeCALL(0x89CC5A8, Check_Button_Action);
        injector::MakeCALL(0x89CDAC0, Check_Button_Action);
        injector::MakeCALL(0x89CDAD0, Check_Button_Action);
        injector::MakeCALL(0x89CE034, Check_Button_Action);
        injector::MakeCALL(0x89CE04C, Check_Button_Action);
        injector::MakeCALL(0x89CE05C, Check_Button_Action);
        injector::MakeCALL(0x89CE0B4, Check_Button_Action);
        injector::MakeCALL(0x89CE0C8, Check_Button_Action);
        injector::MakeCALL(0x89CE0D8, Check_Button_Action);
        injector::MakeCALL(0x89CE164, Check_Button_Action);
        injector::MakeCALL(0x89CE1B0, Check_Button_Action);
        injector::MakeCALL(0x89CE3BC, Check_Button_Action);
        injector::MakeCALL(0x89CE7F8, Check_Button_Action);
        injector::MakeCALL(0x89CE82C, Check_Button_Action);
        injector::MakeCALL(0x89CE87C, Check_Button_Action);
        injector::MakeCALL(0x89CE92C, Check_Button_Action);
        injector::MakeCALL(0x89CE98C, Check_Button_Action);
        injector::MakeCALL(0x89CE9B0, Check_Button_Action);
        injector::MakeCALL(0x89CF974, Check_Button_Action);
        injector::MakeCALL(0x89CFD08, Check_Button_Action);
        injector::MakeCALL(0x89CFD18, Check_Button_Action);
        injector::MakeCALL(0x89CFD28, Check_Button_Action);
        injector::MakeCALL(0x89CFD38, Check_Button_Action);
        injector::MakeCALL(0x89CFD48, Check_Button_Action);
        injector::MakeCALL(0x89CFD58, Check_Button_Action);
        injector::MakeCALL(0x89CFD68, Check_Button_Action);
        injector::MakeCALL(0x89CFD78, Check_Button_Action);
        injector::MakeCALL(0x89CFD88, Check_Button_Action);
        injector::MakeCALL(0x89CFFD8, Check_Button_Action);
        injector::MakeCALL(0x89D00E4, Check_Button_Action);
        injector::MakeCALL(0x89D0124, Check_Button_Action);
        injector::MakeCALL(0x89D01D0, Check_Button_Action);
        injector::MakeCALL(0x89D039C, Check_Button_Action);
        injector::MakeCALL(0x89D03B0, Check_Button_Action);
        injector::MakeCALL(0x89D03C0, Check_Button_Action);
        injector::MakeCALL(0x89D042C, Check_Button_Action);
        injector::MakeCALL(0x89D048C, Check_Button_Action);
        injector::MakeCALL(0x89D0510, Check_Button_Action);
        injector::MakeCALL(0x89D0694, Check_Button_Action);
        injector::MakeCALL(0x89D0744, Check_Button_Action);
        injector::MakeCALL(0x89D0764, Check_Button_Action);
        injector::MakeCALL(0x89D07A4, Check_Button_Action);
        injector::MakeCALL(0x89D081C, Check_Button_Action);
        injector::MakeCALL(0x89D0830, Check_Button_Action);
        injector::MakeCALL(0x89FC894, Check_Button_Action);
        injector::MakeCALL(0x8A2ACD4, Check_Button_Action);
        injector::MakeCALL(0x8A2AD10, Check_Button_Action);
        injector::MakeCALL(0x8A6DD80, Check_Button_Action);
        injector::MakeCALL(0x8A6DD90, Check_Button_Action);
        injector::MakeCALL(0x8A6DDA0, Check_Button_Action);
        injector::MakeCALL(0x8A6DDB0, Check_Button_Action);
        injector::MakeCALL(0x8AC7BBC, Check_Button_Action);
        injector::MakeCALL(0x8AC7C08, Check_Button_Action);

        injector::MakeCALL(0x89D0624, Check_Button_Action2);
        injector::MakeCALL(0x89D0724, Check_Button_Action2);
    }

    if (Enable3rdPersonCamera)
    {
        ReadDAT();

        injector::MakeNOP(0x88704C8); //disable cinematic cam

        injector::MakeCALL(0x88559F0, sub_8933468);

        uintptr_t pCamZ = reinterpret_cast<uintptr_t>(&CamZ);
        //injector::MakeInline(0x88CDC80,
        //    lw(a2, sp, 0x34),
        //    jal((uintptr_t)FindPlayerVehicleCheckCam),
        //    nop(),
        //    beq(v0, zero, 4), //-->
        //    move(a0, a2),
        //    lui(a0, injector::HighWord(pCamZ)),
        //    ori(a0, a0, injector::LowWord(pCamZ)),
        //    lw(a0, a0, 0)
        //);

        //injector::MakeInline(0x88CCDA8,
        //    jal((uintptr_t)FindPlayerVehicleCheckCam),
        //    nop(),
        //    beq(v0, zero, 4), //-->
        //    addu(a1, a1, a0),
        //    lui(a1, injector::HighWord(pCamZ)),
        //    ori(a1, a1, injector::LowWord(pCamZ)),
        //    lw(a1, a1, 0)
        //);

        injector::MakeInline(0x088CBFA8,
            lw(a2, sp, 0xB4),
            jal((uintptr_t)FindPlayerVehicleCheckCam),
            nop(),
            beq(v0, zero, 4), //-->
            move(a0, a2),
            lui(a0, injector::HighWord(pCamZ)),
            ori(a0, a0, injector::LowWord(pCamZ)),
            lw(a0, a0, 0)
        );
    }

    if (RadioInAllVehicles)
    {
        uintptr_t ptr = pattern.get_first("2B 10 02 00 25 10 00 00 00 00 BF 8F 08 00 E0 03 10 00 BD 27", 8);
        injector::MakeInlineWithNOP(ptr,
            lw(ra, sp, 0),
            li(v0, 0x1),
            jr(ra),
            addiu(sp, sp, 0x10)
        );

        ptr = pattern.get_first("01 00 A5 24 2A 20 85 00", 8);
        injector::MakeNOP(ptr);
        injector::MakeNOP(ptr + 16);
    }

    if (UnlockXinMissionsInReplayBoard)
    {
        injector::MakeInline(0x08A06040,
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
    if (injector::InitializeRuntime() != PSP_HOOK_OK) return -1;
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
                    injector::SetGameBaseAddress(info.text_addr, info.text_size);
                    pattern.SetGameBaseAddress(info.text_addr, info.text_size);
                    inireader.SetIniPath(INI_PATH);
                    logger.SetPath(LOG_PATH);
                    result = 1;
                }
                else if (strcmp(info.name, MODULE_NAME) == 0)
                {
                    injector::SetModuleBaseAddress(info.text_addr, info.text_size);
                }
            }

            if (result)
                OnModuleStart();
        }
    }
    return 0;
}

} // extern "C"
