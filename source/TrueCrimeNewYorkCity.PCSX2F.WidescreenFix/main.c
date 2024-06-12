#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"
#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/injector.h"
#include "../../includes/pcsx2/inireader.h"
#include "../../includes/pcsx2/patterns.h"

int CompatibleCRCList[] = { 0x42A9C4EC };
int CompatibleElfCRCList[] = { 0x42A9C4EC, 0x1118ACD0 };
int PCSX2Data[PCSX2Data_Size] = { 1 };
char OSDText[OSDStringNum][OSDStringSize] = { {1} };
char PluginData[MaxIniSize] = { 1 };

struct ScreenX
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float f3DScale;
    float fHudScale;
    float fHudOffset;
} Screen;

void __fastcall HudScaleX(int a1, int a2)
{
    if (a2 == 640)
        a2 = (int)(640.0f * Screen.fHudScale);

    *(int*)(a1 + 0xD0) = a2;
    *(float*)(a1 + 0xD8) = (float)a2 * 0.0015625f;
    *(float*)(a1 + 0xE0) = 1.0 / (float)a2;
}

void init()
{
    //logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    //logger.Write("Loading TrueCrimeNewYorkCity.PCSX2F.WidescreenFix...");

    inireader.SetIniPath((char*)PluginData + sizeof(uint32_t), *(uint32_t*)PluginData);

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);

    if (SkipIntro)
    {
        uintptr_t ptr_100350 = pattern.get(0, "2D 20 40 02 ? ? ? ? 2D 40 20 02", -4);

        if (ptr_100350 != 0)
        {
            //logger.Write("Skipping intro...");
            injector.MakeNOP(ptr_100350);
            return;
        }
    }

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

    //logger.WriteF("Resolution: %dx%d", Screen.nWidth, Screen.nHeight);
    //logger.WriteF("Aspect Ratio: %f", Screen.fAspectRatio);

    float intResX = 640.0f;
    float intResY = 480.0f;

    Screen.fHudScale = (((intResX / intResY)) / (Screen.fAspectRatio));
    Screen.fHudOffset = (1.0f / (448.0f * (4.0f / 3.0f))) * (((448.0f * Screen.fAspectRatio) - 448.0f * (4.0f / 3.0f)) / 2.0f);
    Screen.f3DScale = (((intResX / intResY)) / (Screen.fAspectRatio));

    //3D Scaling
    injector.MakeInlineLUIORI(0x01D97FC, 0.0015625f * Screen.fHudScale); // 3d
    injector.MakeInlineLUIORI(0x0204DD8, 0.0015625f * Screen.fHudScale); // or this

    //Hud Scaling
    //injector.MakeInlineLUIORI(0x020BA4C, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x02346FC, 0.0015625f * Screen.fHudScale); // ui 1
    //injector.MakeInlineLUIORI(0x023830C, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x023AF68, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x023B4D8, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x023E524, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x02432EC, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x02434E8, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0243B00, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0244214, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x024422C, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x02442E0, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0244628, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x02447E8, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0244B18, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0247D58, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0247DA8, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0248210, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0248774, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x02EDDF4, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0318A78, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0325ABC, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x03F3964, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x0449CAC, 0.0015625f * Screen.fHudScale);
    //injector.MakeInlineLUIORI(0x04A3AFC, 0.0015625f * Screen.fHudScale);

    // Hud Pos
    float f = Screen.fHudOffset;
    MakeInlineWrapperWithNOP(0x233DA4,
        lui(v0, HIWORD(f)),
        ori(v0, v0, LOWORD(f)),
        sw(v0, sp, 0x24)
    );
    
    injector.MakeJAL(0x204E74, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x233644, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x2336F4, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x23389C, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x2346AC, (uintptr_t)HudScaleX); // ui stretch
    injector.MakeJAL(0x234990, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x2349CC, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x2349EC, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x255C58, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x26FBF8, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x270338, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x2752A8, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x275330, (uintptr_t)HudScaleX); //background ui
    injector.MakeJAL(0x2C0690, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x2C0BA8, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x49FE6C, (uintptr_t)HudScaleX);
    injector.MakeJAL(0x49FEB8, (uintptr_t)HudScaleX);


    //logger.Write("TrueCrimeNewYorkCity.PCSX2F.WidescreenFix loaded");
    //logger.ClearLog();
}

int main()
{
    return 0;
}
