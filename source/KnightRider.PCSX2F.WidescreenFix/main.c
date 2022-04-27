#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"
#include "../../includes/pcsx2/patterns.h"
#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/injector.h"

int CompatibleCRCList[] = { 0x989192FE };
int PCSX2Data[PCSX2Data_Size] = { 1 };
char OSDText[OSDStringNum][OSDStringSize] = { {1} };

struct ScreenX
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fFOVFactor;
} Screen;

#define li2(r, imm) lui(r, ((uint16_t)(((*(uint32_t*)(&imm)) >> 16) & 0xffff))), addiu(r, r, ((uint16_t)((*(uint32_t*)(&imm)) & 0xffff)))

float AdjustFOV(float f, float ar)
{
    return round((2.0f * atan(((ar) / (4.0f / 3.0f)) * tan(f / 2.0f * ((float)M_PI / 180.0f)))) * (180.0f / (float)M_PI) * 100.0f) / 100.0f;
}

void init()
{
    logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    logger.Write("Loading KnightRider.PCSX2F.WidescreenFix...");

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

    Screen.nWidth43 = (uint32_t)(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = (float)Screen.nWidth43;
    Screen.fHudScale = (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    float f = 80.0f;
    float t = AdjustFOV(f, Screen.fAspectRatio);
    Screen.fFOVFactor = 0.00872665f * (t / f);

    uintptr_t ptr_3E3600 = pattern.get(0, "AA 3F ? 3C", 0);
    injector.MakeInlineLUIORI(ptr_3E3600, Screen.fAspectRatio);
    uintptr_t ptr_3EB864 = pattern.get(1, "AA 3F ? 3C", 0);
    injector.MakeInlineLUIORI(ptr_3EB864, Screen.fAspectRatio);

    MakeInlineWrapperWithNOP(0x125A68, lui(at, HIWORD(Screen.fAspectRatio)), addiu(at, at, LOWORD(Screen.fAspectRatio)), mtc1(at, f13));
    MakeInlineWrapperWithNOP(0x125AF8, lui(at, HIWORD(Screen.fAspectRatio)), addiu(at, at, LOWORD(Screen.fAspectRatio)), mtc1(at, f13));
    MakeInlineWrapperWithNOP(0x151C18, lui(at, HIWORD(Screen.fAspectRatio)), addiu(at, at, LOWORD(Screen.fAspectRatio)), mtc1(at, f13));
    MakeInlineWrapperWithNOP(0x152950, lui(at, HIWORD(Screen.fAspectRatio)), addiu(at, at, LOWORD(Screen.fAspectRatio)), mtc1(at, f13));

    uintptr_t ptr_3EB910 = pattern.get(0, "0E 3C 02 3C 36 FA 42 34 00 00 82 44 D0 FF BD 27", 0);
    injector.MakeInlineLUIORI(ptr_3EB910, Screen.fFOVFactor);
    uintptr_t ptr_3EBA60 = pattern.get(1, "0E 3C 02 3C 36 FA 42 34 00 00 82 44 D0 FF BD 27", 0);
    injector.MakeInlineLUIORI(ptr_3EBA60, Screen.fFOVFactor);

    logger.Write("KnightRider.PCSX2F.WidescreenFix loaded");
    logger.ClearLog();
}

int main()
{
    return 0;
}
