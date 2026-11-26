module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "MouseInput.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/PadInput.hpp"
#include "overlay/OverlayD3D9.hpp"
#include <array>
#include <charconv>

#pragma comment(lib, "version.lib")

// Frame limit, cursor confinement and the developer overlay on the game's device.
export module Renderer;

export namespace PsychonautsRenderer
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline std::atomic<HWND> Window{ nullptr };
}

namespace
{
    uint8_t* nativePadConnected{};
    const uint8_t* nativeXInputEnabled{};
    unsigned(__cdecl* clearInputEdges)(){};
    int(__cdecl* setGameButton)(int, uint8_t){};
    struct PadSource
    {
        XINPUT_STATE state{};
        XINPUT_GAMEPAD previous{};
        ULONGLONG probeAfter{};
        bool connected{};
    };
    std::array<PadSource, XUSER_MAX_COUNT> padSources{};
    DWORD padSlot{ XUSER_MAX_COUNT };
    XINPUT_STATE padState{};
    XINPUT_GAMEPAD heldOnHandoff{};
    bool padSuppressed{};

    bool PadActive(const XINPUT_GAMEPAD& pad, BYTE triggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
    {
        constexpr int nativeDeadzone = int(32767 * 0.15f);
        return pad.wButtons || pad.bLeftTrigger > triggerThreshold || pad.bRightTrigger > triggerThreshold ||
            std::abs(int(pad.sThumbLX)) > nativeDeadzone || std::abs(int(pad.sThumbLY)) > nativeDeadzone ||
            std::abs(int(pad.sThumbRX)) > nativeDeadzone || std::abs(int(pad.sThumbRY)) > nativeDeadzone;
    }

    void UpdatePadSelection(const PadInput::XInputApi& api, ULONGLONG now)
    {
        if (!*nativeXInputEnabled || !api.getState)
        {
            padSources = {};
            padSlot = XUSER_MAX_COUNT;
            padState = {};
            heldOnHandoff = {};
            return;
        }

        DWORD firstConnected = XUSER_MAX_COUNT, firstActive = XUSER_MAX_COUNT;
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            auto& source = padSources[slot];
            if (now < source.probeAfter)
                continue;
            XINPUT_STATE state{};
            if (api.getState(slot, &state) != ERROR_SUCCESS)
            {
                source.connected = false;
                source.probeAfter = now + 500;
                continue;
            }
            source.previous = source.connected ? source.state.Gamepad : state.Gamepad;
            source.state = state;
            source.connected = true;
            if (firstConnected == XUSER_MAX_COUNT)
                firstConnected = slot;
            if (firstActive == XUSER_MAX_COUNT && PadActive(state.Gamepad))
                firstActive = slot;
        }

        const auto oldSlot = padSlot;
        const bool keepConnected = padSlot < XUSER_MAX_COUNT && padSources[padSlot].connected;
        if (!keepConnected || !PadActive(padSources[padSlot].state.Gamepad))
        {
            if (firstActive < XUSER_MAX_COUNT)
                padSlot = firstActive;
            else if (!keepConnected)
                padSlot = firstConnected;
        }
        if (padSlot == XUSER_MAX_COUNT)
        {
            padState = {};
            heldOnHandoff = {};
            return;
        }

        const auto& source = padSources[padSlot];
        if (oldSlot != padSlot)
            heldOnHandoff = source.previous;
        padState = source.state;
        auto& pad = padState.Gamepad;
        // A button already held on the incoming pad needs a release before it can act in the game.
        heldOnHandoff.wButtons &= pad.wButtons;
        pad.wButtons &= ~heldOnHandoff.wButtons;
        if (pad.bLeftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            heldOnHandoff.bLeftTrigger = 0;
        if (pad.bRightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            heldOnHandoff.bRightTrigger = 0;
        if (heldOnHandoff.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            pad.bLeftTrigger = 0;
        if (heldOnHandoff.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            pad.bRightTrigger = 0;
    }

    bool RoutePadOverlay(bool connected)
    {
        auto& overlay = PsychonautsRenderer::Overlay;
        const auto window = PsychonautsRenderer::Window.load(std::memory_order_acquire);
        if (!window || GetForegroundWindow() != window)
            return padSuppressed || overlay.BlocksInput();

        const auto pad = connected ? padState.Gamepad : XINPUT_GAMEPAD{};
        PadInput::ToOverlay(overlay, pad);
        if (overlay.BlocksInput())
            return true;
        return padSuppressed && PadActive(pad, 0);
    }

    unsigned __cdecl PollNativePad()
    {
        const auto result = clearInputEdges();
        const auto& api = PadInput::XInput();
        const auto oldSlot = padSlot;
        UpdatePadSelection(api, GetTickCount64());

        const bool connected = padSlot < XUSER_MAX_COUNT;
        if constexpr (WFP::DeveloperOverlay)
            padSuppressed = RoutePadOverlay(connected);
        if (connected != (*nativePadConnected != 0))
        {
            for (int button = 276; button < 312; ++button)
                setGameButton(button, 0);
            *nativePadConnected = connected;
        }
        if (api.setState && oldSlot < XUSER_MAX_COUNT && oldSlot != padSlot)
        {
            XINPUT_VIBRATION stopped{};
            api.setState(oldSlot, &stopped);
        }
        return result;
    }

    DWORD WINAPI NativePadState(DWORD, XINPUT_STATE* state)
    {
        if (padSlot == XUSER_MAX_COUNT)
            return ERROR_DEVICE_NOT_CONNECTED;
        *state = padState;
        if constexpr (WFP::DeveloperOverlay)
            if (padSuppressed)
                state->Gamepad = {};
        return ERROR_SUCCESS;
    }

    DWORD WINAPI NativePadVibration(DWORD, XINPUT_VIBRATION* vibration)
    {
        const auto setState = PadInput::XInput().setState;
        return setState && padSlot < XUSER_MAX_COUNT ? setState(padSlot, vibration) : ERROR_DEVICE_NOT_CONNECTED;
    }

    void InstallNativePad(decltype(setGameButton) setButton)
    {
        setGameButton = setButton;
        auto mode = hook::pattern("0F B6 0D ? ? ? ? 85 C9 74 12 0F B6 15 ? ? ? ? 85 D2 74 07 E8");
        nativePadConnected = *mode.get_first<uint8_t*>(3);
        nativeXInputEnabled = *mode.get_first<uint8_t*>(14);
        auto poll = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 8D 85 50 FF FF FF 50 E8 ? ? ? ? 83 C4 04 0F B6 0D").get_first();
        clearInputEdges = reinterpret_cast<decltype(clearInputEdges)>(injector::GetBranchDestination(poll).as_int());
        injector::MakeCALL(poll, PollNativePad, true);
        auto buttons = hook::pattern("E8 ? ? ? ? F7 D8 1B C0 83 C0 01 88 45 FF 0F B6 45 FF 85 C0 75 0A B8 05 40 00 80 E9 ? ? ? ? 0F B7 4D F0 81 E1");
        auto axes = hook::pattern("E8 ? ? ? ? F7 D8 1B C0 83 C0 01 88 45 FF 0F B6 45 FF 85 C0 75 0A B8 05 40 00 80 E9 ? ? ? ? 51 D9 05");
        auto vibration = hook::pattern("8D 55 FC 52 6A 00 E8 ? ? ? ? 8B E5 5D C3 55 8B EC A0");
        injector::MakeCALL(buttons.get_first(), NativePadState, true);
        injector::MakeCALL(axes.get_first(), NativePadState, true);
        injector::MakeCALL(vibration.get_first(6), NativePadVibration, true);
    }
}

export namespace PsychonautsRenderer
{
    inline IDirect3DDevice9* CurrentDevice = nullptr;
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

    inline void ClearWindow(HWND window)
    {
        if (Window.compare_exchange_strong(window, nullptr, std::memory_order_acq_rel))
        {
            PsychonautsMouse::SetWindow(nullptr);
            CursorConfinement::Release();
        }
    }

    // Window messages reach the overlay first; the hotkey itself is read at the frame boundary because the game's
    // DirectInput keyboard can take the key messages.
    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if (Window.exchange(window, std::memory_order_acq_rel) != window)
            PsychonautsMouse::SetWindow(window);
        if (message == WM_ACTIVATEAPP && !wParam)
        {
            PsychonautsMouse::Clear();
            CursorConfinement::Release();
        }
        if constexpr (WFP::DeveloperOverlay)
        {
            if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
                wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
            {
                result = 0;
                return true;
            }
            return Overlay.Message(window, message, wParam, lParam, result);
        }
        return false;
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

    inline void PresentOverlay(IDirect3DDevice9* device, HWND window)
    {
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

    inline void Present(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        const HWND window = Window.load(std::memory_order_acquire);
        ApplyFrameLimit(window);
        if constexpr (WFP::DeveloperOverlay)
        {
            const bool blocked = BlocksInput();
            CursorConfinement::Suspend(blocked);
            PsychonautsMouse::Suspend(blocked);
        }
        CursorConfinement::Frame(window);
        if (Frame)
            Frame();
        if constexpr (WFP::DeveloperOverlay)
            PresentOverlay(device, window);
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
        const auto setting = FixSettings().Get("MAIN.FPSLimit");
        int rate = 60;
        const auto parsed = std::from_chars(setting.data(), setting.data() + setting.size(), rate);
        if (parsed.ec != std::errc{} || parsed.ptr != setting.data() + setting.size())
            rate = 60;
        requestedFrameLimit.store(rate, std::memory_order_relaxed);
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

    void Init()
    {
        PsychonautsMouse::Install();
        InstallNativePad(PsychonautsMouse::setButton);
        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") == "1");
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") == "1");
        });
        D3D9Options::frame = Present;
        D3D9Options::HookCreation();

        if constexpr (WFP::DeveloperOverlay)
        {
            D3D9Options::beforeReset = BeforeReset;
            auto original = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.dll", std::forward_as_tuple("DirectInput8Create", CreateInput));
            NativeInputCreate = reinterpret_cast<decltype(NativeInputCreate)>(original.at("DirectInput8Create").get());
        }
    }
}
