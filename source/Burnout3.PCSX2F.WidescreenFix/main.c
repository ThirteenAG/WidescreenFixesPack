#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"
#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/injector.h"

int CompatibleCRCList[] = { 0xBEBF8793 };
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
    float fAspectRatio2;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudScaleHalf;
    float fHudOffset;
    float fHudOffsetNeg;
    float fHudOffsetReal;

    float fHUDWidth;
    float fHUDWidthInv;
    float fHUDXPos;
    float fHUDXPos2;
    float fHUDRightXPos;
    float fHUDLeftXPos;
    float fMenuWidth;
    float fTopBorderXPos;
} Screen;

#define li2(r, imm) lui(r, ((uint16_t)(((*(uint32_t*)(&imm)) >> 16) & 0xffff))), addiu(r, r, ((uint16_t)((*(uint32_t*)(&imm)) & 0xffff)))

void WriteHudValues()
{
    injector.WriteMemoryFloat(0x4E105C, Screen.fHUDWidthInv);  // HUD Width
    injector.WriteMemoryFloat(0x6682B0, Screen.fHUDXPos);      // HUD X Pos
    injector.WriteMemoryFloat(0x669B30, Screen.fHUDXPos);      // HUD X Pos
    injector.WriteMemoryFloat(0x4B7688, Screen.fHUDRightXPos); // HUD Top Right X Pos
    injector.WriteMemoryFloat(0x4B7678, Screen.fHUDRightXPos); // HUD Bottom Right X Pos
    injector.WriteMemoryFloat(0x4B7658, Screen.fHUDLeftXPos);  // HUD Top Left X Pos
    injector.WriteMemoryFloat(0x4B7668, Screen.fHUDLeftXPos);  // HUD Bottom Left X Pos
    injector.WriteMemoryFloat(0x4CA660, Screen.fHUDWidth);     // Crash Cam Border Width
    injector.WriteMemoryFloat(0x4CA640, Screen.fHUDWidth);     // Crash Cam Border Width
    injector.WriteMemoryFloat(0x4CA650, Screen.fHUDWidth);     // Crash Cam Border Width
    injector.WriteMemoryFloat(0x4CA638, Screen.fHudOffsetNeg); // Crash Cam Border X Pos
    injector.WriteMemoryFloat(0x4CA658, Screen.fHudOffsetNeg); // Crash Cam Border X Pos
}

void sub_4D5350()
{
    void(*func_4D5350)() = (void*)0x4D5350;
    func_4D5350();
    WriteHudValues();
}

void sub_4DB330()
{
    void(*func_4DB330)() = (void*)0x4DB330;
    func_4DB330();
    WriteHudValues();
}

