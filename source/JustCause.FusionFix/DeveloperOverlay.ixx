module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

export module DeveloperOverlay;
import Imports;


#pragma comment(lib, "version.lib")

export namespace JCOverlay
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* CurrentDevice = nullptr;
    inline LONG HotkeyDown = 0, HotkeyRequested = 0;
    inline SafetyHookInline CreateDeviceHook, DeviceStateHook, DeviceDataHook;
    inline HRESULT(WINAPI* NativeCreate)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN) = nullptr;

    inline HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    inline std::string BuildTime()
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

    inline bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            if (message == WM_KEYUP || message == WM_SYSKEYUP)
                InterlockedExchange(&HotkeyDown, 0);
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    // The game reads the keyboard through DirectInput, so the hotkey is also polled once per frame
    inline void Keyboard(HWND window)
    {
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down)
        {
            if (!InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
        }
        else
            InterlockedExchange(&HotkeyDown, 0);
        if (InterlockedExchange(&HotkeyRequested, 0) && window && GetForegroundWindow() == window)
            Overlay.SetOpen(!Overlay.IsOpen());
    }

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        if (device == CurrentDevice)
            Overlay.BeforeReset();
    }

    inline void Present(IDirect3DDevice9* device, HWND window)
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
        Keyboard(window);
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
    }

    // Nothing reaches the game's DirectInput devices while the overlay has input
    inline HRESULT WINAPI DeviceState(void* device, DWORD size, void* data)
    {
        const auto result = DeviceStateHook.stdcall<HRESULT>(device, size, data);
        if (SUCCEEDED(result) && data && BlocksInput())
            std::memset(data, 0, size);
        return result;
    }

    inline HRESULT WINAPI DeviceData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        const auto result = DeviceDataHook.stdcall<HRESULT>(device, size, data, count, flags);
        if (SUCCEEDED(result) && count && BlocksInput())
            *count = 0;
        return result;
    }

    inline HRESULT WINAPI CreateDevice(void* input, const GUID* guid, void** device, void* outer)
    {
        const auto result = CreateDeviceHook.stdcall<HRESULT>(input, guid, device, outer);
        if (SUCCEEDED(result) && device && *device && !DeviceStateHook)
        {
            auto table = *reinterpret_cast<void***>(*device);
            DeviceStateHook = safetyhook::create_inline(table[9], DeviceState);
            DeviceDataHook = safetyhook::create_inline(table[10], DeviceData);
        }
        return result;
    }

    inline HRESULT WINAPI Create(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* output, LPUNKNOWN outer)
    {
        const auto result = NativeCreate(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !CreateDeviceHook)
        {
            auto table = *reinterpret_cast<void***>(*output);
            CreateDeviceHook = safetyhook::create_inline(table[3], CreateDevice);
        }
        return result;
    }

    inline void Install()
    {
        NativeCreate = JCImports::Replace("DINPUT8.dll", "DirectInput8Create", &Create);
    }
}
