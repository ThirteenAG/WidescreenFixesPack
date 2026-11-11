module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/Camera.hpp"
#include <atomic>

export module Camera;

import ComVars;
import Frame;
import Frontend;
import Window;

namespace THUG2RawMouse
{
    std::atomic<bool> Enabled{ true }, InvertY{ false }, Gameplay{ false }, Seen{ false };
    std::atomic<float> Sensitivity{ 1.0f };
    std::atomic<int64_t> DeltaX{ 0 }, DeltaY{ 0 };
    std::atomic<HWND> InputWindow{ nullptr };
    SafetyHookInline LookUpdate, LookLock;
    float (__cdecl* NativeMouseAxis)(const int32_t*, int, int) = nullptr;
    size_t LookOffset = 0;
    uintptr_t LookOwner = 0;
    ULONGLONG LastLook = 0, NextRegistration = 0;

    void Clear()
    {
        DeltaX.exchange(0, std::memory_order_relaxed);
        DeltaY.exchange(0, std::memory_order_relaxed);
        Seen.store(false, std::memory_order_release);
    }

    void Message(HWND window, UINT message, LPARAM lParam)
    {
        if (message == WM_KILLFOCUS || message == WM_NCDESTROY)
        {
            Clear();
            if (message == WM_NCDESTROY && window == InputWindow.load(std::memory_order_relaxed))
                InputWindow.store(nullptr, std::memory_order_relaxed);
        }
        if (message != WM_INPUT || window != InputWindow.load(std::memory_order_relaxed) ||
            GetForegroundWindow() != window || !Gameplay.load(std::memory_order_relaxed))
            return;
        RAWINPUT input{};
        UINT size = sizeof(input);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == UINT(-1) ||
            input.header.dwType != RIM_TYPEMOUSE || (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
            return;
        DeltaX.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
        DeltaY.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
        if (input.data.mouse.lLastX || input.data.mouse.lLastY)
            Seen.store(true, std::memory_order_release);
    }

    void Frame(HWND window, bool blocked)
    {
        bool active = Enabled.load(std::memory_order_relaxed) && !blocked && window && GetForegroundWindow() == window &&
            std::any_of(TonyCamera::Cameras.begin(), TonyCamera::Cameras.end(), [](const auto& entry) { return entry.second.gameplay; });
        if (active)
        {
            THUG2Menu::Weak menu;
            menu.Find(THUG2Menu::Checksum("current_menu_anchor"));
            active = !menu.Get();
        }
        if (Gameplay.exchange(active, std::memory_order_relaxed) != active || !active)
            Clear();
        if (!window || !Enabled.load(std::memory_order_relaxed) || GetTickCount64() < NextRegistration)
            return;
        NextRegistration = GetTickCount64() + 250;
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
        {
            InputWindow.store(nullptr, std::memory_order_relaxed);
            Clear();
            return;
        }
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
        {
            InputWindow.store(nullptr, std::memory_order_relaxed);
            Clear();
            return;
        }
        for (UINT i = 0; i < count; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                const auto target = devices[i].hwndTarget ? devices[i].hwndTarget : window;
                InputWindow.store(target == window ? window : nullptr, std::memory_order_relaxed);
                if (target != window)
                    Clear();
                return;
            }
        }
        RAWINPUTDEVICE device{ 1, 2, 0, window };
        InputWindow.store(RegisterRawInputDevices(&device, 1, sizeof(device)) ? window : nullptr, std::memory_order_relaxed);
    }

    float __cdecl MouseAxis(const int32_t* state, int negative, int positive)
    {
        if (Gameplay.load(std::memory_order_relaxed) && InputWindow.load(std::memory_order_relaxed) &&
            Seen.load(std::memory_order_acquire))
        {
            std::array<int32_t, 5> copy{};
            std::copy_n(state, copy.size(), copy.begin());
            copy[0] = copy[1] = 0;
            return NativeMouseAxis(copy.data(), negative, positive);
        }
        return NativeMouseAxis(state, negative, positive);
    }

    void __fastcall SetLookLock(uintptr_t look, void*, uint8_t locked)
    {
        if (LookOwner == look)
            LookOwner = 0;
        LookLock.thiscall<void>(look, locked);
    }

