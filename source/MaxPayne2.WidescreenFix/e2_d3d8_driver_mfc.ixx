module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module e2_d3d8_driver_mfc;

import ComVars;
import x_inputmfc;

BOOL WINAPI DllMainHook(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH || fdwReason == DLL_PROCESS_DETACH)
        return TRUE;

    return shDllMainHook.unsafe_stdcall<BOOL>(hinstDLL, fdwReason, lpvReserved);
}

// Movies (MAIN.FMVWidescreenMode). P_D3D::DRV_playVideo draws each Bink frame as tiles placed by a position and a
// scale, which it fits on the screen keeping the movie's aspect: the same scale across and down, centered, black bars
// at the sides or above and below (0, Original aspect). Fill uses the larger scale, cropping the movie to the screen;
// Stretch scales each way to the screen. Applied per frame, so a change shows at once.
namespace P_D3D
{
    enum eFMVWidescreenMode
    {
        Original,
        Fill,
        Stretch,
    };

    void DrawVideoFrame(SafetyHookContext& regs)
    {
        static auto& nFMVWidescreenMode = FixSettings().Int("MAIN.FMVWidescreenMode");
        auto pBink = (const uint32_t*)regs.ebp; // BINK, the movie's width and height first
        auto& x = *(float*)(regs.esp + 0x00);
        auto& y = *(float*)(regs.esp + 0x04);
        auto& fScaleX = *(float*)(regs.esp + 0x08);
        auto& fScaleY = *(float*)(regs.esp + 0x0C);
        if (nFMVWidescreenMode == Original || !pBink || !pBink[0] || !pBink[1] || !(fScaleX > 0.0f))
            return;

        float fMovieWidth = float(pBink[0]);
        float fMovieHeight = float(pBink[1]);
        float fScreenWidth = 2.0f * x + fMovieWidth * fScaleX;
        float fScreenHeight = 2.0f * y + fMovieHeight * fScaleY;
        if (nFMVWidescreenMode == Stretch)
        {
            fScaleX = fScreenWidth / fMovieWidth;
            fScaleY = fScreenHeight / fMovieHeight;
        }
        else
        {
            fScaleX = fScaleY = std::max(fScreenWidth / fMovieWidth, fScreenHeight / fMovieHeight);
        }
        x = (fScreenWidth - fMovieWidth * fScaleX) * 0.5f;
        y = (fScreenHeight - fMovieHeight * fScaleY) * 0.5f;
    }

    safetyhook::MidHook DrawVideoFrameHook = {};
}

export void InitE2_D3D8_DRIVER_MFC()
{
    // The game window as the driver opens a video mode: its frame for the window mode (MAIN.WindowMode 1 keeps the
    // game's own frame, the others have none) and square corners
    {
        WindowedModeWrapper::bBorderlessWindowed = FixSettings().Int("MAIN.WindowMode") != 1;
        auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "89 86 ? ? ? ? 8B 49");
        if (!pattern.empty())
        {
            BorderlessWindowedHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                WindowedModeWrapper::GameHWND = (HWND)(regs.eax);
                WindowedModeWrapper::afterCreateWindow();
            });
        }
        else
            OutputDebugStringA("Max Payne: window decoration pattern unavailable.\n");
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 08");
    if (!pattern.empty())
        shDllMainHook = safetyhook::create_inline(pattern.get_first(0), DllMainHook);
    else
        OutputDebugStringA("Max Payne: renderer entry pattern unavailable.\n");

    // Videos (intro, "previously" recap, credits), gamepad buttons skip them too
    static auto pVideoPeekMessageA = &VideoPeekMessageA;
    pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "FF 15 ? ? ? ? 85 C0 74 ? 8B 44 24 ? 3D 00 01 00 00 0F 84"); // P_D3D::DRV_playVideo
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &pVideoPeekMessageA, true); //0x1002456E

    // Movie display, see P_D3D::DrawVideoFrame
    pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "68 00 00 80 3F 52 8B 54 24 ? 50 51 52 8B CE E8"); // P_D3D::DRV_playVideo
    if (!pattern.empty())
        P_D3D::DrawVideoFrameHook = safetyhook::create_mid(pattern.get_first(15), P_D3D::DrawVideoFrame); //0x10024607
}

// e2_d3d8_driver_mfc gets unloaded for video mode changes
export void ShutdownE2_D3D8_DRIVER_MFC()
{
    P_D3D::DrawVideoFrameHook.reset();
}
