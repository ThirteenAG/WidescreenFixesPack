module;
#include <stdafx.h>
#include "Shared/D3D9Options.hpp"
#include <commctrl.h>
#include "FixSettings.hpp"

export module Window;
import ComVars;

#pragma comment(lib, "comctl32.lib")

export namespace OnimushaWindow
{
    inline int Mode = 0, LastWindowedMode = 1;
    inline std::atomic<int> RequestedMode{ -1 };
    inline std::atomic<bool> SyncingMode{ false };
    inline int Width = 640, Height = 480;
    inline int RenderWidth = 0, RenderHeight = 0;
    inline int RememberWidth = 0, RememberHeight = 0;
    inline HWND Window = nullptr;
    inline bool FirstDevice = true, Placing = false, Windowed = false;
    inline SafetyHookMid Selection, Transition;
    inline constexpr UINT ApplyResolutionMessage = WM_APP + 0x3D8;
    inline thread_local bool BatchResolution = false;
    inline uint32_t* (__cdecl* NativeState)() = nullptr;
    inline int (__fastcall* ChangeDevice)(const void*, int, char, int) = nullptr;
    inline decltype(&CreateWindowExW) NativeCreate = CreateWindowExW;
    inline decltype(&SetWindowPos) NativePosition = SetWindowPos;
    inline std::function<void(int, int)> Resize;
    inline std::function<UINT(int, int)> RefreshRate;
    inline std::function<void()> DisplayChanged;
    inline std::function<void()> Tick;
    inline std::function<void(IDirect3DDevice9*)> Draw;
    inline std::function<void(HWND, UINT, WPARAM)> Message;
    inline std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&)> Filter;

    inline void RequestResolution()
    {
        if (Window && !BatchResolution)
            PostMessageW(Window, ApplyResolutionMessage, 0, 0);
    }

    inline void SetResolution(int width, int height)
    {
        const auto previous = BatchResolution;
        BatchResolution = true;
        FixSettings().Set("MAIN.ResX", std::to_string(width));
        FixSettings().Set("MAIN.ResY", std::to_string(height));
        BatchResolution = previous;
        RequestResolution();
    }

    inline RECT Monitor();

    inline void RequestMode()
    {
        if (SyncingMode.load())
            return;
        RequestedMode.store(std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2));
        RequestResolution();
    }

    inline void SyncMode()
    {
        if (FixSettings().Int("MAIN.WindowMode") == Mode)
            return;
        SyncingMode.store(true);
        FixSettings().Set("MAIN.WindowMode", std::to_string(Mode));
        SyncingMode.store(false);
    }

    inline void ApplyResolution()
    {
        const int requestedMode = RequestedMode.exchange(-1);
        const int oldMode = Mode, oldLastWindowedMode = LastWindowedMode;
        const int selected = requestedMode >= 0 ? requestedMode : Mode;
        int width = FixSettings().Int("MAIN.ResX"), height = FixSettings().Int("MAIN.ResY");
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            std::tie(width, height) = GetDesktopRes();
        if (width <= 0 || height <= 0)
            std::tie(width, height) = std::make_tuple(640, 480);
        if (selected == 2)
        {
            const auto monitor = Monitor();
            width = monitor.right - monitor.left;
            height = monitor.bottom - monitor.top;
        }
        auto current = reinterpret_cast<uint32_t*>(NativeState()[3]);
        if (!current)
        {
            SyncMode();
            return;
        }
        if (current[4] == uint32_t(width) && current[5] == uint32_t(height) && selected == Mode)
            return;
        std::array<uint32_t, 18> settings;
        std::copy_n(current, settings.size(), settings.begin());
        settings[4] = width;
        settings[5] = height;
        settings[12] = selected != 0;
        if (!settings[12] && RefreshRate)
            settings[16] = RefreshRate(width, height);
        const int oldWidth = Width, oldHeight = Height;
        const bool oldWindowed = Windowed;
        if (Mode)
            LastWindowedMode = Mode;
        Mode = selected;
        if (ChangeDevice(settings.data(), 0, false, false) < 0)
        {
            Mode = oldMode;
            LastWindowedMode = oldLastWindowedMode;
            Windowed = oldWindowed;
            Width = oldWidth;
            Height = oldHeight;
            Resize(Width, Height);
            SyncMode();
        }
        else if (Windowed && Mode == 1)
        {
            RememberWidth = Width;
            RememberHeight = Height;
        }
    }

    inline RECT Monitor()
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &info);
        return info.rcMonitor;
    }

    inline void SquareCorners()
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static const auto set = []()
        {
            auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const DWORD preference = 1;
        if (set)
            set(Window, 33, &preference, sizeof(preference));
    }

    inline void Place(int width, int height)
    {
        if (!Window || Placing || !Windowed)
            return;
        Placing = true;
        const auto monitor = Monitor();
        const DWORD style = Mode == 2 ? WS_POPUP : WS_OVERLAPPEDWINDOW;
        SetWindowLongW(Window, GWL_STYLE, style | (GetWindowLongW(Window, GWL_STYLE) & WS_VISIBLE));
        WindowedModeWrapper::GameHWND = Window;
        RECT frame{ 0, 0, width, height };
        WindowedModeWrapper::AdjustFrame(&frame, style, FALSE, DWORD(GetWindowLongW(Window, GWL_EXSTYLE)));
        int x = monitor.left + (monitor.right - monitor.left - width) / 2 + frame.left;
        int y = monitor.top + (monitor.bottom - monitor.top - height) / 2 + frame.top;
        if (Mode == 2)
        {
            frame = { 0, 0, width, height };
            x = monitor.left;
            y = monitor.top;
        }
        else if (width < monitor.right - monitor.left || height < monitor.bottom - monitor.top)
        {
            x = std::max(x, int(monitor.left));
            y = std::max(y, int(monitor.top));
        }
        NativePosition(Window, nullptr, x, y, frame.right - frame.left, frame.bottom - frame.top,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        SquareCorners();
        Placing = false;
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == ApplyResolutionMessage)
        {
            ApplyResolution();
            return 0;
        }
        LRESULT filtered = 0;
        if (Filter && Filter(window, message, wParam, lParam, filtered))
            return filtered;
        if (message == WM_GETMINMAXINFO && Windowed && Mode == 1 && lParam)
        {
            // A window as big as the monitor keeps its frame outside the screen instead of shrinking the client area.
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            RECT frame{ 0, 0, Width, Height };
            WindowedModeWrapper::AdjustFrame(&frame, DWORD(GetWindowLongW(window, GWL_STYLE)), FALSE, DWORD(GetWindowLongW(window, GWL_EXSTYLE)));
            auto& limits = *reinterpret_cast<MINMAXINFO*>(lParam);
            limits.ptMaxTrackSize.x = std::max(limits.ptMaxTrackSize.x, frame.right - frame.left);
            limits.ptMaxTrackSize.y = std::max(limits.ptMaxTrackSize.y, frame.bottom - frame.top);
            return result;
        }
        const auto result = DefSubclassProc(window, message, wParam, lParam);
        if (Message)
            Message(window, message, wParam);
        if (message == WM_NCDESTROY)
        {
            if (RememberWidth > 0 && RememberHeight > 0)
            {
                FixSettings().Set("MAIN.ResX", std::to_string(RememberWidth));
                FixSettings().Set("MAIN.ResY", std::to_string(RememberHeight));
            }
            RemoveWindowSubclass(window, Procedure, 0);
            Window = nullptr;
        }
        if (message == WM_EXITSIZEMOVE && Windowed && Mode == 1)
        {
            RECT client{};
            if (GetClientRect(window, &client) && client.right > 0 && client.bottom > 0)
            {
                RememberWidth = client.right;
                RememberHeight = client.bottom;
            }
        }
        if (DisplayChanged && (message == WM_DISPLAYCHANGE || message == WM_WINDOWPOSCHANGED))
        {
            static HMONITOR monitor = nullptr;
            const auto current = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
            if (message == WM_DISPLAYCHANGE || current != monitor)
            {
                monitor = current;
                DisplayChanged();
            }
        }
        if (message == 0x02E0 && Windowed)
        {
            Place(Width, Height);
            return 0;
        }
        return result;
    }

    inline HWND WINAPI Create(DWORD extended, LPCWSTR className, LPCWSTR title, DWORD style, int x, int y,
        int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        auto window = NativeCreate(extended, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (window && !parent && !Window)
        {
            Window = window;
            SetWindowSubclass(window, Procedure, 0, 0);
            SquareCorners();
        }
        return window;
    }

    inline BOOL WINAPI Position(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        const auto result = NativePosition(window, after, x, y, width, height, flags);
        if (window == Window && Windowed && !Placing && !(flags & SWP_NOSIZE))
            Place(Width, Height);
        return result;
    }

    inline void Parameters(D3DPRESENT_PARAMETERS* parameters, HWND window)
    {
        parameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        if (window)
        {
            Window = window;
            if (RequestedMode.load() >= 0)
                RequestResolution();
        }
        if (Mode == 2 && parameters->Windowed)
        {
            const auto monitor = Monitor();
            parameters->BackBufferWidth = monitor.right - monitor.left;
            parameters->BackBufferHeight = monitor.bottom - monitor.top;
        }
        if (parameters->Windowed)
            parameters->FullScreen_RefreshRateInHz = 0;
    }

    inline void Frame(IDirect3DDevice9* device)
    {
        if (Tick)
            Tick();
        IDirect3DSurface9* buffer = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &buffer)))
            return;
        D3DSURFACE_DESC desc{};
        const auto result = buffer->GetDesc(&desc);
        buffer->Release();
        if (FAILED(result) || !desc.Width || !desc.Height)
            return;
        if (RenderWidth != int(desc.Width) || RenderHeight != int(desc.Height))
        {
            RenderWidth = int(desc.Width);
            RenderHeight = int(desc.Height);
            Width = int(desc.Width);
            Height = int(desc.Height);
            Resize(Width, Height);
        }
        if (Draw)
            Draw(device);
    }

    inline void SetBeforeReset(void (*callback)(IDirect3DDevice9*))
    {
        D3D9Options::beforeReset = callback;
    }

    inline void Init(int width, int height, std::function<void(int, int)> resize)
    {
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        LastWindowedMode = Mode ? Mode : 1;
        Width = width;
        Height = height;
        Resize = std::move(resize);
        WindowedModeWrapper::EnableDpiAwareness();
        auto resolution = hook::pattern("8B 04 C5 ? ? ? ? 89 0D ? ? ? ? A3");
        NativeWidth.SetAddress(*resolution.get_first<uint32_t*>(9));
        NativeHeight.SetAddress(*resolution.get_first<uint32_t*>(14));
        FixSettings().OnChange("MAIN.ResX", RequestResolution);
        FixSettings().OnChange("MAIN.ResY", RequestResolution);
        FixSettings().OnChange("MAIN.WindowMode", RequestMode);
        auto originals = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::pair{ "CreateWindowExW", reinterpret_cast<void*>(&Create) },
            std::pair{ "SetWindowPos", reinterpret_cast<void*>(&Position) });
        NativeCreate = reinterpret_cast<decltype(NativeCreate)>(originals.at("CreateWindowExW").get());
        NativePosition = reinterpret_cast<decltype(NativePosition)>(originals.at("SetWindowPos").get());

        // Select a windowed adapter configuration before DXUT validates it. Resolution is kept separately:
        // windowed back buffers do not have to be enumerated fullscreen display modes.
        Selection = safetyhook::create_mid(hook::pattern("8D 54 24 18 52 8D 54 24 58 8B CA E8").get_first(),
            [](SafetyHookContext& context)
            {
                auto options = reinterpret_cast<uint32_t*>(context.esp + 0x18);
                auto settings = reinterpret_cast<uint32_t*>(context.esp + 0x54);
                settings[12] = Mode != 0;
                options[2] = 1;
            });
        auto transition = hook::pattern("81 EC 00 02 00 00 53 55 56 57 89 54 24 28 89 4C 24 24");
        ChangeDevice = reinterpret_cast<decltype(ChangeDevice)>(transition.get_first());
        NativeState = reinterpret_cast<decltype(NativeState)>(injector::GetBranchDestination(transition.get_first(18)).as_int());
        Transition = safetyhook::create_mid(transition.get_first(),
            [](SafetyHookContext& context)
            {
                auto settings = reinterpret_cast<uint32_t*>(context.ecx);
                Windowed = settings[12] != 0;
                if (Windowed)
                {
                    if (!Mode)
                        Mode = LastWindowedMode;
                    LastWindowedMode = Mode;
                }
                else
                {
                    if (Mode)
                        LastWindowedMode = Mode;
                    Mode = 0;
                }
                SyncMode();
                if (FirstDevice)
                {
                    settings[4] = Width;
                    settings[5] = Height;
                    FirstDevice = false;
                }
                if (Mode == 2 && settings[12])
                {
                    const auto monitor = Monitor();
                    settings[4] = monitor.right - monitor.left;
                    settings[5] = monitor.bottom - monitor.top;
                }
                if (settings[4] && settings[5])
                {
                    Width = settings[4];
                    Height = settings[5];
                    Resize(Width, Height);
                }
                else if (Windowed && Window)
                {
                    RECT client{};
                    GetClientRect(Window, &client);
                    Width = std::max(int(client.right), 1);
                    Height = std::max(int(client.bottom), 1);
                    Resize(Width, Height);
                }
                // Native Alt+Enter and resize continue through DXUT, including its resource callbacks.
                if (settings[12])
                    Place(Width, Height);
            });
        D3D9Options::presentParameters = Parameters;
        D3D9Options::frame = Frame;
        D3D9Options::HookCreation();
    }
}
