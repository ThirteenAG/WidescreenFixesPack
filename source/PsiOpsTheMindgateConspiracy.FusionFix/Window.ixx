module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include <d3d9.h>

export module Window;
import FrameRate;
import FloatingPoint;


export namespace PsiOpsWindow
{
    inline SafetyHookInline createWindowHook, createD3DHook, createDeviceHook, resetHook, presentHook;
    inline HWND window = nullptr;
    inline int width = 640, height = 480;
    inline WNDPROC originalProc = nullptr;
    inline IDirect3DDevice9* device = nullptr;
    inline DWORD renderThread = 0;
    inline int appliedMode = 0;
    inline thread_local bool placing = false;
    inline thread_local bool repairingSize = false;
    inline thread_local bool presenting = false;
    inline std::function<void(int, int)> resized;
    inline std::function<void(IDirect3DDevice9*)> frame;
    inline std::function<void()> beforeReset;
    inline std::function<bool()> blocksInput;
    inline std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&)> message;
    inline GameRef<uint8_t> cursorVisible;

    struct PlacementScope
    {
        bool previous = placing;
        explicit PlacementScope(bool enabled) { placing = placing || enabled; }
        ~PlacementScope() { placing = previous; }
    };

    inline MONITORINFO Monitor(HWND target)
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    inline void Corners(HWND target)
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

    inline RECT Placement(HWND target, int mode, int clientWidth, int clientHeight, DWORD style, DWORD extended);

    inline void TrackBounds(HWND target, LPARAM parameter)
    {
        auto limits = reinterpret_cast<MINMAXINFO*>(parameter);
        if (!limits || width <= 0 || height <= 0)
            return;
        const auto rectangle = Placement(target, 1, width, height, DWORD(GetWindowLongPtrW(target, GWL_STYLE)),
            DWORD(GetWindowLongPtrW(target, GWL_EXSTYLE)));
        limits->ptMaxTrackSize.x = std::max(limits->ptMaxTrackSize.x, rectangle.right - rectangle.left);
        limits->ptMaxTrackSize.y = std::max(limits->ptMaxTrackSize.y, rectangle.bottom - rectangle.top);
    }

    inline void ConstrainSize(HWND target, WINDOWPOS* position)
    {
        if (!position || position->hwnd != target || target != window || appliedMode != 1 || placing
            || !device || width <= 0 || height <= 0 || (position->flags & SWP_NOSIZE) || IsIconic(target))
            return;
        const auto bounds = Placement(target, 1, width, height, DWORD(GetWindowLongPtrW(target, GWL_STYLE)),
            DWORD(GetWindowLongPtrW(target, GWL_EXSTYLE)));
        position->cx = bounds.right - bounds.left;
        position->cy = bounds.bottom - bounds.top;
    }

    inline void RepairSize(HWND target)
    {
        if (target != window || appliedMode != 1 || placing || repairingSize || !device || IsIconic(target))
            return;
        RECT client{};
        if (!GetClientRect(target, &client) || (client.right == width && client.bottom == height))
            return;
        WINDOWPOS position{ .hwnd = target };
        ConstrainSize(target, &position);
        if (position.cx <= 0 || position.cy <= 0)
            return;
        struct RepairScope
        {
            RepairScope() { repairingSize = true; }
            ~RepairScope() { repairingSize = false; }
        } repairScope;
        SetWindowPos(target, nullptr, 0, 0, position.cx, position.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    inline LRESULT CALLBACK Procedure(HWND target, UINT id, WPARAM wParam, LPARAM lParam)
    {
        if (appliedMode == 1 && id == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_MAXIMIZE)
            return 0;
        if (id == WM_WINDOWPOSCHANGED)
        {
            RepairSize(target);
            ConstrainSize(target, reinterpret_cast<WINDOWPOS*>(lParam));
        }
        CursorConfinement::Message(id, wParam);
        LRESULT result = 0;
        if (message && message(target, id, wParam, lParam, result))
            return result;
        if (id == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT && target == window &&
            GetForegroundWindow() == target && !cursorVisible && (!blocksInput || !blocksInput()))
        {
            SetCursor(nullptr);
            return TRUE;
        }
        if (id == WM_WINDOWPOSCHANGING)
        {
            const auto native = originalProc ? CallWindowProcA(originalProc, target, id, wParam, lParam)
                : DefWindowProcA(target, id, wParam, lParam);
            ConstrainSize(target, reinterpret_cast<WINDOWPOS*>(lParam));
            return native;
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

    inline void Attach(HWND target)
    {
        if (!target || window == target)
            return;
        window = target;
        originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Procedure)));
        Corners(target);
        CursorConfinement::Attach(target);
    }

    inline RECT Placement(HWND target, int mode, int clientWidth, int clientHeight, DWORD style, DWORD extended)
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

    inline HWND WINAPI CreateEngineWindow(DWORD extended, LPCSTR name, LPCSTR title, DWORD style, int x, int y, int w, int h,
        HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const bool game = reinterpret_cast<uintptr_t>(name) > 0xFFFF && std::strcmp(name, "ESP") == 0 && !parent;
        const int mode = appliedMode;
        if (game && mode)
        {
            const auto info = Monitor(nullptr);
            extended &= ~WS_EX_TOPMOST;
            style = mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
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

    inline void Parameters(D3DPRESENT_PARAMETERS& parameters, HWND focus)
    {
        parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        const int mode = appliedMode;
        const auto target = parameters.hDeviceWindow ? parameters.hDeviceWindow : focus;
        if (!target)
            return;
        Attach(target);
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
            const DWORD style = mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            const auto extended = DWORD(GetWindowLongPtrW(target, GWL_EXSTYLE)) & ~WS_EX_TOPMOST;
            const auto rectangle = Placement(target, mode, int(parameters.BackBufferWidth), int(parameters.BackBufferHeight), style, extended);
            SetWindowLongPtrW(target, GWL_STYLE, style | WS_VISIBLE);
            SetWindowLongPtrW(target, GWL_EXSTYLE, extended);
            SetWindowPos(target, HWND_NOTOPMOST, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
                rectangle.bottom - rectangle.top, SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
    }

    inline void Size(const D3DPRESENT_PARAMETERS& parameters)
    {
        width = int(parameters.BackBufferWidth);
        height = int(parameters.BackBufferHeight);
        if (width > 0 && height > 0 && resized)
            resized(width, height);
    }

    inline HRESULT WINAPI Reset(IDirect3DDevice9* target, D3DPRESENT_PARAMETERS* parameters)
    {
        PlacementScope placementScope(target == device && parameters);
        if (target == device && parameters)
        {
            if (beforeReset)
                beforeReset();
            Parameters(*parameters, window);
        }
        const auto result = resetHook.unsafe_stdcall<HRESULT>(target, parameters);
        if (SUCCEEDED(result) && target == device && parameters)
            Size(*parameters);
        return result;
    }

    inline HRESULT WINAPI Present(IDirect3DDevice9* target, const RECT* source, const RECT* destination, HWND overrideWindow, const RGNDATA* dirty)
    {
        if (target == device && !presenting && GetCurrentThreadId() == renderThread)
        {
            struct PresentScope
            {
                PresentScope() { presenting = true; }
                ~PresentScope() { presenting = false; }
            } presentScope;
            PsiOps::FloatingPointScope floatingPoint;
            MSG pending{};
            PeekMessageW(&pending, window, WM_NULL, WM_NULL, PM_NOREMOVE);
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && (!blocksInput || !blocksInput()));
            CursorConfinement::Update(window);
            const int cursorState = (cursorVisible || (blocksInput && blocksInput())) ? 1 : 0;
            static int lastCursorState = -1;
            if (lastCursorState != cursorState)
            {
                lastCursorState = cursorState;
                POINT point{};
                RECT client{};
                if (GetForegroundWindow() == window && GetCursorPos(&point) && ScreenToClient(window, &point) &&
                    GetClientRect(window, &client) && PtInRect(&client, point))
                    PostMessageW(window, WM_SETCURSOR, reinterpret_cast<WPARAM>(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
            }
            static std::once_flag gameInit;
            std::call_once(gameInit, []()
            {
                WFP::onGameInitEvent().executeAll();
            });
            WFP::onGameProcessEvent().executeAll();
            if (frame)
                frame(target);
            PsiOpsFrameRate::Limit(FixSettings().Int("MAIN.FPSLimit"), window);
        }
        return presentHook.unsafe_stdcall<HRESULT>(target, source, destination, overrideWindow, dirty);
    }

    inline HRESULT WINAPI CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND focus, DWORD behavior,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** output)
    {
        PlacementScope placementScope(parameters != nullptr);
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
            if (parameters)
                Size(*parameters);
        }
        return result;
    }

    inline IDirect3D9* WINAPI CreateD3D(UINT version)
    {
        const auto d3d = createD3DHook.unsafe_stdcall<IDirect3D9*>(version);
        if (d3d && !createDeviceHook)
            createDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(d3d))[16], CreateDevice);
        return d3d;
    }

    inline void Init(int initialWidth, int initialHeight)
    {
        auto cursor = hook::pattern("0F B6 C0 85 C0 74 54 0F B6 0D ? ? ? ? 85 C9 75 23 6A 01");
        cursorVisible.SetAddress(*cursor.get_first<uint8_t*>(10));
        appliedMode = FixSettings().Int("MAIN.WindowMode");
        width = initialWidth;
        height = initialHeight;
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        const auto user = GetModuleHandleW(L"user32.dll");
        const auto d3d = LoadLibraryW(L"d3d9.dll");
        createWindowHook = safetyhook::create_inline(GetProcAddress(user, "CreateWindowExA"), CreateEngineWindow);
        createD3DHook = safetyhook::create_inline(GetProcAddress(d3d, "Direct3DCreate9"), CreateD3D);
    }
}
