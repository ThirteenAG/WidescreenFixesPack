module;

#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <dinput.h>
#include "Shared/D3D9Options.hpp"
#include "Shared/PadInput.hpp"
#include "overlay/OverlayD3D9.hpp"

export module Runtime;

import Window;
import Gamepad;
import ComVars;

export namespace FarCryOverlayMouse
{
    template<class Button, class Wheel>
    inline void State(const void* data, uint32_t size, Button button, Wheel wheel)
    {
        if (!data || (size != 16 && size != 20))
            return;
        auto bytes = static_cast<const uint8_t*>(data);
        int32_t delta = 0;
        std::memcpy(&delta, bytes + 8, sizeof(delta));
        for (uint32_t i = 0; i < std::min(size - 12, 5u); ++i)
            button(int(i), (bytes[12 + i] & 0x80) != 0);
        wheel(float(delta) / 120.0f);
    }

    template<class Button, class Wheel>
    inline void Buffered(const void* data, uint32_t stride, uint32_t count, bool peek, Button button, Wheel wheel)
    {
        if (!data || peek || (stride != 16 && stride != 16 + sizeof(uintptr_t)))
            return;
        auto bytes = static_cast<const uint8_t*>(data);
        for (uint32_t i = 0; i < count; ++i, bytes += stride)
        {
            uint32_t offset = 0, value = 0;
            std::memcpy(&offset, bytes, sizeof(offset));
            std::memcpy(&value, bytes + 4, sizeof(value));
            if (offset >= 12 && offset < 17)
                button(int(offset - 12), (value & 0x80) != 0);
            else if (offset == 8)
                wheel(float(int32_t(value)) / 120.0f);
        }
    }
}

#pragma comment(lib, "version.lib")
#pragma comment(lib, "dxguid.lib")

