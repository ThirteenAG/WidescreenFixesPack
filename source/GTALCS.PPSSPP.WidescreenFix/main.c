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

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTALCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/GTALCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTALCS.PPSSPP.WidescreenFix/GTALCS.PPSSPP.WidescreenFix.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
#endif

static const float fPSPResW = 480.0f;
static const float fPSPResH = 272.0f;

enum
{
    EMULATOR_DEVCTL__TOGGLE_FASTFORWARD = 0x30,
    EMULATOR_DEVCTL__GET_ASPECT_RATIO,
    EMULATOR_DEVCTL__GET_SCALE,
};

struct CControllerState
{
    short LEFTSTICKX;
    short LEFTSTICKY;
    short RIGHTSTICKX;
    short RIGHTSTICKY;

    short LEFTSHOULDER1;
    short LEFTSHOULDER2;
    short RIGHTSHOULDER1;
    short RIGHTSHOULDER2;

    short DPADUP;
    short DPADDOWN;
    short DPADLEFT;
    short DPADRIGHT;

    short unk1;
    short unk2;
    short unk3;
    short unk4;

    short START;
    short SELECT;

    short SQUARE;
    short TRIANGLE;
    short CROSS;
    short CIRCLE;

    short LEFTSHOCK;
    short RIGHTSHOCK;
};

struct CPad
{
    char unk[2];
    struct CControllerState NewState;
    char unk2[2];
    struct CControllerState OldState;
    char pad[30];
    int16_t Mode;
    int16_t ShakeDur;
    int16_t DisablePlayerControls;
};

uintptr_t ptr_516C4, ptr_516C4_data;
int (*sub_894D97C)(int a1);
int (*FindPlayerPed)();
int (*FindPlayerVehicle)();
int dword_8B5E144;
char bManualReloadTriggered = 0;
void handleReload(struct CPad* pad)
{
    if (!sub_894D97C || !FindPlayerPed)
        return;

    int v2 = FindPlayerPed();
    if (!v2)
        return;
    uint8_t m_currentWeapon = *(uint8_t*)(v2 + 0x6B8);
    if (!(m_currentWeapon >= 3 && m_currentWeapon <= 8))
        return;
    v2 += 0x590;
    v2 += m_currentWeapon * 0x1C;

    int32_t m_nAmmoInClip = *(int32_t*)(v2 + 0x0C);
    int32_t m_nAmmoTotal = *(int32_t*)(v2 + 0x10);
    if (m_nAmmoInClip == m_nAmmoTotal || m_nAmmoTotal == 0)
        return;

    if (!bManualReloadTriggered)
    {
        //if (pad->NewState.LEFTSHOULDER1 && pad->NewState.DPADUP && pad->OldState.DPADUP == 0)
        if (pad->NewState.LEFTSHOULDER1 && pad->NewState.SQUARE && pad->OldState.SQUARE == 0)
        {
            bManualReloadTriggered = 1;
            injector.WriteMemory32(v2 + 0x08, 2);
            
            int m_nShotTimer = *(int*)(dword_8B5E144) + *(int*)(sub_894D97C(*(int*)(FindPlayerPed() + 0x1C * *(char*)(FindPlayerPed() + 0x6B8) + 0x594)));
            injector.WriteMemory32(v2 + 0x14, m_nShotTimer + 1000); //hack
            pad->NewState.RIGHTSHOULDER1 = 0;
        }
    }
    else
    {
        if (*(int*)(dword_8B5E144) > *(int*)(v2 + 0x14))
        {
            injector.WriteMemory32(ptr_516C4, ptr_516C4_data); // PlayOneShot enable
            injector.WriteMemory32(v2 + 0x08, 2);
            bManualReloadTriggered = 0;
        }
        else
        {
            //*(int32_t*)(v2 + 0x10) += m_nAmmoInClip;
            //*(int32_t*)(v2 + 0x0C) = 0;
            injector.MakeNOP(ptr_516C4); // PlayOneShot disable
            pad->NewState.RIGHTSHOULDER1 = 1;
            injector.WriteMemory32(v2 + 0x08, 2);
        }
    }
}

int16_t CPad__GetAccelerate(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.RIGHTSHOULDER1;
}

int16_t CPad__GetAccelerateNormal(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.CROSS;
}

int16_t CPad__GetBrake(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.LEFTSHOULDER1;
}

int16_t CPad__GetBrakeNormal(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.SQUARE;
}

int16_t CPad__GetRightStickX(struct CPad* pad)
{
    return pad->NewState.RIGHTSTICKX;
}

