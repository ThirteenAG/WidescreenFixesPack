module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/WindowCursor.hpp"
#include "include/WindowStyle.hpp"

export module Window;

import ComVars;
import Layout;
import DeveloperOverlay;
import FrameRate;

// Window modes, focus and the per-frame work on the game's render thread
namespace MatrixWindow
{
    int Mode = 0;
    HWND Window = nullptr;
    WNDPROC PreviousProcedure = nullptr;
    std::atomic<bool> PauseOnFocusLoss{ true };

    std::pair<int32_t, int32_t> MonitorSize()
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
        return { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
    }

    LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        CursorConfinement::Message(message, wParam);
        if (message == WM_SIZE && wParam == SIZE_MINIMIZED)
            CursorConfinement::Release();
        LRESULT result = 0;
        if (MatrixOverlay::Message(window, message, wParam, lParam, result))
            return result;
        // The game stops rendering and updating while it isn't the active application
        if (message == WM_ACTIVATEAPP && !wParam && Mode != 0 && !PauseOnFocusLoss)
            return 0;
        return CallWindowProcA(PreviousProcedure, window, message, wParam, lParam);
    }

    // The game's window frame counted a menu bar it doesn't have
    BOOL WINAPI AdjustRect(LPRECT rect, DWORD style, BOOL)
    {
        return AdjustWindowRect(rect, style, FALSE);
    }

    // Style and size for the window modes: the client area is exactly the back buffer
    void Place(IDirect3DDevice9* device, HWND window)
    {
        IDirect3DSurface9* surface = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            return;
        D3DSURFACE_DESC desc{};
        const bool described = SUCCEEDED(surface->GetDesc(&desc));
        surface->Release();
        if (!described)
            return;
        const DWORD style = Mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT client{};
        GetClientRect(window, &client);
        const DWORD current = static_cast<DWORD>(GetWindowLongW(window, GWL_STYLE)) & ~(WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        if (current == style && client.right == static_cast<LONG>(desc.Width) && client.bottom == static_cast<LONG>(desc.Height))
            return;
        SetWindowLongW(window, GWL_STYLE, static_cast<LONG>(style | WS_VISIBLE));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        WindowStyle::Place(window, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height));
    }

    // Every frame before Present
    void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS creation{};
        if (FAILED(device->GetCreationParameters(&creation)) || !creation.hFocusWindow)
            return;
        const auto window = creation.hFocusWindow;
        if (window != Window)
        {
            Window = window;
            PreviousProcedure = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Procedure)));
        }
        if (GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId())
            WindowCursor::Attach(window);
        CursorConfinement::Suspend(MatrixOverlay::Overlay.BlocksInput());
        if (WindowCursor::Window == window)
            CursorConfinement::Frame(window);
        if (Mode != 0)
            Place(device, window);
        MatrixFrameRate::Frame(window);
        WFP::onGameProcessEvent().executeAll();
        MatrixOverlay::Present(device, window);
    }
}

class Window
{
public:
    Window()
    {
        WFP::onInitEvent() += []()
        {
            WindowCursor::Initialize(FixSettings());
            MatrixFrameRate::Init();
            MatrixWindow::Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
            MatrixWindow::PauseOnFocusLoss = FixSettings().Bool("MAIN.PauseOnFocusLoss");
            FixSettings().OnChange("MAIN.PauseOnFocusLoss", []() { MatrixWindow::PauseOnFocusLoss = FixSettings().Bool("MAIN.PauseOnFocusLoss"); });
            IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("AdjustWindowRect", MatrixWindow::AdjustRect));
            MatrixOverlay::Init();
            D3D9Options::frame = MatrixWindow::Frame;
            D3D9Options::beforeReset = MatrixOverlay::BeforeReset;
            D3D9Options::HookCreation();

            static ptrdiff_t resHookEnd = 0;
            auto original = hook::pattern("A1 ? ? ? ? 89 45 E0 8B 0D ? ? ? ? 89 4D E4 33 D2");
            if (!original.empty()) // original release
            {
                nResolutionWidth.SetAddress(*original.get_first<int32_t*>(1));
                nResolutionHeight.SetAddress(*original.get_first<int32_t*>(10));
                resHookEnd = 186;
            }
            else // v1.2
            {
                auto updated = hook::pattern("8B ? ? ? ? 00 89 55 E0 A1 ? ? ? ? 89 45 E4 33 C9");
                nResolutionWidth.SetAddress(*updated.get_first<int32_t*>(2));
                nResolutionHeight.SetAddress(*updated.get_first<int32_t*>(10));
                resHookEnd = 237;
            }

            // The game's windowed flag, read from MatrixConfig.ini just before the resolution
            bWindowed.SetAddress(*hook::pattern("8A ? ? ? ? ? ? 68 00 00 CF 00 8D ? D8").get_first<uint8_t*>(2));

            auto pattern = hook::pattern("68 ? ? ? ? 6A 00 68 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 89 85 EC FA FF FF 8B 8D");
            struct ResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    std::tie(Screen.nWidth, Screen.nHeight) = MatrixNativeResolution();
                    if (MatrixWindow::Mode == 2)
                        std::tie(Screen.nWidth, Screen.nHeight) = MatrixWindow::MonitorSize();
                    nResolutionWidth = Screen.nWidth;
                    nResolutionHeight = Screen.nHeight;
                    bWindowed = MatrixWindow::Mode != 0;
                    Layout::Update();
                }
            }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(resHookEnd)); //0x7AD916

            // Aspect ratio
            pattern = hook::pattern("D9 05 ? ? ? ? D8 35 ? ? ? ? D9 1D ? ? ? ? 5D C3");
            fAspectRatioDivisor.SetAddress(*pattern.get_first<float*>(8));
            fAspectRatio.SetAddress(*pattern.get_first<float*>(14));
            struct ARHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    fAspectRatioDivisor = 1.0f;
                    fAspectRatio = 1.0f;
                }
            }; injector::MakeInline<ARHook>(pattern.get_first(0), pattern.get_first(18)); //0x6E6024, 0x6E6024+18
        };
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
    }
} Window;
