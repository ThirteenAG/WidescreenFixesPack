#pragma once
#include <d3d9.h>
#include <commctrl.h>
#include <atomic>
#include <array>
#include <functional>
#include <map>
#include <optional>
#pragma comment(lib, "comctl32.lib")

namespace USMWindow
{
    inline HWND Window = nullptr;
    inline uint32_t* NativeWindowed = nullptr;
    inline D3DPRESENT_PARAMETERS* Parameters = nullptr;
    inline IDirect3DDevice9** Device = nullptr;
    inline std::array<int32_t*, 6> Dimensions{};
    inline void(__cdecl* NativeReset)() = nullptr;
    inline void(__cdecl* RestoreResources)() = nullptr;
    inline bool ResourcesLost = false;
    inline ULONGLONG NextRecovery = 0;
    inline uintptr_t ReleaseResume = 0, DestroyResume = 0, TextureReleaseReturn = 0;
    inline void** Query = nullptr;
    inline uintptr_t FailedResetReturn = 0;
    inline int Mode = 0, LastWindowMode = 1;
    inline bool Changing = false, Attached = false;
    inline std::atomic<int> Pending{ -1 };
    inline std::optional<HRESULT> ResetStatus;
    inline std::function<void(int, int)> ResolutionChanged;
    inline std::function<void(int)> ModeChanged;
    inline std::map<std::wstring, SIZE> DesktopSizes;
    inline RECT FramedRect{};
    inline int FramedWidth = 0, FramedHeight = 0;
    inline bool HaveFramedRect = false;

    struct Snapshot
    {
        int mode;
        uint32_t windowed;
        D3DPRESENT_PARAMETERS parameters;
        std::array<int32_t, 6> dimensions;
        LONG style, exStyle;
        RECT rect;
    };
    inline std::optional<Snapshot> Previous;