int16_t CPad__GetRightStickY(struct CPad* pad)
{
    return pad->NewState.RIGHTSTICKY;
}

int16_t aimX(struct CPad* pad) {
    if (pad->NewState.CROSS || pad->NewState.DPADDOWN)
        return pad->NewState.LEFTSTICKX ? pad->NewState.LEFTSTICKX : pad->NewState.RIGHTSTICKX;
    else
        return pad->NewState.LEFTSTICKX ? pad->NewState.LEFTSTICKX * 2 : pad->NewState.RIGHTSTICKX * 2;
}

int16_t aimY(struct CPad* pad) {
    if (pad->NewState.CROSS || pad->NewState.DPADDOWN)
        return pad->NewState.LEFTSTICKY ? pad->NewState.LEFTSTICKY : pad->NewState.RIGHTSTICKY;
    else
        return pad->NewState.LEFTSTICKY ? pad->NewState.LEFTSTICKY * 2 : pad->NewState.RIGHTSTICKY * 2;
}

int16_t CPad__GetTarget(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        handleReload(pad);
        return pad->NewState.LEFTSHOULDER1 != 0;
    }
}

int16_t CPad__JumpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.SQUARE)
    {
        if (!pad->OldState.SQUARE)
        {
            if (CPad__GetTarget(pad)) // for reloading
                return 0;
            return 1;
        }
    }
    return 0;
}

int16_t CPad__TargetJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.LEFTSHOULDER1)
        return pad->OldState.LEFTSHOULDER1 == 0;
    return 0;
}

int16_t CPad__GetWeapon(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.RIGHTSHOULDER1;
}

int16_t CPad__WeaponJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.RIGHTSHOULDER1)
        return pad->OldState.RIGHTSHOULDER1 == 0;
    return 0;
}

int16_t CPad__GetLookBehindForPed(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.CIRCLE != 0;
}

int16_t CPad__EnterFreeAim(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.LEFTSHOULDER1 && (pad->NewState.DPADDOWN != 0/* || pad->NewState.RIGHTSTICKX || pad->NewState.RIGHTSTICKY*/);
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
    return round((2.0f * atan(((ar) / (16.0f / 9.0f)) * tan(f / 2.0f * ((float)M_PI / 180.0f)))) * (180.0f / (float)M_PI) * 100.0f) / 100.0f;
}

float* flt_8B30D64;
void sub_883CBEC(float a1)
{
    *flt_8B30D64 = a1 * fFOVFactor * (AdjustFOV(70.0f, fAspectRatio) / 70.0f);
}

float* flt_8B8ED6C;
float* flt_8B8ED70;
void TextScaling(float a1, float a2)
{
    *flt_8B8ED6C = a1 / fARDiff;
    *flt_8B8ED70 = a2;
}

void UnthrottleEmuEnable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)1, 0, NULL, 0);
}

void UnthrottleEmuDisable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)0, 0, NULL, 0);
}

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

