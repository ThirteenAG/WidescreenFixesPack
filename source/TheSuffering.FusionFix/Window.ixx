module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <cstdio>

export module Window;

import ComVars;
import Camera;
import DeveloperOverlay;

export
{
// Window mode through the game's own windowed flag (bit 0 of its display flags, saved with its startup settings) and
// its window sizing; borderless renders at the size of the window's monitor
int nWindowMode = 0;
uint8_t nWindowOffset = 8; // the window's handle in the display object (Ties That Bind: 0x0C)
HWND hGameWindow = nullptr;
SafetyHookInline LoadDisplaySettingsHook, SizeWindowHook, WindowProcedureHook;
SafetyHookMid SetupDialogHook;
GameRef<uint32_t> SetupRequested;
bool DisplaySettingsMissing = false;
bool ShiftRequested = false;

void ApplyWindowMode()
{
    if (!DisplayFlags.is_initialized())
        return;
    if (nWindowMode)
        DisplayFlags |= 1;
    else
        DisplayFlags &= ~1u;
    if (nWindowMode == 2 && DisplayMode.is_initialized())
    {
        const auto info = WindowedModeWrapper::WindowMonitor();
        DisplayMode.get()[0] = info.rcMonitor.right - info.rcMonitor.left;
        DisplayMode.get()[1] = info.rcMonitor.bottom - info.rcMonitor.top;
    }
}

int __cdecl LoadDisplaySettings()
{
    const int result = LoadDisplaySettingsHook.unsafe_ccall<int>();
    DisplaySettingsMissing = result != 0;
    ApplyWindowMode();
    return result;
}

void AcceptSetup(uintptr_t setup)
{
    if (SetupRequested || ShiftRequested || (GetAsyncKeyState(VK_SHIFT) & 0x8000))
        return;
    const auto dialog = *reinterpret_cast<HWND*>(setup + 8);
    const auto page = *reinterpret_cast<HWND*>(setup + 12);
    const auto modes = GetDlgItem(page, 1036);
    if (!dialog || !page || !modes)
        return;
    const auto count = SendMessageA(modes, CB_GETCOUNT, 0, 0);
    auto selected = SendMessageA(modes, CB_GETCURSEL, 0, 0);
    if (DisplaySettingsMissing || selected == CB_ERR)
    {
        MONITORINFO monitor{ sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(dialog, MONITOR_DEFAULTTONEAREST), &monitor))
            return;
        const int width = monitor.rcMonitor.right - monitor.rcMonitor.left;
        const int height = monitor.rcMonitor.bottom - monitor.rcMonitor.top;
        selected = CB_ERR;
        int bestDepth = 0;
        for (LRESULT i = 0; i < count; ++i)
        {
            const auto length = SendMessageA(modes, CB_GETLBTEXTLEN, i, 0);
            if (length <= 0 || length >= 128)
                continue;
            char text[128]{};
            SendMessageA(modes, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(text));
            int candidateWidth = 0, candidateHeight = 0, depth = 0;
            if (sscanf_s(text, "%d x %d - %d bit", &candidateWidth, &candidateHeight, &depth) == 3 &&
                candidateWidth == width && candidateHeight == height && depth > bestDepth)
            {
                selected = i;
                bestDepth = depth;
            }
        }
        if (selected == CB_ERR)
            return;
        SendMessageA(modes, CB_SETCURSEL, selected, 0);
        SendMessageA(page, WM_COMMAND, MAKEWPARAM(1036, CBN_SELCHANGE), reinterpret_cast<LPARAM>(modes));
        if (DisplayMode.get()[0] != width || DisplayMode.get()[1] != height)
            return;
    }
    PostMessageA(dialog, WM_COMMAND, IDOK, 0);
}

