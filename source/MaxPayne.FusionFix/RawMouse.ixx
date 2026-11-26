module;

#include <stdafx.h>
#include <commctrl.h>
#include <dinput.h>
#include <stdexcept>
#include "Shared/BufferedRawMouse.hpp"
#include "FixSettings.hpp"
#include "DeveloperOverlay.hpp"
#include "rawinput.hpp"

#pragma comment(lib, "comctl32.lib")

export module RawMouse;

namespace
{
    std::atomic<bool> Enabled{ false };
    BufferedRawMouse Movement;
    std::atomic<HWND> Window = nullptr;
    std::atomic<bool> Reading{ false };
    HWND InputWindow = nullptr;
    bool Registered = false;
    bool OwnRegistration = false;
    HWND CandidateWindow = nullptr;
    bool WasEnabled = false;
    ULONGLONG nextRegistrationCheck = 0;
    bool HavePrevious = false;
    RAWINPUTDEVICE Previous{};
    SafetyHookMid MouseHook;
    SafetyHookInline MouseUpdateHook;
    uint8_t* SuspendedMouse = nullptr;
    constexpr UINT_PTR SubclassId = 0x4D505241;

    void ClearMovement()
    {
        Movement.Clear();
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

    // The game reads the mouse's buffered data only for its buttons, 16 events at a time from a 16 event buffer. A high
    // polling rate mouse fills that buffer with movement before each frame and button presses are dropped: the buffer
    // gets room for a few frames of an 8000 Hz mouse and the game gets only the button events.
    injector::scoped_basic<6> MouseDataCall;
    injector::scoped_write<1> EmptyButtonRetry;
    injector::scoped_write<1> ClearedButtonRetry;
    IDirectInputDeviceA* MouseDevice = nullptr;
    std::vector<DIDEVICEOBJECTDATA_DX3> ButtonEvents;
    std::atomic<bool> discardButtons{ false };
    bool bufferConfigured = false;
    ULONGLONG nextBufferAttempt = 0;
    constexpr DWORD MouseBufferSize = 4096;

    HRESULT WINAPI MouseGetDeviceData(IDirectInputDeviceA* device, DWORD size, DIDEVICEOBJECTDATA_DX3* data, DWORD* count, DWORD flags)
    {
        if (discardButtons.exchange(false))
            ButtonEvents.clear();
        for (;;)
        {
            DIDEVICEOBJECTDATA_DX3 events[256];
            DWORD read = DWORD(std::size(events));
            const HRESULT result = device->GetDeviceData(size, reinterpret_cast<DIDEVICEOBJECTDATA*>(events), &read, flags);
            if (FAILED(result))
            {
                ButtonEvents.clear();
                *count = 0;
                return result;
            }
            for (DWORD i = 0; i < read; ++i)
            {
                if (events[i].dwOfs >= DIMOFS_BUTTON0)
                    ButtonEvents.push_back(events[i]);
            }
            if (read < std::size(events))
                break;
        }
        if (discardButtons.load())
            ButtonEvents.clear();
        const DWORD given = std::min<DWORD>(*count, DWORD(ButtonEvents.size()));
        std::copy_n(ButtonEvents.begin(), given, data);
        ButtonEvents.erase(ButtonEvents.begin(), ButtonEvents.begin() + given);
        *count = given;
        return DI_OK;
    }

    void WatchMouseDevice(IDirectInputDeviceA* device)
    {
        if (device != MouseDevice)
        {
            MouseDevice = device;
            ButtonEvents.clear();
            bufferConfigured = false;
            nextBufferAttempt = 0;
        }
        if (!device || bufferConfigured)
            return;
        const auto now = GetTickCount64();
        if (now < nextBufferAttempt)
            return;
        nextBufferAttempt = now + 500;
        // The native update immediately following this one reacquires the mouse.
        if (FAILED(device->Unacquire()))
            return;
        ButtonEvents.clear();
        DIPROPDWORD buffer{ { sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE }, MouseBufferSize };
        bufferConfigured = SUCCEEDED(device->SetProperty(DIPROP_BUFFERSIZE, &buffer.diph));
    }

    void __fastcall UpdateMouse(uint8_t* self, void* edx, float delta)
    {
        WatchMouseDevice(ReadMouse<IDirectInputDeviceA*>(self + 5));
        if (MaxPayneOverlay::BlocksInput())
        {
            if (SuspendedMouse != self)
            {
                // Releasing the window capture does not release an exclusive DirectInput mouse.
                auto device = ReadMouse<IDirectInputDeviceA*>(self + 5);
                if (device)
                    device->Unacquire();
                ButtonEvents.clear();
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
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return false;
        std::vector<RAWINPUTDEVICE> devices(count);
        const auto found = count ? GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) : 0;
        if (found == UINT(-1))
            return false;
        mouse = {};
        for (UINT i = 0; i < found; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                mouse = devices[i];
                break;
            }
        }
        return true;
    }

    void RestoreRegistration()
    {
        Registered = false;
        Reading.store(false);
        ClearMovement();
        if (!OwnRegistration)
            return;

        RAWINPUTDEVICE current{};
        if (!MouseRegistration(current))
            return;
        if (current.hwndTarget == InputWindow && current.dwFlags == RIDEV_INPUTSINK)
        {
            const auto device = HavePrevious ? Previous : RAWINPUTDEVICE{ 1, 2, RIDEV_REMOVE, nullptr };
            if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
                return;
        }
        OwnRegistration = false;
    }

    LRESULT CALLBACK InputWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_INPUT)
        {
            Movement.Read(reinterpret_cast<HRAWINPUT>(lParam), []()
            {
                const auto target = Window.load();
                return Reading.load() && Enabled.load() && target && GetForegroundWindow() == target;
            });
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam))
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