int bNeedsToRunThisFrame = 0;
void (*cAudioManager__Service)(int, float, float, float);
void cAudioManager__Service_Hook(int a1, float a2, float a3, float a4)
{
    if (bNeedsToRunThisFrame == 0)
    {
        bNeedsToRunThisFrame = 1;
        return cAudioManager__Service(a1, a2, a3, a4);
    }
    else
    {
        bNeedsToRunThisFrame = 0;
    }
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
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

    int ModernControlScheme = inireader.ReadInteger("CONTROLS", "ModernControlScheme", 0);

    float fHudScale = inireader.ReadFloat("HUD", "HudScale", 1.0f);

    float fRadarScale = inireader.ReadFloat("RADAR", "RadarScale", 1.0f);
    float fRadarPosX = inireader.ReadFloat("RADAR", "RadarPosX", 12.0f);
    float fRadarPosY = inireader.ReadFloat("RADAR", "RadarPosY", 196.0f);

    float fMPRadarScale = inireader.ReadFloat("MPRADAR", "RadarScale", 1.0f);
    float fMPRadarPosY = inireader.ReadFloat("MPRADAR", "RadarPosY", 170.0f);

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
        injector.MakeJAL(ptr + 0x84, (intptr_t)CPad__GetRightStickX);
        injector.MakeNOP(ptr + 0x100);
        injector.WriteMemory8(ptr + 0x110 + 0x2, 0x00); // beqz -> b
        injector.MakeJAL(ptr + 0x13C, (intptr_t)CPad__GetRightStickY);

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

    if (ModernControlScheme)
    {
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

        // Swap R trigger and cross button
        uintptr_t ptr_294030 = pattern.get(0, "00 00 00 00 ? ? ? ? 25 10 00 00 2A 00 82 84", -8);
        injector.MakeJMPwNOP(ptr_294030, (intptr_t)CPad__GetAccelerate);

        uintptr_t ptr_293E90 = pattern.get(0, "0E 00 82 84 26 00 84 84", 0);
        injector.WriteMemory16(ptr_293E90 + 0x00, offsetof(struct CPad, NewState.CROSS));
        injector.WriteMemory16(ptr_293E90 - 0x30, offsetof(struct CPad, NewState.RIGHTSHOULDER1));

        // Swap L trigger and square button
        uintptr_t ptr_293EB8 = pattern.get(0, "00 00 00 00 ? ? ? ? 25 10 00 00 26 00 82 84", -8);
        injector.MakeJMPwNOP(ptr_293EB8, (intptr_t)CPad__GetBrake);
        injector.WriteMemory16(ptr_2949A0, offsetof(struct CPad, NewState.SQUARE));

        //driveby
        uintptr_t ptr_29388C = pattern.get(0, "FF 00 82 30 ? ? ? ? 25 20 00 02 ? ? ? ? 10 00 B0 8F", 12);
        injector.WriteMemory16(ptr_29388C, -110); // look left threshold
        uintptr_t ptr_293A60 = pattern.get(0, "0B 00 42 28 01 00 42 38", 0);
        injector.WriteMemory16(ptr_293A60, 110);  // look right threshold

        //driveby
        injector.MakeNOP(ptr_2937EC + 4);
        MakeInlineWrapper(ptr_2937EC,
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_293884),
            nop()
        );
        injector.MakeNOP(ptr_2939C0 + 4);
        MakeInlineWrapper(ptr_2939C0,
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_293A58),
            nop()
        );
        injector.MakeNOP(ptr_293BC8 + 4);
        MakeInlineWrapper(ptr_293BC8,
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            //lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)), // fix for firetruck cannon
            _or(a0, a0, k0),
            bne(a0, zero, 2),
            nop(),
            j(ptr_293C18),
            nop()
        );

        injector.WriteMemory16(ptr_294868, offsetof(struct CPad, NewState.SQUARE));
        injector.WriteMemory16(ptr_2932A4, offsetof(struct CPad, NewState.SQUARE));

        injector.WriteMemory16(ptr_292A28, offsetof(struct CPad, NewState.LEFTSHOULDER1));
        injector.WriteMemory16(ptr_2955F4, offsetof(struct CPad, NewState.LEFTSHOULDER1));

        //Shooting with triggers
        uintptr_t ptr_294240 = pattern.get(0, "86 00 85 94 10 00 B0 AF 14 00 BF AF ? ? ? ? 25 80 80 00 ? ? ? ? 25 20 00 02 ? ? ? ? 00 00 00 00 0E 00 02 86", -4);
        uintptr_t ptr_294290 = pattern.get(0, "86 00 85 94 10 00 B0 AF 14 00 BF AF ? ? ? ? 25 80 80 00 ? ? ? ? 25 20 00 02 ? ? ? ? 00 00 00 00 0E 00 05 86", -4);
        uintptr_t ptr_293F90 = pattern.get(0, "00 00 00 00 82 00 85 94 ? ? ? ? 04 00 A5 28 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00 ? ? ? ? 25 10 00 00 2C 00 82 84", -8);
        uintptr_t ptr_293FCC = pattern.get(0, "00 00 00 00 82 00 85 94 ? ? ? ? 04 00 A5 28 ? ? ? ? 00 00 00 00 2C 00 86 84", -8);
        uintptr_t ptr_293C2C = pattern.get(0, "00 00 00 00 86 00 85 94 ? ? ? ? 00 00 00 00 0E 00 86 84", -12);
        uintptr_t ptr_2930EC = pattern.get(0, "00 00 06 34 5E 00 85 84", 4);
        uintptr_t ptr_77B90 = pattern.get(0, "2C 00 82 84 ? ? ? ? 2E 00 82 84", 0);
        uintptr_t ptr_293180 = pattern.get(0, "02 00 07 34 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 12);
        uintptr_t ptr_145C30 = pattern.get(0, "0A 00 64 86 ? ? ? ? B8 06 04 82", 4);
        uintptr_t ptr_1464A0 = pattern.get(0, "0A 00 64 86 ? ? ? ? 00 00 00 00", 4);
        uintptr_t ptr_2945B0 = pattern.get(0, "0E 00 85 84 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00 ? ? ? ? 25 10 00 00 01 00 02 34 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 86 00 85 94", -0);
        uintptr_t ptr_294600 = pattern.get(0, "0E 00 85 84 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00 ? ? ? ? 25 10 00 00 01 00 02 34 10 00 BF 8F 08 00 E0 03 20 00 BD 27 ? ? ? ? 10 00 BF AF", -0);

        injector.MakeJMPwNOP(ptr_294240, (intptr_t)CPad__GetTarget);
        injector.MakeJMPwNOP(ptr_294290, (intptr_t)CPad__TargetJustDown);
        injector.MakeJMPwNOP(ptr_293F90, (intptr_t)CPad__GetWeapon);
        injector.MakeJMPwNOP(ptr_293FCC, (intptr_t)CPad__WeaponJustDown);
        injector.MakeJMPwNOP(ptr_293C2C, (intptr_t)CPad__GetLookBehindForPed);

        //injector.WriteMemory16(0x08A970E0, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        //injector.WriteMemory16(ptr_2930EC, offsetof(struct CPad, OldState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_77B90, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_293180 + 2, 0x1000);
        injector.WriteMemory16(ptr_145C30 + 2, 0x1000);
        injector.WriteMemory16(ptr_1464A0 + 2, 0x1000);

        //ShiftTargetJustDown
        injector.WriteMemory16(ptr_2945B0, offsetof(struct CPad, OldState.LEFTSHOULDER1));
        injector.WriteMemory16(ptr_294600, offsetof(struct CPad, OldState.LEFTSHOULDER1));
        
        //reloading
        ptr_516C4 = pattern.get(0, "37 00 06 34 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 30 05 24 8E", 4);
        ptr_516C4_data = *(uint32_t*)ptr_516C4;
        sub_894D97C = (void*)pattern.get(0, "00 21 04 00 ? ? ? ? ? ? ? ? ? ? ? ? 23 20 C4 00", -12);
        FindPlayerPed = (void*)pattern.get(0, "C0 29 04 00 21 30 A5 00 21 28 A6 00 00 21 04 00 23 20 A4 00 ? ? ? ? ? ? ? ? 21 20 85 00 08 00 E0 03", -8);
        FindPlayerVehicle = (void*)pattern.get(0, "C0 29 04 00 21 30 A5 00 21 28 A6 00 00 21 04 00 23 20 A4 00 ? ? ? ? ? ? ? ? 21 20 85 00 00 00 84 8C ? ? ? ? 00 00 00 00 38 05 85 90", -8);
        uintptr_t ptr_3D20 = pattern.get(0, "70 00 05 8E 23 20 85 00", -8);
        dword_8B5E144 = GetAbsoluteAddress(ptr_3D20, 0, 4);
        uintptr_t ptr_13F2B4 = pattern.get(0, "25 20 40 02 ? ? ? ? 00 00 00 00 4C 03 04 8E", -4);
        injector.MakeJAL(ptr_13F2B4, (intptr_t)CPad__JumpJustDown);

        // Free aim
        uintptr_t ptr_293154 = pattern.get(0, "82 00 86 94", -4);
        injector.MakeJMPwNOP(ptr_293154, (intptr_t)CPad__EnterFreeAim);
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

    //if (strcmp(ForceAspectRatio, "auto") != 0 || fFOVFactor)
    {
        uintptr_t ptr_EAA44 = pattern.get(0, "8E 3C 04 3C 21 F9 84 34", -8);
        flt_8B30D64 = (float*)GetAbsoluteAddress(ptr_EAA44, 0, 4);
        uintptr_t ptr_FE530 = pattern.get(0, "08 01 AC C7 ? ? ? ? B0 0A 04 8E", -4);
        injector.MakeJAL(ptr_FE530, (intptr_t)sub_883CBEC);
    }
    
    if (Enable60FPS)
    {
        //60 fps
        uintptr_t ptr_2030C8 = pattern.get(0, "02 00 84 2C ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 20);
        injector.MakeNOP(ptr_2030C8);

        uintptr_t ptr_2965E4 = pattern.get(0, "14 00 BF AF ? ? ? ? 25 80 80 00 0C 00 04 92", -8);
        cAudioManager__Service = (void(*)(int, float, float, float))ptr_2965E4;
        uintptr_t ptr_606F4 = pattern.get(16, "10 00 BF AF ? ? ? ? ? ? ? ? 10 00 BF 8F 08 00 E0 03 20 00 BD 27", 4);
        injector.MakeJAL(ptr_606F4, (intptr_t)cAudioManager__Service_Hook);
    }

    fARDiff = fAspectRatio / (16.0f / 9.0f);

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
        uintptr_t ptr_183E44 = pattern.get(0, "01 B3 0C 46 ? ? ? ? ? ? ? ? 54 00 A4 AF", 0);
        uintptr_t ptr_184040 = pattern.get(0, "80 40 04 3C 00 60 84 44 01 A5 0C 46", 0);
        uintptr_t ptr_1647CC = pattern.get(0, "C0 40 06 3C 00 78 86 44", 0);
        uintptr_t ptr_657BC = pattern.get(0, "00 78 89 44 00 60 8A 44", 0);
        uintptr_t ptr_221AC8 = pattern.get(0, "F0 43 05 3C 00 70 85 44", 0);
        uintptr_t ptr_1882AC = pattern.get(0, "80 40 04 3C 00 A0 84 44", 0);

        /* Health bar */
        injector.MakeInlineLUIORI(ptr_18323C, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_183244, adjustTopRightY(42.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1832D0, adjustRightX(420.0f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1832D8, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_183248, adjustTopRightY(58.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_183230, fHudScale * 1.09f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_18321C, fHudScale * 1.085938f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_18322C, fHudScale * 1.085938f / fARDiff); // 1%

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
        injector.MakeInlineLUIORI(ptr_1836DC, fHudScale * 1.09f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_1836C8, fHudScale * 1.085938f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_1836D8, fHudScale * 1.085938f / fARDiff); // 1%

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
        MakeInlineWrapper(ptr_183E44,
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
        injector.MakeInlineLUIORI(ptr_183FB4, fHudScale * 16.0f); // Height
        injector.MakeInlineLUIORI(ptr_184040, 4.0f / fARDiff);

        injector.MakeInlineLUIORI(ptr_183D78, adjustRightX(369.0f, fHudScale)); // Time Left X
        injector.MakeInlineLUIORI(ptr_183D88, adjustTopRightY(8.0f, fHudScale)); // Time Top Y

        injector.MakeInlineLUIORI(ptr_183BE0, adjustRightX(369.0f, fHudScale)); // Cash Left X
        injector.MakeInlineLUIORI(ptr_183BF0, adjustTopRightY(60.0f, fHudScale)); // Cash Top Y

        injector.MakeInlineLUIORI(ptr_1865E4, adjustRightX(452.0f, fHudScale)); // Wanted stars Right X
        injector.MakeInlineLUIORI(ptr_186670, adjustTopRightY(77.0f, fHudScale)); // Wanted stars Top Y
        injector.MakeInlineLUIORI(ptr_1866AC, adjustTopRightY(77.0f, fHudScale)); // Wanted stars Top Y (2)
        injector.MakeInlineLUIORI(ptr_1866CC, fHudScale * 17.0f / fARDiff); // Wanted stars Width

        /* Weapon icon & ammo numbers */
        injector.MakeInlineLUIORI(ptr_185C5C, adjustRightX(422.0f, fHudScale)); // Fist icon Left X
        injector.MakeInlineLUIORI(ptr_185C68, adjustTopRightY(7.0f, fHudScale)); // Fist icon Top Y
        injector.MakeInlineLUIORI(ptr_185C70, adjustRightX(488.0f, fHudScale)); // Fist icon Right X
        injector.MakeInlineLUIORI(ptr_185C78, adjustTopRightY(71.0f, fHudScale)); // Fist icon Bottom Y

        injector.MakeInlineLUIORI(ptr_185BDC, adjustRightX(455.0f, fHudScale)); // Weapon icon Center X
        injector.MakeInlineLUIORI(ptr_185BF0, adjustTopRightY(39.0f, fHudScale));  // Weapon icon Center Y
        injector.MakeInlineLUIORI(ptr_185BE4, fHudScale * 33.0f / fARDiff); // Weapon icon Width
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

        /* Text */
        uintptr_t ptr_250DF8 = pattern.get(0, "08 00 E0 03 ? ? ? ? ? ? ? ? ? ? ? ? 08 00 E0 03 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 08 00 E0 03 ? ? ? ? 25 28 80 00", -12);
        flt_8B8ED6C = (float*)(GetAbsoluteAddress(ptr_250DF8, 0, 4) + 4);
        flt_8B8ED70 = (float*)(GetAbsoluteAddress(ptr_250DF8, 0, 4) + 8);
        injector.MakeJMPwNOP(ptr_250DF8, (intptr_t)TextScaling);

        /*Radar Blips*/
        float f6 = 6.0f;
        float f6_pequeno = f6 / fARDiff;
        if (fRadarScale)
        {
            f6 = f6 * fRadarScale;
            f6_pequeno = f6_pequeno * fRadarScale;
        }
        MakeInlineWrapper(ptr_1647CC,
            lui(a2, HIWORD(f6_pequeno)),
            ori(a2, a2, LOWORD(f6_pequeno)),
            mtc1(a2, f30),
            lui(a2, HIWORD(f6)),
            ori(a2, a2, LOWORD(f6))
        );
        injector.WriteInstr(ptr_1647CC + 8, adds(f14, f12, f30));
        injector.WriteInstr(ptr_1647CC + 0x24, subs(f12, f12, f30));
        
        //CRadar::DrawRotatingRadarSprite
        uintptr_t ptr_1646B8 = pattern.get(0, "02 00 0E 46 00 60 00 46", -0);
        MakeInlineWrapper(ptr_1646B8,
            lui(t2, HIWORD(f6_pequeno)),
            ori(t2, t2, LOWORD(f6_pequeno)),
            mtc1(t2, f14),
            muls(f0, f0, f14),
            lui(t2, HIWORD(f6)),
            ori(t2, t2, LOWORD(f6)),
            mtc1(t2, f14)
        );
        
        //Crosshair
        float fCross = fARDiff * 1.125f;
        MakeInlineWrapper(ptr_657BC,
            mtc1(t1, f15),
            mtc1(t1, f16),
            lui(t1, HIWORD(fCross)),
            ori(t1, t1, LOWORD(fCross)),
            mtc1(t1, f29),
            divs(f15, f15, f29)
        );
        injector.MakeNOP(ptr_657BC + 0x10);
        
        //Sprites
        injector.MakeInlineLUIORI(ptr_221AC8, 480.0f * fARDiff);
        
        // Radar Disc
        float f4 = 4.0f;
        float f4_new = 4.0f / fARDiff;
        MakeInlineWrapper(ptr_1882AC,
            lui(a0, HIWORD(f4_new)),
            ori(a0, a0, LOWORD(f4_new)),
            mtc1(a0, f30),
            lui(a0, HIWORD(f4)),
            ori(a0, a0, LOWORD(f4))
        );
        injector.WriteInstr(ptr_1882AC + 0x0C, subs(f22, f0, f30));
        injector.WriteInstr(ptr_1882AC + 0x28, adds(f28, f12, f30));
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

            injector.MakeInlineLUIORI(ptr_182FA0, fRadarScale * 65.8f / fARDiff);  // Size X
            injector.MakeInlineLUIORI(ptr_182FD0, fRadarScale * 65.8f);  // Size Y

        }

        if (fMPRadarScale > 0.0f)
        {
            if (fMPRadarPosY)
                injector.MakeInlineLUIORI(ptr_183018, adjustBottomRightY(fMPRadarPosY, (fMPRadarScale ? fMPRadarScale : 1.0f))); // Top Y (Multiplayer)
            injector.MakeInlineLUIORI(ptr_182F94, fMPRadarScale * 92.0f / fARDiff);  // Size X (Multiplayer)
            injector.MakeInlineLUIORI(ptr_182FC4, fMPRadarScale * 92.0f);  // Size Y (Multiplayer)
        }
    }

    {
        uintptr_t ptr_1BEE84 = pattern.get(1, "B0 FF BD 27 ? ? ? ? ? ? ? ? ? ? ? ? 44 00 B0 AF 48 00 BF AF", 0);
        uintptr_t unk_38AE20 = GetAbsoluteAddress(ptr_1BEE84, 4, 8);
        byte_38AF51 = (uint8_t*)(unk_38AE20 + *(int16_t*)(ptr_1BEE84 + 12));

        uintptr_t ptr_1BD52C = pattern.get(1, "00 00 00 00 ? ? ? ? 00 60 80 44 ? ? ? ? 32 68 0C 46 00 00 00 00", 0);
        flt_3450E4 = (float*)GetAbsoluteAddress(ptr_1BD52C, 4, 12);
        
        uintptr_t ptr_1C0E70 = pattern.get(0, "25 20 60 02 ? ? ? ? 00 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 25 10 20 02", -4);
        injector.MakeJAL(ptr_1C0E70, (intptr_t)GameLoopStuff);
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
