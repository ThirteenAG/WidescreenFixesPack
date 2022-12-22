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

#include "lodl.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTALCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/GTALCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/GTALCS.PPSSPP.WidescreenFix.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);
#endif

static const float fPSPResW = 480.0f;
static const float fPSPResH = 272.0f;

enum
{
    EMULATOR_DEVCTL__TOGGLE_FASTFORWARD = 0x30,
    EMULATOR_DEVCTL__GET_ASPECT_RATIO,
    EMULATOR_DEVCTL__GET_SCALE,
};

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

short lcsAcceleration(short* pad) {
    if (pad[67] == 0)
        return pad[PAD_RTRIGGER];
    return 0;
}

short lcsBrake(short* pad) {
    if (pad[67] == 0)
        return pad[PAD_LTRIGGER];
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

void UnthrottleEmuEnable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)1, 0, NULL, 0);
}

void UnthrottleEmuDisable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)0, 0, NULL, 0);
}

float fAspectRatio = 16.0f / 9.0f;
int UnthrottleEmuDuringLoading = 0;
int once = 0;
float* flt_3450E4;
uint8_t* byte_38AF51;
void GameLoopStuff()
{
    if (once != 1)
    {
        if (UnthrottleEmuDuringLoading)
            UnthrottleEmuDisable();
        once = 1;
    }

    if (UnthrottleEmuDuringLoading)
    {
        uint8_t gMenuActivated = *byte_38AF51;
        float gBlackScreenTime = *flt_3450E4;
        if (gBlackScreenTime && !gMenuActivated)
            UnthrottleEmuEnable();
        else
            UnthrottleEmuDisable();
    }
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
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
    sceKernelDelayThread(100000); //added cuz game gets stuck on a loadscreen between islands otherwise, idk why

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first("00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 25 20 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 25 10 00 00", -4);
        injector.MakeNOP(ptr);
    }

    UnthrottleEmuDuringLoading = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1);

    if (UnthrottleEmuDuringLoading)
    {
        UnthrottleEmuEnable();
    }

    char szForceAspectRatio[100];
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    char* ForceAspectRatio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", szForceAspectRatio, sizeof(szForceAspectRatio));
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);

    int SwapRBCross = inireader.ReadInteger("VEHICLECONTROLS", "SwapRBCross", 0);
    int SwapLBSquare = inireader.ReadInteger("VEHICLECONTROLS", "SwapLBSquare", 0);

    float fHudScale = inireader.ReadFloat("HUD", "HudScale", 1.0f);

    float fRadarScale = inireader.ReadFloat("RADAR", "RadarScale", 1.0f);
    float fRadarPosX = inireader.ReadFloat("RADAR", "RadarPosX", 12.0f);
    float fRadarPosY = inireader.ReadFloat("RADAR", "RadarPosY", 196.0f);

    float fMPRadarScale = inireader.ReadFloat("MPRADAR", "RadarScale", 1.0f);
    float fMPRadarPosY = inireader.ReadFloat("MPRADAR", "RadarPosY", 170.0f);

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 0);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 900);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 500.0f);

    if (DualAnalogPatch)
    {
        // Implement right analog stick
        uintptr_t ptr = pattern.get_first("06 00 00 10 ? 00 A7 A3", 0);
        injector.WriteInstr(ptr + 0x24, 
            sh(a1, sp, 0x10)
        );
        injector.WriteInstr(ptr + 0x1C,
            lhu(a1, sp, 0x1E)
        );

        // Redirect camera movement
        ptr = pattern.get_first("34 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 14 00 40 10", 0);
        injector.MakeNOP(ptr + 0x00);
        injector.WriteMemory8(ptr + 0x10 + 0x2, 0x00);  // beqz -> b
        injector.MakeJAL(ptr + 0x84, (intptr_t)cameraX);
        injector.MakeNOP(ptr + 0x100);
        injector.WriteMemory8(ptr + 0x110 + 0x2, 0x00); // beqz -> b
        injector.MakeJAL(ptr + 0x13C, (intptr_t)cameraY);

        // Redirect gun aim movement
        ptr = pattern.get_first("36 00 80 04 ? ? ? ? 34 00 80 10", 0);
        injector.MakeJAL(ptr + 0x3C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x68, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x78, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x130, (intptr_t)aimY);
        injector.MakeJAL(ptr + 0x198, (intptr_t)aimY);
        
        // Allow using L trigger when walking
        ptr = pattern.get(0, "0E 00 A0 14 ? ? ? ? ? ? ? ? ? ? ? ? 08 00 A0 10 ? ? ? ? ? ? ? ? 03 00 A0 04", 0);
        injector.MakeNOP(ptr + 0x10);
        injector.MakeNOP(ptr + 0x74);
        
        // Force L trigger value in the L+camera movement function
        ptr = pattern.get_first("0A 00 0A 85", 0);
        injector.WriteInstr(ptr + 0x00,
            li(t2, -0x1)
        );
    }

    if (SwapRBCross)
    {
        // Swap R trigger and cross button
        uintptr_t ptr = pattern.get_first("86 00 85 94 03 00 A0 10 00 00 00 00 02 00 00 10 25 10 00 00 2A 00 82 84", 0);
        injector.MakeJMPwNOP(ptr, (intptr_t)lcsAcceleration);
        
        ptr = pattern.get_first("0E 00 A0 14 ? ? ? ? ? ? ? ? ? ? ? ? 0C 00 C0 50", 0);
        injector.WriteMemory16(ptr + 0x20, PAD_RTRIGGER * 2);
        injector.WriteMemory16(ptr + 0x50, PAD_CROSS * 2);
    }

    if (SwapLBSquare)
    {
        // Swap L trigger and square button
        uintptr_t ptr_292A28 = pattern.get(0, "26 00 05 86", 0);
        uintptr_t ptr_2932A4 = pattern.get(0, "66 00 02 A6 0A 00 04 86", 4);
        uintptr_t ptr_2937EC = pattern.get(0, "0A 00 04 86 ? ? ? ? ? ? ? ? ? ? ? ? 25 10 00 00", 0); // count = 3
        uintptr_t ptr_293BC8 = pattern.get(0, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 00 02", 0);
        uintptr_t ptr_2939C0 = pattern.get(1, "0A 00 04 86 ? ? ? ? ? ? ? ? ? ? ? ? 25 10 00 00", 0); // count = 3
        uintptr_t ptr_294868 = pattern.get(2, "0A 00 04 86 ? ? ? ? ? ? ? ? ? ? ? ? 25 10 00 00", 0); // count = 3
        uintptr_t ptr_293EB4 = pattern.get(1, "86 00 85 94 ? ? ? ? ? ? ? ? ? ? ? ? 25 10 00 00", 0); // count = 3
        uintptr_t ptr_2949A0 = pattern.get(0, "0A 00 85 84 ? ? ? ? 00 00 10 34", 0);
        uintptr_t ptr_2955F4 = pattern.get(1, "26 00 04 86", 0); // count = 2
        uintptr_t ptr_293884 = pattern.get(0, "25 20 00 02 F6 FF 42 28 10 00 B0 8F 14 00 BF 8F", -4);
        uintptr_t ptr_293A58 = pattern.get(0, "25 20 00 02 0B 00 42 28 01 00 42 38", -4);
        uintptr_t ptr_293C18 = pattern.get(0, "01 00 42 38 25 10 00 00 14 00 B0 8F 18 00 BF 8F 08 00 E0 03 20 00 BD 27", 4);

        injector.MakeJMPwNOP(ptr_293EB4, (intptr_t)lcsBrake);
        injector.WriteMemory16(ptr_2949A0, PAD_SQUARE * 2);
        //driveby
        injector.MakeNOP(ptr_2937EC+4);
        MakeInlineWrapper(ptr_2937EC,
            lh(a0, s0, PAD_SQUARE * 2),
            lh(k0, s0, PAD_CIRCLE * 2),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_293884),
            nop()
        );
        injector.MakeNOP(ptr_2939C0+4);
        MakeInlineWrapper(ptr_2939C0,
            lh(a0, s0, PAD_SQUARE * 2),
            lh(k0, s0, PAD_CIRCLE * 2),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_293A58),
            nop()
        );
        injector.MakeNOP(ptr_293BC8+4);
        MakeInlineWrapper(ptr_293BC8,
            lh(a0, s0, PAD_SQUARE * 2),
            lh(k0, s0, PAD_CIRCLE * 2),
            _or(a0, a0, k0),
            bne(a0, zero, 2),
            nop(),
            j(ptr_293C18),
            nop()
        );
        injector.WriteMemory16(ptr_294868, PAD_SQUARE * 2);
        injector.WriteMemory16(ptr_2932A4, PAD_SQUARE * 2);

        injector.WriteMemory16(ptr_292A28, PAD_LTRIGGER * 2);
        injector.WriteMemory16(ptr_2955F4, PAD_LTRIGGER * 2);
    }

    if (0)
    {
        // Swap R trigger and Circle
        injector.WriteMemory16(0x293FC0, PAD_LTRIGGER * 2); //shoot
        injector.WriteMemory16(0x293FEC, PAD_LTRIGGER * 2); //melee
        
        injector.WriteMemory16(0x2942B8, PAD_CIRCLE * 2); //aim
        injector.WriteMemory16(0x294268, PAD_CIRCLE * 2); //aim
        injector.WriteMemory16(0x293C48, PAD_CIRCLE * 2);
        injector.WriteMemory16(0x2945B0, PAD_CIRCLE * 2);
        injector.WriteMemory16(0x294600, PAD_CIRCLE * 2);
    }

    if (strcmp(ForceAspectRatio, "auto") != 0)
    {
        char* ch;
        ch = strtok(ForceAspectRatio, ":");
        int x = str2int(ch, 10);
        ch = strtok(NULL, ":");
        int y = str2int(ch, 10);
        fAspectRatio = (float)x / (float)y;
    }
    else
    {
        float ar = 0.0f;
        sceIoDevctl("kemulator:", EMULATOR_DEVCTL__GET_ASPECT_RATIO, NULL, 0, &ar, sizeof(ar));
        if (ar)
            fAspectRatio = ar;
    }
    uintptr_t ptr_130C4C = pattern.get(0, "E3 3F 05 3C 39 8E A5 34 00 68 85 44 25 28 00 00", 0);
    injector.MakeInlineLUIORI(ptr_130C4C, fAspectRatio);
    
    if (Enable60FPS)
    {
        //60 fps
        uintptr_t ptr_2030C8 = pattern.get(0, "02 00 84 2C ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 20);
        injector.MakeNOP(ptr_2030C8);
    }

    if (fHudScale > 0.0f)
    {
        /* Patterns */
        uintptr_t ptr_18321C = pattern.get(0, "8B 3F 04 3C 80 42 04 3C", 0); // count = 2
        uintptr_t ptr_18322C = pattern.get(0, "8B 3F 04 3C 1F 85 84 34", 0); // count = 2
        uintptr_t ptr_183230 = pattern.get(0, "1F 85 84 34 00 60 84 44 42 6B 0C 46", 0); // count = 2
        uintptr_t ptr_18323C = pattern.get(0, "B0 43 04 3C", 0); // count = 2
        uintptr_t ptr_183244 = pattern.get(0, "28 42 05 3C", 0);
        uintptr_t ptr_183248 = pattern.get(0, "68 42 06 3C", 0);
        uintptr_t ptr_1832D0 = pattern.get(0, "D2 43 05 3C 06 F3 00 46", 0); // count = 2
        uintptr_t ptr_1832D8 = pattern.get(0, "48 E1 A5 34 46 A3 00 46", 0); // count = 2
        uintptr_t ptr_183490 = pattern.get(2, "3E A0 0C 46 00 00 00 00 ? ? ? ? ? ? ? ? 25 20 00 02", 4); // count = 4
        uintptr_t ptr_1834A4 = pattern.get(0, "06 D3 00 46 00 40 05 3C 06 05 00 46", 0); // count = 2
        uintptr_t ptr_1834A8 = pattern.get(0, "00 40 05 3C 06 05 00 46", 0); // count = 2
        uintptr_t ptr_1834D0 = pattern.get(0, "33 3F 05 3C 33 33 A5 34 25 20 00 02", 0); // count = 2
        uintptr_t ptr_1834E4 = pattern.get(0, "B3 3F 04 3C 06 05 00 46", 0); // count = 2
        uintptr_t ptr_1834EC = pattern.get(0, "33 33 84 34 00 60 84 44 ? ? ? ? 25 20 00 02", 0); // count = 2
        uintptr_t ptr_183508 = pattern.get(0, "B9 43 04 3C 00 80 84 34", 0); // count = 2
        uintptr_t ptr_183518 = pattern.get(1, "18 42 04 3C 00 68 84 44", 0); // count = 2
        uintptr_t ptr_1836C8 = pattern.get(1, "8B 3F 04 3C 80 42 04 3C", 0);
        uintptr_t ptr_1836D8 = pattern.get(1, "8B 3F 04 3C 1F 85 84 34", 0);
        uintptr_t ptr_1836DC = pattern.get(1, "1F 85 84 34 00 60 84 44 42 6B 0C 46", 0);
        uintptr_t ptr_1836E8 = pattern.get(1, "B0 43 04 3C", 0);
        uintptr_t ptr_1836F0 = pattern.get(0, "C8 41 05 3C 24 42 06 3C", 0);
        uintptr_t ptr_1836F4 = pattern.get(0, "24 42 06 3C", 0);
        uintptr_t ptr_18377C = pattern.get(1, "D2 43 05 3C 06 F3 00 46", 0);
        uintptr_t ptr_183784 = pattern.get(1, "48 E1 A5 34 46 A3 00 46", 0);
        uintptr_t ptr_18393C = pattern.get(3, "3E A0 0C 46 00 00 00 00 ? ? ? ? ? ? ? ? 25 20 00 02", 4); // count = 4
        uintptr_t ptr_183950 = pattern.get(1, "06 D3 00 46 00 40 05 3C 06 05 00 46", 0);
        uintptr_t ptr_183954 = pattern.get(1, "00 40 05 3C 06 05 00 46", 0);
        uintptr_t ptr_18397C = pattern.get(1, "33 3F 05 3C 33 33 A5 34 25 20 00 02", 0);
        uintptr_t ptr_183990 = pattern.get(1, "B3 3F 04 3C 06 05 00 46", 0);
        uintptr_t ptr_183998 = pattern.get(1, "33 33 84 34 00 60 84 44 ? ? ? ? 25 20 00 02", 0);
        uintptr_t ptr_1839B4 = pattern.get(1, "B9 43 04 3C 00 80 84 34", 0);
        uintptr_t ptr_1839C4 = pattern.get(0, "A8 41 04 3C", 0);
        uintptr_t ptr_183BE0 = pattern.get(0, "B8 43 06 3C", 0); // count = 2
        uintptr_t ptr_183BF0 = pattern.get(0, "70 42 06 3C 01 00 05 34", 0);
        uintptr_t ptr_183D78 = pattern.get(1, "B8 43 06 3C", 0);
        uintptr_t ptr_183D88 = pattern.get(0, "00 41 06 3C 25 28 00 00", 0);
        uintptr_t ptr_183E2C = pattern.get(0, "50 41 04 3C ? ? ? ? 00 B0 84 44", 0);
        uintptr_t ptr_183FB4 = pattern.get(0, "80 41 04 3C 00 78 84 44 25 20 A0 02", 0);
        uintptr_t ptr_185BDC = pattern.get(0, "E3 43 04 3C 00 80 84 34", 0);
        uintptr_t ptr_185BE4 = pattern.get(0, "04 42 05 3C", 0);
        uintptr_t ptr_185BF0 = pattern.get(0, "1C 42 04 3C", 0);
        uintptr_t ptr_185BF4 = pattern.get(0, "00 42 06 3C 00 68 84 44", 0);
        uintptr_t ptr_185C5C = pattern.get(0, "D3 43 05 3C", 0);
        uintptr_t ptr_185C68 = pattern.get(0, "E0 40 05 3C 00 68 85 44", 0);
        uintptr_t ptr_185C70 = pattern.get(0, "F4 43 05 3C", 0);
        uintptr_t ptr_185C78 = pattern.get(1, "8E 42 05 3C", 0);
        uintptr_t ptr_185CFC = pattern.get(0, "57 3E 04 3C", 0);
        uintptr_t ptr_185D08 = pattern.get(0, "11 3F 04 3C 85 EB 84 34 ? ? ? ? 00 68 84 44", 0);
        uintptr_t ptr_186000 = pattern.get(0, "34 42 06 3C 00 A0 86 44", 0);
        uintptr_t ptr_18600C = pattern.get(0, "DF 43 06 3C", 0); // count = 2
        uintptr_t ptr_186034 = pattern.get(0, "E0 43 05 3C", 0);
        uintptr_t ptr_18605C = pattern.get(0, "E2 43 06 3C", 0);
        uintptr_t ptr_1860A8 = pattern.get(1, "DF 43 06 3C", 0);
        uintptr_t ptr_1860B8 = pattern.get(0, "34 42 06 3C 25 28 00 00", 0);
        uintptr_t ptr_1865E4 = pattern.get(0, "E2 43 04 3C", 0);
        uintptr_t ptr_186670 = pattern.get(0, "9A 42 06 3C", 0); // count = 2
        uintptr_t ptr_1866AC = pattern.get(1, "9A 42 06 3C", 0);
        uintptr_t ptr_1866CC = pattern.get(0, "88 41 04 3C 00 60 84 44", 0);
        uintptr_t ptr_18721C = pattern.get(4, "F8 3E 04 3C 89 B5 84 34 00 60 84 44 87 3F 04 3C 02 2B 84 34 ? ? ? ? 00 68 84 44", 0); // count = 14
        uintptr_t ptr_187228 = pattern.get(0, "87 3F 04 3C 02 2B 84 34 ? ? ? ? 00 68 84 44 ? ? ? ? 00 60 84 44", 0);
        uintptr_t ptr_1872D4 = pattern.get(1, "EB 43 05 3C 00 60 85 44", 0); // count = 2
        uintptr_t ptr_1872E0 = pattern.get(0, "86 43 05 3C ? ? ? ? ? ? ? ? 00 68 85 44", 0);
        uintptr_t ptr_18788C = pattern.get(5, "F8 3E 04 3C 89 B5 84 34 00 60 84 44 87 3F 04 3C 02 2B 84 34 ? ? ? ? 00 68 84 44", 0);
        uintptr_t ptr_187898 = pattern.get(0, "87 3F 04 3C 02 2B 84 34 ? ? ? ? 00 68 84 44 ? ? ? ? 00 60 84 44", 0);
        uintptr_t ptr_1878BC = pattern.get(0, "EB 43 04 3C", 0);
        uintptr_t ptr_1878C4 = pattern.get(0, "7A 43 04 3C 06 B3 00 46", 0);


        /* Health bar */
        injector.MakeInlineLUIORI(ptr_18323C, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_183244, adjustTopRightY(42.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1832D0, adjustRightX(420.0f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1832D8, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_183248, adjustTopRightY(58.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_183230, fHudScale * 1.09f); // 1%
        injector.MakeInlineLUIORI(ptr_18321C, fHudScale * 1.085938f); // 1%
        injector.MakeInlineLUIORI(ptr_18322C, fHudScale * 1.085938f); // 1%

        injector.MakeInlineLUIORI(ptr_183508, adjustRightX(371.0f, fHudScale));
        injector.MakeInlineLUIORI(ptr_183518, adjustTopRightY(38.0f, fHudScale));
        injector.MakeInlineLUIORI(ptr_1834D0, fHudScale * 0.7f); 
        injector.MakeInlineLUIORI(ptr_1834E4, fHudScale * 1.4f); 
        injector.MakeInlineLUIORI(ptr_1834EC, fHudScale * 1.4f); //Small "+" Font scale

        injector.WriteInstr(ptr_183490, lui(a1, 0));
        injector.MakeInlineLUIORI(ptr_183490, fHudScale * 1.0f); //large "+" Font scale
        injector.WriteInstr(ptr_1834A4, mtc1(a1, f12));
        injector.MakeInlineLUIORI(ptr_1834A8, fHudScale * 2.0f); //large "+" Font scale

        /* Armor bar */
        injector.MakeInlineLUIORI(ptr_1836E8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1836F0, adjustTopRightY(25.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_18377C, adjustRightX(420.0f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_183784, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1836F4, adjustTopRightY(41.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_1836DC, fHudScale * 1.09f); // 1%
        injector.MakeInlineLUIORI(ptr_1836C8, fHudScale * 1.085938f); // 1%
        injector.MakeInlineLUIORI(ptr_1836D8, fHudScale * 1.085938f); // 1%

        injector.MakeInlineLUIORI(ptr_1839B4, adjustRightX(371.0f, fHudScale));
        injector.MakeInlineLUIORI(ptr_1839C4, adjustTopRightY(21.0f, fHudScale));
        injector.MakeInlineLUIORI(ptr_18397C, fHudScale * 0.7f);
        injector.MakeInlineLUIORI(ptr_183990, fHudScale * 1.4f);
        injector.MakeInlineLUIORI(ptr_183998, fHudScale * 1.4f); // Small "+" Font scale

        injector.WriteInstr(ptr_18393C, lui(a1, 0));
        injector.MakeInlineLUIORI(ptr_18393C, fHudScale * 1.0f); //large "+" Font scale
        injector.WriteInstr(ptr_183950, mtc1(a1, f12));
        injector.MakeInlineLUIORI(ptr_183954, fHudScale * 2.0f); //large "+" Font scale

        /* Time, cash numbers, wanted stars */
        injector.MakeInlineLUIORI(ptr_183E2C, fHudScale * 13.0f); // Width
        injector.MakeInlineLUIORI(ptr_183FB4, fHudScale * 16.0f); // Height

        injector.MakeInlineLUIORI(ptr_183D78, adjustRightX(369.0f, fHudScale)); // Time Left X
        injector.MakeInlineLUIORI(ptr_183D88, adjustTopRightY(8.0f, fHudScale)); // Time Top Y

        injector.MakeInlineLUIORI(ptr_183BE0, adjustRightX(369.0f, fHudScale)); // Cash Left X
        injector.MakeInlineLUIORI(ptr_183BF0, adjustTopRightY(60.0f, fHudScale)); // Cash Top Y

        injector.MakeInlineLUIORI(ptr_1865E4, adjustRightX(452.0f, fHudScale)); // Wanted stars Right X
        injector.MakeInlineLUIORI(ptr_186670, adjustTopRightY(77.0f, fHudScale)); // Wanted stars Top Y
        injector.MakeInlineLUIORI(ptr_1866AC, adjustTopRightY(77.0f, fHudScale)); // Wanted stars Top Y (2)
        injector.MakeInlineLUIORI(ptr_1866CC, fHudScale * 17.0f); // Wanted stars Width

        /* Weapon icon & ammo numbers */
        injector.MakeInlineLUIORI(ptr_185C5C, adjustRightX(422.0f, fHudScale)); // Fist icon Left X
        injector.MakeInlineLUIORI(ptr_185C68, adjustTopRightY(7.0f, fHudScale)); // Fist icon Top Y
        injector.MakeInlineLUIORI(ptr_185C70, adjustRightX(488.0f, fHudScale)); // Fist icon Right X
        injector.MakeInlineLUIORI(ptr_185C78, adjustTopRightY(71.0f, fHudScale)); // Fist icon Bottom Y

        injector.MakeInlineLUIORI(ptr_185BDC, adjustRightX(455.0f, fHudScale)); // Weapon icon Center X
        injector.MakeInlineLUIORI(ptr_185BF0, adjustTopRightY(39.0f, fHudScale));  // Weapon icon Center Y
        injector.MakeInlineLUIORI(ptr_185BE4, fHudScale * 33.0f); // Weapon icon Width
        injector.MakeInlineLUIORI(ptr_185BF4, fHudScale * 32.0f); // Weapon icon Height
        injector.MakeInlineLUIORI(ptr_186000, adjustTopRightY(45.0f, fHudScale)); // Ammo Top Y
        injector.MakeInlineLUIORI(ptr_1860B8, adjustTopRightY(45.0f, fHudScale)); // Ammo Top Y
        injector.MakeInlineLUIORI(ptr_18600C, adjustRightX(446.0f, fHudScale)); // Single clip ammo number Left X
        injector.MakeInlineLUIORI(ptr_186034, adjustRightX(448.0f, fHudScale)); // Ammo Left X
        injector.MakeInlineLUIORI(ptr_18605C, adjustRightX(452.0f, fHudScale)); // '-' Left X
        injector.MakeInlineLUIORI(ptr_1860A8, adjustRightX(447.0f, fHudScale)); // Clip ammo Left X
        injector.MakeInlineLUIORI(ptr_185CFC, fHudScale * 0.21f); // Font scale
        injector.MakeInlineLUIORI(ptr_185D08, fHudScale * 0.57f); // Font scale

        /* Zone name */
        //injector.MakeInlineLUIORI(ptr_1872D4, adjustRightX(470.0f, fHudScale)); // Right X
        ////injector.MakeInlineLUIORI(ptr_1872E0, adjustBottomRightY(268.0f, fHudScale)); // Top Y
        //injector.MakeInlineLUIORI(ptr_18721C, fHudScale * 0.48576f); // Font scale
        //injector.MakeInlineLUIORI(ptr_187228, fHudScale * 1.056f); // Font scale

        /* Vehicle name */
        //injector.MakeInlineLUIORI(ptr_1878BC, adjustRightX(470.0f, fHudScale)); // Right X
        //injector.MakeInlineLUIORI(ptr_1878C4, adjustBottomRightY(250.0f, fHudScale)); // Top Y
        //injector.MakeInlineLUIORI(ptr_18788C, fHudScale * 0.48576f); // Font scale
        //injector.MakeInlineLUIORI(ptr_187898, fHudScale * 1.056f); // Font scale
    }

    /* Radar */
    {
        uintptr_t ptr_182F94 = pattern.get(0, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0); // count = 2
        uintptr_t ptr_182FA0 = pattern.get(0, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0); // count = 2
        uintptr_t ptr_182FC4 = pattern.get(1, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_182FD0 = pattern.get(1, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0);
        uintptr_t ptr_182FEC = pattern.get(0, "40 41 05 3C ? ? ? ? 00 00 85 44", 0);
        uintptr_t ptr_183018 = pattern.get(0, "2A 43 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_183024 = pattern.get(0, "44 43 04 3C 00 00 84 44", 0);

        if (fRadarScale > 0.0f)
        {
            if (fRadarPosX)
                injector.MakeInlineLUIORI(ptr_182FEC, fRadarScale * fRadarPosX);  // Left X
            else
                injector.MakeInlineLUIORI(ptr_182FEC, fRadarScale * 12.0f);  // Left X

            if (fRadarPosY)
                injector.MakeInlineLUIORI(ptr_183024, adjustBottomRightY(fRadarPosY, fRadarScale)); // Top Y
            else
                injector.MakeInlineLUIORI(ptr_183024, adjustBottomRightY(196.0f, fRadarScale)); // Top Y

            injector.MakeInlineLUIORI(ptr_182FA0, fRadarScale * 65.8f);  // Size X
            injector.MakeInlineLUIORI(ptr_182FD0, fRadarScale * 65.8f);  // Size Y

        }

        if (fMPRadarScale > 0.0f)
        {
            if (fMPRadarPosY)
                injector.MakeInlineLUIORI(ptr_183018, adjustBottomRightY(fMPRadarPosY, (fMPRadarScale ? fMPRadarScale : 1.0f))); // Top Y (Multiplayer)
            injector.MakeInlineLUIORI(ptr_182F94, fMPRadarScale * 92.0f);  // Size X (Multiplayer)
            injector.MakeInlineLUIORI(ptr_182FC4, fMPRadarScale * 92.0f);  // Size Y (Multiplayer)
        }
    }

    if (RenderLodLights)
    {
        uintptr_t ptr_1F6098 = pattern.get(0, "B0 FF BD 27 50 00 A2 8F FF 00 4A 31", 0);
        CCoronas__RegisterCorona = (void*)ptr_1F6098;
        uintptr_t ptr_15700 = pattern.get(0, "00 29 05 00 21 30 05 00 C0 28 05 00 21 30 C5 00 80 28 05 00 21 28 C5 00 21 20 A4 00 AC 01 84 84 10 00 05 34", -12);
        uintptr_t TheCamera = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_15700 + 0)) << 16 | (uint32_t)(*(uint16_t*)(ptr_15700 + 4)));
        pCamPos = (CVector*)(TheCamera + 0xA70); //0xA70 at 0xEB598
        uintptr_t ptr_38358 = pattern.get(0, "0C 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 80 3F 04 3C 00 60 84 44", 4);
        CurrentTimeHoursOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_38358 + 0)) << 16) + *(int16_t*)(ptr_38358 + 4);
        CurrentTimeMinutesOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_38358 + 8)) << 16) + *(int16_t*)(ptr_38358 + 12);
        uintptr_t ptr_2723C = pattern.get(0, "34 00 B0 AE 00 60 84 44", -8);
        CTimer__m_snTimeInMillisecondsPauseModeOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_2723C + 0)) << 16) + *(int16_t*)(ptr_2723C + 4);
        uintptr_t ptr_15E4E4 = pattern.get(0, "48 42 04 3C 00 68 84 44 C3 63 0D 46", -8);
        CTimer__ms_fTimeStepOffset = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_15E4E4 + 0)) << 16) + *(int16_t*)(ptr_15E4E4 + 4);

        SceUID block_id = 0;
        const int corona_struct_size = 112;
        uintptr_t aCoronas = injector.AllocMemBlock(corona_struct_size * CoronaLimit, &block_id);

        uintptr_t ptr_1F5ED0 = pattern.get(0, "24 00 B1 AF 2C 00 B3 AF 3C 60 0D 46", -4);
        uintptr_t ptr_1F5EE4 = pattern.get(0, "3C 60 0D 46 21 28 A4 00", 8);
        uintptr_t ptr_1F5F94 = pattern.get(0, "38 00 04 2A ? ? ? ? 70 00 52 26", 0);
        uintptr_t ptr_1F6048 = pattern.get(0, "00 00 05 34 ? ? ? ? C0 31 05 00 00 39 05 00", -4); // count = 2
        uintptr_t ptr_1F6050 = pattern.get(0, "C0 31 05 00 00 39 05 00 23 30 C7 00 21 30 C4 00", -4);
        uintptr_t ptr_1F6070 = pattern.get(0, "38 00 A6 28 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_1F6150 = pattern.get(0, "00 00 07 34 01 00 08 34", -4);
        uintptr_t ptr_1F6160 = pattern.get(1, "00 00 00 00 ? ? ? ? C0 41 07 00", -8); // count = 2
        uintptr_t ptr_1F6198 = pattern.get(0, "38 00 E8 28", 0);
        uintptr_t ptr_1F619C = pattern.get(0, "38 00 04 34 ? ? ? ? 00 00 00 00 C0 21 07 00", 0);
        uintptr_t ptr_1F71E0 = pattern.get(0, "64 00 B0 AF C0 40 04 3C", -8);
        uintptr_t ptr_1F7208 = pattern.get(0, "06 00 17 34 68 00 A5 AF", -4);
        uintptr_t ptr_1F78E0 = pattern.get(0, "38 00 C4 2B", 0);
        uintptr_t ptr_1F7ADC = pattern.get(0, "38 00 04 2A ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_1F7B70 = pattern.get(1, "B4 00 B4 E7 B8 00 B6 E7 BC 00 B8 E7", -8); // count = 3
        uintptr_t ptr_1F7BB0 = pattern.get(0, "00 00 05 34 BF FF 04 24", -4);
        uintptr_t ptr_1F7BE4 = pattern.get(0, "38 00 A6 28 ? ? ? ? C0 31 05 00", 0);
        uintptr_t ptr_1F7FF4 = pattern.get(0, "38 00 24 2A", 0);
        uintptr_t ptr_1F8A3C = pattern.get(0, "00 00 0F 34 01 00 19 34", -4);
        uintptr_t ptr_1F8A48 = pattern.get(0, "38 00 18 34", 0);
        uintptr_t ptr_1F8A4C = pattern.get(0, "C0 C9 0F 00 00 81 0F 00 23 C8 30 03", -8); // count = 2
        uintptr_t ptr_1F8A7C = pattern.get(0, "38 00 F9 29 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_1F8AD0 = pattern.get(0, "38 00 F9 29 ? ? ? ? C0 C9 0F 00", 0);
        uintptr_t ptr_1C0E78 = pattern.get(0, "FF 00 04 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 60 02 ? ? ? ? 00 00 00 00", 28);

        injector.WriteInstr(ptr_1F5ED0, lui(s2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6048, lui(a0, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6150, lui(a2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F71E0, lui(s2, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F7B70, lui(s5, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F8A3C, lui(t6, HIWORD(aCoronas)));
        injector.WriteInstr(ptr_1F5EE4, addiu(s2, s2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6050, addiu(a0, a0, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F6160, addiu(a2, a2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F7208, addiu(s2, s2, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F7BB0, addiu(s5, s5, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F8A4C, addiu(t6, t6, LOWORD(aCoronas)));
        injector.WriteInstr(ptr_1F5F94, slti(a0, s0, CoronaLimit));
        injector.WriteInstr(ptr_1F6070, slti(a2, a1, CoronaLimit));
        injector.WriteInstr(ptr_1F6198, slti(t0, a3, CoronaLimit));
        injector.WriteInstr(ptr_1F619C, li(a0, CoronaLimit));
        injector.WriteInstr(ptr_1F78E0, slti(a0, fp, CoronaLimit));
        injector.WriteInstr(ptr_1F7ADC, slti(a0, s0, CoronaLimit));
        injector.WriteInstr(ptr_1F7FF4, slti(a0, s1, CoronaLimit));
        injector.WriteInstr(ptr_1F7BE4, slti(a2, a1, CoronaLimit));
        injector.WriteInstr(ptr_1F8A7C, slti(t9, t7, CoronaLimit));
        injector.WriteInstr(ptr_1F8A48, li(t8, CoronaLimit));
        injector.WriteInstr(ptr_1F8AD0, slti(t9, t7, CoronaLimit));

        // Coronas Render
        injector.MakeJAL(ptr_1C0E78, (intptr_t)RegisterLODLights);
    }

    {
        uintptr_t ptr_1BEE84 = pattern.get(1, "B0 FF BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 44 00 B0 AF 48 00 BF AF", 0);
        uintptr_t unk_38AE20 = GetAbsoluteAddress(ptr_1BEE84, 4, 8);
        byte_38AF51 = (uint8_t*)(unk_38AE20 + *(int16_t*)(ptr_1BEE84 + 12));

        uintptr_t ptr_1BD52C = pattern.get(1, "00 00 00 00 ? ? ? ? 00 60 80 44 ? ? ? ? 32 68 0C 46 00 00 00 00", 0);
        flt_3450E4 = (float*)GetAbsoluteAddress(ptr_1BD52C, 4, 12);
        
        uintptr_t ptr_1C086C = pattern.get(0, "A4 00 BE AF A8 00 BF AF ? ? ? ? 00 00 00 00", 8);
        injector.MakeJAL(ptr_1C086C, (intptr_t)GameLoopStuff);
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