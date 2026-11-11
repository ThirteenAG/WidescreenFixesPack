module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include <d3d9.h>
#include <atomic>
#include <stdexcept>
#include "FloatingPoint.hpp"

export module Window;

import ComVars;

export namespace GodfatherWindow
{
    SafetyHookInline createWindowHook, createD3DHook, createDeviceHook, resetHook, presentHook;
    SafetyHookInline beginSceneHook, endSceneHook;
    SafetyHookInline swapHook;
    void* createReturn{};
    int mode = 0;
    bool verticalSync = false;
    HWND window = nullptr;
    int width = 640, height = 480;
    WNDPROC originalProc = nullptr;
    IDirect3DDevice9* device = nullptr;
    DWORD renderThread = 0;
    thread_local bool presenting = false;
    thread_local bool sceneActive = false;
    std::function<void(int, int)> resized;
    std::function<void(IDirect3DDevice9*)> frame;
    std::function<void()> pace;
    std::function<void()> beforeReset;
    std::function<void()> service;
    std::atomic<bool> sizing{}, toggleRequested{};
    bool altEnterDown{};
    std::function<bool()> blocksInput;
    std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&)> message;

    MONITORINFO Monitor(HWND target)
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    void Corners(HWND target)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttribute
        {
            const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const DWORD preference = 1;
        if (setAttribute)
            setAttribute(target, 33, &preference, sizeof(preference));
    }

    RECT Placement(HWND target, int mode, int clientWidth, int clientHeight, DWORD style, DWORD extended);

    void Place()
    {
        if (!window || !mode)
            return;
        const DWORD style = mode == 2 ? WS_POPUP : WS_OVERLAPPEDWINDOW;
        const auto extended = DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)) & ~WS_EX_TOPMOST;
        const auto rectangle = Placement(window, mode, width, height, style, extended);
        SetWindowLongPtrW(window, GWL_STYLE, style | WS_VISIBLE);
        SetWindowLongPtrW(window, GWL_EXSTYLE, extended);
        SetWindowPos(window, HWND_NOTOPMOST, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
            rectangle.bottom - rectangle.top, SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void Hotkeys()
    {
        const bool focused = window && GetForegroundWindow() == window;
        const bool down = focused && mode && (!blocksInput || !blocksInput())
            && (GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(VK_RETURN) & 0x8000);
        if (down && !altEnterDown)
            toggleRequested.store(true, std::memory_order_relaxed);
        altEnterDown = down;
    }

    void TrackBounds(HWND target, LPARAM parameter)
    {
        auto limits = reinterpret_cast<MINMAXINFO*>(parameter);
        if (!limits || width <= 0 || height <= 0)
            return;
        const auto rectangle = Placement(target, 1, width, height, DWORD(GetWindowLongPtrW(target, GWL_STYLE)),
            DWORD(GetWindowLongPtrW(target, GWL_EXSTYLE)));
        limits->ptMaxTrackSize.x = std::max(limits->ptMaxTrackSize.x, rectangle.right - rectangle.left);
        limits->ptMaxTrackSize.y = std::max(limits->ptMaxTrackSize.y, rectangle.bottom - rectangle.top);
    }

    LRESULT CALLBACK Procedure(HWND target, UINT id, WPARAM wParam, LPARAM lParam)
    {
        CursorConfinement::Message(id, wParam);
        LRESULT result = 0;
        if (message && message(target, id, wParam, lParam, result))
            return result;
        if (mode && id == WM_SYSKEYDOWN && wParam == VK_RETURN && (lParam & (1u << 29)))
        {
            if (!(lParam & (1u << 30)))
                toggleRequested.store(true, std::memory_order_relaxed);
            return 0;
        }
        if (mode == 1 && id == WM_ENTERSIZEMOVE)
            sizing.store(true, std::memory_order_relaxed);
        if (mode == 1 && id == WM_EXITSIZEMOVE)
            sizing.store(false, std::memory_order_relaxed);
        if (id == WM_PAINT)
        {
            // Rendering here can re-enter the movie decoder during a display change.
            PAINTSTRUCT paint{};
            BeginPaint(target, &paint);
            EndPaint(target, &paint);
            return 0;
        }
        if (id == WM_GETMINMAXINFO)
        {
            const auto native = originalProc ? CallWindowProcA(originalProc, target, id, wParam, lParam)
                : DefWindowProcA(target, id, wParam, lParam);
            TrackBounds(target, lParam);
            return native;
        }
        if (id == WM_NCDESTROY)
        {
            CursorConfinement::Detach();
            const auto native = originalProc;
            window = nullptr;
            originalProc = nullptr;
            return native ? CallWindowProcA(native, target, id, wParam, lParam) : DefWindowProcA(target, id, wParam, lParam);
        }
        return originalProc ? CallWindowProcA(originalProc, target, id, wParam, lParam) : DefWindowProcA(target, id, wParam, lParam);
    }

    void Attach(HWND target)
    {
        if (!target || window == target)
            return;
        if (window && originalProc && IsWindow(window)
            && GetWindowLongPtrA(window, GWLP_WNDPROC) == reinterpret_cast<LONG_PTR>(Procedure))
            SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalProc));
        CursorConfinement::Detach();
        window = target;
        originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Procedure)));
        Corners(target);
        CursorConfinement::Attach(target);
    }

    RECT Placement(HWND target, int mode, int clientWidth, int clientHeight, DWORD style, DWORD extended)
    {
        const auto info = Monitor(target);
        RECT rectangle{ 0, 0, clientWidth, clientHeight };
        using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using GetDpi = UINT(WINAPI*)(HWND);
        using GetSystemDpi = UINT(WINAPI*)();
        const auto user = GetModuleHandleW(L"user32.dll");
        const auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(user, "AdjustWindowRectExForDpi"));
        const auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user, "GetDpiForWindow"));
        const auto getSystemDpi = reinterpret_cast<GetSystemDpi>(GetProcAddress(user, "GetDpiForSystem"));
        const auto dpi = target && getDpi ? getDpi(target) : getSystemDpi ? getSystemDpi() : 96u;
        if (!adjust || !adjust(&rectangle, style, FALSE, extended, dpi ? dpi : 96u))
            AdjustWindowRectEx(&rectangle, style, FALSE, extended);
        const int x = info.rcMonitor.left + (info.rcMonitor.right - info.rcMonitor.left - clientWidth) / 2;
        const int y = info.rcMonitor.top + (info.rcMonitor.bottom - info.rcMonitor.top - clientHeight) / 2;
        OffsetRect(&rectangle, x, y);
        return mode == 2 ? info.rcMonitor : rectangle;
    }

    HWND WINAPI CreateEngineWindow(DWORD extended, LPCSTR name, LPCSTR title, DWORD style, int x, int y, int w, int h,
        HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const auto expected = GodfatherWindowClass.is_initialized() ? GodfatherWindowClass.get() : nullptr;
        const bool game = !parent && instance == GetModuleHandleW(nullptr)
            && ((_ReturnAddress() == createReturn && createReturn)
                || (reinterpret_cast<uintptr_t>(name) > 0xFFFF && reinterpret_cast<uintptr_t>(expected) > 0xFFFF
                    && std::strcmp(name, expected) == 0));
        if (game && mode)
        {
            const auto info = Monitor(nullptr);
            extended &= ~WS_EX_TOPMOST;
            style = mode == 2 ? WS_POPUP : WS_OVERLAPPEDWINDOW;
            if (mode == 2)
            {
                width = info.rcMonitor.right - info.rcMonitor.left;
                height = info.rcMonitor.bottom - info.rcMonitor.top;
            }
            const auto rectangle = Placement(nullptr, mode, width, height, style, extended);
            x = rectangle.left;
            y = rectangle.top;
            w = rectangle.right - rectangle.left;
            h = rectangle.bottom - rectangle.top;
        }
        const auto result = createWindowHook.unsafe_stdcall<HWND>(extended, name, title, style, x, y, w, h, parent, menu, instance, parameter);
        if (game)
            Attach(result);
        return result;
    }

    void Parameters(D3DPRESENT_PARAMETERS& parameters, HWND focus)
    {
        const auto target = parameters.hDeviceWindow ? parameters.hDeviceWindow : focus;
        if (!target || target != window)
            return;
        Attach(target);
        parameters.PresentationInterval = verticalSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
        if (mode)
        {
            parameters.Windowed = TRUE;
            parameters.FullScreen_RefreshRateInHz = 0;
            if (mode == 2)
            {
                const auto info = Monitor(target);
                parameters.BackBufferWidth = info.rcMonitor.right - info.rcMonitor.left;
                parameters.BackBufferHeight = info.rcMonitor.bottom - info.rcMonitor.top;
            }
            else if (!parameters.BackBufferWidth || !parameters.BackBufferHeight)
            {
                RECT client{};
                GetClientRect(target, &client);
                parameters.BackBufferWidth = (std::max)(1L, client.right - client.left);
                parameters.BackBufferHeight = (std::max)(1L, client.bottom - client.top);
            }
            width = int(parameters.BackBufferWidth);
            height = int(parameters.BackBufferHeight);
            const DWORD style = mode == 2 ? WS_POPUP : WS_OVERLAPPEDWINDOW;
            // A window that already has this size and style (a resize by the player) stays where it is.
            RECT client{};
            GetClientRect(target, &client);
            if (client.right - client.left == width && client.bottom - client.top == height
                && (DWORD(GetWindowLongPtrW(target, GWL_STYLE)) & (WS_POPUP | WS_OVERLAPPEDWINDOW)) == style)
                return;
            const auto extended = DWORD(GetWindowLongPtrW(target, GWL_EXSTYLE)) & ~WS_EX_TOPMOST;
            const auto rectangle = Placement(target, mode, int(parameters.BackBufferWidth), int(parameters.BackBufferHeight), style, extended);
            SetWindowLongPtrW(target, GWL_STYLE, style | WS_VISIBLE);
            SetWindowLongPtrW(target, GWL_EXSTYLE, extended);
            SetWindowPos(target, HWND_NOTOPMOST, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
                rectangle.bottom - rectangle.top, SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
    }

    void Size(const D3DPRESENT_PARAMETERS& parameters)
    {
        width = int(parameters.BackBufferWidth);
        height = int(parameters.BackBufferHeight);
        IDirect3DSurface9* surface = nullptr;
        if (device && SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
        {
            D3DSURFACE_DESC description{};
            if (SUCCEEDED(surface->GetDesc(&description)))
            {
                width = int(description.Width);
                height = int(description.Height);
            }
            surface->Release();
        }
        if (width > 0 && height > 0 && resized)
            resized(width, height);
    }

    HRESULT WINAPI Reset(IDirect3DDevice9* target, D3DPRESENT_PARAMETERS* parameters)
    {
        if (target == device && parameters)
        {
            sceneActive = false;
            if (beforeReset)
                beforeReset();
            Parameters(*parameters, window);
        }
        const auto result = resetHook.unsafe_stdcall<HRESULT>(target, parameters);
        if (SUCCEEDED(result) && target == device && parameters)
            Size(*parameters);
        return result;
    }

    HRESULT WINAPI Present(IDirect3DDevice9* target, const RECT* source, const RECT* destination, HWND overrideWindow, const RGNDATA* dirty)
    {
        if (target == device && !presenting && GetCurrentThreadId() == renderThread)
        {
            struct PresentScope
            {
                PresentScope() { presenting = true; }
                ~PresentScope() { presenting = false; }
            } presentScope;
            Godfather::FloatingPointScope floatingPoint;
            Hotkeys();
            if (service && !sceneActive)
                service();
            MSG pending{};
            PeekMessageW(&pending, window, WM_NULL, WM_NULL, PM_NOREMOVE);
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && (!blocksInput || !blocksInput()));
            CursorConfinement::Update(window);
            if (frame)
                frame(target);
            if (pace)
                pace();
        }
        return presentHook.unsafe_stdcall<HRESULT>(target, source, destination, overrideWindow, dirty);
    }

    HRESULT WINAPI BeginScene(IDirect3DDevice9* target)
    {
        const auto result = beginSceneHook.unsafe_stdcall<HRESULT>(target);
        if (SUCCEEDED(result) && target == device)
            sceneActive = true;
        return result;
    }

    HRESULT WINAPI EndScene(IDirect3DDevice9* target)
    {
        const auto result = endSceneHook.unsafe_stdcall<HRESULT>(target);
        if (target == device)
            sceneActive = false;
        return result;
    }

    HRESULT WINAPI CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND focus, DWORD behavior,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** output)
    {
        if (!window || (!parameters || (parameters->hDeviceWindow ? parameters->hDeviceWindow : focus) != window))
            return createDeviceHook.unsafe_stdcall<HRESULT>(d3d, adapter, type, focus, behavior, parameters, output);
        if (parameters)
            Parameters(*parameters, focus);
        const auto result = createDeviceHook.unsafe_stdcall<HRESULT>(d3d, adapter, type, focus, behavior, parameters, output);
        if (SUCCEEDED(result) && output && *output)
        {
            device = *output;
            renderThread = GetCurrentThreadId();
            const auto methods = *reinterpret_cast<void***>(device);
            if (!resetHook)
                resetHook = safetyhook::create_inline(methods[16], Reset);
            if (!presentHook)
                presentHook = safetyhook::create_inline(methods[17], Present);
            if (!beginSceneHook)
                beginSceneHook = safetyhook::create_inline(methods[41], BeginScene);
            if (!endSceneHook)
                endSceneHook = safetyhook::create_inline(methods[42], EndScene);
            if (parameters)
                Size(*parameters);
        }
        return result;
    }

    IDirect3D9* WINAPI CreateD3D(UINT version)
    {
        const auto d3d = createD3DHook.unsafe_stdcall<IDirect3D9*>(version);
        if (d3d && !createDeviceHook)
            createDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(d3d))[16], CreateDevice);
        return d3d;
    }

    BOOL __cdecl Swap(void* raster, int parameter, unsigned char flags)
    {
        flags = (flags & ~1u) | (verticalSync ? 1u : 0u);
        return swapHook.unsafe_ccall<BOOL>(raster, parameter, flags);
    }

    BOOL WINAPI SystemParameters(UINT action, UINT parameter, PVOID data, UINT flags)
    {
        // The original focus handler changes desktop keyboard/accessibility settings.
        if (action == SPI_SETKEYBOARDSPEED || action == SPI_SETSTICKYKEYS || action == SPI_SETSCREENSAVETIMEOUT)
            return TRUE;
        return SystemParametersInfoA(action, parameter, data, flags);
    }

    HHOOK WINAPI KeyboardHook(int id, HOOKPROC callback, HINSTANCE module, DWORD thread)
    {
        // Keep the system's task switching and Windows keys available.
        if (id == WH_KEYBOARD_LL)
            return nullptr;
        return SetWindowsHookExA(id, callback, module, thread);
    }

    void Init(int initialWidth, int initialHeight)
    {
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        verticalSync = FixSettings().Bool("MAIN.VSync");
        auto swap = hook::pattern("A1 ? ? ? ? 85 C0 74 18 A1 ? ? ? ? 8B 08 50 FF 91 A8 00 00 00 C7 05 ? ? ? ? 00 00 00 00 8A 4C 24 0C B8 01 00 00 00 84 C8");
        swapHook = safetyhook::create_inline(swap.get_first(), Swap);
        if (mode)
        {
            auto movie = hook::pattern("6A 11 51 50 FF 52 50 85 C0 7D 1C 8B 0D ? ? ? ? 8B 03 8B 10 6A 08 51 50 FF 52 50");
            injector::WriteMemory<uint8_t>(movie.get_first<uint8_t>(1), 8, true);
        }
        width = initialWidth;
        height = initialHeight;
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("SystemParametersInfoA", SystemParameters),
            std::forward_as_tuple("SetWindowsHookExA", KeyboardHook));
        const auto user = GetModuleHandleW(L"user32.dll");
        const auto d3d = LoadLibraryW(L"d3d9.dll");
        auto classSite = hook::pattern("8B 0D ? ? ? ? 52 50 A1 ? ? ? ? 50 51 6A 00 FF 15 ? ? ? ? 8B F0 85 F6");
        createReturn = classSite.get_first(23);
        GodfatherWindowClass.SetAddress(*classSite.get_first<const char**>(2));
        createWindowHook = safetyhook::create_inline(GetProcAddress(user, "CreateWindowExA"), CreateEngineWindow);
        createD3DHook = safetyhook::create_inline(GetProcAddress(d3d, "Direct3DCreate9"), CreateD3D);
    }
}