    void __fastcall UpdateLook(uintptr_t look, void*)
    {
        bool active = false, ownerAlive = false;
        for (const auto& [camera, owner] : TonyCamera::Cameras)
        {
            if (!owner.component)
                continue;
            const auto componentLook = *reinterpret_cast<uintptr_t*>(owner.component + LookOffset);
            ownerAlive |= componentLook == LookOwner;
            active |= componentLook == look && owner.gameplay;
        }
        if (!ownerAlive)
            LookOwner = 0;
        auto* locked = reinterpret_cast<uint8_t*>(look + 0x18);
        auto* animated = reinterpret_cast<uint8_t*>(look + 0x19);
        if (LookOwner == look && !*locked)
            LookOwner = 0;
        const bool allowed = active && Gameplay.load(std::memory_order_relaxed) &&
            InputWindow.load(std::memory_order_relaxed) && !*animated;
        const auto x = active ? DeltaX.exchange(0, std::memory_order_relaxed) : 0;
        const auto y = active ? DeltaY.exchange(0, std::memory_order_relaxed) : 0;
        const auto now = GetTickCount64();
        if (allowed && (x || y) && (!*locked || LookOwner == look))
        {
            *locked = 1;
            LookOwner = look;
            LastLook = now;
            const float configured = Sensitivity.load(std::memory_order_relaxed);
            const float sensitivity = (std::isfinite(configured) ? std::clamp(configured, 0.1f, 5.0f) : 1.0f) * 0.0025f;
            auto* pitch = reinterpret_cast<float*>(look + 0x20);
            auto* yaw = reinterpret_cast<float*>(look + 0x24);
            *yaw = std::remainder(*yaw - static_cast<float>(x) * sensitivity, 6.283185307f);
            *pitch = std::clamp(*pitch + static_cast<float>(y) * sensitivity * (InvertY.load(std::memory_order_relaxed) ? 1.0f : -1.0f), -0.7f, 0.98f);
        }
        if (LookOwner == look && (!allowed || now - LastLook >= 1500))
        {
            if (!*animated)
                *locked = 0;
            LookOwner = 0;
        }
        LookUpdate.thiscall<void>(look);
    }

    void Install()
    {
        auto updateSettings = []()
        {
            Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"), std::memory_order_relaxed);
            InvertY.store(FixSettings().Bool("MAIN.InvertMouseY"), std::memory_order_relaxed);
            Sensitivity.store(FixSettings().Float("MAIN.MouseSensitivity"), std::memory_order_relaxed);
            Clear();
        };
        updateSettings();
        FixSettings().OnChange("MAIN.RawInputMouse", updateSettings);
        FixSettings().OnChange("MAIN.InvertMouseY", updateSettings);
        FixSettings().OnChange("MAIN.MouseSensitivity", updateSettings);
        auto cameraX = hook::pattern("8B 8F ? ? ? ? 8B 97 ? ? ? ? 51 52 55 E8 ? ? ? ? D8 0D ? ? ? ? 83 C4 0C E8 ? ? ? ? 66 85 C0 74 ? 8A 0D ? ? ? ? 84 C9 0F BF C0 74 ? F7 D8 66 89 46 12");
        auto cameraY = hook::pattern("66 89 46 12 8B 87 ? ? ? ? 8B 8F ? ? ? ? 50 51 55 E8 ? ? ? ? D8 0D ? ? ? ? 83 C4 0C");
        NativeMouseAxis = reinterpret_cast<decltype(NativeMouseAxis)>(injector::GetBranchDestination(cameraX.get_first(15)).as_int());
        injector::MakeCALL(cameraX.get_first(15), MouseAxis, true);
        injector::MakeCALL(cameraY.get_first(19), MouseAxis, true);
        auto component = hook::pattern("56 8B F1 8B 4E 0C 68 2D 12 C7 81 E8 ? ? ? ? 89 86 ? ? ? ? 5E C3");
        LookOffset = *component.get_first<uint32_t>(18);
        auto update = hook::pattern("83 EC 08 56 57 6A 00 68 ? ? ? ? 8B F1 E8 ? ? ? ? 8B 7E 40 83 C4 08 85 C0");
        auto lock = hook::pattern("8A 44 24 04 88 41 18 C2 04 00");
        LookUpdate = safetyhook::create_inline(update.get_first(), UpdateLook);
        LookLock = safetyhook::create_inline(lock.get_first(), SetLookLock);
        if (!LookUpdate || !LookLock)
            std::terminate();
        THUG2Device::MouseFrame = Frame;
        THUG2Device::MouseMessage = Message;
    }
}

export void InstallCameraFix()
{
    hook::pattern pattern("");
    // FOV and its native projection/culling scale use the same corrected angle.
    TonyCamera::Install(
        {
            "THUG2",
            "8B 54 24 20 8B 4A 18 E8 ? ? ? ? 8B 44 24 38 83 C0 FB 83 F8 07 77 ? FF 24 85",
            7, 0x0,
            "C7 06 ? ? ? ? 89 BE C0 00 00 00 E8 ? ? ? ? D9 9E AC 00 00 00 8B CE E8",
            "8B F1 8B 86 70 01 00 00 33 ED 3B C5 0F 84 ? ? ? ? 8B 86 78 01 00 00 3B C5 74 ? 8B 40 2C",
            "E8 ? ? ? ? D9 44 24 04 D9 F2 DD D8 D9 54 24 04 D8 B6 C0 00 00 00 DE C9",
            0xac, 0xc0, 0x170, 0xbc, 0x178, 0x118
        }, &FixSettings().Float("MAIN.FOVFactor"));
    pattern = hook::pattern("D9 96 B0 00 00 00 D8 0D ? ? ? ? D9 F2 DD D8");
    struct FovHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float fov = 0.0f;
            _asm { fstp dword ptr[fov] }
            fov = TonyCamera::Correct(regs.esi, fov, THUG2Frame::FovAspect());
            *reinterpret_cast<float*>(regs.esi + 0xB0) = fov;
            _asm { fld dword ptr[fov] }
        }
    };
    injector::MakeInline<FovHook>(pattern.get_first(), pattern.get_first(6));
    THUG2RawMouse::Install();
}
