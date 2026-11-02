module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/Window.hpp"
#include <d3d9.h>

export module Window;

import ComVars;
import DeveloperOverlay;
import Focus;
import Splitscreen;

bool DeviceWindowed = false;
HWND GameWindow = nullptr;
void SuspendMouse(bool suspend);

namespace
{
    using ResizeRequest = void(__cdecl*)(void*, int, int, int, int);
    ResizeRequest RequestResize = nullptr;
    UINT ResizeMessage = 0;
    std::atomic<bool> PendingSize = false;
    std::atomic<bool> ResizePosted = false;
    int PendingWidth = 0, PendingHeight = 0;
    thread_local bool PlacingWindow = false;

    struct Placement
    {
        bool previous = std::exchange(PlacingWindow, true);
        ~Placement()
        {
            PlacingWindow = previous;
        }
    };
}

SafetyHookInline shsub_46E388{};
// UD3DRenderDevice::SetRes(viewport, width, height, fullscreen, ...)
int __fastcall sub_46E388(void* a1, void* edx, void* a2, int a3, int a4, int fullscreen, int a6)
{
    if (bEnableSplitscreen)
    {
        auto rect = GetSplitscreenRect();
        a3 = rect.right - rect.left;
        a4 = rect.bottom - rect.top;
    }
    if (bWindowedMode || bEnableSplitscreen)
        fullscreen = 0;
    auto device = static_cast<uint8_t*>(a1);
    DeviceWindowed = fullscreen == -1 ? *reinterpret_cast<int*>(device + 0xCC) == 0 : fullscreen == 0;
    if (DeviceWindowed && !bEnableSplitscreen && FixSettings().Int("MAIN.WindowMode") == 2)
    {
        auto area = SCWindow::Monitor(GameWindow).rcMonitor;
        a3 = area.right - area.left;
        a4 = area.bottom - area.top;
    }
    auto& width = *reinterpret_cast<int*>(device + 0xD4);
    auto& height = *reinterpret_cast<int*>(device + 0xD8);
    const int oldWidth = width, oldHeight = height;
    const bool resized = DeviceWindowed && a3 > 0 && a4 > 0 && (a3 != width || a4 != height);
    if (resized)
    {
        // SetRes otherwise clamps growth to the old device and skips rebuilding its targets.
        width = a3;
        height = a4;
        *reinterpret_cast<int*>(device + 0x48) = 1;
    }
    const auto result = shsub_46E388.fastcall<int>(a1, edx, a2, a3, a4, fullscreen, a6);
    if (result)
    {
        DeveloperOverlay::Attach(*reinterpret_cast<IDirect3DDevice9**>(device + 0x12C));
        if (a2)
            DeviceWindowed = (*reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(a2) + 0x404) & 1) == 0;
        else if (DeviceWindowed && !bEnableSplitscreen && hGameWindow &&
            GetWindowThreadProcessId(hGameWindow, nullptr) == GetCurrentThreadId())
        {
            // The device can be initialized before a viewport requests its first placement.
            const int mode = FixSettings().Int("MAIN.WindowMode");
            Placement placement;
            SCWindow::Track(hGameWindow, mode, true, BackBufferWidth, BackBufferHeight);
            SCWindow::Place(hGameWindow, BackBufferWidth, BackBufferHeight, mode == 2);
        }
        if (DeviceWindowed && hGameWindow && GetForegroundWindow() == hGameWindow && !DeveloperOverlay::BlocksInput())
        {
            // The native HUD rectangle and final placement are available after SetRes.
            SuspendMouse(true);
            SuspendMouse(false);
        }
    }
    if (!result && resized)
    {
        width = oldWidth;
        height = oldHeight;
    }
    return result;
}


uintptr_t MouseViewport = 0;
bool MouseSuspended = false;
bool RestoreCapture = false;

void SuspendMouse(bool suspend)
{
    if (!MouseViewport || suspend == MouseSuspended || !hGameWindow ||
        GetWindowThreadProcessId(hGameWindow, nullptr) != GetCurrentThreadId())
        return;
    auto table = *reinterpret_cast<uintptr_t**>(MouseViewport);
    const auto capture = reinterpret_cast<void(__thiscall*)(uintptr_t, int32_t, int32_t, int32_t)>(table[0xB0 / 4]);
    if (suspend)
    {
        RestoreCapture = *reinterpret_cast<int32_t*>(MouseViewport + 0x40C) != 0;
        if (RestoreCapture)
            capture(MouseViewport, 0, 0, 0);
    }
    else if (RestoreCapture && GetForegroundWindow() == hGameWindow)
    {
        capture(MouseViewport, 1, 1, 0);
    }
    MouseSuspended = suspend;
}

