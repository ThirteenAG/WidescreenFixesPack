module;

#include <stdafx.h>
#include <commctrl.h>
#include <dinput.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include "DeveloperOverlay.hpp"

#pragma comment(lib, "comctl32.lib")

export module RawMouse;

namespace
{
    std::atomic<bool> Enabled{ false };
    std::atomic<bool> Relative{ false };
    std::atomic<int64_t> DeltaX{ 0 }, DeltaY{ 0 };
    HWND Window = nullptr;
    bool Registered = false;
    bool OwnRegistration = false;
    HWND CandidateWindow = nullptr;
    bool WasEnabled = false;
    bool HavePrevious = false;
    RAWINPUTDEVICE Previous{};
    SafetyHookMid MouseHook;
    SafetyHookInline MouseUpdateHook;
    uint8_t* SuspendedMouse = nullptr;
    constexpr UINT_PTR SubclassId = 0x4D505241;

    void ClearMovement()
    {
        Relative.store(false);
        DeltaX.store(0);
        DeltaY.store(0);
    }

    template<typename T>
    T ReadMouse(uint8_t* address)
    {
        T value;
        std::memcpy(&value, address, sizeof(value));
        return value;
    }

    template<typename T>
    void WriteMouse(uint8_t* address, const T& value)
    {
        std::memcpy(address, &value, sizeof(value));
    }

    void NeutralMouse(uint8_t* mouse)
    {
        WriteMouse(mouse + 9, DIMOUSESTATE2{});
        for (auto offset : { 91, 107 })
            WriteMouse(mouse + offset + 8, ReadMouse<std::array<float, 2>>(mouse + offset));
        WriteMouse(mouse + 127, ReadMouse<float>(mouse + 123));
        auto buttons = ReadMouse<uint8_t*>(mouse + 73);
        int count = ReadMouse<int>(mouse + 45);
        if (buttons && count >= 0 && count <= 8)
        {
            for (int i = 0; i < count; ++i)
            {
                WriteMouse(buttons + i * 15, int{});
                WriteMouse(buttons + i * 15 + 8, uint16_t{});
            }
        }
    }

    void __fastcall UpdateMouse(uint8_t* self, void* edx, float delta)
    {
        if (MaxPayneOverlay::BlocksInput())
        {
            if (SuspendedMouse != self)
            {
                // Releasing the window capture does not release an exclusive DirectInput mouse.
                auto device = ReadMouse<IDirectInputDeviceA*>(self + 5);
                if (device)
                    device->Unacquire();
                SuspendedMouse = self;
            }
            ClearMovement();
            NeutralMouse(self);
            return;
        }
        if (SuspendedMouse == self)
        {
            SuspendedMouse = nullptr;
            ClearMovement();
        }
        MouseUpdateHook.unsafe_fastcall(self, edx, delta);
    }

    bool MouseRegistration(RAWINPUTDEVICE& mouse)
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1) || !count)
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        for (auto& device : devices)
        {
            if (device.usUsagePage == 1 && device.usUsage == 2)
            {
                mouse = device;
                return true;
            }
        }
        return false;
    }

    void RestoreRegistration()
    {
        RAWINPUTDEVICE current{};
        if (OwnRegistration && MouseRegistration(current) && current.hwndTarget == Window && current.dwFlags == 0)
        {
            RAWINPUTDEVICE device = HavePrevious ? Previous : RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr };
            RegisterRawInputDevices(&device, 1, sizeof(device));
        }
        Registered = false;
        OwnRegistration = false;
        ClearMovement();
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == WM_INPUT && Registered && Enabled.load() && GetForegroundWindow() == window)
        {
            RAWINPUT input{};
            UINT size = sizeof(input);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
                input.header.dwType == RIM_TYPEMOUSE)
            {
                if (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
                    ClearMovement();
                else
                {
                    DeltaX.fetch_add(input.data.mouse.lLastX);
                    DeltaY.fetch_add(input.data.mouse.lLastY);
                    Relative.store(true);
                }
            }
        }
        else if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
            ClearMovement();
        else if (message == WM_NCDESTROY)
        {
            RestoreRegistration();
            RemoveWindowSubclass(window, WindowProc, SubclassId);
            Window = nullptr;
            CandidateWindow = nullptr;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
}