export void DiscardMouseButtons()
{
    discardButtons.store(true);
}

export void UpdateRawMouseWindow(HWND window)
{
    if (!MouseHook)
        return;
    bool enabled = Enabled.load();
    const auto now = GetTickCount64();
    if (OwnRegistration && !Registered)
    {
        if (now < nextRegistrationCheck)
            return;
        nextRegistrationCheck = now + 500;
        RestoreRegistration();
        if (OwnRegistration)
            return;
    }
    if (CandidateWindow == window && WasEnabled == enabled)
    {
        if (!enabled || !window || (!Registered && now < nextRegistrationCheck))
            return;
        nextRegistrationCheck = now + 500;
        RAWINPUTDEVICE current{};
        if (Registered && !MouseRegistration(current))
            return;
        if (Registered && (current.hwndTarget != InputWindow || current.dwFlags != RIDEV_INPUTSINK))
        {
            // A later registration belongs to its caller; keep it and return to DirectInput.
            OwnRegistration = false;
            RestoreRegistration();
            if (Window.load())
                RemoveWindowSubclass(Window.load(), WindowProc, SubclassId);
            Window = nullptr;
            OutputDebugStringA("Max Payne: raw mouse ownership changed; using DirectInput.\n");
        }
        if (Registered)
            return;
    }
    nextRegistrationCheck = now + 500;
    CandidateWindow = window;
    WasEnabled = enabled;
    RestoreRegistration();
    if (Window.load())
        RemoveWindowSubclass(Window.load(), WindowProc, SubclassId);
    Window = nullptr;
    if (OwnRegistration || !enabled || !window || !IsWindow(window))
        return;
    if (!InputWindow)
        InputWindow = CreateRawInputWindow(L"MaxPayneRawMouse", InputWindowProc);
    if (!InputWindow)
        return;

    RAWINPUTDEVICE current{};
    if (!MouseRegistration(current))
        return;
    HavePrevious = current.usUsage != 0;
    // Another plugin's explicit target keeps ownership of its input.
    if (HavePrevious && (current.hwndTarget || current.dwFlags))
    {
        return;
    }
    Previous = current;
    if (!SetWindowSubclass(window, WindowProc, SubclassId, 0))
        return;
    Window = window;
    RAWINPUTDEVICE mouse{ 1, 2, RIDEV_INPUTSINK, InputWindow };
    OwnRegistration = RegisterRawInputDevices(&mouse, 1, sizeof(mouse)) != FALSE;
    Registered = OwnRegistration;
    Reading.store(Registered);
    if (!Registered)
    {
        RemoveWindowSubclass(window, WindowProc, SubclassId);
        Window = nullptr;
        OutputDebugStringA("Max Payne: raw mouse registration failed; using DirectInput.\n");
    }
}

void InitRawMouse()
{
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    // Replace the mouse update's GetDeviceData call. Other DirectInput clients keep their native calls.
    auto readButtons = hook::pattern("C7 45 F0 10 00 00 00 8B 10 50 FF 52 28 85 C0 74").get_first<uint8_t>(7);
    MouseDataCall.save(readButtons, 6, true);
    injector::WriteMemory<uint8_t>(readButtons, 0x50, true); // push eax (device)
    injector::MakeCALL(readButtons + 1, MouseGetDeviceData, true);
    // Finish the frame after the native button reset if the device stays unacquired.
    auto emptyRetry = hook::pattern("85 C0 7E ? 33 DB 57 8B CE E8 ? ? ? ? 8B 56 49").get_first<uint8_t>(3);
    auto clearedRetry = hook::pattern("3B F8 7C ? EB ? 8B CE 33 FF E8").get_first<uint8_t>(5);
    auto eventCount = hook::pattern("8B 45 F0 33 DB 85 C0 76 ? BF").get_first<uint8_t>();
    EmptyButtonRetry.write(emptyRetry, static_cast<int8_t>(eventCount - (emptyRetry + 1)), true);
    ClearedButtonRetry.write(clearedRetry, static_cast<int8_t>(eventCount - (clearedRetry + 1)), true);
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
        if (!Enabled.load() || !Registered || GetForegroundWindow() != Window)
            return;
        // Replace only relative movement. Native sensitivity, inversion, wheel, buttons and smoothing follow it.
        const auto movement = Movement.Take();
        if (!movement.relative)
            return;
        constexpr int64_t minimum = std::numeric_limits<LONG>::min();
        constexpr int64_t maximum = std::numeric_limits<LONG>::max();
        WriteMouse(mouse + 9, static_cast<LONG>(std::clamp(movement.x, minimum, maximum)));
        WriteMouse(mouse + 13, static_cast<LONG>(std::clamp(movement.y, minimum, maximum)));
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
    if (Window.load())
        RemoveWindowSubclass(Window.load(), WindowProc, SubclassId);
    Window = nullptr;
    CandidateWindow = nullptr;
    MouseHook.reset();
    MouseUpdateHook.reset();
    ClearedButtonRetry.restore();
    EmptyButtonRetry.restore();
    MouseDataCall.restore();
    MouseDevice = nullptr;
    ButtonEvents.clear();
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