bool bFocus = false;
SafetyHookInline shWndProc{};
int __fastcall WndProc(HDC _this, void* edx, UINT Msg, int wparam, unsigned int lparam)
{
    MouseViewport = reinterpret_cast<uintptr_t>(_this);
    hGameWindow = *reinterpret_cast<HWND*>(*reinterpret_cast<uintptr_t*>(MouseViewport + 0x3F4) + 4);
    if (ResizeMessage && Msg == ResizeMessage)
    {
        ResizePosted.store(false);
        if (DeviceWindowed && RequestResize && !IsIconic(hGameWindow) &&
            !FullscreenVideo.load(std::memory_order_relaxed) && PendingSize.exchange(false))
            RequestResize(_this, PendingWidth, PendingHeight, 0, -1);
        return 0;
    }

    if (Msg == WM_DESTROY)
    {
        PendingSize.store(false);
        ResizePosted.store(false);
    }
    else if (Msg == WM_WINDOWPOSCHANGING && DeviceWindowed && RequestResize && !PlacingWindow &&
        BackBufferWidth > 0 && BackBufferHeight > 0 && FullscreenVideo.load(std::memory_order_relaxed))
    {
        auto position = reinterpret_cast<WINDOWPOS*>(lparam);
        if (!(position->flags & SWP_NOSIZE) && !IsIconic(hGameWindow))
        {
            auto frame = SCWindow::Frame(hGameWindow, GetWindowLong(hGameWindow, GWL_STYLE), GetWindowLong(hGameWindow, GWL_EXSTYLE));
            const int width = position->cx - (frame.right - frame.left);
            const int height = position->cy - (frame.bottom - frame.top);
            if (width > 0 && height > 0)
            {
                PendingWidth = width;
                PendingHeight = height;
                const bool changed = width != BackBufferWidth || height != BackBufferHeight;
                PendingSize.store(changed);
                if (changed)
                {
                    // The movie loop defers device resets; keep its text and picture at the same size.
                    position->cx = BackBufferWidth + frame.right - frame.left;
                    position->cy = BackBufferHeight + frame.bottom - frame.top;
                    position->flags |= SWP_NOMOVE;
                }
            }
        }
    }

    LRESULT overlayResult = 0;
    const bool handled = DeveloperOverlay::Message(hGameWindow, Msg, wparam, lparam, overlayResult);
    SuspendMouse(DeveloperOverlay::BlocksInput());
    Focus::Message(Msg, wparam);
    if (handled)
        return int(overlayResult);

    if ((Msg == WM_KEYDOWN || Msg == WM_KEYUP) && wparam == VK_F1 && NativeMenuHelp &&
        NativeMenuHelp(Msg == WM_KEYDOWN && !(lparam & (1u << 30))))
        return 0;

    switch (Msg)
    {
    //case WM_SETFOCUS:
    //    SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    //    bFocus = true;
    //    break;
    //
    //case WM_KILLFOCUS:
    //    SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    //    bFocus = false;
    //    break;

    case WM_ACTIVATE:
        if (!DeviceWindowed)
            break;
        // split screen: both windows stay over the taskbar
        if (LOWORD(wparam) == WA_INACTIVE && !bEnableSplitscreen)
        {
            SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            bFocus = false;
        }
        else
        {
            SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            bFocus = true;
        }
        break;
    }

    auto ret = shWndProc.fastcall<int>(_this, edx, Msg, wparam, lparam);

    if (DeviceWindowed && (Msg == WM_MOVE || Msg == WM_EXITSIZEMOVE || (Msg == WM_SIZE && wparam != SIZE_MINIMIZED)) &&
        GetForegroundWindow() == hGameWindow && !DeveloperOverlay::BlocksInput())
    {
        // Native capture caches its screen-space clip before the window is placed.
        SuspendMouse(true);
        SuspendMouse(false);
    }

    return ret;
}

