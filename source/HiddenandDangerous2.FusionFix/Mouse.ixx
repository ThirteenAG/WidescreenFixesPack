module;

#include <stdafx.h>
#include "Shared/DirectInputRawMouse.hpp"
#include "rawinput.hpp"
#include <dinput.h>
#include <intrin.h>

export module Mouse;

// DirectInput reads the mouse through raw input sent to the game window, so a high polling rate mouse floods the
// game's message loop. The mouse registration goes to a reader thread instead, and each poll gets the movement,
// wheel and buttons gathered since the last one.
namespace HD2Mouse
{
    SafetyHookInline registerHook;
    DIMOUSESTATE2* nativeMouse{};

    BOOL WINAPI RegisterMouse(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        HMODULE caller = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(_ReturnAddress()), &caller);
        if (!caller || caller != GetModuleHandleW(L"dinput8.dll"))
            return registerHook.stdcall<BOOL>(devices, count, size);
        return DirectInputRawMouse::RegisterDevices(devices, count, size, [](PCRAWINPUTDEVICE list, UINT entries, UINT bytes)
        {
            return registerHook.stdcall<BOOL>(list, entries, bytes);
        });
    }

    void ReadMouse(SafetyHookContext&)
    {
        if (!DirectInputRawMouse::Active())
            return;
        const auto state = DirectInputRawMouse::Take();
        nativeMouse->lX = state.x;
        nativeMouse->lY = state.y;
        nativeMouse->lZ = state.wheel;
        for (size_t i = 0; i < std::size(nativeMouse->rgbButtons); ++i)
            nativeMouse->rgbButtons[i] = ((state.down | state.pressed) & (1u << i)) ? 0x80 : 0;
    }

    void Install()
    {
        auto graphics = GetModuleHandleW(L"LS3DF.dll");
        auto reads = hook::module_pattern(graphics, "68 ? ? ? ? 6A 14 50 FF 51 24 85 C0 7D ? A1");
        auto read = reads.get_first<uint8_t>();
        nativeMouse = *reads.get_first<DIMOUSESTATE2*>(1);
        // Both successful GetDeviceState paths (the first read and the one after reacquiring) join here
        static auto pollHook = safetyhook::create_mid(read + 15 + static_cast<int8_t>(read[14]), ReadMouse);
        DirectInputRawMouse::Start();
        registerHook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterMouse);
        MessagePumpGuard::Install(graphics);
        MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    }
}

class Mouse
{
public:
    Mouse()
    {
        WFP::onInitEvent() += []()
        {
            HD2Mouse::Install();
        };
    }
} Mouse;
