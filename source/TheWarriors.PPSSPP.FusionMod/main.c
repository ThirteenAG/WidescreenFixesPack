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

#define MODULE_NAME_INTERNAL "WARR"
#define MODULE_NAME "TheWarriors.PPSSPP.FusionMod"
#define LOG_PATH "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionMod/TheWarriors.PPSSPP.FusionMod.log"
#define INI_PATH "ms0:/PSP/PLUGINS/TheWarriors.PPSSPP.FusionMod/TheWarriors.PPSSPP.FusionMod.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, 0x1007, 1, 0);
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

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

uintptr_t byte_8E8C2FC;
void GetRs(uintptr_t addr, char a1, char a2)
{
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);

    //*(int8_t*)(addr + 0x1A) = a1;//pad.Rsrv[0];
    //*(int8_t*)(addr + 0x1B) = a2;//pad.Rsrv[1];

    *(uint8_t*)(addr + 0x1A) = pad.Rsrv[0];
    float Ry = -(((float)pad.Rsrv[1] - 255.0f) / 255.0f);
    if (byte_8E8C2FC && *(uint8_t*)byte_8E8C2FC == 0)
        *(uint8_t*)(addr + 0x1B) = Ry * 255.0f;
    else
        *(uint8_t*)(addr + 0x1B) = -pad.Rsrv[1] + -1;
}

int OnModuleStart() 
{
    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);
    int UnthrottleEmuDuringLoading = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1);
    
    char szForceAspectRatio[100];
    char* ForceAspectRatio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", szForceAspectRatio, sizeof(szForceAspectRatio));

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first("10 00 A5 27 ? ? ? ? ? ? ? ? ? ? ? ? 21 28 00 00", 0);
        injector.MakeNOP(ptr + 12);
        injector.MakeNOP(ptr + 28);
        injector.MakeNOP(ptr + 44);
    }
    
    if (DualAnalogPatch)
    {
        uintptr_t ptr = pattern.get_first("21 10 A2 00 FF 00 C6 30 21 28 A4 00 28 04 A6 A0 08 00 E0 03 00 00 46 A0", 0);
        byte_8E8C2FC = GetAbsoluteAddress(ptr, -8, -4);
        
        ptr = pattern.get_first("21 10 51 00 ? ? ? ? 1C 00 50 A4", 4);

        MakeInlineWrapper(ptr + 0x00,
            sh(s0, v0, 0x1C),
            lbu(a2, sp, 0x1B),
            lbu(a1, sp, 0x1A)
        );

        injector.MakeNOP(ptr + 0x04);
        injector.WriteInstr(ptr + 0x08, b(21));
        injector.MakeNOP(ptr + 0x0C);

        MakeInlineWrapperWithNOP(ptr + 0x80,
            sw(zero, s1, 0x10),
            sw(zero, s1, 0x14),
            move(a0, s1),
            jal((intptr_t)GetRs),
            nop()

        );

        //injector.WriteInstr(ptr + 0x6C, sb(a2, s1, 0x1B));
        //injector.WriteInstr(ptr + 0x8C, sb(a1, s1, 0x1A));

        injector.MakeNOP(ptr + 0x8C);

        sceKernelIcacheInvalidateRange((const void*)ptr, 0x90);
    }
    
    if (Enable60FPS)
    {
        uintptr_t ptr = pattern.get_first("02 00 42 2C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 00 00 A2 8C", 4);
        injector.MakeNOP(ptr);
    }

    if (strlen(ForceAspectRatio) && ForceAspectRatio[0] != '0')
    {
        // Default is 512/320 for some reason
        int x = 512;
        int y = 320;
        float fAspectRatio = (float)x / (float)y;
        
        if (strcmp(ForceAspectRatio, "auto") != 0)
        {
            char* ch;
            ch = strtok(ForceAspectRatio, ":");
            if (ch)
            {
                x = str2int(ch, 10);
                ch = strtok(NULL, ":");
                if (ch)
                {
                    y = str2int(ch, 10);
                    fAspectRatio = (float)x / (float)y;
                }
            }
        }
        else
        {
            float ar = 0.0f;
            sceIoDevctl("kemulator:", EMULATOR_DEVCTL__GET_ASPECT_RATIO, NULL, 0, &ar, sizeof(ar));
            if (ar)
                fAspectRatio = ar;
            else
                fAspectRatio = 16.0f / 9.0f;
        }

        uintptr_t ptr_28C = pattern.get(0, "94 18 C1 E7 03 03 01 46", 4);
        MakeInlineWrapper(ptr_28C,
            lui(t9, HIWORD(fAspectRatio)),
            ori(t9, t9, LOWORD(fAspectRatio)),
            mtc1(t9, f12)
        );

        float fHudScale = fAspectRatio;
        uintptr_t ptr_B90 = pattern.get(0, "02 00 02 46 00 00 C3 8F", -4);
        MakeInlineWrapper(ptr_B90,
            lui(t9, HIWORD(fHudScale)),
            ori(t9, t9, LOWORD(fHudScale)),
            mtc1(t9, f1)
        );
    }

    //{
    //    float fHudSize = 1.5f;
    //    uintptr_t ptr_2FC = pattern.get(0, "CD 08 00 46 ? ? ? ? ? ? ? ? ? ? ? ? 05 00 63 34", 8);
    //    MakeInlineWrapper(ptr_2FC,
    //        lui(v0, HIWORD(fHudSize)),
    //        ori(v0, v0, LOWORD(fHudSize)),
    //        mtc1(v0, f4)
    //    );
    //}

    if (UnthrottleEmuDuringLoading)
    {
        uintptr_t ptr = pattern.get_first("01 00 05 24 08 00 B2 AF 00 00 B0 AF", 0);
        MakeInlineWrapperWithNOP(ptr,
            addiu(a1, zero, 1),
            sw(s2, sp, 8),
            jal((uintptr_t)UnthrottleEmuEnable),
            nop()
        );

        ptr = pattern.get_first("08 00 BF AF 04 00 B1 AF ? ? ? ? 00 00 B0 AF 06 00 03 24", 0);
        MakeInlineWrapperWithNOP(ptr,
            sw(ra, sp, 8),
            sw(s1, sp, 4),
            jal((uintptr_t)UnthrottleEmuDisable),
            nop()
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

                if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
                {
                    injector.SetGameBaseAddress(info.text_addr, info.text_size);
                    pattern.SetGameBaseAddress(info.text_addr, info.text_size);
                    inireader.SetIniPath(INI_PATH); //crashes on sceIoClose for some reason
                    inireader.SetIniPath(INI_PATH); //but works if called twice (!?)
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