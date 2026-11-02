module;
#include <stdafx.h>
#include <d3d9.h>
#include "Shared/WindowCursor.hpp"
#include "FixSettings.hpp"

export module Window;
import ComVars;
import Resolution;
import FrameRate;
import DeveloperOverlay;
import Updates;
import Imports;
import Hud;
import Video;
import Bars;
import Camera;


void DrawFrameBorders(IDirect3DDevice9* device)
{
    if (JCVideo::seen)
        JCBars::Clear(device, Screen.nWidth, Screen.nHeight, JCVideo::picture);
    else if (Screen.bBoxedBackground || Screen.bDrawBorders || JCCamera::Pillarbox())
    {
        const RECT area{ static_cast<LONG>(Screen.fHudOffsetReal), 0,
            static_cast<LONG>(Screen.fHudOffsetReal + Screen.fWidth43), Screen.nHeight };
        JCBars::Clear(device, Screen.nWidth, Screen.nHeight, area);
    }
    JCVideo::seen = false;
    Screen.bDrawBorders = false;
    Screen.bBoxedBackground = false;
}


export namespace JCWindow
{
    inline HWND window = nullptr;
    inline IDirect3DDevice9* currentDevice = nullptr;
    inline SafetyHookInline resetHook, presentHook, swapChainHook;
    inline thread_local bool presenting = false;
    inline bool awake = false;
    inline int appliedMode = 0;
    inline decltype(&CreateWindowExA) originalWindow = CreateWindowExA;
    inline void Frame();

    inline DWORD Style()
    {
        return appliedMode == 2 ? WS_POPUP : WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    inline HWND WINAPI CreateNativeWindow(DWORD extended, LPCSTR name, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        if (appliedMode != 0 && !parent && !(style & WS_CHILD) && width >= 256 && height >= 256)
        {
            style = (style & ~(WS_POPUP | WS_THICKFRAME | WS_MAXIMIZEBOX)) | Style();
            RECT frame{ 0, 0, Screen.nWidth, Screen.nHeight };
            WindowedModeWrapper::AdjustFrame(&frame, style, menu != nullptr, extended);
            width = frame.right - frame.left;
            height = frame.bottom - frame.top;
        }
        return originalWindow(extended, name, title, style, x, y, width, height, parent, menu, instance, parameter);
    }

    inline LRESULT CALLBACK Procedure(HWND target, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (target == window)
        {
            LRESULT result = 0;
            if (JCOverlay::Message(target, message, wParam, lParam, result))
                return result;
        }
        if (target == window && appliedMode == 1 && message == WM_WINDOWPOSCHANGING)
        {
            const auto position = reinterpret_cast<WINDOWPOS*>(lParam);
            if (position && !(position->flags & SWP_NOSIZE) && Screen.nWidth > 0 && Screen.nHeight > 0)
            {
                RECT frame{ 0, 0, Screen.nWidth, Screen.nHeight };
                WindowedModeWrapper::AdjustFrame(&frame, Style(), FALSE, static_cast<DWORD>(GetWindowLongPtrW(target, GWL_EXSTYLE)));
                position->cx = frame.right - frame.left;
                position->cy = frame.bottom - frame.top;
            }
        }
        if (message == WM_NCDESTROY)
            RemoveWindowSubclass(target, Procedure, 0);
        return DefSubclassProc(target, message, wParam, lParam);
    }

    inline bool Focused()
    {
        return window && GetForegroundWindow() == window && !IsIconic(window);
    }

    inline HWND WINAPI ActiveWindow()
    {
        const auto actual = GetActiveWindow();
        return window && !FixSettings().Bool("MAIN.PauseOnFocusLoss") ? window : actual;
    }

    inline BOOL WINAPI CursorPosition(int x, int y)
    {
        return Focused() ? SetCursorPos(x, y) : TRUE;
    }

    inline int WINAPI CursorVisibility(BOOL show)
    {
        return Focused() ? ShowCursor(show) : 0;
    }

    inline HCURSOR WINAPI Cursor(HCURSOR cursor)
    {
        return Focused() ? SetCursor(cursor) : GetCursor();
    }

    inline BOOL WINAPI SystemParameters(UINT action, UINT parameter, PVOID data, UINT flags)
    {
        switch (action)
        {
        case SPI_SETSCREENSAVEACTIVE:
        case SPI_SETSTICKYKEYS:
        case SPI_SETTOGGLEKEYS:
        case SPI_SETFILTERKEYS:
        case SPI_SETMOUSETRAILS:
            return TRUE;
        }
        return SystemParametersInfoA(action, parameter, data, flags);
    }

    inline void Place(int width, int height)
    {
        const auto mode = appliedMode;
        if (!window || mode == 0) return;
        MONITORINFO monitor{ sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor)) return;
        const auto style = Style();
        SetWindowLongPtrW(window, GWL_STYLE, style | WS_VISIBLE);
        RECT frame{ 0, 0, width, height };
        WindowedModeWrapper::GameHWND = window;
        WindowedModeWrapper::AdjustFrame(&frame, style, FALSE, static_cast<DWORD>(GetWindowLongPtrW(window, GWL_EXSTYLE)));
        const auto x = monitor.rcMonitor.left + (monitor.rcMonitor.right - monitor.rcMonitor.left - width) / 2 + frame.left;
        const auto y = monitor.rcMonitor.top + (monitor.rcMonitor.bottom - monitor.rcMonitor.top - height) / 2 + frame.top;
        SetWindowPos(window, nullptr, x, y, frame.right - frame.left, frame.bottom - frame.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);
        const DWORD corners = 1;
        if (const auto library = LoadLibraryW(L"dwmapi.dll"))
        {
            using Attribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            if (const auto attribute = reinterpret_cast<Attribute>(GetProcAddress(library, "DwmSetWindowAttribute")))
                attribute(window, 33, &corners, sizeof(corners));
            FreeLibrary(library);
        }
    }

