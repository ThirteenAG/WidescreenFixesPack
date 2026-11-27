module;

#include "stdafx.h"
#include "Shared/DirectInputRawMouse.hpp"
#include "rawinput.hpp"
#include <dinput.h>
#include <intrin.h>

export module Mouse;

namespace
{
    SafetyHookInline registrationHook;
    std::mutex registrationMutex;
    RAWINPUTDEVICE originalMouse{};
    bool suppressLegacy = false;
    std::atomic<bool> ownsMouse = false;
    ptrdiff_t stateOffset;
    bool reacquired = false;
    bool checkMouseRegistration = true;

    bool IsMouse(const RAWINPUTDEVICE& device)
    {
        return device.usUsagePage == 1 && device.usUsage == 2;
    }

    bool IsDirectInputMouse(const RAWINPUTDEVICE& device)
    {
        if (!IsMouse(device) || !device.hwndTarget)
            return false;
        DWORD owner = 0;
        GetWindowThreadProcessId(device.hwndTarget, &owner);
        wchar_t name[16];
        return owner == GetCurrentProcessId() &&
            GetClassNameW(device.hwndTarget, name, _countof(name)) && !wcscmp(name, L"DIEmWin");
    }

    BOOL RegisterAndRememberMouse(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        const BOOL result = registrationHook.stdcall<BOOL>(devices, count, size);
        if (result)
        {
            for (UINT i = 0; i < count; ++i)
            {
                if (IsMouse(devices[i]))
                {
                    originalMouse = devices[i];
                    suppressLegacy = false;
                    ownsMouse = !(originalMouse.dwFlags & RIDEV_REMOVE);
                }
            }
        }
        return result;
    }

    void AdoptExistingMouseRegistration()
    {
        // The native mouse can register before the reader window is ready.
        if (!DirectInputRawMouse::detail::window.load())
            return;

        std::lock_guard guard(registrationMutex);
        checkMouseRegistration = false;
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1) || !count)
            return;
        std::vector<RAWINPUTDEVICE> devices(count);
        const UINT registered = GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE));
        if (registered == UINT(-1))
            return;
        const auto end = devices.begin() + registered;
        const auto mouse = std::find_if(devices.begin(), end, IsDirectInputMouse);
        if (mouse != end)
            DirectInputRawMouse::RegisterDevices(&*mouse, 1, sizeof(RAWINPUTDEVICE), RegisterAndRememberMouse);
    }

    BOOL WINAPI RegisterMouse(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        if (!devices || size != sizeof(RAWINPUTDEVICE) ||
            std::none_of(devices, devices + count, IsMouse))
            return registrationHook.stdcall<BOOL>(devices, count, size);

        HMODULE caller = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(_ReturnAddress()), &caller);
        wchar_t path[MAX_PATH];
        const DWORD length = caller ? GetModuleFileNameW(caller, path, MAX_PATH) : 0;
        const auto name = length && length < MAX_PATH ? wcsrchr(path, L'\\') : nullptr;
        std::lock_guard guard(registrationMutex);
        if ((name && !_wcsicmp(name + 1, L"dinput8.dll")) ||
            std::any_of(devices, devices + count, IsDirectInputMouse))
            return DirectInputRawMouse::RegisterDevices(devices, count, size, RegisterAndRememberMouse);

        const BOOL result = registrationHook.stdcall<BOOL>(devices, count, size);
        if (result && ownsMouse.exchange(false))
            DirectInputRawMouse::Clear();
        return result;
    }

    bool CursorOverClient()
    {
        const HWND window = GetForegroundWindow();
        POINT point{};
        RECT client{};
        if (!window || GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId() ||
            !GetCursorPos(&point) || WindowFromPoint(point) != window ||
            !ScreenToClient(window, &point) || !GetClientRect(window, &client))
            return false;
        return PtInRect(&client, point) != FALSE;
    }
}

export void UpdateMouseRegistration()
{
    const bool want = CursorOverClient();
    std::lock_guard guard(registrationMutex);
    if (!ownsMouse || want == suppressLegacy || (originalMouse.dwFlags & RIDEV_NOLEGACY))
        return;
    auto mouse = originalMouse;
    if (want)
        mouse.dwFlags |= RIDEV_NOLEGACY;
    if (registrationHook.stdcall<BOOL>(&mouse, 1u, UINT(sizeof(mouse))))
        suppressLegacy = want;
}

export void InitMouse()
{
    // Both GetDeviceState attempts join here before the game converts the HRESULT to a bool.
    auto retry = hook::pattern("85 C0 79 ? 48 8B 4F 10 48 8B 01 FF 50 38 48 8B 4F 10 4C 8D 87 ? ? ? ? BA 14 00 00 00 48 8B 01 FF 50 48 48 8B 5C 24 30 C1 E8 1F 34 01");
    stateOffset = *retry.get_first<int32_t>(0x15);
    DirectInputRawMouse::Start();
    registrationHook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterMouse);
    static auto acquire = safetyhook::create_mid(retry.get_first(4), [](SafetyHookContext&)
    {
        reacquired = true;
        checkMouseRegistration = true;
    });
    static auto sample = safetyhook::create_mid(retry.get_first(0x24), [](SafetyHookContext& ctx)
    {
        const bool failed = FAILED(HRESULT(ctx.rax));
        const bool reset = std::exchange(reacquired, false) || failed;
        const bool active = ownsMouse && DirectInputRawMouse::Active();
        if (active && reset)
            DirectInputRawMouse::Clear();
        if (failed)
        {
            checkMouseRegistration = true;
            return;
        }
        if (active)
        {
            auto& state = *reinterpret_cast<DIMOUSESTATE2*>(ctx.rdi + stateOffset);
            const auto mouse = DirectInputRawMouse::Take();
            state.lX = mouse.x;
            state.lY = mouse.y;
            state.lZ = mouse.wheel;
            for (size_t i = 0; i < 5; ++i)
                state.rgbButtons[i] = ((mouse.down | mouse.pressed) & (1u << i)) ? 0x80 : 0;
        }
        if (checkMouseRegistration)
            AdoptExistingMouseRegistration();
    });
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
}