export namespace FarCryRuntime
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline std::atomic<IDirect3DDevice9*> Device{ nullptr };
    inline bool HotkeyDown = false;
    inline std::atomic<void*> MouseDevice{ nullptr }, KeyboardDevice{ nullptr };
    inline SafetyHookInline InputCreateHook, InputEnumHook, InputStateHook, InputDataHook;
    inline HRESULT(WINAPI* NativeInput)(HINSTANCE, DWORD, REFIID, void**, IUnknown*) = nullptr;

    inline bool BlocksInput()
    {
        if constexpr (WFP::DeveloperOverlay)
            return Overlay.BlocksInput();
        return false;
    }

    inline HRESULT WINAPI InputState(void* device, DWORD size, void* data)
    {
        const auto result = InputStateHook.stdcall<HRESULT>(device, size, data);
        const auto mouse = device == MouseDevice.load();
        const auto keyboard = device == KeyboardDevice.load();
        const auto blocked = BlocksInput();
        if (SUCCEEDED(result) && mouse && blocked)
            FarCryOverlayMouse::State(data, size,
                [](int button, bool down) { Overlay.MouseButton(button, down); },
                [](float steps) { Overlay.MouseWheel(steps); });
        if (SUCCEEDED(result) && data && (mouse || keyboard) && blocked)
            std::memset(data, 0, size);
        else if (SUCCEEDED(result) && mouse)
            FarCryPad::MouseState(data, size);
        else if (SUCCEEDED(result) && keyboard)
            FarCryPad::KeyboardState(data, size);
        return result;
    }

    inline HRESULT WINAPI InputData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        const auto capacity = count ? *count : 0;
        const auto result = InputDataHook.stdcall<HRESULT>(device, size, data, count, flags);
        const auto mouse = device == MouseDevice.load();
        const auto keyboard = device == KeyboardDevice.load();
        const auto blocked = BlocksInput();
        if (SUCCEEDED(result) && count && *count <= capacity && mouse && blocked)
            FarCryOverlayMouse::Buffered(data, size, *count, (flags & DIGDD_PEEK) != 0,
                [](int button, bool down) { Overlay.MouseButton(button, down); },
                [](float steps) { Overlay.MouseWheel(steps); });
        if (SUCCEEDED(result) && count && (mouse || keyboard) && blocked)
            *count = 0;
        else if (SUCCEEDED(result) && count && *count <= capacity && mouse)
            FarCryPad::MouseData(data, size, count, capacity, (flags & DIGDD_PEEK) != 0);
        else if (SUCCEEDED(result) && count && *count <= capacity && keyboard)
            FarCryPad::KeyboardData(data, size, count, capacity, (flags & DIGDD_PEEK) != 0);
        return result;
    }

    inline HRESULT WINAPI InputCreate(void* input, const GUID* guid, void** device, void* outer)
    {
        const auto result = InputCreateHook.stdcall<HRESULT>(input, guid, device, outer);
        if (SUCCEEDED(result) && device && *device)
        {
            if (guid && IsEqualGUID(*guid, GUID_SysMouse))
                MouseDevice.store(*device);
            else
            {
                auto previous = *device;
                MouseDevice.compare_exchange_strong(previous, nullptr);
            }
            if (guid && IsEqualGUID(*guid, GUID_SysKeyboard))
                KeyboardDevice.store(*device);
            else
            {
                auto previous = *device;
                KeyboardDevice.compare_exchange_strong(previous, nullptr);
            }
        }
        if (SUCCEEDED(result) && device && *device && !InputStateHook)
        {
            const auto table = *reinterpret_cast<void***>(*device);
            InputStateHook = safetyhook::create_inline(table[9], InputState);
            InputDataHook = safetyhook::create_inline(table[10], InputData);
        }
        return result;
    }

    inline HRESULT WINAPI InputEnum(void* input, DWORD type, void* callback, void* context, DWORD flags)
    {
        if (FarCryPad::HideDevices(type))
            return DI_OK;
        return InputEnumHook.stdcall<HRESULT>(input, type, callback, context, flags);
    }

    inline HRESULT WINAPI Input(HINSTANCE instance, DWORD version, REFIID iid, void** output, IUnknown* outer)
    {
        const auto result = NativeInput(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !InputCreateHook)
        {
            const auto table = *reinterpret_cast<void***>(*output);
            InputCreateHook = safetyhook::create_inline(table[3], InputCreate);
            InputEnumHook = safetyhook::create_inline(table[4], InputEnum);
        }
        return result;
    }

    inline void InitInput()
    {
        if (NativeInput)
            return;
        auto original = IATHook::Replace(GetModuleHandleW(L"CryInput.dll"), "DINPUT8.dll",
            std::forward_as_tuple("DirectInput8Create", Input));
        const auto found = original.find("DirectInput8Create");
        if (found != original.end())
            NativeInput = reinterpret_cast<decltype(NativeInput)>(found->second.get());
    }

    inline HMODULE Module()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Module), &module);
        return module;
    }

    inline std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(Module(), path, DWORD(std::size(path)));
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

    inline void Frame(IDirect3DDevice9* device)
    {
        if (device != Device.load())
            return;
        static std::once_flag once;
        std::call_once(once, []() { WFP::onGameInitEvent().executeAll(); });
        WFP::onGameProcessEvent().executeAll();
        if constexpr (WFP::DeveloperOverlay)
        {
            const auto window = FarCryWindow::Window;
            const bool down = window && GetForegroundWindow() == window &&
                (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
            if (down && !HotkeyDown)
                Overlay.SetOpen(!Overlay.IsOpen());
            HotkeyDown = down;
            Overlay.Present(device, Module());
        }
        auto limit = FarCryLive::Read().fps;
        if (limit == -1)
        {
            static HMONITOR previousMonitor = nullptr;
            static ULONGLONG nextRefresh = 0;
            static int refreshRate = 60;
            const auto currentMonitor = FarCryWindow::CurrentMonitor();
            const auto now = GetTickCount64();
            if (currentMonitor != previousMonitor || now >= nextRefresh)
            {
                previousMonitor = currentMonitor;
                nextRefresh = now + 2000;
                const auto monitor = FarCryWindow::Monitor(currentMonitor);
                DEVMODEW mode{ .dmSize = sizeof(DEVMODEW) };
                refreshRate = EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode) &&
                    mode.dmDisplayFrequency > 1 ? int(mode.dmDisplayFrequency) : 60;
            }
            limit = refreshRate;
        }
        else if (limit != 0)
            limit = std::clamp(limit, 30, 1000);
        D3D9Options::frameLimit = limit;
    }

    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    inline void Bind(IDirect3DDevice9* device)
    {
        Screen.RefreshLayout();
        CursorConfinement::SetEnabled(FarCryLive::Read().confine && !BlocksInput());
        Device.store(device);
        if (device)
            D3D9Options::HookDevice(device);
    }

    inline void Init()
    {
        D3D9Options::frame = Frame;
        D3D9Options::limitDevice = [](IDirect3DDevice9* device)
        {
            return device == Device.load() && D3D9Options::frameLimit > 0;
        };
        FarCryWindow::OnFrame = Bind;
        // The renderer gives up when its device can't be made: with the display off or asleep, or another program
        // in exclusive fullscreen, CreateDevice reports a lost device; wait for it and make it then
        D3D9Options::waitForDevice = true;
        D3D9Options::HookCreation();
        if constexpr (WFP::DeveloperOverlay)
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto build = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", build.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                if (const auto device = Device.load())
                {
                    IDirect3DSurface9* surface = nullptr;
                    if (SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                    {
                        D3DSURFACE_DESC desc{};
                        surface->GetDesc(&desc);
                        surface->Release();
                        ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                    }
                }
            });
            D3D9Options::beforeReset = [](IDirect3DDevice9* device)
            {
                if (device == Device.load())
                    Overlay.BeforeReset();
            };
            FarCryWindow::OnMessage = Message;
            FarCryPad::SetOverlayInput(BlocksInput, [](const XINPUT_GAMEPAD& pad) { PadInput::ToOverlay(Overlay, pad); });
        }
    }
}

