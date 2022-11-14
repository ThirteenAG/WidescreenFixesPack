#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <systemctrl.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#include "lodl.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);
#endif

static const float fPSPResW = 480.0f;
static const float fPSPResH = 272.0f;

enum CControllerState
{
    LEFTSTICKX = 1,
    LEFTSTICKY,
    RIGHTSTICKX,
    RIGHTSTICKY,
    LEFTSHOULDER1,
    LEFTSHOULDER2,
    RIGHTSHOULDER1,
    RIGHTSHOULDER2,
    DPADUP,
    DPADDOWN,
    DPADLEFT,
    DPADRIGHT,
    unk1,
    unk2,
    unk3,
    unk4,
    START,
    SELECT,
    SQUARE,
    TRIANGLE,
    CROSS,
    CIRCLE,
    LEFTSHOCK,
    RIGHTSHOCK
};

short CPad__GetAccelerate(short* pad) {
    if (pad[77] == 0)
        return pad[RIGHTSHOULDER1];
    return 0;
}

short CPad__GetAccelerateNormal(short* pad) {
    if (pad[77] == 0)
        return pad[CROSS];
    return 0;
}

short CPad__GetBrake(short* pad) {
    if (pad[77] == 0)
        return pad[LEFTSHOULDER1];
    return 0;
}

short CPad__GetBrakeNormal(short* pad) {
    if (pad[77] == 0)
        return pad[SQUARE];
    return 0;
}

short cameraX(short* pad) {
    return pad[RIGHTSTICKX];
}

short cameraY(short* pad) {
    return pad[RIGHTSTICKY];
}

short aimX(short* pad) {
    return pad[LEFTSTICKX] ? pad[LEFTSTICKX] : pad[RIGHTSTICKX];
}

short aimY(short* pad) {
    return pad[LEFTSTICKY] ? pad[LEFTSTICKY] : pad[RIGHTSTICKY];
}

short CPad__GetTarget(short* pad)
{
    if (pad[77])
        return 0;
    //if (CPad__ForceCameraBehindPlayer(pad))
    //    return 0;
    return pad[LEFTSHOULDER1] != 0;
}

short CPad__TargetJustDown(short* pad)
{
    if (pad[77])
        return 0;
    //if (CPad__ForceCameraBehindPlayer(a1, 0, v1))
    //    return 0;
    if (pad[LEFTSHOULDER1])
        return pad[LEFTSHOULDER1 * 6] == 0; //old state
    return 0;
}

short CPad__GetWeapon(short* pad)
{
    if (pad[77] == 0)
        return pad[RIGHTSHOULDER1];
    return 0;
}

short CPad__WeaponJustDown(short* pad)
{
    if (pad[77])
        return 0;
    //if (CPad__ForceCameraBehindPlayer(a1, 0, v1))
    //    return 0;
    if (pad[RIGHTSHOULDER1])
        return pad[RIGHTSHOULDER1 * 6] == 0; //old state
}

short CPad__GetLookBehindForPed(short* pad)
{
    if (pad[77] == 0)
        return pad[CIRCLE];
    return 0;
}

float fAspectRatio = 16.0f / 9.0f;
float fFOVFactor = 1.0f;
float fARDiff;
float adjustRightX(float in, float scale)
{
    float fRightOffset = fPSPResW - in;
    return in + (fRightOffset - (scale / fARDiff * fRightOffset));
}

float adjustTopRightY(float in, float scale)
{
    return in * scale;
}

float adjustBottomRightY(float in, float scale)
{
    float fBottomOffset = fPSPResH - in;
    return in + (fBottomOffset - (scale * fBottomOffset));
}

float AdjustFOV(float f, float ar)
{
    return round((2.0f * atan(((ar) / (4.0f / 3.0f)) * tan(f / 2.0f * ((float)M_PI / 180.0f)))) * (180.0f / (float)M_PI) * 100.0f) / 100.0f;
}

float sub_171D44(float a1)
{
    return a1 * fFOVFactor * (AdjustFOV(70.0f, fAspectRatio) / 70.0f);
}

float TextScaling(int a1)
{
    return *(float*)(a1 + 32) / fARDiff;
}

float TextScalingVertical(int a1)
{
    return *(float*)(a1 + 32);
}

void isLittleWillie(uintptr_t a1)
{
    if (*(float*)(a1 + 1944) == 1.0f && (*(int16_t*)(*(uint32_t*)(a1 + 1984) + 86) == 0xAD))
        *(float*)(a1 + 1944) = *(float*)(a1 + 1944) + 1.0f;
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
                //logger.Write("Hello World\n");
            }
        }
        sceKernelDelayThread(1000);
    }
    sceKernelExitDeleteThread(0);
    */
    return 0;
}

