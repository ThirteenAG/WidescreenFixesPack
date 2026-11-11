module;

#include <stdafx.h>
#include <atomic>
#include <limits>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

import Window;

export namespace PunisherOverlay
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* CurrentDevice = nullptr;
    inline LONG HotkeyDown = 0, HotkeyRequested = 0;
    inline SafetyHookInline CreateDeviceHook, DeviceStateHook, DeviceDataHook;
    inline SafetyHookMid MouseSampleHook;
    inline std::atomic<bool> RawEnabled{ true }, RawSeen{ false };
    inline std::atomic<int64_t> RawX{ 0 }, RawY{ 0 };
    inline std::atomic<HWND> RawWindow{ nullptr };

    inline void ClearRawMouse()
    {
        RawX.exchange(0, std::memory_order_relaxed);
        RawY.exchange(0, std::memory_order_relaxed);
        RawSeen.store(false, std::memory_order_release);
    }

    inline void RawMouseMessage(HWND window, UINT message, LPARAM lParam)
    {
        if (message == WM_KILLFOCUS || message == WM_NCDESTROY)
        {
            ClearRawMouse();
            if (message == WM_NCDESTROY)
                RawWindow.store(nullptr, std::memory_order_relaxed);
        }
        if (message != WM_INPUT || window != RawWindow.load(std::memory_order_relaxed) ||
            GetForegroundWindow() != window || !RawEnabled.load(std::memory_order_relaxed) || Overlay.BlocksInput())
            return;
        RAWINPUT input{};
        UINT size = sizeof(input);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == UINT(-1) ||
            input.header.dwType != RIM_TYPEMOUSE || (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
            return;
        RawX.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
        RawY.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
        if (input.data.mouse.lLastX || input.data.mouse.lLastY)
            RawSeen.store(true, std::memory_order_release);
    }

    inline void AttachRawMouse(HWND window)
    {
        if (!window || !RawEnabled.load(std::memory_order_relaxed))
        {
            RawWindow.store(nullptr, std::memory_order_relaxed);
            ClearRawMouse();
            return;
        }
        static ULONGLONG nextCheck = 0;
        const auto now = GetTickCount64();
        if (now < nextCheck)
            return;
        nextCheck = now + 250;
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
        {
            RawWindow.store(nullptr, std::memory_order_relaxed);
            ClearRawMouse();
            return;
        }
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
        {
            RawWindow.store(nullptr, std::memory_order_relaxed);
            ClearRawMouse();
            return;
        }
        for (UINT i = 0; i < count; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                const auto target = devices[i].hwndTarget ? devices[i].hwndTarget : window;
                if (target != window)
                {
                    RawWindow.store(nullptr, std::memory_order_relaxed);
                    ClearRawMouse();
                }
                else if (RawWindow.exchange(window, std::memory_order_relaxed) != window)
                    ClearRawMouse();
                return;
            }
        }
        RAWINPUTDEVICE device{ 1, 2, 0, window };
        RawWindow.store(RegisterRawInputDevices(&device, 1, sizeof(device)) ? window : nullptr, std::memory_order_relaxed);
        ClearRawMouse();
    }
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
        RawMouseMessage(window, message, lParam);
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

    inline void Frame(IDirect3DDevice9* device)
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
                ui.Value("Graphics API", "Direct3D 8 (through Direct3D 9)");
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
        AttachRawMouse(PunisherWindow::Window);
        if (!RawEnabled.load(std::memory_order_relaxed) || BlocksInput())
            ClearRawMouse();
        Keyboard(PunisherWindow::Window);
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && !BlocksInput());
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

    inline void Init()
    {
        RawEnabled.store(FixSettings().Bool("MAIN.RawInputMouse"), std::memory_order_relaxed);
        FixSettings().OnChange("MAIN.RawInputMouse", []()
        {
            RawEnabled.store(FixSettings().Bool("MAIN.RawInputMouse"), std::memory_order_relaxed);
        });
        auto mouse = hook::pattern("8B 84 24 94 02 00 00 8B 8C 24 98 02 00 00 A3 ? ? ? ?");
        MouseSampleHook = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& regs)
        {
            const auto window = RawWindow.load(std::memory_order_relaxed);
            if (!RawEnabled.load(std::memory_order_relaxed) || !window || GetForegroundWindow() != window || BlocksInput())
            {
                ClearRawMouse();
                return;
            }
            if (!RawSeen.load(std::memory_order_acquire))
                return;
            auto* axes = reinterpret_cast<int32_t*>(regs.esp + 0x294);
            constexpr auto low = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
            constexpr auto high = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
            axes[0] = static_cast<int32_t>(std::clamp(RawX.exchange(0, std::memory_order_relaxed), low, high));
            axes[1] = static_cast<int32_t>(std::clamp(RawY.exchange(0, std::memory_order_relaxed), low, high));
        });
        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.dll", std::forward_as_tuple("DirectInput8Create", Create));
        NativeCreate = reinterpret_cast<decltype(NativeCreate)>(original.at("DirectInput8Create").get());
        PunisherWindow::Message = Message;
        D3D8Compatibility::BeforeReset = []() { Overlay.BeforeReset(); };
        D3D8Compatibility::Frame = Frame;
    }
}

