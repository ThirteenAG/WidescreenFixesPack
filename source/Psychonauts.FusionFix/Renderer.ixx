module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

// Frame limit, cursor confinement and the developer overlay on the game's device.
export module Renderer;


export namespace PsychonautsRenderer
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* CurrentDevice = nullptr;
    inline std::atomic<HWND> Window{ nullptr };
    inline bool HotkeyDown = false;
    inline SafetyHookInline CreateInputDeviceHook, DeviceStateHook, DeviceDataHook;
    inline HRESULT(WINAPI* NativeInputCreate)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN) = nullptr;
    inline void (*Frame)() = nullptr;

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

    // Window messages reach the overlay first; the hotkey itself is read at the frame boundary because the game's
    // DirectInput keyboard can take the key messages.
    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        Window.store(window, std::memory_order_release);
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            result = 0;
            return true;
        }
        if (message == WM_ACTIVATEAPP && !wParam)
            CursorConfinement::Release();
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    inline void Keyboard(HWND window)
    {
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down && !HotkeyDown)
            Overlay.SetOpen(!Overlay.IsOpen());
        HotkeyDown = down;
    }

    inline void ApplyFrameLimit(HWND target);

    inline void Present(IDirect3DDevice9* device)
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
        const HWND window = Window.load(std::memory_order_acquire);
        ApplyFrameLimit(window);
        CursorConfinement::Suspend(BlocksInput());
        CursorConfinement::Frame(window);
        if (Frame)
            Frame();
        Keyboard(window);
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
    }

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        if (device == CurrentDevice)
            Overlay.BeforeReset();
    }

    // The game reads keyboard and mouse through DirectInput; nothing reaches it while the overlay has input.
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

    inline HRESULT WINAPI CreateInputDevice(void* input, const GUID* guid, void** device, void* outer)
    {
        const auto result = CreateInputDeviceHook.stdcall<HRESULT>(input, guid, device, outer);
        if (SUCCEEDED(result) && device && *device && !DeviceStateHook)
        {
            auto table = *reinterpret_cast<void***>(*device);
            DeviceStateHook = safetyhook::create_inline(table[9], DeviceState);
            DeviceDataHook = safetyhook::create_inline(table[10], DeviceData);
        }
        return result;
    }

    inline HRESULT WINAPI CreateInput(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* output, LPUNKNOWN outer)
    {
        const auto result = NativeInputCreate(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !CreateInputDeviceHook)
        {
            auto table = *reinterpret_cast<void***>(*output);
            CreateInputDeviceHook = safetyhook::create_inline(table[3], CreateInputDevice);
        }
        return result;
    }

    inline std::atomic<int> requestedFrameLimit{ 0 };

    inline void UpdateFrameLimit()
    {
        requestedFrameLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
    }

    inline void ApplyFrameLimit(HWND target)
    {
        const int requested = requestedFrameLimit.load(std::memory_order_relaxed);
        if (requested >= 0)
        {
            D3D9Options::frameLimit = requested > 0 ? std::clamp(requested, 30, 1000) : 0;
            return;
        }
        static HMONITOR previous = nullptr;
        static ULONGLONG lastCheck = 0;
        static int refresh = 60;
        const auto monitor = MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (monitor != previous || now - lastCheck >= 2000)
        {
            previous = monitor;
            lastCheck = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            refresh = GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
                int(mode.dmDisplayFrequency) : 60;
        }
        D3D9Options::frameLimit = refresh;
    }

    inline void Init()
    {
        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        D3D9Options::frame = Present;
        D3D9Options::beforeReset = BeforeReset;
        if (!D3D9Options::HookCreation())
            OutputDebugStringA("Psychonauts Fusion Fix: Direct3D 9 hooks unavailable.\n");

        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.dll", std::forward_as_tuple("DirectInput8Create", CreateInput));
        auto create = original.find("DirectInput8Create");
        if (create == original.end() || !(NativeInputCreate = reinterpret_cast<decltype(NativeInputCreate)>(create->second.get())))
            OutputDebugStringA("Psychonauts Fusion Fix: developer overlay input hook unavailable.\n");
    }
}
