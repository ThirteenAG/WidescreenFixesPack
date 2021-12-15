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

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);
#endif

static const float fPSPResW = 480.0f;
static const float fPSPResH = 272.0f;

enum GtaPad {
    PAD_LX = 1,
    PAD_LY = 2,
    PAD_RX = 3,
    PAD_RY = 4,
    PAD_LTRIGGER = 5,
    PAD_RTRIGGER = 7,
    PAD_SQUARE = 19,
    PAD_TRIANGLE = 20,
    PAD_CROSS = 21,
    PAD_CIRCLE = 22,
};

short vcsAcceleration(short* pad) {
    if (pad[77] == 0)
        return pad[PAD_RTRIGGER];
    return 0;
}

short vcsAccelerationNormal(short* pad) {
    if (pad[77] == 0)
        return pad[PAD_CROSS];
    return 0;
}

short vcsBrake(short* pad) {
    if (pad[77] == 0)
        return pad[PAD_LTRIGGER];
    return 0;
}

short vcsBrakeNormal(short* pad) {
    if (pad[77] == 0)
        return pad[PAD_SQUARE];
    return 0;
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

float adjustRightX(float in, float scale)
{
    float fRightOffset = fPSPResW - in;
    return in + (fRightOffset - (scale * fRightOffset));
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

    char szForceAspectRatio[100];
    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    char* ForceAspectRatio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", szForceAspectRatio, sizeof(szForceAspectRatio));
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);

    int SwapRBCross = inireader.ReadInteger("VEHICLECONTROLS", "SwapRBCross", 0);
    int SwapLBSquare = inireader.ReadInteger("VEHICLECONTROLS", "SwapLBSquare", 0);

    float fHudScale = inireader.ReadFloat("HUD", "HudScale", 1.0f);
    float fRadarScale = inireader.ReadFloat("HUD", "RadarScale", 1.0f);

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first("00 00 00 00 ? ? ? ? 00 00 00 00 ? ? 04 3C 25 28 00 00", -4);
        injector.MakeNOP(ptr);
    }

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

    if (SwapRBCross)
    {
        // Swap R trigger and cross button
        uintptr_t ptr = pattern.get_first("9A 00 85 94 2B 28 05 00 FF 00 A5 30 03 00 A0 10 00 00 00 00 02 00 00 10 25 10 00 00 2A 00 82 84", 0);
        injector.MakeJMP(ptr, (intptr_t)vcsAcceleration);
        
        // Use normal button for flying plane
        ptr = pattern.get_first("80 07 0E C6 02 63 0D 46 42 73 0D 46", 0);
        injector.MakeJAL(ptr + 0x1C, (intptr_t)vcsAccelerationNormal);
        injector.MakeJAL(ptr + 0x3D0, (intptr_t)vcsAccelerationNormal);

        // Use normal button for flying helicoper
        ptr = pattern.get_first("18 00 40 16 ? ? ? ? ? ? ? ? 25 20 20 02", 0);
        injector.MakeJAL(ptr + 0x14, (intptr_t)vcsAccelerationNormal);
        
        ptr = pattern.get_first("0C 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 0A 00 A0 50", 0);
        injector.WriteMemory16(ptr + 0x20, PAD_RTRIGGER * 2);
        injector.WriteMemory16(ptr + 0x68, PAD_RTRIGGER * 2);
        injector.WriteMemory16(ptr + 0x80, PAD_CROSS * 2);
    }

    if (SwapLBSquare)
    {
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

        uintptr_t ptr_1D5E44 = pattern.get(0, "25 20 40 00 23 20 62 02 00 60 84 44 7F 43 04 3C", 4);
        uintptr_t ptr_1D610C = pattern.get(0, "25 20 40 00 ? ? ? ? ? ? ? ? B3 3E 04 3C", 4);
        uintptr_t ptr_1D6144 = pattern.get(0, "25 20 40 00 00 60 82 44 00 3F 04 3C", 4);
        uintptr_t ptr_1D6174 = pattern.get(0, "25 20 40 00 00 60 82 44 ? ? ? ? 03 63 14 46", 4);
        uintptr_t ptr_1D61C8 = pattern.get(0, "25 20 40 00 0A 00 44 28", 4); // count = 2
        uintptr_t ptr_1EB2BC = pattern.get(0, "25 20 20 02 23 20 42 02", 4);


        injector.MakeJMP(ptr_189140, (intptr_t)vcsBrake);
        injector.WriteMemory16(ptr_189D60, PAD_SQUARE * 2);
        //driveby
        MakeInlineWrapper(ptr_18886C, 
            lh(a0, s0, PAD_SQUARE * 2),
            lh(k0, s0, PAD_CIRCLE * 2),
            _or(a0, a0, k0)
        );
        MakeInlineWrapper(ptr_188AC0,
            lh(a0, s0, PAD_SQUARE * 2),
            lh(k0, s0, PAD_CIRCLE * 2),
            _or(a0, a0, k0)
        );
        MakeInlineWrapper(ptr_188DAC,
            lh(a0, s0, PAD_SQUARE * 2),
            lh(k0, s0, PAD_CIRCLE * 2),
            _or(a0, a0, k0)
        );
        injector.WriteMemory16(ptr_189BB0, PAD_SQUARE * 2);
        injector.WriteMemory16(ptr_18B560, PAD_SQUARE * 2);
        
        injector.WriteMemory16(ptr_187918, PAD_LTRIGGER * 2);
        injector.WriteMemory16(ptr_18AAAC, PAD_LTRIGGER * 2);

        // Use normal button for flying plane
        uintptr_t ptr = pattern.get_first("80 07 0E C6 02 63 0D 46 42 73 0D 46", 0);
        injector.MakeJAL(0x1D5AE4 + 0x360, (intptr_t)vcsBrakeNormal);
        injector.MakeJAL(0x1D5AE4 + 0x628, (intptr_t)vcsBrakeNormal);
        injector.MakeJAL(0x1D5AE4 + 0x660, (intptr_t)vcsBrakeNormal);
        injector.MakeJAL(0x1D5AE4 + 0x690, (intptr_t)vcsBrakeNormal);
        injector.MakeJAL(0x1D5AE4 + 0x6E4, (intptr_t)vcsBrakeNormal);
        
        // Use normal button for flying helicoper
        ptr = pattern.get_first("18 00 40 16 ? ? ? ? ? ? ? ? 25 20 20 02", 0);
        injector.MakeJAL(0x1EB2BC, (intptr_t)vcsBrakeNormal);
    }

    if (strcmp(ForceAspectRatio, "auto") != 0)
    {
        char* ch;
        ch = strtok(ForceAspectRatio, ":");
        int x = str2int(ch, 10);
        ch = strtok(NULL, ":");
        int y = str2int(ch, 10);
        float fAspectRatio = (float)x / (float)y;
        uintptr_t ptr_130C4C = pattern.get(0, "E3 3F 05 3C 39 8E A5 34 00 68 85 44 25 28 00 00", 4);
        injector.MakeInlineLUIORI(ptr_130C4C, fAspectRatio);
    }

    if (Enable60FPS)
    {
        //60 fps
        uintptr_t ptr_2030C8 = pattern.get(0, "02 00 84 2C ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 20);
        injector.MakeNOP(ptr_2030C8);
    }

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


        /* Health bar */
        injector.MakeInlineLUIORI(ptr_1B7CB8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B7CC0, adjustTopRightY(42.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1B7D40, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1B7CC4, adjustTopRightY(58.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_1B7CA8, fHudScale * 1.09f); // 1%
        injector.MakeInlineLUIORI(ptr_1B7C98, fHudScale * 1.085938f); // 1%

        injector.MakeInlineLI(ptr_1B7F20, (int32_t)(adjustRightX(366.0f, fHudScale)));
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
        injector.MakeInlineLUIORI(ptr_1BB738, fHudScale * 1.09f); // 1%
        injector.MakeInlineLUIORI(ptr_1BB728, fHudScale * 1.085938f); // 1%

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
        injector.MakeInlineLUIORI(ptr_2ABCBC, fHudScale * 1.09f); // 1%

        /* Time, cash numbers, wanted stars */
        injector.MakeInlineLUIORI(ptr_1B97A8, fHudScale * 13.0f); // Width
        injector.MakeInlineLUIORI(ptr_1B992C, fHudScale * 16.0f); // Height

        injector.MakeInlineLUIORI(ptr_1B96F8, adjustRightX(369.0f, fHudScale)); // Time Left X
        injector.MakeInlineLUIORI(ptr_1B9708, adjustTopRightY(8.0f, fHudScale)); // Time Top Y

        injector.MakeInlineLUIORI(ptr_1B74C8, adjustRightX(369.0f, fHudScale)); // Cash Left X
        injector.MakeInlineLUIORI(ptr_1B7570, adjustTopRightY(7.0f, fHudScale)); // Cash Top Y
        injector.MakeInlineLUIORI(ptr_1B7594, adjustTopRightY(60.0f, fHudScale)); // Cash Top Y (2)

        injector.MakeInlineLUIORI(ptr_1B9B00, adjustRightX(452.0f, fHudScale)); // Wanted stars Right X
        injector.MakeInlineLUIORI(ptr_1B9B0C, adjustTopRightY(77.0f, fHudScale)); // Wanted stars Top Y
        injector.MakeInlineLUIORI(ptr_1B9B18, fHudScale * 17.0f); // Wanted stars Width

        /* Weapon icon & ammo numbers */
        injector.MakeInlineLUIORI(ptr_1C1304, adjustRightX(422.0f, fHudScale)); // Fist icon Left X
        injector.MakeInlineLUIORI(ptr_1C1310, adjustTopRightY(7.0f, fHudScale)); // Fist icon Top Y
        injector.MakeInlineLUIORI(ptr_1C131C, adjustRightX(488.0f, fHudScale)); // Fist icon Right X
        injector.MakeInlineLUIORI(ptr_1C1324, adjustTopRightY(71.0f, fHudScale)); // Fist icon Bottom Y

        injector.MakeInlineLUIORI(ptr_1C1274, adjustRightX(455.0f, fHudScale)); // Weapon icon Center X
        injector.MakeInlineLUIORI(ptr_1C1284, adjustTopRightY(39.0f, fHudScale));  // Weapon icon Center Y
        injector.MakeInlineLUIORI(ptr_1C1280, fHudScale * 33.0f); // Weapon icon Width
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
    }

    if (fRadarScale > 0.0f)
    {
        /* Radar */
        uintptr_t ptr_1B6A8C = pattern.get(0, "40 41 05 3C ? ? ? ? 00 00 85 44", 0);
        uintptr_t ptr_1B6AB4 = pattern.get(0, "2A 43 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_1B6AC0 = pattern.get(0, "44 43 04 3C 00 00 84 44", 0);
        uintptr_t ptr_1B6ADC = pattern.get(0, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0); // count = 2
        uintptr_t ptr_1B6AE8 = pattern.get(0, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0); // count = 2
        uintptr_t ptr_1B6B08 = pattern.get(1, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_1B6B14 = pattern.get(1, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0);

        injector.MakeInlineLUIORI(ptr_1B6A8C, fRadarScale * 12.0f);  // Left X
        injector.MakeInlineLUIORI(ptr_1B6AC0, adjustBottomRightY(196.0f, fRadarScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1B6AB4, adjustBottomRightY(170.0f, fRadarScale)); // Top Y (Multiplayer)
        injector.MakeInlineLUIORI(ptr_1B6AE8, fRadarScale * 65.8f);  // Size X
        injector.MakeInlineLUIORI(ptr_1B6ADC, fRadarScale * 92.0f);  // Size X (Multiplayer)
        injector.MakeInlineLUIORI(ptr_1B6B14, fRadarScale * 65.8f);  // Size Y
        injector.MakeInlineLUIORI(ptr_1B6B08, fRadarScale * 92.0f);  // Size Y (Multiplayer)
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