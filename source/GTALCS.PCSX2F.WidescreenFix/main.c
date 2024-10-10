#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"

//#define NANOPRINTF_IMPLEMENTATION
#include "../../includes/pcsx2/nanoprintf.h"
#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/memalloc.h"
#include "../../includes/pcsx2/patterns.h"
#include "../../includes/pcsx2/mips.h"
#include "../../includes/pcsx2/injector.h"
#include "../../includes/pcsx2/inireader.h"

#include "lodl.h"
#include "cpad.h"

int CompatibleCRCList[] = { 0x7EA439F5 };
char PluginData[MaxIniSize] = { 1 };
int PCSX2Data[PCSX2Data_Size] = { 1 };
char OSDText[OSDStringNum][OSDStringSize] = { {1} };

struct ScreenX
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fDefaultAR;
    float fHudScale;
    float fHudOffset;
} Screen;

float BlipsScale = 0.0f;
float __f6 = 6.0f;
void __attribute__((naked)) BlipsScaling(float a1, float a2, float a3, float a4)
{
    {
        asm volatile ("lw      $v1,  %[x]" ::[x] "m" (__f6));
        asm volatile ("mtc1    $v1, $f21");
        asm volatile ("add.s   $f12, $f12, $f21");
        asm volatile ("add.s   $f13, $f13, $f21");
        asm volatile ("sub.s   $f14, $f14, $f21");
        asm volatile ("sub.s   $f15, $f15, $f21");

        asm volatile ("lw      $v1,  %[x]" ::[x] "m" (BlipsScale));
        asm volatile ("mtc1    $v1,  $f31");
        asm volatile ("sub.s   $f13, $f13, $f21");
        asm volatile ("add.s   $f15, $f15, $f21");
        asm volatile ("mul.s   $f21, $f21, $f31");
        asm volatile ("sub.s   $f12, $f12, $f21");
        asm volatile ("add.s   $f14, $f14, $f21");
    }

    /*
    a1 += 6.0f;
    a2 += 6.0f;
    a3 -= 6.0f;
    a4 -= 6.0f;

    a1 -= 6.0f * Screen.fHudScale;
    a2 -= 6.0f * Screen.fHudScale;
    a3 += 6.0f * Screen.fHudScale;
    a4 += 6.0f * Screen.fHudScale;
    */

    //original code
    asm volatile ("move    $v0,  $a0");
    asm volatile ("swc1    $f12, 0x0($a0)");
    asm volatile ("swc1    $f13, 0xC($a0)");
    asm volatile ("swc1    $f14, 0x8($a0)");
    asm volatile ("swc1    $f15, 0x4($a0)");

    asm volatile ("jr      $ra");
    asm volatile ("nop");
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

float AdjustFOV(float f, float ar, float t)
{
    uint32_t CGame__currArea = *(uint32_t*)(0x3D8430);
    return ((2.0f * atanf(((ar) / ((CGame__currArea == 0) ? (16.0f / 9.0f) : (4.0f / 3.0f))) * t)) * (180.0f / (float)M_PI) * 100.0f) / 100.0f;
}

const float fDefaultFOV = 70.0f;
float temp_t = 0.0f;
float* flt_487484 = NULL;
void sub_2083A0(float a1)
{
    *(float*)((uintptr_t)injector.GetGP() - 0x1AE8) = a1 * (AdjustFOV(fDefaultFOV, Screen.fAspectRatio, temp_t) / fDefaultFOV);
}

float sub_2083A8(int a1)
{
    *(float*)((uintptr_t)injector.GetGP() - 0x1AE4) = Screen.fAspectRatio;
    return Screen.fAspectRatio;
}

int log_cleared = 0;
void GameLoopStuff()
{
    if (log_cleared != 1)
    {
        logger.ClearLog();
        log_cleared = 1;
    }

    // Cam Pos OSD
    {
        //extern CVector* GetCamPos();
        //npf_snprintf(OSDText[0], 255, "Cam Pos: %f %f %f", GetCamPos()->x, GetCamPos()->y, GetCamPos()->z);
    }
}

void init()
{
    logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    logger.Write("Loading GTALCS.PCSX2F.WidescreenFix...");

    uint32_t DesktopSizeX = PCSX2Data[PCSX2Data_DesktopSizeX];
    uint32_t DesktopSizeY = PCSX2Data[PCSX2Data_DesktopSizeY];
    Screen.nWidth = PCSX2Data[PCSX2Data_WindowSizeX];
    Screen.nHeight = PCSX2Data[PCSX2Data_WindowSizeY];
    Screen.fWidth = (float)Screen.nWidth;
    Screen.fHeight = (float)Screen.nHeight;
    uint32_t IsFullscreen = PCSX2Data[PCSX2Data_IsFullscreen];
    uint32_t AspectRatioSetting = PCSX2Data[PCSX2Data_AspectRatioSetting];

    if (IsFullscreen || !Screen.nWidth || !Screen.nHeight)
    {
        Screen.nWidth = DesktopSizeX;
        Screen.nHeight = DesktopSizeY;
    }

    switch (AspectRatioSetting)
    {
    case RAuto4_3_3_2: //not implemented
        //if (GSgetDisplayMode() == GSVideoMode::SDTV_480P)
        //    AspectRatio = 3.0f / 2.0f;
        //else
        Screen.fAspectRatio = 4.0f / 3.0f;
        break;
    case R4_3:
        Screen.fAspectRatio = 4.0f / 3.0f;
        break;
    case R16_9:
        Screen.fAspectRatio = 16.0f / 9.0f;
        break;
    case Stretch:
    default:
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        break;
    }

    logger.WriteF("Resolution: %dx%d", Screen.nWidth, Screen.nHeight);
    logger.WriteF("Aspect Ratio: %f", Screen.fAspectRatio);

    Screen.fDefaultAR = (4.0f / 3.0f); //Screen.fAspectRatio >= (16.0f / 9.0f) ? (16.0f / 9.0f) : (4.0f / 3.0f);
    Screen.fHudScale = 1.0f / ((Screen.fAspectRatio) / (Screen.fDefaultAR));
    Screen.fHudOffset = ((((480.0f * (Screen.fAspectRatio)) - (480.0f * Screen.fDefaultAR))) / 2.0f) / (Screen.fAspectRatio);
    BlipsScale = 1.0f / ((Screen.fAspectRatio) / (16.0f / 9.0f));

    inireader.SetIniPath((char*)PluginData + sizeof(uint32_t), *(uint32_t*)PluginData);

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);

    if (SkipIntro)
    {
        logger.Write("Skipping intro...");
        injector.MakeNOP(0x1F35EC);
    }

    int ImprovedWidescreenSupport = inireader.ReadInteger("MAIN", "ImprovedWidescreenSupport", 0);
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);
    int DisableBlur = inireader.ReadInteger("MAIN", "DisableBlur", 0);

    int ModernControlScheme = inireader.ReadInteger("CONTROLS", "ModernControlScheme", 1);

    int RenderLodLights = inireader.ReadInteger("PROJECT2DFX", "RenderLodLights", 0);
    int CoronaLimit = inireader.ReadInteger("PROJECT2DFX", "CoronaLimit", 900);
    fCoronaRadiusMultiplier = inireader.ReadFloat("PROJECT2DFX", "CoronaRadiusMultiplier", 1.0f);
    fCoronaFarClip = inireader.ReadFloat("PROJECT2DFX", "CoronaFarClip", 500.0f);

    if (ImprovedWidescreenSupport)
    {
        //Proper FOV Scaling
        {
            logger.Write("Fixing FOV...");
            temp_t = tan(fDefaultFOV / 2.0f * ((float)M_PI / 180.0f));
            injector.MakeJMPwNOP(0x2083A0, (intptr_t)sub_2083A0);
        }

        //Current res aspect ratio
        {
            logger.Write("Overriding aspect ratio...");
            injector.MakeJAL(0x1F3FF0, (intptr_t)sub_2083A8);
            injector.MakeJAL(0x1F4194, (intptr_t)sub_2083A8);
            injector.MakeJAL(0x1F5E48, (intptr_t)sub_2083A8);
        }
    }

    if (Enable60FPS)
    {
        logger.Write("Enabling 60fps...");
        injector.MakeNOP(0x2D92C4);
    }

    if (DisableBlur)
    {
        injector.MakeNOP(0x3D0E64);
        injector.MakeNOP(0x194194);
    }

    logger.ClearLog();
    return;
    // The rest is VCS code
    CMenuManager__m_PrefsInvertLook = (int*)0x487A58;
    pCPad__GetLookBehindForCar = (void*)0x285C20;
    if (ModernControlScheme)
    {
        logger.Write("Enabling Modern Control Scheme...");
        ReplacePadFuncsWithModernControls();
    }

    if (RenderLodLights)
    {
        logger.Write("Enabling Project2DFX LCS...");
        uintptr_t ptr_27DD10 = pattern.get(0, "FF 00 4A 31 ? ? ? ? ? ? ? ? ? ? ? ? 80 50 0A 00 ? ? ? ? 21 50 42 01", 0);
        CCoronas__RegisterCoronaINT = (void*)ptr_27DD10;
        uintptr_t ptr_2A2ED4 = pattern.get(0, "10 01 B2 FF 08 01 B1 FF 2D 90 40 00 28 01 B5 FF", -4);
        uintptr_t TheCamera = GetAbsoluteAddress(ptr_2A2ED4, 0, 20);
        pCamPos = (CVector*)(TheCamera + 0xA50); //0xA50 at 0x377460
        uintptr_t ptr_228700 = pattern.get(0, "80 3F 01 3C 00 00 81 44 ? ? ? ? ? ? ? 3C ? ? ? 3C ? ? ? 3C ? ? ? ? ? ? ? 3C ? ? ? 3C ? ? ? 3C ? ? ? 3C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 08 00 E0 03", -4);
        CurrentTimeHoursOffset = GetAbsoluteAddress(ptr_228700, 24, 56);
        CurrentTimeMinutesOffset = GetAbsoluteAddress(ptr_228700, 32, 64);
        uintptr_t ptr_101EC0 = pattern.get(0, "FF EF 04 3C FF FF 84 34 24 10 45 00", 0);
        CTimer__m_snTimeInMillisecondsPauseModeOffset = GetAbsoluteAddress(ptr_101EC0, 12, 36);
        uintptr_t ptr_132678 = pattern.get(0, "02 08 00 46 82 08 02 46 42 08 03 46 00 21 00 46", 0);
        CTimer__ms_fTimeStepOffset = GetAbsoluteAddress(ptr_132678, -20, -8);

        IncreaseCoronasLimit(CoronaLimit);

        // Coronas Render
        uintptr_t ptr_21ED38 = pattern.get(0, "00 00 00 00 48 00 02 3C 48 7A 43 8C", -4);
        injector.MakeJAL(ptr_21ED38, (intptr_t)RegisterLODLights);
    }

    {
        injector.MakeJAL(0x21E988, (intptr_t)GameLoopStuff);
    }

    logger.Write("GTALCS.PCSX2F.WidescreenFix loaded");
}

int main()
{
    return 0;
}