    inline void Parameters(D3DPRESENT_PARAMETERS* parameters, HWND focus, bool reset)
    {
        if (!parameters) return;
        window = parameters->hDeviceWindow ? parameters->hDeviceWindow : focus;
        const auto mode = appliedMode;
        if (mode != 0)
        {
            parameters->Windowed = TRUE;
            parameters->FullScreen_RefreshRateInHz = 0;
            if (mode == 2)
            {
                MONITORINFO monitor{ sizeof(monitor) };
                if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
                {
                    parameters->BackBufferWidth = monitor.rcMonitor.right - monitor.rcMonitor.left;
                    parameters->BackBufferHeight = monitor.rcMonitor.bottom - monitor.rcMonitor.top;
                }
            }
            else if (reset)
            {
                RECT client{};
                if (GetClientRect(window, &client) && client.right > 0 && client.bottom > 0)
                {
                    parameters->BackBufferWidth = client.right;
                    parameters->BackBufferHeight = client.bottom;
                }
            }
        }
    }

    inline HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (device != currentDevice) return resetHook.stdcall<HRESULT>(device, parameters);
        Parameters(parameters, window, true);
        JCOverlay::BeforeReset(device);
        JCUpdates::BeforeReset();
        const auto result = resetHook.stdcall<HRESULT>(device, parameters);
        if (SUCCEEDED(result))
        {
            JCUpdates::AfterReset();
            UpdateScreen(parameters->BackBufferWidth, parameters->BackBufferHeight);
            Place(Screen.nWidth, Screen.nHeight);
        }
        return result;
    }

    inline HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination, HWND target, const RGNDATA* dirty)
    {
        if (!presenting && device == currentDevice) Frame();
        const auto previous = presenting;
        presenting = true;
        const auto result = presentHook.stdcall<HRESULT>(device, source, destination, target, dirty);
        presenting = previous;
        return result;
    }

    inline HRESULT WINAPI SwapChainPresent(IDirect3DSwapChain9* chain, const RECT* source, const RECT* destination,
        HWND target, const RGNDATA* dirty, DWORD flags)
    {
        IDirect3DDevice9* device = nullptr;
        if (!presenting && SUCCEEDED(chain->GetDevice(&device)))
        {
            if (device == currentDevice) Frame();
            device->Release();
        }
        const auto previous = presenting;
        presenting = true;
        const auto result = swapChainHook.stdcall<HRESULT>(chain, source, destination, target, dirty, flags);
        presenting = previous;
        return result;
    }

    inline void Attach(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        currentDevice = device;
        UpdateScreen(parameters->BackBufferWidth, parameters->BackBufferHeight);
        SetWindowSubclass(window, Procedure, 0, 0);
        Place(Screen.nWidth, Screen.nHeight);
        WindowCursor::Attach(window);
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        if (!resetHook)
            resetHook = safetyhook::create_inline((*reinterpret_cast<void***>(device))[16], Reset);
        if (!presentHook)
            presentHook = safetyhook::create_inline((*reinterpret_cast<void***>(device))[17], Present);
        if (!swapChainHook)
        {
            IDirect3DSwapChain9* chain = nullptr;
            if (SUCCEEDED(device->GetSwapChain(0, &chain)))
            {
                swapChainHook = safetyhook::create_inline((*reinterpret_cast<void***>(chain))[3], SwapChainPresent);
                chain->Release();
            }
        }
    }

    inline void ProcessFrame()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && !JCOverlay::BlocksInput());
        CursorConfinement::Frame(window);
        const auto active = Focused();
        if (active != awake)
        {
            SetThreadExecutionState(ES_CONTINUOUS | (active ? ES_DISPLAY_REQUIRED : 0));
            awake = active;
        }
        JCFrameRate::Frame(window);
        JCCamera::Frame();
        DrawFrameBorders(currentDevice);
        JCUpdates::Tick();
        JCUpdates::Draw(currentDevice, Screen.nWidth, Screen.nHeight, JCHud::MenuShown());
        JCOverlay::Present(currentDevice, window);
    }

    inline void Frame()
    {
        static std::once_flag gameInit;
        std::call_once(gameInit, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
    }

    class FrameEvents
    {
    public:
        FrameEvents()
        {
            WFP::onGameProcessEvent() += []()
            {
                ProcessFrame();
            };
        }
    } frameEvents;

    inline void Install()
    {
        appliedMode = FixSettings().Int("MAIN.WindowMode");
        if (const auto original = JCImports::Replace("USER32.dll", "CreateWindowExA", &CreateNativeWindow))
            originalWindow = original;
        JCImports::Replace("USER32.dll", "GetActiveWindow", &ActiveWindow);
        JCImports::Replace("USER32.dll", "SetCursorPos", &CursorPosition);
        JCImports::Replace("USER32.dll", "ShowCursor", &CursorVisibility);
        JCImports::Replace("USER32.dll", "SetCursor", &Cursor);
        JCImports::Replace("USER32.dll", "SystemParametersInfoA", &SystemParameters);
    }
}