// UWindowsViewport::SetFocus(focus) (sub_8F17D8), sets +414h, captures the mouse only in fullscreen (virtual +8Ch),
// windowed it waits for a key press or a click (the cursor doesn't show): capture it (virtual +B0h, captured at +40Ch) when it gets focus
SafetyHookInline shViewportSetFocus{};
int __fastcall ViewportSetFocus(uintptr_t viewport, void* edx, int32_t focus)
{
    MouseViewport = viewport;
    auto result = shViewportSetFocus.fastcall<int>(viewport, edx, focus);
    auto vtable = *reinterpret_cast<uintptr_t**>(viewport);
    auto hWnd = *reinterpret_cast<HWND*>(*reinterpret_cast<uintptr_t*>(viewport + 0x3F4) + 4);
    if (focus && !DeveloperOverlay::BlocksInput() && *reinterpret_cast<int32_t*>(viewport + 0x40C) == 0 && GetForegroundWindow() == hWnd &&
        reinterpret_cast<int(__thiscall*)(uintptr_t)>(vtable[0x8C / 4])(viewport) == 0)
        reinterpret_cast<void(__thiscall*)(uintptr_t, int32_t, int32_t, int32_t)>(vtable[0xB0 / 4])(viewport, 1, 1, 0);
    return result;
}

BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    Placement placement;
    if (BackBufferWidth <= 0 || BackBufferHeight <= 0)
        return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    GameWindow = hWnd;
    hGameWindow = hWnd;
    SCWindow::SquareCorners(hWnd);
    const int width = BackBufferWidth;
    const int height = BackBufferHeight;
    if (!bEnableSplitscreen)
        SCWindow::Track(hWnd, FixSettings().Int("MAIN.WindowMode"), DeviceWindowed, width, height);
    if (!DeviceWindowed)
        return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    if (bEnableSplitscreen)
    {
        SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        auto rect = GetSplitscreenRect();
        return SetWindowPos(hWnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    SCWindow::Place(hWnd, width, height, FixSettings().Int("MAIN.WindowMode") == 2);
    const auto result = TRUE;

    // the window starts without focus
    static bool focused = false;
    if (!std::exchange(focused, true))
        SetForegroundWindow(hWnd);
    return result;
}

export void InitWindow()
{
    DeviceWindowed = bWindowedMode;
    SuspendGameMouse = SuspendMouse;
    auto resize = hook::pattern("55 8B EC 8B 0D ? ? ? ? 83 EC 54 85 C9 74 ? 83 3D ? ? ? ? 00 56 8B 75 08");
    RequestResize = reinterpret_cast<ResizeRequest>(resize.get_first());
    ResizeMessage = RegisterWindowMessageW(L"FusionFix.Conviction.WindowResize");
    ProcessWindowResize = []()
    {
        if (ResizeMessage && PendingSize.load() && hGameWindow && !IsIconic(hGameWindow) && !ResizePosted.exchange(true))
        {
            if (!PostMessageW(hGameWindow, ResizeMessage, 0, 0))
                ResizePosted.store(false);
        }
    };
    {
        // The windowed WM_SIZE path otherwise replaces the requested height with width * 9 / 16.
        auto aspect = hook::pattern("39 B3 9C 01 00 00 74 0E 8B C7 6B C0 09 99 83 E2 0F 03 C2 C1 F8 04");
        injector::WriteMemory<uint8_t>(aspect.get_first<uint8_t>(6), 0xEB, true);
    }
    {
        auto pattern = hook::pattern("55 8B EC 83 EC 40 53 56 83 C8 FF");
        shsub_46E388 = safetyhook::create_inline(pattern.get_first(), sub_46E388);

        pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 ? ? ? ? 8B 45 0C");
        shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);

        pattern = hook::pattern("53 56 33 C0 33 DB 57 8B 7C 24 10 3B FB 0F 95 C0 8B F1 33 C9 39 9E 14 04 00 00");
        shViewportSetFocus = safetyhook::create_inline(pattern.get_first(), ViewportSetFocus);

        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("SetWindowPos", SetWindowPosHook)
        );
    }

    auto pattern = hook::pattern("A3 ? ? ? ? 83 BE");
    static auto GetPresentationParametersHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto PresentationParameters = (D3DPRESENT_PARAMETERS*)regs.edi;

        if (PresentationParameters->Windowed && !*reinterpret_cast<int*>(regs.esi + 0x90))
            PresentationParameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

        BackBufferWidth = PresentationParameters->BackBufferWidth;
        BackBufferHeight = PresentationParameters->BackBufferHeight;
        DeviceWindowed = PresentationParameters->Windowed != FALSE;
        hGameWindow = PresentationParameters->hDeviceWindow;
        SCWindow::SquareCorners(PresentationParameters->hDeviceWindow);
    });
}
