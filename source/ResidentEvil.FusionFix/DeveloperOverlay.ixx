module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

export module DeveloperOverlay;

import FrameRate;

#pragma comment(lib, "version.lib")

export namespace REOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
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
            if (message == WM_KEYUP || message == WM_SYSKEYUP)
                InterlockedExchange(&HotkeyDown, 0);
            else if (!(lParam & (1u << 30)) && Active(window) &&
                !InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
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

    constexpr wchar_t WindowProperty[] = L"ResidentEvil.FusionFix.Overlay.Window";
    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto original = reinterpret_cast<WNDPROC>(GetPropW(window, WindowProperty));
        if (!original) return DefWindowProcW(window, message, wParam, lParam);
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
        return Overlay.BlocksInput();
    }

    void Draw(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                ui.Value("Frame rate", std::to_string(static_cast<int>(std::lround(REFrameRate::actualFPS))));
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
