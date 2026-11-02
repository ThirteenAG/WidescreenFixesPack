module;

#include <stdafx.h>
#include "Shared/TonyHawk/Camera.hpp"
#include <atomic>
#include <limits>

export module RawMouse;

import Frontend;
import Controller;

export namespace THAWRawMouse
{
    inline constexpr auto PreviousProperty = L"WFP.THAW.RawMouse.Previous";
    inline const int32_t* Enabled = nullptr;
    inline const float* Sensitivity = nullptr;
    inline const int32_t* InvertY = nullptr;
    inline int32_t* (__cdecl* NativeMouse)() = nullptr;
    inline float (__cdecl* NativeMouseAxis)(const int32_t*, int, int) = nullptr;
    inline std::atomic<bool> Gameplay = false, Observing = false, Relative = true;
    inline std::atomic<int64_t> DeltaX = 0, DeltaY = 0;
    inline std::atomic<int64_t> LookX = 0, LookY = 0;
    inline std::atomic<HWND> InputWindow = nullptr;
    inline bool InputUnicode = false, HadFocus = false;
    inline ULONGLONG NextCheck = 0;
    inline double RemainderX = 0.0, RemainderY = 0.0;
    inline SafetyHookInline LookUpdate, LookLock;
    inline uintptr_t LookOwner = 0;
    inline size_t LookOffset = 0;
    inline ULONGLONG LastLook = 0;

    inline void Clear()
    {
        DeltaX.exchange(0, std::memory_order_relaxed);
        DeltaY.exchange(0, std::memory_order_relaxed);
        LookX.exchange(0, std::memory_order_relaxed);
        LookY.exchange(0, std::memory_order_relaxed);
        RemainderX = RemainderY = 0.0;
    }