int OnModuleStart() {
    sceKernelDelayThread(100000);
    //SceUID thid = sceKernelCreateThread(MODULE_NAME, module_thread, 0, 0x10000, 0, NULL);
    //if (thid >= 0)
    //    sceKernelStartThread(thid, 0, 0);

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

    // --> gv(0x123, 0x456);
    */

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first("00 00 00 00 ? ? ? ? 00 00 00 00 ? ? 04 3C 25 28 00 00", -4);
        injector.MakeNOP(ptr);
    }

    char szForceAspectRatio[10];
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    char* ForceAspectRatio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", szForceAspectRatio, sizeof(szForceAspectRatio));
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);

    int ModernControlScheme = inireader.ReadInteger("CONTROLS", "ModernControlScheme", 0);

    float fHudScale = inireader.ReadFloat("HUD", "HudScale", 1.0f);

    float fRadarScale = inireader.ReadFloat("RADAR", "RadarScale", 1.0f);
    float fRadarPosX = inireader.ReadFloat("RADAR", "RadarPosX", 12.0f);
    float fRadarPosY = inireader.ReadFloat("RADAR", "RadarPosY", 196.0f);

    float fMPRadarScale = inireader.ReadFloat("MPRADAR", "RadarScale", 1.0f);
    float fMPRadarPosY = inireader.ReadFloat("MPRADAR", "RadarPosY", 196.0f);

    fFOVFactor = inireader.ReadFloat("FOV", "FOVFactor", 0.0f);
    if (fFOVFactor <= 0.0f)
        fFOVFactor = 1.0f;

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 0);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 900);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 500.0f);

    if (DualAnalogPatch)
    {
        // Implement right analog stick
        uintptr_t ptr = pattern.get_first("06 00 00 10 03 00 A7 A3", 0);
        injector.WriteInstr(ptr + 0x24,
            sh(a1, sp, 0)
        );
        injector.WriteInstr(ptr + 0x1C,
            lhu(a1, sp, 0xE)
        );

        // Redirect camera movement
        ptr = pattern.get_first("36 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 16 00 40 10", 0);
        injector.MakeNOP(ptr + 0x00);
        injector.WriteMemory8(ptr + 0x10 + 0x2, 0x00);  // beqz -> b
        injector.MakeJAL(ptr + 0x8C, (intptr_t)cameraX);
        injector.MakeNOP(ptr + 0x108);
        injector.WriteMemory8(ptr + 0x118 + 0x2, 0x00); // beqz -> b
        injector.MakeJAL(ptr + 0x144, (intptr_t)cameraY);

        // Redirect gun aim movement
        ptr = pattern.get_first("40 00 80 04 ? ? ? ? 3E 00 80 10", 0);
        injector.MakeJAL(ptr + 0x50, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x7C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x8C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x158, (intptr_t)aimY);
        injector.MakeJAL(ptr + 0x1BC, (intptr_t)aimY);

        // Allow using L trigger when walking
        ptr = pattern.get_first("0E 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 08 00 80 10 ? ? ? ? ? ? ? ? 03 00 80 04", 0);
        injector.MakeNOP(ptr + 0x10);
        injector.MakeNOP(ptr + 0x9C);

        // Force L trigger value in the L+camera movement function
        ptr = pattern.get_first("0A 00 C7 84", 0);
        injector.WriteInstr(ptr + 0x00,
            li(a3, -0x1)
        );
    }

    if (ModernControlScheme)
    {
        // Swap R trigger and cross button
        uintptr_t ptr_1892D8 = pattern.get_first("9A 00 85 94 2B 28 05 00 FF 00 A5 30 03 00 A0 10 00 00 00 00 02 00 00 10 25 10 00 00 2A 00 82 84", 0);
        injector.MakeJMPwNOP(ptr_1892D8, (intptr_t)CPad__GetAccelerate);

        // Use normal button for flying plane
        uintptr_t ptr_1D5E14 = pattern.get_first("80 07 0E C6 02 63 0D 46 42 73 0D 46", 0);
        injector.MakeJAL(ptr_1D5E14 + 0x1C, (intptr_t)CPad__GetAccelerateNormal);
        injector.MakeJAL(ptr_1D5E14 + 0x3D0, (intptr_t)CPad__GetAccelerateNormal);

        // Use normal button for flying helicoper
        uintptr_t ptr_1EB29C = pattern.get_first("18 00 40 16 ? ? ? ? ? ? ? ? 25 20 20 02", 0);
        injector.MakeJAL(ptr_1EB29C + 0x14, (intptr_t)CPad__GetAccelerateNormal);

        uintptr_t ptr_18906C = pattern.get_first("0C 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 0A 00 A0 50", 0);
        injector.WriteMemory16(ptr_18906C + 0x20, RIGHTSHOULDER1 * 2);
        injector.WriteMemory16(ptr_18906C + 0x68, RIGHTSHOULDER1 * 2);
        injector.WriteMemory16(ptr_18906C + 0x80, CROSS * 2);

        // Swap L trigger and square button
        uintptr_t ptr_187918 = pattern.get(0, "26 00 05 86 ? ? ? ? 00 00 04 34", 0);
        uintptr_t ptr_18886C = pattern.get(0, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00", 0); // count = 3
        uintptr_t ptr_188AC0 = pattern.get(1, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00", 0);
        uintptr_t ptr_188DAC = pattern.get(0, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 00 02", 0);
        uintptr_t ptr_189140 = pattern.get(4, "9A 00 85 94 2B 28 05 00 FF 00 A5 30 ? ? ? ? 00 00 00 00", 0); // count = 15
        uintptr_t ptr_189BB0 = pattern.get(2, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00", 0);
        uintptr_t ptr_189D60 = pattern.get(0, "0A 00 04 86 ? ? ? ? 00 00 11 34", 0);
        uintptr_t ptr_18AAAC = pattern.get(0, "26 00 04 86 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_18B560 = pattern.get(2, "0A 00 04 86 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00", 0); // count = 3

        uintptr_t ptr_1EB2BC = pattern.get(0, "25 20 20 02 23 20 42 02", -4);
        uintptr_t ptr_188904 = pattern.get(0, "25 20 00 02 F6 FF 42 28 00 00 B0 8F", -4);
        uintptr_t ptr_188B58 = pattern.get(0, "25 20 00 02 0B 00 42 28 01 00 42 38", -4);
        uintptr_t ptr_188DFC = pattern.get(0, "01 00 42 38 25 10 00 00 04 00 B0 8F 08 00 B1 8F 0C 00 BF 8F 08 00 E0 03 10 00 BD 27", 4);
        uintptr_t ptr_1D5AE4 = pattern.get(0, "80 FF BD 27 54 00 B4 E7 58 00 B6 E7 5C 00 B8 E7", 0);

        uintptr_t ptr_5EE58 = pattern.get(0, "2C 00 82 84 ? ? ? ? 2E 00 82 84", 0);
        uintptr_t ptr_14CCA8 = pattern.get(2, "04 00 84 8C 27 00 05 34", 36);
        uintptr_t ptr_14D3F4 = pattern.get(1, "0A 00 64 86 ? ? ? ? 00 00 00 00", 0); // count = 2
        uintptr_t ptr_188118 = pattern.get(1, "02 00 07 34 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 12); // count = 2
        uintptr_t ptr_188078 = pattern.get(1, "2C 00 85 84 ? ? ? ? 00 00 06 34 5E 00 85 84", 0); // count = 2
        uintptr_t ptr_188084 = pattern.get(1, "5E 00 85 84 ? ? ? ? 28 00 87 84 01 00 06 34 28 00 87 84", 0); // count = 2
        uintptr_t ptr_188E14 = pattern.get(0, "EC FB 85 8F ? ? ? ? 00 00 00 00 9A 00 85 94", 0);
        uintptr_t ptr_18922C = pattern.get(6, "9A 00 85 94 2B 28 05 00 FF 00 A5 30 ? ? ? ? 00 00 00 00", 0); // count = 15
        uintptr_t ptr_189270 = pattern.get(7, "9A 00 85 94 2B 28 05 00 FF 00 A5 30 ? ? ? ? 00 00 00 00", 0); // count = 15
        uintptr_t ptr_189560 = pattern.get(7, "F0 FF BD 27 9A 00 85 94 00 00 B0 AF 25 80 80 00 2B 20 05 00", 0); // count = 11
        uintptr_t ptr_1895BC = pattern.get(8, "F0 FF BD 27 9A 00 85 94 00 00 B0 AF 25 80 80 00 2B 20 05 00", 0); // count = 11

        injector.MakeJMPwNOP(ptr_189140, (intptr_t)CPad__GetBrake);
        injector.WriteMemory16(ptr_189D60, SQUARE * 2);
        //driveby
        injector.MakeNOP(ptr_18886C+4);
        MakeInlineWrapper(ptr_18886C,
            lh(a0, s0, SQUARE * 2),
            lh(k0, s0, CIRCLE * 2),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_188904),
            nop()
        );
        injector.MakeNOP(ptr_188AC0+4);
        MakeInlineWrapper(ptr_188AC0,
            lh(a0, s0, SQUARE * 2),
            lh(k0, s0, CIRCLE * 2),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_188B58),
            nop()
        );
        injector.MakeNOP(ptr_188DAC+4);
        MakeInlineWrapper(ptr_188DAC,
            lh(a0, s0, SQUARE * 2),
            lh(k0, s0, CIRCLE * 2),
            _or(a0, a0, k0),
            bne(a0, zero, 2),
            nop(),
            j(ptr_188DFC),
            nop()
        );
        injector.WriteMemory16(ptr_189BB0, SQUARE * 2);
        injector.WriteMemory16(ptr_18B560, SQUARE * 2);

        injector.WriteMemory16(ptr_187918, LEFTSHOULDER1 * 2);
        injector.WriteMemory16(ptr_18AAAC, LEFTSHOULDER1 * 2);

        // Use normal button for flying plane
        injector.MakeJAL(ptr_1D5AE4 + 0x360, (intptr_t)CPad__GetBrakeNormal);
        injector.MakeJAL(ptr_1D5AE4 + 0x628, (intptr_t)CPad__GetBrakeNormal);
        injector.MakeJAL(ptr_1D5AE4 + 0x660, (intptr_t)CPad__GetBrakeNormal);
        injector.MakeJAL(ptr_1D5AE4 + 0x690, (intptr_t)CPad__GetBrakeNormal);
        injector.MakeJAL(ptr_1D5AE4 + 0x6E4, (intptr_t)CPad__GetBrakeNormal);

        // Use normal button for flying helicoper
        injector.MakeJAL(ptr_1EB2BC, (intptr_t)CPad__GetBrakeNormal);

        //Shooting with triggers
        injector.MakeJMPwNOP(ptr_189560, (intptr_t)CPad__GetTarget);
        injector.MakeJMPwNOP(ptr_1895BC, (intptr_t)CPad__TargetJustDown);
        injector.MakeJMPwNOP(ptr_18922C, (intptr_t)CPad__GetWeapon);
        injector.MakeJMPwNOP(ptr_189270, (intptr_t)CPad__WeaponJustDown);
        injector.MakeJMPwNOP(ptr_188E14, (intptr_t)CPad__GetLookBehindForPed);
        //injector.WriteMemory16(ptr_188078, RIGHTSHOULDER1 * 2);
        injector.WriteMemory16(ptr_188084, RIGHTSHOULDER1 * 6);
        injector.WriteMemory16(ptr_5EE58, RIGHTSHOULDER1 * 2);
        injector.WriteMemory16(ptr_188118 + 2, 0x1000);
        injector.WriteMemory16(ptr_14CCA8 + 2, 0x1000);
        injector.MakeNOP(ptr_14D3F4);
        //injector.MakeNOP(0x1880F8);
    }

    if (strcmp(ForceAspectRatio, "auto") != 0)
    {
        char* ch;
        ch = strtok(ForceAspectRatio, ":");
        int x = str2int(ch, 10);
        ch = strtok(NULL, ":");
        int y = str2int(ch, 10);
        fAspectRatio = (float)x / (float)y;
        uintptr_t ptr_130C4C = pattern.get(0, "E3 3F 05 3C 39 8E A5 34 00 68 85 44 25 28 00 00", 0);
        injector.MakeInlineLUIORI(ptr_130C4C, fAspectRatio);
    }

    if (strcmp(ForceAspectRatio, "auto") != 0 || fFOVFactor)
    {
        uintptr_t ptr_21FE98 = pattern.get(0, "00 63 00 46 ? ? ? ? ? ? ? ? 25 88 40 00", -4);
        MakeInlineWrapper(ptr_21FE98,
            jal((intptr_t)sub_171D44),
            nop(),
            swc1(f0, gp, -0x4250)
        );
    }

    if (Enable60FPS)
    {
        //60 fps
        uintptr_t ptr_2030C8 = pattern.get(0, "02 00 84 2C ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 20);
        injector.MakeNOP(ptr_2030C8);
    }

    fARDiff = fAspectRatio / (16.0f / 9.0f);

    if (fHudScale > 0.0f)
    {
        /* Patterns */
        uintptr_t ptr_1B74C8 = pattern.get(0, "B8 43 05 3C", 0);
        uintptr_t ptr_1B7570 = pattern.get(0, "E0 40 07 3C 06 A3 00 46", 0);
        uintptr_t ptr_1B7594 = pattern.get(0, "70 42 07 3C 06 A3 00 46", 0);
        uintptr_t ptr_1B7C98 = pattern.get(0, "8B 3F 04 3C 80 42 04 3C 06 D6 00 46", 0);
        uintptr_t ptr_1B7CA8 = pattern.get(0, "8B 3F 04 3C 1F 85 84 34 00 60 84 44", 0); // count = 2
        uintptr_t ptr_1B7CB8 = pattern.get(0, "B0 43 04 3C ? ? ? ? 28 42 05 3C", 0);
        uintptr_t ptr_1B7CC0 = pattern.get(0, "28 42 05 3C 68 42 06 3C", 0);
        uintptr_t ptr_1B7CC4 = pattern.get(0, "68 42 06 3C", 0);
        uintptr_t ptr_1B7D40 = pattern.get(0, "D2 43 05 3C 06 F3 00 46 48 E1 A5 34 46 A3 00 46", 0);
        uintptr_t ptr_1B7EEC = pattern.get(0, "A0 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6A 01 05 34", 0); // count = 2
        uintptr_t ptr_1B7EFC = pattern.get(0, "6A 01 05 34 ? ? ? ? 23 00 06 34", 0);
        uintptr_t ptr_1B7F04 = pattern.get(0, "23 00 06 34 ? ? ? ? ? ? ? ? 40 3F 04 3C", 0);
        uintptr_t ptr_1B7F10 = pattern.get(0, "40 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6E 01 05 34", 0); // count = 2
        uintptr_t ptr_1B7F20 = pattern.get(0, "6E 01 05 34 ? ? ? ? 28 00 06 34", 0);
        uintptr_t ptr_1B7F28 = pattern.get(0, "6E 01 05 34 ? ? ? ? 28 00 06 34", 8);
        uintptr_t ptr_1B8A18 = pattern.get(0, "D8 41 05 3C", 0);
        uintptr_t ptr_1B8A1C = pattern.get(0, "A0 42 06 3C ? ? ? ? D0 41 07 3C", 0);
        uintptr_t ptr_1B8A24 = pattern.get(0, "D0 41 07 3C 3A 43 08 3C", 0);
        uintptr_t ptr_1B8A28 = pattern.get(0, "3A 43 08 3C", 0);
        uintptr_t ptr_1B8AF0 = pattern.get(0, "4C 3F 04 3C 02 63 18 46", 0);
        uintptr_t ptr_1B8B5C = pattern.get(0, "24 42 05 3C 02 63 18 46", 0);
        uintptr_t ptr_1B8B6C = pattern.get(0, "20 42 06 3C 48 00 04 34", 0);
        uintptr_t ptr_1B8B90 = pattern.get(0, "F8 41 07 3C", 0);
        uintptr_t ptr_1B8B94 = pattern.get(0, "29 43 08 3C", 0);
        uintptr_t ptr_1B8B98 = pattern.get(0, "50 42 09 3C", 0);
        uintptr_t ptr_1B8BB4 = pattern.get(0, "1C 42 05 3C", 0);
        uintptr_t ptr_1B96F8 = pattern.get(0, "B8 43 07 3C", 0);
        uintptr_t ptr_1B9708 = pattern.get(0, "00 41 07 3C 25 28 A0 03", 0);
        uintptr_t ptr_1B97A8 = pattern.get(0, "50 41 04 3C ? ? ? ? 00 B0 84 44", 0);
        uintptr_t ptr_1B992C = pattern.get(0, "80 41 04 3C 00 78 84 44 25 20 A0 02", 0);
        uintptr_t ptr_1B99B4 = pattern.get(0, "80 40 04 3C 00 60 84 44 01 A5 0C 46", 0);
        uintptr_t ptr_1B9B00 = pattern.get(0, "E2 43 04 3C", 0);
        uintptr_t ptr_1B9B0C = pattern.get(0, "9A 42 04 3C", 0);
        uintptr_t ptr_1B9B18 = pattern.get(0, "88 41 04 3C 00 A0 84 44", 0);
        uintptr_t ptr_1B9DB8 = pattern.get(0, "B0 43 05 3C ? ? ? ? ? ? ? ? B0 43 05 3C 04 00 05 34", 0);
        uintptr_t ptr_1B9DC4 = pattern.get(0, "B0 43 05 3C 04 00 05 34", 0);
        uintptr_t ptr_1B9DD0 = pattern.get(0, "B0 43 05 3C 02 00 04 34", 0);
        uintptr_t ptr_1B9DF8 = pattern.get(0, "B0 43 05 3C 03 00 04 34", 0);
        uintptr_t ptr_1B9E04 = pattern.get(0, "B0 43 05 3C 00 60 85 44", 0);
        uintptr_t ptr_1B9E10 = pattern.get(0, "24 42 05 3C 00 68 85 44", 0);
        uintptr_t ptr_1B9E18 = pattern.get(0, "D0 43 05 3C", 0);
        uintptr_t ptr_1B9E20 = pattern.get(0, "C8 41 05 3C ? ? ? ? 00 78 85 44", 0);
        uintptr_t ptr_1BB728 = pattern.get(0, "8B 3F 04 3C 80 42 04 3C 86 A6 00 46", 0);
        uintptr_t ptr_1BB738 = pattern.get(1, "8B 3F 04 3C 1F 85 84 34 00 60 84 44", 0);
        uintptr_t ptr_1BB748 = pattern.get(0, "B0 43 04 3C ? ? ? ? C8 41 05 3C", 0);
        uintptr_t ptr_1BB750 = pattern.get(0, "C8 41 05 3C 24 42 06 3C", 0);
        uintptr_t ptr_1BB754 = pattern.get(0, "24 42 06 3C", 0);
        uintptr_t ptr_1BB7D0 = pattern.get(0, "D2 43 05 3C 06 F3 00 46 48 E1 A5 34 46 B3 00 46", 0);
        uintptr_t ptr_1BB97C = pattern.get(1, "A0 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6A 01 05 34", 0);
        uintptr_t ptr_1BB98C = pattern.get(0, "6A 01 05 34 ? ? ? ? 12 00 06 34", 0);
        uintptr_t ptr_1BB994 = pattern.get(0, "12 00 06 34 ? ? ? ? ? ? ? ? 40 3F 04 3C", 0);
        uintptr_t ptr_1BB9A0 = pattern.get(1, "40 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6E 01 05 34", 0);
        uintptr_t ptr_1BB9B0 = pattern.get(0, "6E 01 05 34 ? ? ? ? 17 00 06 34", 0);
        uintptr_t ptr_1BB9B8 = pattern.get(0, "6E 01 05 34 ? ? ? ? 17 00 06 34", 8);
        uintptr_t ptr_1C0EF8 = pattern.get(0, "99 3F 04 3C 9A 99 84 34 ? ? ? ? 00 60 84 44 FF 00 04 34", 0);
        uintptr_t ptr_1C0F24 = pattern.get(0, "ED 43 04 3C 00 80 84 34 00 B0 84 44 52 43 04 3C", 0);
        uintptr_t ptr_1C0F30 = pattern.get(0, "52 43 04 3C ? ? ? ? 00 C0 84 44", 0);
        uintptr_t ptr_1C1274 = pattern.get(0, "E3 43 04 3C", 0);
        uintptr_t ptr_1C1280 = pattern.get(0, "04 42 05 3C", 0);
        uintptr_t ptr_1C1284 = pattern.get(0, "1C 42 04 3C", 0);
        uintptr_t ptr_1C128C = pattern.get(0, "00 42 06 3C 00 68 84 44", 0);
        uintptr_t ptr_1C1304 = pattern.get(0, "D3 43 05 3C 00 60 85 44", 0);
        uintptr_t ptr_1C1310 = pattern.get(0, "E0 40 05 3C 00 68 85 44 25 20 A0 02", 0);
        uintptr_t ptr_1C131C = pattern.get(0, "F4 43 05 3C", 0);
        uintptr_t ptr_1C1324 = pattern.get(0, "8E 42 05 3C", 0);
        uintptr_t ptr_1C13B8 = pattern.get(0, "D4 3E 04 3C", 0);
        uintptr_t ptr_1C14DC = pattern.get(0, "EC 43 05 3C", 0);
        uintptr_t ptr_1C14E8 = pattern.get(0, "34 42 06 3C 6C 42 05 3C", 0);
        uintptr_t ptr_1C14EC = pattern.get(0, "6C 42 05 3C", 0);
        uintptr_t ptr_1C162C = pattern.get(0, "E0 43 05 3C", 0);
        uintptr_t ptr_1C1644 = pattern.get(0, "D4 43 05 3C 86 D3 00 46", 0);
        uintptr_t ptr_1C1688 = pattern.get(0, "E3 43 05 3C 06 D3 00 46", 0);
        uintptr_t ptr_1C1744 = pattern.get(0, "D3 43 05 3C 46 A3 00 46", 0);
        uintptr_t ptr_1C1C30 = pattern.get(3, "99 3F 04 3C 9A 99 84 34 ? ? ? ? 00 60 84 44", 0); // count = 4
        uintptr_t ptr_1C1C7C = pattern.get(0, "ED 43 04 3C 00 80 84 34 00 A0 84 44", 0);
        uintptr_t ptr_1C1C88 = pattern.get(0, "6A 43 04 3C", 0);
        uintptr_t ptr_2ABCBC = pattern.get(0, "8B 3F 04 3C 1F 85 84 34 ? ? ? ? 02 E3 0C 46", 0);
        uintptr_t ptr_1B9924 = pattern.get(0, "80 A3 0C 46 1C 00 AD C7", 0);
        uintptr_t ptr_17013C = pattern.get(0, "08 00 E0 03 20 00 80 C4 08 00 E0 03 24 00 85 A0 08 00 E0 03 24 00 82 90", 0);
        uintptr_t ptr_16F9B0 = pattern.get(0, "06 00 24 96 00 68 84 44", -8);
        uintptr_t ptr_170DD8 = pattern.get(0, "25 20 20 02 06 06 00 46 ? ? ? ? 25 20 20 02", -4); // count = 2
        uintptr_t ptr_170FF4 = pattern.get(1, "25 20 20 02 06 06 00 46 ? ? ? ? 25 20 20 02", -4); // count = 2
        uintptr_t ptr_9804 = pattern.get(0, "00 78 85 44 80 63 0F 46", -4);
        uintptr_t ptr_C9CC = pattern.get(0, "06 E4 00 46 86 B4 00 46", 0);
        uintptr_t ptr_214638 = pattern.get(0, "00 78 89 44 00 60 8A 44 00 68 8B 44", 0);
        uintptr_t ptr_2A43F0 = pattern.get(0, "F0 43 04 3C 02 63 0D 46", 0);
        uintptr_t ptr_1B9438 = pattern.get(0, "80 40 04 3C 00 A0 84 44 25 20 00 02", 0);
        uintptr_t ptr_1B97C0 = pattern.get(0, "01 B3 0C 46 24 00 A4 AF", 0);

        /* Health bar */
        injector.MakeInlineLUIORI(ptr_1B7CB8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B7CC0, adjustTopRightY(42.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1B7D40, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1B7CC4, adjustTopRightY(58.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_1B7CA8, fHudScale * 1.09f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_1B7C98, fHudScale * 1.085938f / fARDiff); // 1%

        injector.MakeInlineLI(ptr_1B7F20, (int32_t)(adjustRightX(366.0f, fHudScale / fARDiff)));
        injector.MakeInlineLI(ptr_1B7F28, (int32_t)(adjustTopRightY(40.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1B7F10, fHudScale * 0.75f); // Small "+" Font scale

        injector.MakeInlineLI(ptr_1B7EFC, (int32_t)(adjustRightX(362.0f, fHudScale)));
        injector.MakeInlineLI(ptr_1B7F04, (int32_t)(adjustTopRightY(35.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1B7EEC, fHudScale * 1.25f); // Large "+" Font scale

        /* Armor bar */
        injector.MakeInlineLUIORI(ptr_1BB748, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1BB750, adjustTopRightY(25.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1BB7D0, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1BB754, adjustTopRightY(41.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_1BB738, fHudScale * 1.09f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_1BB728, fHudScale * 1.085938f / fARDiff); // 1%

        injector.MakeInlineLI(ptr_1BB9B0, (int32_t)(adjustRightX(366.0f, fHudScale)));
        injector.MakeInlineLI(ptr_1BB9B8, (int32_t)(adjustTopRightY(23.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1BB9A0, fHudScale * 0.75f); // Small "+" Font scale

        injector.MakeInlineLI(ptr_1BB98C, (int32_t)(adjustRightX(362.0f, fHudScale)));
        injector.MakeInlineLI(ptr_1BB994, (int32_t)(adjustTopRightY(18.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1BB97C, fHudScale * 1.25f); // Large "+" Font scale

        /* Oxygen bar */
        injector.MakeInlineLUIORI(ptr_1B9DB8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9DC4, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9DD0, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9DF8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9E04, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9E20, adjustTopRightY(25.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1B9E18, adjustRightX(352.0f, fHudScale) + 64.0f); // Right X
        injector.MakeInlineLUIORI(ptr_1B9E10, adjustTopRightY(41.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_2ABCBC, fHudScale * 1.09f / fARDiff); // 1%

        /* Time, cash numbers, wanted stars */
        MakeInlineWrapper(ptr_1B97C0,
            lui(t9, HIWORD(fARDiff)),
            ori(t9, t9, LOWORD(fARDiff)),
            mtc1(t9, f30),
            divs(f22, f22, f30),
            divs(f12, f12, f30),
            lui(t9, HIWORD(fHudScale)),
            ori(t9, t9, LOWORD(fHudScale)),
            mtc1(t9, f30),
            muls(f22, f22, f30),
            subs(f12, f22, f12)
        );
        injector.MakeInlineLUIORI(ptr_1B992C, fHudScale * 16.0f); // Height
        injector.MakeInlineLUIORI(ptr_1B99B4, 4.0f / fARDiff);

        injector.MakeInlineLUIORI(ptr_1B96F8, adjustRightX(369.0f, fHudScale)); // Time Left X
        injector.MakeInlineLUIORI(ptr_1B9708, adjustTopRightY(8.0f, fHudScale)); // Time Top Y

        injector.MakeInlineLUIORI(ptr_1B74C8, adjustRightX(369.0f, fHudScale)); // Cash Left X
        injector.MakeInlineLUIORI(ptr_1B7570, adjustTopRightY(7.0f, fHudScale)); // Cash Top Y
        injector.MakeInlineLUIORI(ptr_1B7594, adjustTopRightY(60.0f, fHudScale)); // Cash Top Y (2)

        injector.MakeInlineLUIORI(ptr_1B9B00, adjustRightX(452.0f, fHudScale)); // Wanted stars Right X
        injector.MakeInlineLUIORI(ptr_1B9B0C, adjustTopRightY(77.0f, fHudScale)); // Wanted stars Top Y
        injector.MakeInlineLUIORI(ptr_1B9B18, fHudScale * 17.0f / fARDiff); // Wanted stars Width

        /* Weapon icon & ammo numbers */
        injector.MakeInlineLUIORI(ptr_1C1304, adjustRightX(422.0f, fHudScale)); // Fist icon Left X
        injector.MakeInlineLUIORI(ptr_1C1310, adjustTopRightY(7.0f, fHudScale)); // Fist icon Top Y
        injector.MakeInlineLUIORI(ptr_1C131C, adjustRightX(488.0f, fHudScale)); // Fist icon Right X
        injector.MakeInlineLUIORI(ptr_1C1324, adjustTopRightY(71.0f, fHudScale)); // Fist icon Bottom Y

        injector.MakeInlineLUIORI(ptr_1C1274, adjustRightX(455.0f, fHudScale)); // Weapon icon Center X
        injector.MakeInlineLUIORI(ptr_1C1284, adjustTopRightY(39.0f, fHudScale));  // Weapon icon Center Y
        injector.MakeInlineLUIORI(ptr_1C1280, fHudScale * 33.0f / fARDiff); // Weapon icon Width
        injector.MakeInlineLUIORI(ptr_1C128C, fHudScale * 32.0f); // Weapon icon Height
        injector.MakeInlineLUIORI(ptr_1C14E8, adjustTopRightY(45.0f, fHudScale)); // Ammo Top Y
        injector.MakeInlineLUIORI(ptr_1C14EC, adjustTopRightY(59.0f, fHudScale)); // Ammo Bottom Y
        injector.MakeInlineLUIORI(ptr_1C1744, adjustRightX(422.0f, fHudScale)); // Single clip ammo number Left X
        injector.MakeInlineLUIORI(ptr_1C1644, adjustRightX(425.0f, fHudScale)); // Ammo Left X
        injector.MakeInlineLUIORI(ptr_1C162C, adjustRightX(449.0f, fHudScale)); // '-' Left X
        injector.MakeInlineLUIORI(ptr_1C1688, adjustRightX(455.0f, fHudScale)); // Clip ammo Left X
        injector.MakeInlineLUIORI(ptr_1C14DC, adjustRightX(473.0f, fHudScale)); // Clip ammo Right X
        injector.MakeInlineLUIORI(ptr_1C13B8, fHudScale * 0.415f); // Font scale

        /* Zone name */
        injector.MakeInlineLUIORI(ptr_1C1C7C, adjustRightX(475.0f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1C1C88, adjustBottomRightY(234.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1C1C30, fHudScale * 1.2f); // Font scale

        /* Vehicle name */
        injector.MakeInlineLUIORI(ptr_1C0F24, adjustRightX(475.0f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1C0F30, adjustBottomRightY(210.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1C0EF8, fHudScale * 1.2f); // Font scale

        /* Pager */
        //injector.MakeInlineLUIORI(ptr_1B8A24, fHudScale * 26.0f); // Left X
        //injector.MakeInlineLUIORI(ptr_1B8A18, fHudScale * 27.0f); // Top Y
        //injector.MakeInlineLUIORI(ptr_1B8A28, fHudScale * 186.0f); // Right X
        //injector.MakeInlineLUIORI(ptr_1B8A1C, fHudScale * 80.0f); // Height
        //injector.MakeInlineLUIORI(ptr_1B8B6C, fHudScale * 40.0f); // Text rectangle Left X
        //injector.MakeInlineLUIORI(ptr_1B8B5C, fHudScale * 41.0f); // Text rectangle Top Y
        //injector.MakeInlineLUIORI(ptr_1B8BB4, fHudScale * 39.0f); // Text rectangle Left X
        //injector.MakeInlineLUIORI(ptr_1B8B90, fHudScale * 31.0f); // Text rectangle Top Y
        //injector.MakeInlineLUIORI(ptr_1B8B94, fHudScale * 169.0f); // Text rectangle Right X
        //injector.MakeInlineLUIORI(ptr_1B8B98, fHudScale * 52.0f); // Text rectangle Bottom Y
        //injector.MakeInlineLUIORI(ptr_1B8AF0, fHudScale * 0.8f); // Font scale

        /* Text */
        injector.MakeJMPwNOP(ptr_17013C, (intptr_t)TextScaling);
        injector.MakeJAL(ptr_16F9B0, (intptr_t)TextScalingVertical);
        injector.MakeJAL(ptr_170FF4, (intptr_t)TextScalingVertical);
        injector.MakeJAL(ptr_170DD8, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F8F4, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F91C, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F958, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F980, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16FBF8, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16FC38, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170264, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x17029C, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x17032C, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170364, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170548, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170580, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170628, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170660, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170BAC, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170C20, (intptr_t)TextScalingVertical);

        /*Radar Blips*/
        float f6 = 6.0f;
        if (fRadarScale)
            f6 = f6 * fRadarScale;
        float f6_pequeno = f6 / fARDiff;
        MakeInlineWrapper(ptr_9804,
            lui(a1, HIWORD(f6_pequeno)),
            ori(a1, a1, LOWORD(f6_pequeno)),
            mtc1(a1, f30),
            lui(a1, HIWORD(f6)),
            ori(a1, a1, LOWORD(f6))
        );
        injector.WriteInstr(ptr_9804 + 8, adds(f14, f12, f30));
        injector.WriteInstr(ptr_9804 + 0x24, subs(f12, f12, f15));

        //Crosshair
        MakeInlineWrapper(ptr_C9CC,
            movs(f16, f28),
            lui(t9, HIWORD(fARDiff)),
            ori(t9, t9, LOWORD(fARDiff)),
            mtc1(t9, f29),
            divs(f28, f28, f29)
        );
        MakeInlineWrapper(ptr_214638,
            mtc1(t1, f15),
            mtc1(t1, f16),
            lui(t1, HIWORD(fARDiff)),
            ori(t1, t1, LOWORD(fARDiff)),
            mtc1(t1, f29),
            divs(f15, f15, f29)
        );
        injector.MakeNOP(ptr_214638 + 0x10);

        //Sprites
        injector.MakeInlineLUIORI(ptr_2A43F0, 480.0f * fARDiff);

        // Radar Disc
        float f4 = 4.0f;
        float f4_new = 4.0f / fARDiff;
        MakeInlineWrapper(ptr_1B9438,
            lui(a0, HIWORD(f4_new)),
            ori(a0, a0, LOWORD(f4_new)),
            mtc1(a0, f30),
            lui(a0, HIWORD(f4)),
            ori(a0, a0, LOWORD(f4))
        );
        injector.WriteInstr(ptr_1B9438 + 0x10, subs(f22, f0, f30));
        injector.WriteInstr(ptr_1B9438 + 0x38, adds(f28, f12, f30));
    }

    /* Radar */
    {
        uintptr_t ptr_1B6A8C = pattern.get(0, "40 41 05 3C ? ? ? ? 00 00 85 44", 0);
        uintptr_t ptr_1B6AB4 = pattern.get(0, "2A 43 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_1B6AC0 = pattern.get(0, "44 43 04 3C 00 00 84 44", 0);
        uintptr_t ptr_1B6ADC = pattern.get(0, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0); // count = 2
        uintptr_t ptr_1B6AE8 = pattern.get(0, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0); // count = 2
        uintptr_t ptr_1B6B08 = pattern.get(1, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_1B6B14 = pattern.get(1, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0);

        if (fRadarScale > 0.0f)
        {
            if (fRadarPosX)
                injector.MakeInlineLUIORI(ptr_1B6A8C, fRadarScale * fRadarPosX);  // Left X
            else
                injector.MakeInlineLUIORI(ptr_1B6A8C, fRadarScale * 12.0f);  // Left X

            if (fRadarPosY)
                injector.MakeInlineLUIORI(ptr_1B6AC0, adjustBottomRightY(fRadarPosY, fRadarScale)); // Top Y
            else
                injector.MakeInlineLUIORI(ptr_1B6AC0, adjustBottomRightY(196.0f, fRadarScale)); // Top Y

            injector.MakeInlineLUIORI(ptr_1B6AE8, fRadarScale * 65.8f / fARDiff);  // Size X
            injector.MakeInlineLUIORI(ptr_1B6B14, fRadarScale * 65.8f);  // Size Y
        }

        if (fMPRadarScale > 0.0f)
        {
            if (fMPRadarPosY)
                injector.MakeInlineLUIORI(ptr_1B6AB4, adjustBottomRightY(fMPRadarPosY, (fMPRadarScale ? fMPRadarScale : 1.0f))); // Top Y (Multiplayer)
            injector.MakeInlineLUIORI(ptr_1B6ADC, fMPRadarScale * 92.0f / fARDiff);  // Size X (Multiplayer)
            injector.MakeInlineLUIORI(ptr_1B6B08, fMPRadarScale * 92.0f);  // Size Y (Multiplayer)
        }
    }

    if (RenderLodLights)
    {
        uintptr_t ptr_17D324 = pattern.get(0, "E0 FF BD 27 20 00 A2 8F", 0);
        CCoronas__RegisterCorona = (void*)ptr_17D324;
        uintptr_t ptr_3CF00 = pattern.get(0, "25 20 20 02 30 00 64 26 00 00 80 D8 00 00 41 D8", -16);
        TheCamera = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_3CF00 + 0)) << 16) + *(int16_t*)(ptr_3CF00 + 4);
        pCamPos = (CVector*)(TheCamera + 0x9B0); //0x9B0 at 0x218648
        CDraw__ms_fNearClipZOffset = *(int16_t*)pattern.get(0, "25 90 A0 00 25 30 80 00 FF 00 13 31", 12);
        CDraw__ms_fFarClipZOffset = *(int16_t*)pattern.get(0, "25 90 A0 00 25 30 80 00 FF 00 13 31", 24);
        CurrentTimeHoursOffset = *(int16_t*)pattern.get(0, "0C 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? 80 3F 04 3C", 4);
        CurrentTimeMinutesOffset = *(int16_t*)pattern.get(0, "0C 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? 80 3F 04 3C", 8);
        CTimer__m_snTimeInMillisecondsPauseModeOffset = *(int16_t*)pattern.get(0, "34 00 B0 AE 00 60 84 44", -4);
        CTimer__ms_fTimeStepOffset = *(int16_t*)pattern.get(0, "48 42 04 3C 00 68 84 44 C3 63 0D 46", -4);

        SceUID block_id = 0;
        const int corona_struct_size = 112;
        uintptr_t aCoronas = injector.AllocMemBlock(corona_struct_size * CoronaLimit, &block_id);

        uintptr_t ptr_17D174 = pattern.get(0, "3C 68 0C 46 21 20 85 00", -4);
        uintptr_t ptr_17D180 = pattern.get(0, "14 00 B1 AF 18 00 BF AF ? ? ? ? 06 6B 00 46", -4);
        uintptr_t ptr_17D22C = pattern.get(0, "38 00 24 2A ? ? ? ? 70 00 10 26", 0);
        uintptr_t ptr_17D2D4 = pattern.get(0, "00 00 05 34 ? ? ? ? C0 31 05 00 00 39 05 00", -4); // count = 3
        uintptr_t ptr_17D2DC = pattern.get(0, "C0 31 05 00 00 39 05 00 23 30 C7 00 21 30 C4 00", -4);
        uintptr_t ptr_17D2FC = pattern.get(0, "38 00 A6 28 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_17D3DC = pattern.get(0, "00 00 07 34 01 00 08 34", -4);
        uintptr_t ptr_17D3EC = pattern.get(0, "00 00 00 00 ? ? ? ? C0 41 07 00", -8);
        uintptr_t ptr_17D424 = pattern.get(0, "38 00 E8 28", 0);
        uintptr_t ptr_17D428 = pattern.get(0, "38 00 04 34 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_17E410 = pattern.get(0, "44 00 A4 AF ? ? ? ? 00 00 17 34", -8);
        uintptr_t ptr_17E42C = pattern.get(0, "EF FF 1E 24", -4);
        uintptr_t ptr_17EAF0 = pattern.get(0, "38 00 E4 2A", 0);
        uintptr_t ptr_17ED08 = pattern.get(0, "38 00 04 2A", 0);
        uintptr_t ptr_17ED94 = pattern.get(2, "94 00 B4 E7 98 00 B6 E7 9C 00 B8 E7", -8); // count = 4
        uintptr_t ptr_17EDD8 = pattern.get(0, "00 00 05 34 BF FF 04 24", -4);
        uintptr_t ptr_17F218 = pattern.get(0, "38 00 24 2A ? ? ? ? 00 60 85 44", 0);
        uintptr_t ptr_17FCB0 = pattern.get(0, "00 00 0F 34 01 00 19 34", -4);
        uintptr_t ptr_17FCBC = pattern.get(0, "38 00 18 34", 0);
        uintptr_t ptr_17FCC0 = pattern.get(0, "C0 C9 0F 00 00 81 0F 00 23 C8 30 03", -8); // count = 2
        uintptr_t ptr_17FCF0 = pattern.get(0, "38 00 F9 29 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_17FD44 = pattern.get(0, "38 00 F9 29 ? ? ? ? C0 C9 0F 00", 0);
        uintptr_t ptr_133F80 = pattern.get(0, "FF 00 04 34 ? ? ? ? ? ? ? ? 00 00 00 00", 16);
        uintptr_t ptr_2FDD50 = pattern.get(0, "A0 42 04 3C 00 70 84 44 3C 60 0E 46", 0);
        uintptr_t ptr_2FDDA0 = pattern.get(0, "A0 42 04 3C 00 68 84 44 3C 60 0D 46", 0);

        injector.WriteInstr(ptr_17D174, lui(s0, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17D2D4, lui(a0, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17D3DC, lui(a2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17E410, lui(s2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17ED94, lui(s5, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17FCB0, lui(t6, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_17D180, ori(s0, s0, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17D2DC, ori(a0, a0, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17D3EC, ori(a2, a2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17E42C, ori(s2, s2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17EDD8, ori(s5, s5, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17FCC0, ori(t6, t6, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_17D22C, slti(a0, s1, CoronaLimit));
        injector.WriteInstr(ptr_17D2FC, slti(a2, a1, CoronaLimit));
        injector.WriteInstr(ptr_17D424, slti(t0, a3, CoronaLimit));
        injector.WriteInstr(ptr_17D428, li(a0, CoronaLimit));
        injector.WriteInstr(ptr_17EAF0, slti(a0, s7, CoronaLimit));
        injector.WriteInstr(ptr_17ED08, slti(a0, s0, CoronaLimit));
        injector.WriteInstr(ptr_17F218, slti(a0, s1, CoronaLimit));
        injector.WriteInstr(ptr_17FCF0, slti(t9, t7, CoronaLimit));
        injector.WriteInstr(ptr_17FCBC, li(t8, CoronaLimit));
        injector.WriteInstr(ptr_17FD44, slti(t9, t7, CoronaLimit));

        // Coronas Render
        injector.MakeJAL(ptr_133F80, (intptr_t)RegisterLODLights);

        // Heli Height Limit
        injector.MakeInlineLUIORI(ptr_2FDD50, 800.0f);
        injector.MakeInlineLUIORI(ptr_2FDDA0, 800.0f);
    }

    //Little Willie Cam Fix
    {
        //MakeInlineWrapper(0x225130,
        //    move(a0, s1),
        //    jal(isLittleWillie),
        //    nop(),
        //    lui(a0, 0x40A0)
        //);
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