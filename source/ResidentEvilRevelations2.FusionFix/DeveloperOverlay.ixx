module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

export module DeveloperOverlay;

#pragma comment(lib, "version.lib")

export namespace Rev2Overlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    float actualFPS = 0.0f;
    std::atomic<HWND> cursorWindow{ nullptr };
    std::atomic<bool> PauseOnFocusLoss{ true };
    std::function<void(HWND)> FocusSettingChanged;

    UINT FocusSettingMessage()
    {
        static const UINT message = RegisterWindowMessageW(L"ResidentEvilRevelations2.FusionFix.FocusSetting");
        return message;
    }
    LONG HotkeyDown = 0, HotkeyRequested = 0;

    HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(ThisModule(), path, DWORD(std::size(path)));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    void BeforeReset(IDirect3DDevice9* device)
    {
        if (device == CurrentDevice) Overlay.BeforeReset();
    }

    bool Active(HWND window)
    {
        const auto foreground = GetForegroundWindow();
        DWORD process = 0;
        GetWindowThreadProcessId(foreground, &process);
        return process == GetCurrentProcessId() && (foreground == window || GetAncestor(foreground, GA_ROOTOWNER) == window);
    }

    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    void Keyboard(HWND window)
    {
        const bool down = window && Active(window) &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down)
        {
            if (!InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
        }
        else
            InterlockedExchange(&HotkeyDown, 0);
        if (InterlockedExchange(&HotkeyRequested, 0) && window && Active(window))
            Overlay.SetOpen(!Overlay.IsOpen());
    }

    constexpr wchar_t WindowProperty[] = L"ResidentEvilRevelations2.FusionFix.Overlay.Window";
    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto original = reinterpret_cast<WNDPROC>(GetPropW(window, WindowProperty));
        if (!original) return DefWindowProcW(window, message, wParam, lParam);
        const auto focusMessage = FocusSettingMessage();
        if (focusMessage && message == focusMessage)
        {
            if (FocusSettingChanged)
                FocusSettingChanged(window);
            return 0;
        }
        if (message == WM_NULL && cursorWindow.load() != window)
        {
            cursorWindow.store(window);
        }
        if (message == WM_NCDESTROY && cursorWindow.load() == window)
            cursorWindow.store(nullptr);
        LRESULT result = 0;
        if (Message(window, message, wParam, lParam, result)) return result;
        if (message == WM_NCDESTROY) RemovePropW(window, WindowProperty);
        return CallWindowProcW(original, window, message, wParam, lParam);
    }
    void AttachWindow(HWND window)
    {
        if (!window || GetPropW(window, WindowProperty)) return;
        auto original = GetWindowLongPtrW(window, GWLP_WNDPROC);
        if (!original || !SetPropW(window, WindowProperty, reinterpret_cast<HANDLE>(original))) return;
        SetLastError(0);
        if (!SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)) && GetLastError())
            RemovePropW(window, WindowProperty);
    }

    bool BlocksInput()
    {
        if (Overlay.BlocksInput())
            return true;
        if (PauseOnFocusLoss.load())
            return false;
        if (const auto window = cursorWindow.load())
            return !Active(window);
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        return process != GetCurrentProcessId();
    }

    void Draw(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        static LARGE_INTEGER frequency{}, start{};
        static uint32_t frames = 0;
        LARGE_INTEGER now{};
        if (!frequency.QuadPart) QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&now);
        ++frames;
        if (!start.QuadPart) { start = now; frames = 0; }
        else if (frequency.QuadPart > 0 && now.QuadPart - start.QuadPart >= frequency.QuadPart)
        {
            actualFPS = static_cast<float>(frames * static_cast<double>(frequency.QuadPart) / (now.QuadPart - start.QuadPart));
            start = now;
            frames = 0;
        }
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                ui.Value("Frame rate", std::to_string(static_cast<int>(std::lround(actualFPS))));
                IDirect3DSurface9* surface = nullptr;
                if (CurrentDevice && SUCCEEDED(CurrentDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                {
                    D3DSURFACE_DESC desc{};
                    surface->GetDesc(&desc);
                    surface->Release();
                    ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                }
            });
            return true;
        }();
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
    }

}