export void UpdateRawMouseWindow(HWND window)
{
    if (!MouseHook)
        return;
    bool enabled = Enabled.load();
    if (CandidateWindow == window && WasEnabled == enabled)
    {
        RAWINPUTDEVICE current{};
        if (Registered && (!MouseRegistration(current) || current.hwndTarget != Window || (OwnRegistration && current.dwFlags != 0)))
        {
            // A later registration belongs to its caller; keep it and return to DirectInput.
            OwnRegistration = false;
            RestoreRegistration();
            if (Window)
                RemoveWindowSubclass(Window, WindowProc, SubclassId);
            Window = nullptr;
            OutputDebugStringA("Max Payne: raw mouse ownership changed; using DirectInput.\n");
        }
        return;
    }
    CandidateWindow = window;
    WasEnabled = enabled;
    RestoreRegistration();
    if (Window)
        RemoveWindowSubclass(Window, WindowProc, SubclassId);
    Window = nullptr;
    if (!enabled || !window || !IsWindow(window))
        return;

    RAWINPUTDEVICE current{};
    HavePrevious = MouseRegistration(current);
    // Another plugin's explicit target keeps ownership of its input.
    if (HavePrevious && current.hwndTarget && current.hwndTarget != window)
    {
        OutputDebugStringA("Max Payne: another window owns raw mouse input; using DirectInput.\n");
        return;
    }
    Previous = current;
    Window = window;
    if (!SetWindowSubclass(window, WindowProc, SubclassId, 0))
    {
        Window = nullptr;
        return;
    }
    if (HavePrevious && current.hwndTarget == window)
        Registered = true;
    else
    {
        RAWINPUTDEVICE mouse{ 1, 2, 0, window };
        OwnRegistration = RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) != FALSE;
        Registered = OwnRegistration;
    }
    if (!Registered)
    {
        RemoveWindowSubclass(window, WindowProc, SubclassId);
        Window = nullptr;
        OutputDebugStringA("Max Payne: raw mouse registration failed; using DirectInput.\n");
    }
}

void InitRawMouse()
{
    auto update = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 30 8B 45 08 53 56 8B F1 89 46 4D 8B 46 05 8B 08 57 50 FF 51 1C");
    MouseUpdateHook = safetyhook::create_inline(update.get_first(), UpdateMouse);
    auto pattern = hook::pattern("8B 46 5B 8B 56 53 8B 4E 5F 89 46 63 52 89 4E 67");
    MouseHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& context)
    {
        auto mouse = reinterpret_cast<uint8_t*>(context.esi);
        MaxPayneOverlay::Mouse(mouse + 9);
        if (MaxPayneOverlay::BlocksInput())
        {
            ClearMovement();
            WriteMouse(mouse + 9, DIMOUSESTATE2{});
            return;
        }
        if (!Enabled.load() || !Registered || !Relative.load() || GetForegroundWindow() != Window)
            return;
        // Replace only relative movement. Native sensitivity, inversion, wheel, buttons and smoothing follow it.
        constexpr int64_t minimum = std::numeric_limits<LONG>::min();
        constexpr int64_t maximum = std::numeric_limits<LONG>::max();
        WriteMouse(mouse + 9, static_cast<LONG>(std::clamp(DeltaX.exchange(0), minimum, maximum)));
        WriteMouse(mouse + 13, static_cast<LONG>(std::clamp(DeltaY.exchange(0), minimum, maximum)));
    });
    Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
    FixSettings().OnChange("MAIN.RawInputMouse", []()
    {
        Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
        ClearMovement();
    });
}

void ShutdownRawMouse()
{
    Enabled.store(false);
    RestoreRegistration();
    if (Window)
        RemoveWindowSubclass(Window, WindowProc, SubclassId);
    Window = nullptr;
    CandidateWindow = nullptr;
    MouseHook.reset();
    MouseUpdateHook.reset();
    SuspendedMouse = nullptr;
}

class RawMouse
{
public:
    RawMouse()
    {
        WFP::onInitEvent() += []()
        {
            InitRawMouse();
        };

        WFP::onShutdownEvent() += []()
        {
            ShutdownRawMouse();
        };
    }
} RawMouse;
