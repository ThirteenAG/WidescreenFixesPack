#include "stdafx.h"
#include "pcsx2/pcsx2.h"

struct Screen
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

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(L"WINSTA.dll", []() {
            if (!PCSX2::pcsx2_crc_pattern.empty())
            {
                void PCSX2Thread();
                std::thread t(PCSX2Thread);
                t.detach();
            }
        });
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////
//////////////////////Experimental PCSX2 support///////////////////////
///////////////////////////////////////////////////////////////////////

void PCSX2Thread()
{
    using namespace mips;

    static auto ps2 = PCSX2({ 0xBEBF8793 }, 0x485940, [](PCSX2& ps2)
        {
            Screen.nWidth = ps2.GetWindowWidth();
            Screen.nHeight = ps2.GetWindowHeight();
            Screen.fAspectRatio = ps2.GetAspectRatio();
            Screen.fAspectRatio2 = Screen.fAspectRatio * 2.0f;
            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
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
        });

    while (!ps2.isCRCValid())
        std::this_thread::yield();

    ps2.EnableCallback();

    ps2.vecPatches.push_back(PCSX2Memory(L"gametitle=Burnout 3: Takedown (SLUS-21050)"));
    ps2.vecPatches.push_back(PCSX2Memory(L"comment=Widescreen Fix by Aero_ https://thirteenag.github.io/wfp#burnout3"));
    ps2.vecPatches.push_back(PCSX2Memory(L""));
    ps2.vecPatches.push_back(PCSX2Memory(L"// Selected Resolution: " + std::to_wstring(Screen.nWidth) + L"x" + std::to_wstring(Screen.nHeight) + L", Aspect Ratio: " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(L""));

    ps2.vecPatches.push_back(PCSX2Memory(L"// FOV"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x665ECD, BYTE_T, NONE, 1, L"// enables widescreen FOV values"));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4E0A38, WORD_T, NONE, &Screen.fFieldOfView, L"// FOV (Single Player): " + std::to_wstring(Screen.fFieldOfView)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4E0C70, WORD_T, NONE, &Screen.fAspectRatio, L"// Aspect Ratio (Single Player): " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4E0C7C, WORD_T, NONE, &Screen.fAspectRatio, L"// FOV (Multiplayer): " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4E0C80, WORD_T, NONE, &Screen.fAspectRatio2, L"// Aspect Ratio (Multiplayer): " + std::to_wstring(Screen.fAspectRatio2)));

    ps2.vecPatches.push_back(PCSX2Memory(L"// HUD"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4E105C, WORD_T, NONE, &Screen.fHUDWidthInv, L"// HUD Width: " + std::to_wstring(Screen.fHUDWidthInv)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x6682B0, WORD_T, NONE, &Screen.fHUDXPos, L"// HUD X Pos: " + std::to_wstring(Screen.fHUDXPos)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x669B30, WORD_T, NONE, &Screen.fHUDXPos, L"// HUD X Pos: " + std::to_wstring(Screen.fHUDXPos)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4B7688, WORD_T, NONE, &Screen.fHUDRightXPos, L"// HUD Top Right X Pos: " + std::to_wstring(Screen.fHUDRightXPos)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4B7678, WORD_T, NONE, &Screen.fHUDRightXPos, L"// HUD Bottom Right X Pos: " + std::to_wstring(Screen.fHUDRightXPos)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4B7658, WORD_T, NONE, &Screen.fHUDLeftXPos, L"// HUD Top Left X Pos: " + std::to_wstring(Screen.fHUDLeftXPos)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4B7668, WORD_T, NONE, &Screen.fHUDLeftXPos, L"// HUD Bottom Left X Pos: " + std::to_wstring(Screen.fHUDLeftXPos)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4CA660, WORD_T, NONE, &Screen.fHUDWidth, L"// Crash Cam Border Width: " + std::to_wstring(Screen.fHUDWidth)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4CA640, WORD_T, NONE, &Screen.fHUDWidth, L"// Crash Cam Border Width: " + std::to_wstring(Screen.fHUDWidth)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4CA650, WORD_T, NONE, &Screen.fHUDWidth, L"// Crash Cam Border Width: " + std::to_wstring(Screen.fHUDWidth)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4CA638, WORD_T, NONE, &Screen.fHudOffsetNeg, L"// Crash Cam Border X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4CA658, WORD_T, NONE, &Screen.fHudOffsetNeg, L"// Crash Cam Border X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x3D7238, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Crash Cam List Backing Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x3D70BC, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t0, Screen.fHudOffsetNeg);
            mtc1(buf, t0, f1);
        }),
        L"// Crash Cam List Backing X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x3D72F4, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudOffsetNeg);
            mtc1(buf, t9, f2);
        }),
        L"// Crash Cam List Text X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x1A176C, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// HUD Render Width (Player 1): " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x1A17D0, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// HUD Render Width (Player 2): " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x1A1774, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v1, Screen.fHudOffsetNeg);
            sw(buf, v0, sp, 0xA0);
            sw(buf, v1, sp, 0xA8);
        }),
        L"// HUD Render X Pos (Player 1): " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x1A17D8, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v1, Screen.fHudOffsetNeg);
            sw(buf, v0, sp, 0x90);
            sw(buf, v1, sp, 0x98);
        }),
        L"// HUD Render X Pos (Player 2): " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x134F2C, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Get Ready, GO! Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x134F74, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v1, Screen.fHudScaleHalf);
            sw(buf, v1, sp, 0x168);
            sw(buf, v0, sp, 0x174);
        }),
        L"// Get Ready, GO! X Pos: " + std::to_wstring(Screen.fHudScaleHalf)));

    ps2.vecPatches.push_back(PCSX2Memory(L"// Menus"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x30D7E4, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v1, Screen.fMenuWidth);
        }),
        L"// Menu Width: " + std::to_wstring(Screen.fMenuWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x30D834, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHUDXPos);
            swc1(buf, f3, a0, 0x8);
            mtc1(buf, t9, f3);
            swc1(buf, f3, a0, 0x0);
        }),
        L"// Menu X Pos: " + std::to_wstring(Screen.fHUDXPos)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4DC71C, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t7, Screen.fHudOffsetNeg);
            sw(buf, t7, v1, 0x1568);
            lui(buf, a2, 0x4D);
        }),
        L"// Car Select Overlay X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x4DC738, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t7, Screen.fHUDWidth);
            sw(buf, t7, v1, 0x1570);
        }),
        L"// Car Select Overlay Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31B180, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v1, Screen.fHUDWidth);
        }),
        L"// Top Border Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31B1F4, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fTopBorderXPos);
            mtc1(buf, t9, f7);
            swc1(buf, f7, a0, 0x0);
            swc1(buf, f5, a0, 0x4);
        }),
        L"// Top Border X Pos: " + std::to_wstring(Screen.fTopBorderXPos)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x38AE00, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Bottom Border Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x38AE38, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Bottom Border Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x38AE4C, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudOffsetNeg);
            sw(buf, t9, sp, 0x68);
        }),
        L"// Bottom Border X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x38AE04, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHUDXPos2);
            sw(buf, t9, sp, 0x78);    // saves new x pos value
        }),
        L"// Bottom Border X Pos: " + std::to_wstring(Screen.fHUDXPos2)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D6E4, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Checkerboard Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D73C, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Checkerboard Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D7E8, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Checkerboard Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D790, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Checkerboard Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D840, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Checkerboard Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D6F8, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHUDXPos2);
            sw(buf, t9, sp, 0x110);
        }),
        L"// Middle Checkerboard X Pos: " + std::to_wstring(Screen.fHUDXPos2)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D750, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHUDXPos2);
            sw(buf, t9, sp, 0xF8);
        }),
        L"// Top Checkerboard X Pos: " + std::to_wstring(Screen.fHUDXPos2)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D7FC, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHUDXPos2);
            sw(buf, t9, sp, 0xC8);
        }),
        L"// Top Middle Checkerboard X Pos: " + std::to_wstring(Screen.fHUDXPos2)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D7A4, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHUDXPos2);
            sw(buf, t9, sp, 0xE0);
        }),
        L"// Bottom Checkerboard X: " + std::to_wstring(Screen.fHUDXPos2)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31D854, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHUDXPos2);
            sw(buf, t9, sp, 0xB0);
        }),
        L"// Bottom Middle Checkerboard X Pos: " + std::to_wstring(Screen.fHUDXPos2)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31DA20, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Loading Background Width Pos: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x31DA40, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudOffsetNeg);
            sw(buf, t9, sp, 0x1D8);
        }),
        L"// Loading Background X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x3A6984, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Replay Top Bar Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x3A6998, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudOffsetNeg);
            sw(buf, t9, sp, 0x78);
        }),
        L"// Replay Top Bar X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x3A69C8, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, v0, Screen.fHUDWidth);
        }),
        L"// Replay Bottom Bar Width: " + std::to_wstring(Screen.fHUDWidth)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, 0x3A69DC, WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudOffsetNeg);
            sw(buf, t9, sp, 0x60);
        }),
        L"// Replay Bottom Bar X Pos: " + std::to_wstring(Screen.fHudOffsetNeg)));

    ps2.WritePnach();
    ps2.WriteMemoryLoop();
}