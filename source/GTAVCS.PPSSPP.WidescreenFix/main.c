#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <systemctrl.h>

#include "includes/log.h"
#include "includes/injector.h"
#include "includes/patterns.h"
#include "includes/inireader.h"
#include "includes/gvm.h"
#define gv(...) _get_addr_for_game_version(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#include "includes/mips.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.ini"

PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);

SceKernelModuleInfo ModuleInfo;

enum GtaPad {
  PAD_LX = 1,
  PAD_LY = 2,
  PAD_RX = 3,
  PAD_RY = 4,
};

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

int OnModuleStart(SceKernelModuleInfo* mod) {
    logger.Write(LOG_PATH, "Hello...\n");
    logger.WriteF(LOG_PATH, "%x\n", 0);

    injector.base_addr = mod->text_addr;
    pattern.base_addr = mod->text_addr;
    inireader.SetIniPath(INI_PATH);

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

    // --> gv(0x123, 0x456)
    */

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first(mod->text_addr, mod->text_size, "00 00 00 00 ? ? ? ? 00 00 00 00 ? ? 04 3C 25 28 00 00", -4);
        injector.MakeNOP(ptr);
    }

    if (DualAnalogPatch)
    {
        // Implement right analog stick
        uintptr_t ptr = pattern.get_first(mod->text_addr, mod->text_size, "06 00 00 10 03 00 A7 A3", 0);
        injector.WriteInstr(ptr + 0x24, 
            sh(a1, sp, 0)
        );
        injector.WriteInstr(ptr + 0x1C,
            lhu(a1, sp, 0xE)
        );

        // Redirect camera movement
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "36 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 16 00 40 10", 0);
        injector.MakeNOP(ptr + 0x00);
        injector.WriteMemory8(ptr + 0x10 + 0x2, 0x00);  // beqz -> b
        injector.MakeJAL(ptr + 0x8C, (intptr_t)cameraX);
        injector.MakeNOP(ptr + 0x108);
        injector.WriteMemory8(ptr + 0x118 + 0x2, 0x00); // beqz -> b
        injector.MakeJAL(ptr + 0x144, (intptr_t)cameraY);

        // Redirect gun aim movement
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "40 00 80 04 ? ? ? ? 3E 00 80 10", 0);
        injector.MakeJAL(ptr + 0x50, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x7C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x8C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x158, (intptr_t)aimY);
        injector.MakeJAL(ptr + 0x1BC, (intptr_t)aimY);

        // Allow using L trigger when walking
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "0E 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 08 00 80 10 ? ? ? ? ? ? ? ? 03 00 80 04", 0);
        injector.MakeNOP(ptr + 0x10);
        injector.MakeNOP(ptr + 0x9C);

        // Force L trigger value in the L+camera movement function
        ptr = pattern.get_first(mod->text_addr, mod->text_size, "0A 00 C7 84", 0);
        injector.WriteInstr(ptr + 0x00,
            li(a3, -0x1)
        );
    }



    //if (true)
    {
        float f = 0.0f;
        //radar
        {
            float fRadarScale = 0.75f;
            float fRadarXScale = 1.0f;
            float fRadarYScale = 1.0f;
            float fRadarXOffset = 10.0f;
            float fRadarYOffset = 60.0f;

            uintptr_t ptr_1B6A8C = pattern.get_first(mod->text_addr, mod->text_size, "40 41 05 3C ? ? ? ? 00 00 85 44", 0);
            uintptr_t ptr_1B6AB4 = pattern.get_first(mod->text_addr, mod->text_size, "2A 43 04 3C ? ? ? ? 00 00 84 44", 0);
            uintptr_t ptr_1B6AC0 = pattern.get_first(mod->text_addr, mod->text_size, "44 43 04 3C 00 00 84 44", 0);
            uintptr_t ptr_1B6ADC = pattern.get(0, mod->text_addr, mod->text_size, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0);
            uintptr_t ptr_1B6AEC = pattern.get(0, mod->text_addr, mod->text_size, "83 42 04 3C 9A 99 84 34 00 00 84 44 08 00 E0 03", 4);
            uintptr_t ptr_1B6B08 = pattern.get(1, mod->text_addr, mod->text_size, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0);
            uintptr_t ptr_1B6B18 = pattern.get(1, mod->text_addr, mod->text_size, "83 42 04 3C 9A 99 84 34 00 00 84 44 08 00 E0 03", 4);

            injector.MakeInlineLUIORI(ptr_1B6A8C, a1, fRadarXOffset + fRadarScale * 12.0f);
            injector.MakeInlineLUIORI(ptr_1B6AB4, a0, fRadarYOffset + fRadarScale * 170.0f);
            injector.MakeInlineLUIORI(ptr_1B6AC0, a0, fRadarYOffset + fRadarScale * 196.0f);
            injector.MakeInlineLUIORI(ptr_1B6ADC, a0, fRadarXOffset + fRadarScale * 92.0f);
            injector.MakeInlineLUIORI(ptr_1B6AEC, a0, fRadarScale * fRadarXScale * 65.8f);
            injector.MakeInlineLUIORI(ptr_1B6B08, a0, fRadarScale * 92.0f);
            injector.MakeInlineLUIORI(ptr_1B6B18, a0, fRadarScale * 65.8f);
        }

        {
            float fHudScale = 0.75f;
            float fHudXOffset = 110.0f;
            float fHudYOffset = 0.0f;

            uintptr_t ptr_1B74C8 = pattern.get_first(mod->text_addr, mod->text_size, "B8 43 05 3C", 0);
            uintptr_t ptr_1B7570 = pattern.get_first(mod->text_addr, mod->text_size, "E0 40 07 3C 06 A3 00 46", 0);
            uintptr_t ptr_1B7594 = pattern.get_first(mod->text_addr, mod->text_size, "70 42 07 3C 06 A3 00 46", 0);
            uintptr_t ptr_1B7C98 = pattern.get_first(mod->text_addr, mod->text_size, "8B 3F 04 3C 80 42 04 3C 06 D6 00 46", 0);
            uintptr_t ptr_1B7CA8 = pattern.get_first(mod->text_addr, mod->text_size, "8B 3F 04 3C 1F 85 84 34", 0);
            uintptr_t ptr_1B7CAC = pattern.get_first(mod->text_addr, mod->text_size, "1F 85 84 34 00 60 84 44 42 6B 0C 46", 0);
            uintptr_t ptr_1B7CB8 = pattern.get_first(mod->text_addr, mod->text_size, "B0 43 04 3C 2C 00 B4 E7", 0);
            uintptr_t ptr_1B7CC0 = pattern.get_first(mod->text_addr, mod->text_size, "28 42 05 3C 68 42 06 3C 00 A0 85 44", 0);
            uintptr_t ptr_1B7CC4 = pattern.get_first(mod->text_addr, mod->text_size, "68 42 06 3C 00 A0 85 44", 0);
            uintptr_t ptr_1B7D48 = pattern.get_first(mod->text_addr, mod->text_size, "48 E1 A5 34 46 A3 00 46", 0);
            //uintptr_t ptr_1B7EEC = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B7EFC = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B7F10 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B7F20 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8A1C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8A28 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8AF0 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8B5C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8B6C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8B90 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8B94 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8B98 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B8BB4 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B96FC = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9708 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B97A8 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B992C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9B00 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9B0C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9B18 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9DB8 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9DC4 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9DD0 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9DF8 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9E04 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9E10 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9E18 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1B9E20 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB728 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB738 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB73C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB748 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB750 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB754 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB7D8 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB97C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB98C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB994 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB9A0 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB9B0 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1BB9B8 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C0EFC = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C0F28 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1278 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1280 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1284 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C128C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1304 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1310 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C131C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1324 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C13BC = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C14E0 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C14E8 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C14EC = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C162C = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1644 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1688 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1744 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1C34 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_1C1C80 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);
            //uintptr_t ptr_2ABCC0 = pattern.get_first(mod->text_addr, mod->text_size, "", 0);

            injector.MakeInlineLUIORI(ptr_1B74C8, a1, fHudXOffset + fHudScale * 368.0f);
            injector.MakeInlineLUIORI(ptr_1B7570, a3, 00.0f + fHudScale * 7.0f);
            injector.MakeInlineLUIORI(ptr_1B7594, a3, 00.0f + fHudScale * 60.0f);
            injector.MakeInlineLUIORI(ptr_1B7C98, a0, 00.0f + fHudScale * 1.085938f);
            injector.MakeInlineLUIORI(ptr_1B7CA8, a0, 00.0f + fHudScale * 1.085938f);
            injector.MakeInlineLUIORI(ptr_1B7CAC, a0, 00.0f + fHudScale * 1.09f);
            injector.MakeInlineLUIORI(ptr_1B7CB8, a0, fHudXOffset + fHudScale * 352.0f);
            injector.MakeInlineLUIORI(ptr_1B7CC0, a1, 00.0f + fHudScale * 42.0f);
            injector.MakeInlineLUIORI(ptr_1B7CC4, a2, 00.0f + fHudScale * 58.0f);
            injector.MakeInlineLUIORI(ptr_1B7D48, a1, fHudXOffset + fHudScale * 421.76f);
            //injector.MakeInlineLUIORI(ptr_1B7EEC, a0, 00.0f + fHudScale * 1.25f);
            //injector.WriteInstr(ptr_1B7EFC, li(a1, (int32_t)(fHudXOffset + fHudScale * 362.0f)));
            //injector.MakeInlineLUIORI(ptr_1B7F10, a0, 00.0f + fHudScale * 0.75f);
            //injector.WriteInstr(ptr_1B7F20, li(a1, (int32_t)(fHudXOffset + fHudScale * 366.0f)));
            //injector.MakeInlineLUIORI(ptr_1B8A1C, a2, 00.0f + fHudScale * 80.0f);
            //injector.MakeInlineLUIORI(ptr_1B8A28, t0, 00.0f + fHudScale * 186.0f);
            //injector.MakeInlineLUIORI(ptr_1B8AF0, a0, 00.0f + fHudScale * 0.796875f);
            //injector.MakeInlineLUIORI(ptr_1B8B5C, a1, 00.0f + fHudScale * 41.0f);
            //injector.MakeInlineLUIORI(ptr_1B8B6C, a2, 00.0f + fHudScale * 40.0f);
            //injector.MakeInlineLUIORI(ptr_1B8B90, a3, 00.0f + fHudScale * 31.0f);
            //injector.MakeInlineLUIORI(ptr_1B8B94, t0, 00.0f + fHudScale * 169.0f);
            //injector.MakeInlineLUIORI(ptr_1B8B98, t1, 00.0f + fHudScale * 52.0f);
            //injector.MakeInlineLUIORI(ptr_1B8BB4, a1, 00.0f + fHudScale * 39.0f);
            //injector.MakeInlineLUIORI(ptr_1B96FC, a3, fHudXOffset + fHudScale * 369.0f);
            //injector.MakeInlineLUIORI(ptr_1B9708, a3, 00.0f + fHudScale * 8.0f);
            //injector.MakeInlineLUIORI(ptr_1B97A8, a0, 00.0f + fHudScale * 13.0f);
            //injector.MakeInlineLUIORI(ptr_1B992C, a0, 00.0f + fHudScale * 16.0f);
            //injector.MakeInlineLUIORI(ptr_1B9B00, a0, fHudXOffset + fHudScale * 452.0f);
            //injector.MakeInlineLUIORI(ptr_1B9B0C, a0, 00.0f + fHudScale * 77.0f);
            //injector.MakeInlineLUIORI(ptr_1B9B18, a0, 00.0f + fHudScale * 17.0f);
            //injector.MakeInlineLUIORI(ptr_1B9DB8, a1, fHudXOffset + fHudScale * 352.0f);
            //f = fHudXOffset + fHudScale * 352.0f;
            //injector.WriteInstr(ptr_1B9DC4, li(a1, HIWORD(f)));
            //injector.WriteInstr(ptr_1B9DD0, li(a1, HIWORD(f)));
            //injector.WriteInstr(ptr_1B9DF8, li(a1, HIWORD(f)));
            //injector.MakeInlineLUIORI(ptr_1B9E04, a1, fHudXOffset + fHudScale * 352.0f);
            //injector.MakeInlineLUIORI(ptr_1B9E10, a1, 00.0f + fHudScale * 41.0f);
            //injector.MakeInlineLUIORI(ptr_1B9E18, a1, fHudXOffset + fHudScale * 416.0f);
            //injector.MakeInlineLUIORI(ptr_1B9E20, a1, 00.0f + fHudScale * 25.0f);
            //injector.MakeInlineLUIORI(ptr_1BB728, a0, 00.0f + fHudScale * 1.085938f);
            //injector.MakeInlineLUIORI(ptr_1BB738, a0, 00.0f + fHudScale * 1.085938f);
            //injector.MakeInlineLUIORI(ptr_1BB73C, a0, 00.0f + fHudScale * 1.09f);
            //injector.MakeInlineLUIORI(ptr_1BB748, a0, fHudXOffset + fHudScale * 352.0f);
            //injector.MakeInlineLUIORI(ptr_1BB750, a1, 00.0f + fHudScale * 25.0f);
            //injector.MakeInlineLUIORI(ptr_1BB754, a2, 00.0f + fHudScale * 41.0f);
            //injector.MakeInlineLUIORI(ptr_1BB7D8, a1, fHudXOffset + fHudScale * 421.76f);
            //injector.MakeInlineLUIORI(ptr_1BB97C, a0, 00.0f + fHudScale * 1.25f);
            //injector.WriteInstr(ptr_1BB98C, li(a1, (int32_t)(fHudXOffset + fHudScale * 362.0f)));
            //injector.WriteInstr(ptr_1BB994, li(a2, (int32_t)(00.0f + fHudScale * 18.0f)));
            //injector.MakeInlineLUIORI(ptr_1BB9A0, a0, 00.0f + fHudScale * 0.75f);
            //injector.WriteInstr(ptr_1BB9B0, li(a1, (int32_t)(fHudXOffset + fHudScale * 366.0f)));
            //injector.WriteInstr(ptr_1BB9B8, li(a2, (int32_t)(00.0f + fHudScale * 23.0f)));
            //injector.MakeInlineLUIORI(ptr_1C0EFC, a0, 00.0f + fHudScale * 1.2f);
            //injector.MakeInlineLUIORI(ptr_1C0F28, a0, fHudXOffset + fHudScale * 475.0f);
            //injector.MakeInlineLUIORI(ptr_1C1278, a0, fHudXOffset + fHudScale * 455.0f);
            //injector.MakeInlineLUIORI(ptr_1C1280, a1, 00.0f + fHudScale * 33.0f);
            //injector.MakeInlineLUIORI(ptr_1C1284, a0, 00.0f + fHudScale * 39.0f);
            //injector.MakeInlineLUIORI(ptr_1C128C, a2, 00.0f + fHudScale * 32.0f);
            //f = fHudXOffset + fHudScale * 422.0f;
            //injector.WriteInstr(ptr_1C1304, li(a1, HIWORD(f)));
            //injector.MakeInlineLUIORI(ptr_1C1310, a1, 00.0f + fHudScale * 7.0f);
            //injector.MakeInlineLUIORI(ptr_1C131C, a1, fHudXOffset + fHudScale * 488.0f);
            //injector.MakeInlineLUIORI(ptr_1C1324, a1, 00.0f + fHudScale * 71.0f);
            //injector.MakeInlineLUIORI(ptr_1C13BC, a0, 00.0f + fHudScale * 0.415f);
            //injector.MakeInlineLUIORI(ptr_1C14E0, a1, fHudXOffset + fHudScale * 473.0f);
            //injector.MakeInlineLUIORI(ptr_1C14E8, a1, 40.0f + fHudScale * 45.0f); // ammo y pos
            //injector.MakeInlineLUIORI(ptr_1C14EC, a1, 40.0f + fHudScale * 49.0f); // ammo y pos
            //injector.MakeInlineLUIORI(ptr_1C162C, a1, fHudXOffset + fHudScale * 448.0f);
            //injector.MakeInlineLUIORI(ptr_1C1644, a1, fHudXOffset + fHudScale * 424.0f);
            //injector.MakeInlineLUIORI(ptr_1C1688, a1, fHudXOffset + fHudScale * 454.0f);
            //injector.MakeInlineLUIORI(ptr_1C1744, a1, fHudXOffset + fHudScale * 422.0f);
            //injector.MakeInlineLUIORI(ptr_1C1C34, a0, 00.0f + fHudScale * 1.2f);
            //injector.MakeInlineLUIORI(ptr_1C1C80, a0, fHudXOffset + fHudScale * 475.0f);
            //injector.MakeInlineLUIORI(ptr_2ABCC0, a0, 00.0f + fHudScale * 1.09f);
        }
    }

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
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
                //hook_new_select_table();

                //LOG_Message("Hello world!\n");

            }

        }

        sceKernelDelayThread(1000);
    }
    sceKernelExitDeleteThread(0);
    */
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
                    OnModuleStart(&ModuleInfo);
                    SceUID thid = sceKernelCreateThread(MODULE_NAME, module_thread, 0, 0x10000, 0, NULL);
                    if (thid >= 0)
                        sceKernelStartThread(thid, 0, argp);
                }
            }
        }
    }
    return 0;
}