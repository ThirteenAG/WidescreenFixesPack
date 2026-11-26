module;

#include <stdafx.h>
#include <dinput.h>
#include <cwchar>
#include <intrin.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/DirectInputRawMouse.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/WindowCursor.hpp"
#include "overlay/OverlayD3D9.hpp"
#include "rawinput.hpp"

#pragma comment(lib, "version.lib")

// Cursor confinement and the developer overlay on the game's device.

export module Renderer;


export namespace NeoRenderer
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* CurrentDevice = nullptr;
    inline std::atomic<HWND> Window{ nullptr };
    inline thread_local HWND ResetWindow = nullptr;
    inline bool HotkeyDown = false;
    inline SafetyHookInline KeyboardStateHook, ControllerStateHook;
    inline SafetyHookInline RegisterRawMouseHook;
    inline SafetyHookMid MouseStateHook;
    inline std::atomic<bool> RecoverMouse{ false };
    inline DWORD OverlayPadSlot = XUSER_MAX_COUNT;
    inline bool ControllerSuppressed = false;
    inline SafetyHookInline CreateDeviceHook;
    inline IDirect3D9*(WINAPI* NativeCreate9)(UINT) = nullptr;
    inline void (*DeviceCreated)(IDirect3DDevice9*) = nullptr;
    inline void (*Started)() = nullptr;

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
        if (Window.load() == window && (message == WM_ACTIVATEAPP || message == WM_NCDESTROY))
            DirectInputRawMouse::Clear();
        if constexpr (!WFP::DeveloperOverlay)
            return false;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            result = 0;
            return true;
        }
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

    inline bool OverlayChord(const XINPUT_GAMEPAD& pad)
    {
        constexpr WORD chord = XINPUT_GAMEPAD_BACK | XINPUT_GAMEPAD_START;
        return (pad.wButtons & chord) == chord;
    }

    inline bool PadHeld(const XINPUT_GAMEPAD& pad)
    {
        return pad.wButtons != 0 || std::abs(int(pad.sThumbLX)) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
            std::abs(int(pad.sThumbLY)) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
            std::abs(int(pad.sThumbRX)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
            std::abs(int(pad.sThumbRY)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
            pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD || pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    }

    inline XINPUT_GAMEPAD ReadOverlayPad()
    {
        const auto getState = PadInput::XInput().getState;
        XINPUT_STATE state{};
        if (!getState)
            return {};
        if (OverlayPadSlot < XUSER_MAX_COUNT && getState(OverlayPadSlot, &state) == ERROR_SUCCESS)
            return state.Gamepad;
        OverlayPadSlot = XUSER_MAX_COUNT;
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            if (getState(slot, &state) == ERROR_SUCCESS &&
                (OverlayChord(state.Gamepad) || (Overlay.IsOpen() && PadHeld(state.Gamepad))))
            {
                OverlayPadSlot = slot;
                return state.Gamepad;
            }
        }
        return {};
    }

    inline bool NativeDigitalHeld(const DIJOYSTATE2& pad)
    {
        return LOWORD(pad.rgdwPOV[0]) != 0xFFFF ||
            std::any_of(pad.rgbButtons, pad.rgbButtons + 24, [](BYTE button) { return (button & 0x80) != 0; });
    }

    inline bool NativeAxesHeld(const DIJOYSTATE2& pad)
    {
        // The game consumes these four signed axes on the first DirectInput device.
        constexpr LONG deadZone = 32;
        const LONG axes[] = { pad.lX, pad.lY, pad.lZ, pad.lRz };
        return std::any_of(std::begin(axes), std::end(axes), [](LONG axis) { return axis < -deadZone || axis > deadZone; });
    }

    inline bool BlocksController(const DIJOYSTATE2& first, const DIJOYSTATE2& second)
    {
        const auto pad = ReadOverlayPad();
        ControllerSuppressed = BlocksInput() || OverlayChord(pad) ||
            (ControllerSuppressed && (PadHeld(pad) || NativeAxesHeld(first) || NativeDigitalHeld(first) || NativeDigitalHeld(second)));
        return ControllerSuppressed;
    }

    inline void UpdateOverlayPad()
    {
        const auto pad = ReadOverlayPad();
        PadInput::ToOverlay(Overlay, pad);
        if (!ControllerSuppressed && !BlocksInput() && !PadHeld(pad))
            OverlayPadSlot = XUSER_MAX_COUNT;
    }

    inline HWND DeviceWindow(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        return device && SUCCEEDED(device->GetCreationParameters(&parameters)) ? parameters.hFocusWindow : nullptr;
    }

    inline bool OwnsDeviceWindow(IDirect3DDevice9* device, HWND window)
    {
        return window && DeviceWindow(device) == window;
    }

    inline void Present(IDirect3DDevice9* device)
    {
        const HWND window = Window.load(std::memory_order_acquire);
        if (!OwnsDeviceWindow(device, window))
            return;
        if (Started)
            Started();
        WindowCursor::Follow(window);
        if (GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId())
        {
            CursorConfinement::Suspend(BlocksInput());
            CursorConfinement::Frame(window);
        }
        WFP::onGameProcessEvent().executeAll();
        if constexpr (!WFP::DeveloperOverlay)
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
        UpdateOverlayPad();
    }

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        const auto window = Window.load(std::memory_order_acquire);
        ResetWindow = OwnsDeviceWindow(device, window) ? window : nullptr;
        if constexpr (WFP::DeveloperOverlay)
        {
            if (device == CurrentDevice)
                Overlay.BeforeReset();
        }
    }

    inline HRESULT __fastcall KeyboardState(uint8_t* input)
    {
        const auto result = KeyboardStateHook.thiscall<HRESULT>(input);
        if (BlocksInput())
            std::memset(input + 0x2AC, 0, 256);
        return result;
    }

    inline void MouseState(SafetyHookContext& regs)
    {
        auto& mouse = *reinterpret_cast<DIMOUSESTATE2*>(regs.esi + 0x238);
        const auto window = Window.load();
        const bool focused = window && window == GetForegroundWindow();
        if (!DirectInputRawMouse::Active() && focused && SUCCEEDED(static_cast<HRESULT>(regs.eax)) &&
            GetFocus() == window && RecoverMouse.exchange(false))
        {
            // A menu transition can remove the raw registration while the native mouse still reports success.
            const auto device = *reinterpret_cast<IDirectInputDevice8A**>(regs.esi + 0x14);
            if (device)
            {
                device->Unacquire();
                device->Acquire();
            }
        }
        if (DirectInputRawMouse::Active())
        {
            const auto state = DirectInputRawMouse::Take();
            if (focused)
            {
                mouse.lX = state.x;
                mouse.lY = state.y;
                mouse.lZ = state.wheel;
                for (size_t button = 0; button < 5; ++button)
                    mouse.rgbButtons[button] = ((state.down | state.pressed) & (1u << button)) ? 0x80 : 0;
            }
            else
                mouse = {};
        }
        if constexpr (WFP::DeveloperOverlay)
        {
            if (BlocksInput())
                mouse = {};
        }
    }

    inline BOOL WINAPI RegisterRawMouse(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        const auto original = [](PCRAWINPUTDEVICE list, UINT entries, UINT bytes)
        {
            return RegisterRawMouseHook.stdcall<BOOL>(list, entries, bytes);
        };
        HMODULE caller = nullptr;
        wchar_t path[MAX_PATH]{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(_ReturnAddress()), &caller))
            return original(devices, count, size);
        const auto length = GetModuleFileNameW(caller, path, MAX_PATH);
        const auto name = length && length < MAX_PATH ? std::wcsrchr(path, L'\\') : nullptr;
        if (!name || (_wcsicmp(name + 1, L"dinput8.dll") != 0 && _wcsicmp(name + 1, L"dinput.dll") != 0))
            return original(devices, count, size);
        const bool hadMouse = DirectInputRawMouse::Active();
        const BOOL result = DirectInputRawMouse::RegisterDevices(devices, count, size, original);
        if (result && hadMouse && !DirectInputRawMouse::Active())
            RecoverMouse.store(true);
        return result;
    }

    inline HRESULT __fastcall ControllerState(uint8_t* input)
    {
        const auto result = ControllerStateHook.thiscall<HRESULT>(input);
        auto& first = *reinterpret_cast<DIJOYSTATE2*>(input + 0x18);
        auto& second = *reinterpret_cast<DIJOYSTATE2*>(input + 0x128);
        // Both APIs must report neutral before the native controller resumes.
        if (BlocksController(first, second))
        {
            // The game configures signed axes and uses the same neutral state when a pad is disconnected.
            DIJOYSTATE2 neutral{};
            std::fill(std::begin(neutral.rgdwPOV), std::end(neutral.rgdwPOV), DWORD(-1));
            first = neutral;
            second = neutral;
            std::memset(input + 0x24C, 0, 48);
        }
        return result;
    }

    inline HRESULT STDMETHODCALLTYPE CreateDevice(IDirect3D9* factory, UINT adapter, D3DDEVTYPE type, HWND window,
        DWORD flags, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** output)
    {
        if (parameters)
            D3D9Options::presentParameters(parameters, window);
        const HRESULT result = CreateDeviceHook.unsafe_stdcall<HRESULT>(factory, adapter, type, window, flags, parameters, output);
        if (SUCCEEDED(result) && output && *output)
        {
            D3D9Options::HookDevice(*output);
            DeviceCreated(*output);
        }
        return result;
    }

    inline IDirect3D9* WINAPI Create9(UINT sdkVersion)
    {
        auto* factory = NativeCreate9(sdkVersion);
        if (factory)
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            if (!CreateDeviceHook)
            {
                auto table = *reinterpret_cast<void***>(factory);
                CreateDeviceHook = safetyhook::create_inline(table[IDirect3D9VTBL::CreateDevice], CreateDevice);
            }
        }
        return factory;
    }

    inline void Init()
    {
        WindowCursor::Initialize(FixSettings());
        MessagePumpGuard::Install(GetModuleHandleW(nullptr));
        D3D9Options::frame = Present;
        D3D9Options::beforeReset = BeforeReset;
        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "D3D9.dll", std::forward_as_tuple("Direct3DCreate9", Create9));
        NativeCreate9 = reinterpret_cast<decltype(NativeCreate9)>(original.at("Direct3DCreate9").get());

        // Replace the successful DirectInput sample before the game translates wheel motion to actions.
        auto mouse = hook::pattern("3D 1E 00 07 80 74 ? 8B 86 40 02 00 00 85 C0 7E");
        MouseStateHook = safetyhook::create_mid(mouse.get_first(7), MouseState);
        if (MouseStateHook)
        {
            DirectInputRawMouse::Start();
            RegisterRawMouseHook = safetyhook::create_inline(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterRawMouse);
        }

        if constexpr (!WFP::DeveloperOverlay)
            return;
        auto keyboard = hook::pattern("56 57 8B F1 8B 46 10 8B 08 8D BE AC 02 00 00 57 68 00 01 00 00 50 FF 51 24");
        KeyboardStateHook = safetyhook::create_inline(keyboard.get_first(), KeyboardState);
        auto controller = hook::pattern("53 55 8B D9 8B 43 08 56 33 ED 85 C0 57 0F 84 ? ? ? ? 8B 08 50 FF 51 64");
        ControllerStateHook = safetyhook::create_inline(controller.get_first(), ControllerState);
    }
}