    inline MONITORINFOEXW Monitor()
    {
        MONITORINFOEXW info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    inline SIZE DesktopSize()
    {
        const auto info = Monitor();
        const auto found = DesktopSizes.find(info.szDevice);
        if (found != DesktopSizes.end())
            return found->second;
        DEVMODEW desktop{};
        desktop.dmSize = sizeof(desktop);
        if (EnumDisplaySettingsW(info.szDevice, ENUM_REGISTRY_SETTINGS, &desktop) && desktop.dmPelsWidth && desktop.dmPelsHeight)
            return { static_cast<LONG>(desktop.dmPelsWidth), static_cast<LONG>(desktop.dmPelsHeight) };
        return { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
    }

    inline void RememberDesktop()
    {
        const auto info = Monitor();
        DesktopSizes[info.szDevice] = { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
    }

    inline void SquareCorners(HWND window)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttribute
        {
            const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const int preference = 1;
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference));
    }

    inline void StoreParameters(const D3DPRESENT_PARAMETERS& value)
    {
        static_assert(sizeof(value) == 56);
        const auto source = reinterpret_cast<const uint32_t*>(&value);
        const auto target = reinterpret_cast<uint32_t*>(Parameters);
        for (size_t i = 0; i < sizeof(value) / sizeof(uint32_t); ++i)
            ProtectedGameRef<uint32_t>{ target + i } = source[i];
    }

    inline void StoreDimensions(int width, int height)
    {
        for (size_t i = 0; i < Dimensions.size(); ++i)
            ProtectedGameRef<int32_t>{ Dimensions[i] } = (i & 1) ? height : width;
        if (ResolutionChanged)
            ResolutionChanged(width, height);
    }

    inline void Place(int width, int height)
    {
        if (!Window || width <= 0 || height <= 0)
            return;
        const DWORD style = WS_POPUP | WS_CLIPSIBLINGS | (GetWindowLongW(Window, GWL_STYLE) & WS_VISIBLE) |
            (Mode == 1 ? WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX : 0);
        const auto exStyle = GetWindowLongW(Window, GWL_EXSTYLE) & ~WS_EX_TOPMOST;
        SetWindowLongW(Window, GWL_STYLE, style);
        SetWindowLongW(Window, GWL_EXSTYLE, exStyle);
        RECT rect{ 0, 0, width, height };
        WindowedModeWrapper::GameHWND = Window;
        WindowedModeWrapper::AdjustFrame(&rect, style, FALSE, exStyle);
        const auto info = Monitor();
        const auto& area = Mode == 1 && width < info.rcMonitor.right - info.rcMonitor.left && height < info.rcMonitor.bottom - info.rcMonitor.top
            ? info.rcWork : info.rcMonitor;
        int x = Mode == 0 ? area.left : area.left + (area.right - area.left - width) / 2 + rect.left;
        int y = Mode == 0 ? area.top : area.top + (area.bottom - area.top - height) / 2 + rect.top;
        if (Mode == 1 && HaveFramedRect && width == FramedWidth && height == FramedHeight)
        {
            x = FramedRect.left;
            y = FramedRect.top;
        }
        SetWindowPos(Window, Mode == 0 ? HWND_TOP : HWND_NOTOPMOST, x, y, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        SquareCorners(Window);
    }

    inline Snapshot Save()
    {
        Snapshot value{};
        value.mode = Mode;
        value.windowed = *NativeWindowed;
        value.parameters = *Parameters;
        for (size_t i = 0; i < Dimensions.size(); ++i)
            value.dimensions[i] = *Dimensions[i];
        value.style = GetWindowLongW(Window, GWL_STYLE);
        value.exStyle = GetWindowLongW(Window, GWL_EXSTYLE);
        GetWindowRect(Window, &value.rect);
        return value;
    }

    inline void Restore(const Snapshot& value)
    {
        Mode = value.mode;
        ProtectedGameRef<uint32_t>{ NativeWindowed } = value.windowed;
        StoreParameters(value.parameters);
        for (size_t i = 0; i < Dimensions.size(); ++i)
            ProtectedGameRef<int32_t>{ Dimensions[i] } = value.dimensions[i];
        if (ResolutionChanged)
            ResolutionChanged(value.dimensions[0], value.dimensions[1]);
        SetWindowLongW(Window, GWL_STYLE, value.style);
        SetWindowLongW(Window, GWL_EXSTYLE, value.exStyle);
        SetWindowPos(Window, (value.exStyle & WS_EX_TOPMOST) ? HWND_TOPMOST : HWND_NOTOPMOST, value.rect.left, value.rect.top,
            value.rect.right - value.rect.left, value.rect.bottom - value.rect.top,
            SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
    }

    inline void CaptureReset(SafetyHookContext& regs)
    {
        const auto status = static_cast<HRESULT>(regs.eax);
        if (Query)
            ProtectedGameRef<void*>{ Query } = nullptr;
        if (!Changing || !Previous)
        {
            if (FAILED(status))
            {
                ResourcesLost = true;
                regs.eip = FailedResetReturn;
            }
            return;
        }
        ResetStatus = status;
        if (SUCCEEDED(*ResetStatus))
            return;
        Restore(*Previous);
        const auto retry = (*Device)->Reset(Parameters);
        ResourcesLost = FAILED(retry);
        if (ResourcesLost)
        {
            OutputDebugStringA("Ultimate Spider-Man: previous display reset failed; native recovery remains active.\n");
            regs.eax = static_cast<uint32_t>(retry);
            regs.eip = FailedResetReturn;
        }
    }

    inline void GuardBufferRelease(SafetyHookContext& regs)
    {
        if (!regs.eax)
            regs.eip = ReleaseResume;
    }

    inline void GuardBufferDestroy(SafetyHookContext& regs)
    {
        if (!regs.eax)
            regs.eip = DestroyResume;
    }

    inline void GuardTextureRelease(SafetyHookContext& regs)
    {
        const auto texture = static_cast<uintptr_t>(regs.ecx);
        if (!texture || (!(*reinterpret_cast<const uint32_t*>(texture + 12) & 0x2000u) &&
            !*reinterpret_cast<void* const*>(texture + 64)))
            regs.eip = TextureReleaseReturn;
    }

    inline void Frame()
    {
        if (ResourcesLost)
        {
            if (!Window || !Device || !*Device || !RestoreResources || GetForegroundWindow() != Window ||
                GetTickCount64() < NextRecovery || (*Device)->TestCooperativeLevel() == D3DERR_DEVICELOST)
            {
                MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                return;
            }
            NextRecovery = GetTickCount64() + 100;
            if (FAILED((*Device)->Reset(Parameters)))
                return;
            RestoreResources();
            ResourcesLost = false;
        }
        const int requested = Pending.exchange(-1);
        if (requested < 0)
            return;
        const bool dimensionsAvailable = std::all_of(Dimensions.begin(), Dimensions.end(), [](const auto pointer)
        {
            return pointer != nullptr;
        });
        if (!Attached || !Window || !Device || !*Device || !Parameters || !NativeWindowed || !NativeReset ||
            !dimensionsAvailable || Changing ||
            GetForegroundWindow() != Window || (*Device)->TestCooperativeLevel() != D3D_OK)
        {
            int empty = -1;
            Pending.compare_exchange_strong(empty, requested);
            return;
        }
        const int next = requested == 3 ? (Mode ? 0 : LastWindowMode) : std::clamp(requested, 0, 2);
        if (next == Mode)
            return;
        if (Mode)
            RememberDesktop();
        Previous = Save();
        const int oldMode = Mode;
        int width = static_cast<int>(Parameters->BackBufferWidth), height = static_cast<int>(Parameters->BackBufferHeight);
        if (oldMode == 1)
        {
            FramedRect = Previous->rect;
            FramedWidth = width;
            FramedHeight = height;
            HaveFramedRect = true;
        }
        if (next == 2)
        {
            const auto desktop = DesktopSize();
            width = desktop.cx;
            height = desktop.cy;
        }
        else if (next == 1 && HaveFramedRect)
        {
            width = FramedWidth;
            height = FramedHeight;
        }
        if (width < 640 || height < 480 || width > 16384 || height > 16384)
        {
            Previous.reset();
            return;
        }
        Changing = true;
        Mode = next;
        ProtectedGameRef<uint32_t>{ NativeWindowed } = next ? 1u : 0u;
        auto parameters = *Parameters;
        parameters.Windowed = next != 0;
        parameters.BackBufferWidth = width;
        parameters.BackBufferHeight = height;
        parameters.FullScreen_RefreshRateInHz = next ? 0 : Previous->parameters.FullScreen_RefreshRateInHz;
        StoreParameters(parameters);
        StoreDimensions(width, height);
        Place(width, height);
        ResetStatus.reset();
        NativeReset();
        if (!ResetStatus || FAILED(*ResetStatus))
        {
            Restore(*Previous);
            OutputDebugStringA("Ultimate Spider-Man: display change failed; restored the previous mode.\n");
        }
        else
        {
            if (oldMode)
                LastWindowMode = oldMode;
            Place(width, height);
        }
        Changing = false;
        Previous.reset();
        if (ModeChanged && Pending.load() < 0)
            ModeChanged(Mode);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if ((message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && wParam == VK_RETURN)
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
                Pending.store(3);
            return 0;
        }
        if (Changing && (message == WM_SIZE || message == WM_ACTIVATE || message == WM_ACTIVATEAPP || message == WM_KILLFOCUS || message == WM_SETFOCUS))
            return DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Procedure, 0);
            Attached = false;
            Window = nullptr;
            Pending.store(-1);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline void Attach(HWND window)
    {
        Window = window;
        Attached = window && SetWindowSubclass(window, Procedure, 0, 0);
        if (!Attached)
            OutputDebugStringA("Ultimate Spider-Man: window messages unavailable; live mode changes disabled.\n");
        SquareCorners(window);
    }

    inline void Shutdown()
    {
        if (Attached && Window && IsWindow(Window))
            RemoveWindowSubclass(Window, Procedure, 0);
        Window = nullptr;
        Attached = false;
        Pending.store(-1);
        ResourcesLost = false;
    }
}