BOOL __fastcall SizeWindow(uintptr_t self, void*, LONG width, LONG height)
{
    ApplyWindowMode();
    if (!nWindowMode)
        return SizeWindowHook.unsafe_thiscall<BOOL>(self, width, height);

    WindowedModeWrapper::GameHWND = *reinterpret_cast<HWND*>(self + nWindowOffset);
    WindowedModeWrapper::bBorderlessWindowed = nWindowMode == 2;
    WindowedModeWrapper::desiredClientWidth = width;
    WindowedModeWrapper::desiredClientHeight = height;
    WindowedModeWrapper::afterCreateWindow();
    if (nWindowMode == 2)
    {
        const auto info = WindowedModeWrapper::WindowMonitor();
        SetWindowPos(WindowedModeWrapper::GameHWND, nullptr, info.rcMonitor.left, info.rcMonitor.top,
            info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    return TRUE;
}

void InitWindowMode()
{
    ShiftRequested = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    auto flags = hook::pattern("8B 15 ? ? ? ? 83 E2 01");
    auto mode = hook::pattern("68 ? ? ? ? 56 8D 4C 24");
    auto load = hook::pattern("81 EC D0 01 00 00 8D 4C 24 50 E8 ? ? ? ? 8D 4C 24 58 E8 ? ? ? ? 8D 8C 24 2C 01 00 00 E8");
    auto size = hook::pattern("83 EC 10 53 8B 1D ? ? ? ? 56 57 8B F1 8B 46 ? 6A F0 50 FF D3 8B 4E ? 6A EC 51");
    DisplayFlags.SetAddress(*flags.get_first<uint32_t*>(2)); //0x5C6CE8
    DisplayMode.SetAddress(*mode.get_first<int32_t*>(1)); //0x5C6CEC
    nWindowOffset = *size.get_first<uint8_t>(16);
    LoadDisplaySettingsHook = safetyhook::create_inline(load.get_first(), LoadDisplaySettings); //0x439A00
    SizeWindowHook = safetyhook::create_inline(size.get_first(), SizeWindow); //0x4B3BF0
    auto setupRequest = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 85 C0 0F 84 ? ? ? ? 8B 8C 24");
    SetupRequested.SetAddress(*setupRequest.get_first<uint32_t*>(1));
    auto setupDialog = hook::pattern("6A 01 8B CE E8 ? ? ? ? 8B 1D ? ? ? ? 8B 2D ? ? ? ?");
    SetupDialogHook = safetyhook::create_mid(setupDialog.get_first(9), [](SafetyHookContext& regs)
    {
        AcceptSetup(regs.esi);
    });
}

// Focus: with MAIN.PauseOnFocusLoss off, a window mode keeps the game running and audible in the background (the
// game's deactivation stops its input, sound and timer). The overlay gets the window's messages first.
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    hGameWindow = window;
    LRESULT result = 0;
    if (SufferingOverlay::Message(window, message, wParam, lParam, result))
        return result;
    if (message == WM_DESTROY)
        RWGTA::UpdateCheck::OnExit();
    if (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE && nWindowMode && !FixSettings().Bool("MAIN.PauseOnFocusLoss"))
    {
        CursorConfinement::Release();
        return 0;
    }
    return WindowProcedureHook.unsafe_stdcall<LRESULT>(window, message, wParam, lParam);
}

void InitResolution()
{
    hook::pattern pattern;
    //resolution
    (void)*(uintptr_t**)hook::get_pattern("68 ? ? ? ? 56 8D 4C 24", 1);
    pattern = hook::pattern("8B 52 08 89 50 08 C2 08 00"); //0x4460E7
    struct GetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.edx + 0x08);
            *reinterpret_cast<uint32_t*>(regs.eax + 0x08) = regs.edx;

            auto width = static_cast<int32_t>(DisplayMode.get()[0]);
            auto height = static_cast<int32_t>(DisplayMode.get()[1]);
            if (width <= 0 || height <= 0 || width > 32767 || height > 32767)
                return;
            Screen.Width = width;
            Screen.Height = height;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
            Screen.fAspectRatioDiff = (Screen.fAspectRatio / (4.0f / 3.0f));
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            if (NativeFOV.is_initialized())
                Screen.fFieldOfView = CurrentFieldOfView();
        }
    }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(6));


}

}
