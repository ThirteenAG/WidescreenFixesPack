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
#include "overlay/OverlayD3D9.hpp"

export module Runtime;

import Window;

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
    inline IDirect3DDevice9* Device = nullptr;
    inline bool HotkeyDown = false;
    inline std::atomic<void*> MouseDevice{ nullptr };
    inline SafetyHookInline InputCreateHook, InputStateHook, InputDataHook;
    inline HRESULT(WINAPI* NativeInput)(HINSTANCE, DWORD, REFIID, void**, IUnknown*) = nullptr;

    inline HRESULT WINAPI InputState(void* device, DWORD size, void* data)
    {
        const auto result = InputStateHook.stdcall<HRESULT>(device, size, data);
        if (SUCCEEDED(result) && device == MouseDevice.load() && Overlay.BlocksInput())
            FarCryOverlayMouse::State(data, size,
                [](int button, bool down) { Overlay.MouseButton(button, down); },
                [](float steps) { Overlay.MouseWheel(steps); });
        if (SUCCEEDED(result) && data && Overlay.BlocksInput())
            std::memset(data, 0, size);
        return result;
    }

    inline HRESULT WINAPI InputData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        const auto capacity = count ? *count : 0;
        const auto result = InputDataHook.stdcall<HRESULT>(device, size, data, count, flags);
        if (SUCCEEDED(result) && count && *count <= capacity && device == MouseDevice.load() && Overlay.BlocksInput())
            FarCryOverlayMouse::Buffered(data, size, *count, (flags & DIGDD_PEEK) != 0,
                [](int button, bool down) { Overlay.MouseButton(button, down); },
                [](float steps) { Overlay.MouseWheel(steps); });
        if (SUCCEEDED(result) && count && Overlay.BlocksInput())
            *count = 0;
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
        }
        if (SUCCEEDED(result) && device && *device && !InputStateHook)
        {
            const auto table = *reinterpret_cast<void***>(*device);
            InputStateHook = safetyhook::create_inline(table[9], InputState);
            InputDataHook = safetyhook::create_inline(table[10], InputData);
        }
        return result;
    }

    inline HRESULT WINAPI Input(HINSTANCE instance, DWORD version, REFIID iid, void** output, IUnknown* outer)
    {
        const auto result = NativeInput(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !InputCreateHook)
        {
            const auto table = *reinterpret_cast<void***>(*output);
            InputCreateHook = safetyhook::create_inline(table[3], InputCreate);
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
        static std::once_flag once;
        std::call_once(once, []() { WFP::onGameInitEvent().executeAll(); });
        WFP::onGameProcessEvent().executeAll();
        Device = device;
        const auto window = FarCryWindow::Window;
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down && !HotkeyDown)
            Overlay.SetOpen(!Overlay.IsOpen());
        HotkeyDown = down;
        Overlay.Present(device, Module());
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && !Overlay.BlocksInput());
        auto limit = FixSettings().Int("MAIN.FPSLimit");
        if (limit == -1)
        {
            const auto monitor = FarCryWindow::Monitor(FarCryWindow::CurrentMonitor());
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            limit = EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode) ? int(mode.dmDisplayFrequency) : 60;
        }
        D3D9Options::frameLimit = limit == 0 ? 0 : std::clamp(limit, 30, 1000);
    }

    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    inline void Bind(IDirect3DDevice9* device)
    {
        if (device)
            D3D9Options::HookDevice(device);
    }

    inline void Init()
    {
        asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
        {
            static const auto build = BuildTime();
            ui.Value("Version", rsc_ProductVersion);
            ui.Value("BuildTimeUtc", build.c_str());
            ui.Value("Graphics API", "Direct3D 9");
            if (Device)
            {
                IDirect3DSurface9* surface = nullptr;
                if (SUCCEEDED(Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                {
                    D3DSURFACE_DESC desc{};
                    surface->GetDesc(&desc);
                    surface->Release();
                    ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                }
            }
        });
        D3D9Options::frame = Frame;
        D3D9Options::beforeReset = [](IDirect3DDevice9*) { Overlay.BeforeReset(); };
        FarCryWindow::OnFrame = Bind;
        FarCryWindow::OnMessage = Message;

    }
}