    inline LRESULT CALLBACK InputProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto previous = reinterpret_cast<WNDPROC>(GetPropW(window, PreviousProperty));
        if (message == WM_INPUT && Observing.load(std::memory_order_relaxed) &&
            Gameplay.load(std::memory_order_relaxed) && WindowedModeWrapper::GameHWND &&
            GetForegroundWindow() == WindowedModeWrapper::GameHWND)
        {
            RAWINPUT input{};
            UINT size = sizeof(input);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size,
                                sizeof(RAWINPUTHEADER)) != UINT(-1) && input.header.dwType == RIM_TYPEMOUSE)
            {
                const bool relative = !(input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE);
                Relative.store(relative, std::memory_order_relaxed);
                if (relative)
                {
                    DeltaX.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
                    DeltaY.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
                    LookX.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
                    LookY.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
                    if (input.data.mouse.lLastX || input.data.mouse.lLastY)
                        THAWController::UsingPad.store(false, std::memory_order_relaxed);
                }
                else
                {
                    DeltaX.exchange(0, std::memory_order_relaxed);
                    DeltaY.exchange(0, std::memory_order_relaxed);
                    LookX.exchange(0, std::memory_order_relaxed);
                    LookY.exchange(0, std::memory_order_relaxed);
                }
            }
        }
        const auto result = previous ? (IsWindowUnicode(window) ? CallWindowProcW(previous, window, message, wParam, lParam) :
                                        CallWindowProcA(previous, window, message, wParam, lParam)) :
                                       DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_NCDESTROY)
        {
            if (InputWindow.load(std::memory_order_relaxed) == window)
                Observing.store(false, std::memory_order_relaxed);
            RemovePropW(window, PreviousProperty);
        }
        return result;
    }

    inline void Detach()
    {
        Observing.store(false, std::memory_order_relaxed);
        const HWND window = InputWindow.exchange(nullptr, std::memory_order_relaxed);
        if (window && IsWindow(window))
        {
            const auto current = InputUnicode ? GetWindowLongPtrW(window, GWLP_WNDPROC) :
                                               GetWindowLongPtrA(window, GWLP_WNDPROC);
            auto previous = reinterpret_cast<WNDPROC>(GetPropW(window, PreviousProperty));
            if (current == reinterpret_cast<LONG_PTR>(InputProcedure) && previous)
            {
                if (InputUnicode) SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(previous));
                else SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(previous));
            }
        }
        Clear();
    }

    inline HWND RegisteredMouse()
    {
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1) || !count)
            return nullptr;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return nullptr;
        for (UINT i = 0; i < count; ++i)
        {
            if (devices[i].usUsagePage != 1 || devices[i].usUsage != 2) continue;
            const auto target = devices[i].hwndTarget ? devices[i].hwndTarget : WindowedModeWrapper::GameHWND;
            DWORD process = 0;
            if (target) GetWindowThreadProcessId(target, &process);
            return process == GetCurrentProcessId() ? target : nullptr;
        }
        return nullptr;
    }

    inline void Attach(HWND target)
    {
        if (!target || !IsWindow(target)) return;
        const bool unicode = IsWindowUnicode(target) != FALSE;
        const auto current = unicode ? GetWindowLongPtrW(target, GWLP_WNDPROC) : GetWindowLongPtrA(target, GWLP_WNDPROC);
        auto saved = reinterpret_cast<WNDPROC>(GetPropW(target, PreviousProperty));
        if (saved)
        {
            if (current == reinterpret_cast<LONG_PTR>(saved))
            {
                SetLastError(0);
                const auto replaced = unicode ? SetWindowLongPtrW(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputProcedure)) :
                                                SetWindowLongPtrA(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputProcedure));
                if (!replaced && GetLastError()) return;
            }
            else if (current != reinterpret_cast<LONG_PTR>(InputProcedure))
                return;
            InputWindow = target;
            InputUnicode = unicode;
            Observing.store(true, std::memory_order_relaxed);
            return;
        }
        if (!current || !SetPropW(target, PreviousProperty, reinterpret_cast<HANDLE>(current))) return;
        SetLastError(0);
        const auto previous = unicode ? SetWindowLongPtrW(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputProcedure)) :
                                        SetWindowLongPtrA(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputProcedure));
        if (!previous && GetLastError()) { RemovePropW(target, PreviousProperty); return; }
        InputWindow = target;
        InputUnicode = unicode;
        Relative.store(true, std::memory_order_relaxed);
        Observing.store(true, std::memory_order_relaxed);
    }

    inline void Frame(bool gameplayCamera)
    {
        const HWND game = WindowedModeWrapper::GameHWND;
        const bool focus = game && GetForegroundWindow() == game;
        const bool enabled = Enabled && *Enabled;
        const bool gameplay = enabled && focus && gameplayCamera && !THAWMenu::Lookup("current_menu_anchor");
        if (Gameplay.exchange(gameplay, std::memory_order_relaxed) != gameplay)
            Clear();
        if (!enabled || !focus)
        {
            if (InputWindow.load(std::memory_order_relaxed)) Detach();
            HadFocus = false;
            return;
        }
        const auto now = GetTickCount64();
        if (!HadFocus || now >= NextCheck)
        {
            HadFocus = true;
            NextCheck = now + 250;
            const auto target = RegisteredMouse();
            const HWND current = InputWindow.load(std::memory_order_relaxed);
            if (target != current || (current && !IsWindow(current)))
            {
                Detach();
                Attach(target);
            }
        }
    }

    inline int Scale(int64_t delta, double& remainder)
    {
        const double sensitivity = std::clamp(static_cast<double>(*Sensitivity), 0.1, 5.0);
        const double value = std::clamp(delta * sensitivity + remainder,
                                        static_cast<double>(std::numeric_limits<int32_t>::min()),
                                        static_cast<double>(std::numeric_limits<int32_t>::max()));
        const auto result = static_cast<int32_t>(std::round(value));
        remainder = value - result;
        return result;
    }

    inline int32_t* __cdecl GameMouse()
    {
        auto* state = NativeMouse();
        if (!state) return state;
        if (Gameplay.load(std::memory_order_relaxed) && Observing.load(std::memory_order_relaxed) &&
            Relative.load(std::memory_order_relaxed))
        {
            state[0] = Scale(DeltaX.exchange(0, std::memory_order_relaxed), RemainderX);
            state[1] = Scale(DeltaY.exchange(0, std::memory_order_relaxed), RemainderY);
        }
        return state;
    }

    inline float __cdecl CameraMouseAxis(const int32_t* state, int negative, int positive)
    {
        if (Gameplay.load(std::memory_order_relaxed) && Observing.load(std::memory_order_relaxed) &&
            Relative.load(std::memory_order_relaxed))
        {
            std::array<int32_t, 5> copy{};
            std::copy_n(state, copy.size(), copy.begin());
            copy[0] = copy[1] = 0;
            return NativeMouseAxis(copy.data(), negative, positive);
        }
        return NativeMouseAxis(state, negative, positive);
    }

    inline void __fastcall SetLookLock(uintptr_t look, void*, uint8_t locked)
    {
        if (LookOwner == look) LookOwner = 0;
        LookLock.thiscall<void>(look, locked);
    }

    inline void __fastcall UpdateLook(uintptr_t look, void*)
    {
        bool active = false, ownerAlive = false;
        for (const auto& [camera, owner] : TonyCamera::Cameras)
        {
            if (!owner.component) continue;
            const auto componentLook = *reinterpret_cast<uintptr_t*>(owner.component + LookOffset);
            ownerAlive |= componentLook == LookOwner;
            active |= componentLook == look && owner.gameplay;
        }
        if (!ownerAlive) LookOwner = 0;
        auto* locked = reinterpret_cast<uint8_t*>(look + 0x18);
        auto* animated = reinterpret_cast<uint8_t*>(look + 0x19);
        if (LookOwner == look && !*locked) LookOwner = 0;
        const bool allowed = active && Gameplay.load(std::memory_order_relaxed) &&
            Observing.load(std::memory_order_relaxed) && Relative.load(std::memory_order_relaxed) &&
            !THAWController::UsingPad.load(std::memory_order_relaxed) && !*animated;
        const auto x = active ? LookX.exchange(0, std::memory_order_relaxed) : 0;
        const auto y = active ? LookY.exchange(0, std::memory_order_relaxed) : 0;
        const auto now = GetTickCount64();
        if (allowed && (x || y) && (!*locked || LookOwner == look))
        {
            *locked = 1;
            LookOwner = look;
            LastLook = now;
            const float sensitivity = std::clamp(*Sensitivity, 0.1f, 5.0f) * 0.0025f;
            auto* pitch = reinterpret_cast<float*>(look + 0x20);
            auto* yaw = reinterpret_cast<float*>(look + 0x24);
            *yaw = std::remainder(*yaw - static_cast<float>(x) * sensitivity, 6.283185307f);
            *pitch = std::clamp(*pitch + static_cast<float>(y) * sensitivity * (*InvertY ? 1.0f : -1.0f), -0.7f, 0.98f);
        }
        if (LookOwner == look && (!allowed || now - LastLook >= 1500))
        {
            if (!*animated) *locked = 0;
            LookOwner = 0;
        }
        LookUpdate.thiscall<void>(look);
    }

    inline void Install(const int32_t* enabled, const float* sensitivity, const int32_t* invertY)
    {
        Enabled = enabled;
        Sensitivity = sensitivity;
        InvertY = invertY;
        auto input = hook::pattern("E8 ? ? ? ? 8B 74 24 18 8B E8 33 C0 8D 4E 06 33 D2 3B D8");
        NativeMouse = reinterpret_cast<decltype(NativeMouse)>(injector::GetBranchDestination(input.get_first()).as_int());
        injector::MakeCALL(input.get_first(), GameMouse, true);
        auto cameraX = hook::pattern("8B 8F ? ? ? ? 8B 97 ? ? ? ? 51 52 55 E8 ? ? ? ? D8 0D ? ? ? ? 83 C4 0C E8 ? ? ? ? 66 85 C0 74 ? 8A 0D ? ? ? ? 84 C9 0F BF C0 74 ? F7 D8 66 89 46 12");
        auto cameraY = hook::pattern("66 89 46 12 8B 87 ? ? ? ? 8B 8F ? ? ? ? 50 51 55 E8 ? ? ? ? D8 0D ? ? ? ? 83 C4 0C");
        NativeMouseAxis = reinterpret_cast<decltype(NativeMouseAxis)>(injector::GetBranchDestination(cameraX.get_first(15)).as_int());
        injector::MakeCALL(cameraX.get_first(15), CameraMouseAxis, true);
        injector::MakeCALL(cameraY.get_first(19), CameraMouseAxis, true);
        auto component = hook::pattern("56 8B F1 8B 4E 0C 68 2D 12 C7 81 E8 ? ? ? ? 89 86 ? ? ? ? 5E C3");
        LookOffset = *component.get_first<uint32_t>(18);
        auto update = hook::pattern("83 EC 08 56 57 6A 00 68 ? ? ? ? 8B F1 E8 ? ? ? ? 8B 7E 44 83 C4 08 85 C0");
        auto lock = hook::pattern("8A 44 24 04 88 41 18 C2 04 00");
        LookUpdate = safetyhook::create_inline(update.get_first(), UpdateLook);
        LookLock = safetyhook::create_inline(lock.get_first(), SetLookLock);
    }
}
