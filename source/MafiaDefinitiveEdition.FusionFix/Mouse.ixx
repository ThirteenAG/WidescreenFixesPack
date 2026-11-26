module;

#include "stdafx.h"
#include "Shared/DirectInputRawMouse.hpp"
#include "rawinput.hpp"
#include <dinput.h>
#include <intrin.h>

export module Mouse;

namespace
{
    SafetyHookInline pollHook, registrationHook;
    ptrdiff_t mouseStateOffset;
    bool resetOnAcquire = false;

    HRESULT __fastcall Poll(uint8_t* device)
    {
        const HRESULT result = pollHook.fastcall<HRESULT>(device);
        if (FAILED(result))
        {
            resetOnAcquire = true;
            if (DirectInputRawMouse::Active())
                DirectInputRawMouse::Clear();
        }
        else if (DirectInputRawMouse::Active())
        {
            if (std::exchange(resetOnAcquire, false))
                DirectInputRawMouse::Clear();
            auto& state = *reinterpret_cast<DIMOUSESTATE2*>(device + mouseStateOffset);
            const auto mouse = DirectInputRawMouse::Take();
            state.lX = mouse.x;
            state.lY = mouse.y;
            state.lZ = mouse.wheel;
            for (size_t button = 0; button < 5; ++button)
                state.rgbButtons[button] = ((mouse.down | mouse.pressed) & (1u << button)) ? 0x80 : 0;
        }
        return result;
    }

    BOOL WINAPI RegisterMouse(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        HMODULE caller = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(_ReturnAddress()), &caller);
        wchar_t path[MAX_PATH];
        const DWORD length = caller ? GetModuleFileNameW(caller, path, MAX_PATH) : 0;
        const wchar_t* name = length && length < MAX_PATH ? wcsrchr(path, L'\\') : nullptr;
        if (!name || _wcsicmp(name + 1, L"dinput8.dll"))
            return registrationHook.stdcall<BOOL>(devices, count, size);
        return DirectInputRawMouse::RegisterDevices(devices, count, size, [](PCRAWINPUTDEVICE list, UINT entries, UINT bytes)
        {
            return registrationHook.stdcall<BOOL>(list, entries, bytes);
        });
    }
}

export void InitMouse()
{
    // The game's mouse poll clears its DIMOUSESTATE2 and passes it to GetDeviceState.
    auto poll = hook::pattern("48 89 5C 24 08 57 48 83 EC 20 33 C0 48 8B F9 48 89 81 ? ? ? ? BA 14 00 00 00");
    mouseStateOffset = *poll.get_first<int32_t>(18);
    auto retry = hook::pattern("85 C0 79 ? 48 8B 4F 10 48 8B 01 FF 50 38 48 8B 4F 10 4C 8D 87 ? ? ? ? BA 14 00 00 00");
    static auto reacquire = safetyhook::create_mid(retry.get_first(4), [](SafetyHookContext&)
    {
        resetOnAcquire = true;
    });
    pollHook = safetyhook::create_inline(poll.get_first(), Poll);
    DirectInputRawMouse::Start();
    registrationHook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterMouse);
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
}