void init()
{
    logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    logger.Write("Loading Burnout3.PCSX2F.WidescreenFix...");

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

    Screen.fAspectRatio2 = Screen.fAspectRatio * 2.0f;
    Screen.nWidth43 = (uint32_t)(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = (float)Screen.nWidth43;
    Screen.fHudScale = (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fHudScaleHalf = Screen.fHudScale / 2.0f;
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetNeg = -Screen.fHudOffset;
    Screen.fFieldOfView = 1.0f / Screen.fHudScale;
    Screen.fHUDWidth = 480.0f * Screen.fAspectRatio; //853
    Screen.fHUDWidthInv = 1.0f / Screen.fHUDWidth; // 1/853
    Screen.fHUDXPos = (640.0f / 2.0f) - ((640.0f / 2.0f) * Screen.fHudScale);
    Screen.fHUDXPos2 = Screen.fHUDXPos / 640.0f;
    float f = ((Screen.fHUDWidth - 640.0f) / 1000.0f) / 1.155f;
    Screen.fHUDRightXPos = 1.0f + f;
    Screen.fHUDLeftXPos = 0.0f - f;
    Screen.fMenuWidth = 640.0f * Screen.fHudScale;
    Screen.fTopBorderXPos = 0.0f;

    // FOV
    injector.WriteInstr(0x228194, addiu(v0, zero, 1)); // enables widescreen FOV values
    injector.WriteMemoryFloat(0x4E0A38, Screen.fFieldOfView);  // FOV (Single Player)
    injector.WriteMemoryFloat(0x4E0C70, Screen.fAspectRatio);  // Aspect Ratio (Single Player)
    injector.WriteMemoryFloat(0x4E0C7C, Screen.fAspectRatio);  // FOV (Multiplayer)
    injector.WriteMemoryFloat(0x4E0C80, Screen.fAspectRatio2); // Aspect Ratio (Multiplayer)

    // HUD
    MakeInlineWrapper(0x1D475C,
        jal((intptr_t)WriteHudValues),
        nop(),
        addu(v0, s1, s2)
    );

    MakeInlineWrapperWithNOP(0x1D5180,
        jal(0x2B6E20),
        sq(v0, v1, 0x30),
        jal((intptr_t)WriteHudValues),
        nop()
    );

    injector.WriteMemory32(0x4DD840, (intptr_t)sub_4D5350);
    injector.WriteMemory32(0x4DD9E4, (intptr_t)sub_4DB330);

    MakeInlineWrapper(0x3D7238,
        li2(v0, Screen.fHUDWidth)
    ); // Crash Cam List Backing Width

    MakeInlineWrapper(0x3D70BC,
        li2(t0, Screen.fHudOffsetNeg),
        mtc1(t0, f1)
    ); // Crash Cam List Backing X Pos

    MakeInlineWrapper(0x3D72F4,
        li2(t9, Screen.fHudOffsetNeg),
        mtc1(t9, f2)
    ); // Crash Cam List Text X Pos

    MakeInlineWrapper(0x1A176C,
        li2(v0, Screen.fHUDWidth)
    ); // HUD Render Width (Player 1)

    MakeInlineWrapper(0x1A17D0,
        li2(v0, Screen.fHUDWidth)
    ); // HUD Render Width (Player 2)

    MakeInlineWrapper(0x1A1774,
        li2(v1, Screen.fHudOffsetNeg),
        sw(v0, sp, 0xA0),
        sw(v1, sp, 0xA8)
    ); // HUD Render X Pos (Player 1)

    MakeInlineWrapper(0x1A17D8,
        li2(v1, Screen.fHudOffsetNeg),
        sw(v0, sp, 0x90),
        sw(v1, sp, 0x98)
    ); // HUD Render X Pos (Player 2)

    MakeInlineWrapper(0x134F2C,
        li2(v0, Screen.fHUDWidth)
    ); // Get Ready, GO! Width

    MakeInlineWrapper(0x134F74,
        li2(v1, Screen.fHudScaleHalf),
        sw(v1, sp, 0x168),
        sw(v0, sp, 0x174)
    ); // Get Ready, GO! X Pos

    // Menus
    MakeInlineWrapper(0x30D7E4,
        li2(v1, Screen.fMenuWidth)
    ); // Menu Width

    MakeInlineWrapper(0x30D834,
        li2(t9, Screen.fHUDXPos),
        swc1(f3, a0, 0x8),
        mtc1(t9, f3),
        swc1(f3, a0, 0x0)
    ); // Menu X Pos

    MakeInlineWrapper(0x4DC71C,
        li2(t7, Screen.fHudOffsetNeg),
        sw(t7, v1, 0x1568),
        lui(a2, 0x4D)
    ); // Car Select Overlay X Pos

    MakeInlineWrapper(0x4DC738,
        li2(t7, Screen.fHUDWidth),
        sw(t7, v1, 0x1570)
    ); // Car Select Overlay Width

    MakeInlineWrapper(0x31B180,
        li2(v1, Screen.fHUDWidth)
    ); // Top Border Width

    MakeInlineWrapper(0x31B1F4,
        li2(t9, Screen.fTopBorderXPos),
        mtc1(t9, f7),
        swc1(f7, a0, 0x0),
        swc1(f5, a0, 0x4)
    ); // Top Border X Pos

    MakeInlineWrapper(0x38AE04,
        li2(v0, Screen.fHUDWidth), // Bottom Border Width
        li2(t9, Screen.fHUDXPos2),
        sw(t9, sp, 0x78)    // saves new x pos value
    ); // Bottom Border X Pos

    MakeInlineWrapper(0x38AE38,
        li2(v0, Screen.fHUDWidth)
    ); // Bottom Border Width

    MakeInlineWrapper(0x38AE4C,
        li2(t9, Screen.fHudOffsetNeg),
        sw(t9, sp, 0x68)
    ); // Bottom Border X Pos

    MakeInlineWrapper(0x31D6E4,
        li2(v0, Screen.fHUDWidth)
    ); // Checkerboard Width

    MakeInlineWrapper(0x31D73C,
        li2(v0, Screen.fHUDWidth)
    ); // Checkerboard Width

    MakeInlineWrapper(0x31D7E8,
        li2(v0, Screen.fHUDWidth)
    ); // Checkerboard Width

    MakeInlineWrapper(0x31D790,
        li2(v0, Screen.fHUDWidth)
    ); // Checkerboard Width

    MakeInlineWrapper(0x31D840,
        li2(v0, Screen.fHUDWidth)
    ); // Checkerboard Width

    MakeInlineWrapper(0x31D6F8,
        li2(t9, Screen.fHUDXPos2),
        sw(t9, sp, 0x110)
    ); // Middle Checkerboard X Pos

    MakeInlineWrapper(0x31D750,
        li2(t9, Screen.fHUDXPos2),
        sw(t9, sp, 0xF8)
    ); // Top Checkerboard X Pos

    MakeInlineWrapper(0x31D7FC,
        li2(t9, Screen.fHUDXPos2),
        sw(t9, sp, 0xC8)
    ); // Top Middle Checkerboard X Pos

    MakeInlineWrapper(0x31D7A4,
        li2(t9, Screen.fHUDXPos2),
        sw(t9, sp, 0xE0)
    ); // Bottom Checkerboard X

    MakeInlineWrapper(0x31D854,
        li2(t9, Screen.fHUDXPos2),
        sw(t9, sp, 0xB0)
    ); // Bottom Middle Checkerboard X Pos

    MakeInlineWrapper(0x31DA20,
        li2(v0, Screen.fHUDWidth)
    ); // Loading Background Width Pos

    MakeInlineWrapper(0x31DA40,
        li2(t9, Screen.fHudOffsetNeg),
        sw(t9, sp, 0x1D8)
    ); // Loading Background X Pos

    MakeInlineWrapper(0x3A6984,
        li2(v0, Screen.fHUDWidth)
    ); // Replay Top Bar Width

    MakeInlineWrapper(0x3A6998,
        li2(t9, Screen.fHudOffsetNeg),
        sw(t9, sp, 0x78)
    ); // Replay Top Bar X Pos

    MakeInlineWrapper(0x3A69C8,
        li2(v0, Screen.fHUDWidth)
    ); // Replay Bottom Bar Width

    MakeInlineWrapper(0x3A69DC,
        li2(t9, Screen.fHudOffsetNeg),
        sw(t9, sp, 0x60)
    ); // Replay Bottom Bar X Pos

    logger.Write("Burnout3.PCSX2F.WidescreenFix loaded");
    logger.ClearLog();
}

int main()
{
    return 0;
}
