module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/Camera.hpp"
#include "Shared/TonyHawk/RawMouse.hpp"

export module Camera;

import ComVars;
import Frame;
import Frontend;
import Window;

namespace THUG2RawMouse
{
    SafetyHookInline LookUpdate, LookLock;
    SafetyHookMid MouseState;
    float (__cdecl* NativeMouseAxis)(const int32_t*, int, int) = nullptr;
    size_t LookOffset = 0;
    uintptr_t LookOwner = 0;
    ULONGLONG LastLook = 0;

    void Frame(HWND window, bool blocked)
    {
        TonyRawMouse::Frame(window, blocked, L"THUG2RawMouse", []()
        {
            if (!std::any_of(TonyCamera::Cameras.begin(), TonyCamera::Cameras.end(), [](const auto& entry) { return entry.second.gameplay; }))
                return false;
            THUG2Menu::Weak menu;
            menu.Find(THUG2Menu::Checksum("current_menu_anchor"));
            return !menu.Get();
        });
    }

    float __cdecl MouseAxis(const int32_t* state, int negative, int positive)
    {
        if (TonyRawMouse::Gameplay.load(std::memory_order_relaxed) && TonyRawMouse::Enabled.load(std::memory_order_relaxed) &&
            TonyRawMouse::Seen.load(std::memory_order_acquire))
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
        const bool allowed = active && TonyRawMouse::Gameplay.load(std::memory_order_relaxed) &&
            TonyRawMouse::Enabled.load(std::memory_order_relaxed) && !*animated;
        std::array<int64_t, 2> delta{};
        if (active)
        {
            delta = TonyRawMouse::TakeMotion();
        }
        const auto [x, y] = delta;
        const auto now = GetTickCount64();
        if (allowed && (x || y) && (!*locked || LookOwner == look))
        {
            *locked = 1;
            LookOwner = look;
            LastLook = now;
            const float configured = TonyRawMouse::Sensitivity.load(std::memory_order_relaxed);
            const float sensitivity = (std::isfinite(configured) ? std::clamp(configured, 0.1f, 5.0f) : 1.0f) * 0.0025f;
            auto* pitch = reinterpret_cast<float*>(look + 0x20);
            auto* yaw = reinterpret_cast<float*>(look + 0x24);
            *yaw = std::remainder(*yaw - static_cast<float>(x) * sensitivity, 6.283185307f);
            *pitch = std::clamp(*pitch + static_cast<float>(y) * sensitivity * (TonyRawMouse::InvertY.load(std::memory_order_relaxed) ? 1.0f : -1.0f), -0.7f, 0.98f);
        }
        if (LookOwner == look && (!allowed || !TonyRawMouse::Seen.load(std::memory_order_acquire) || now - LastLook >= 1500))
        {
            if (!*animated)
                *locked = 0;
            LookOwner = 0;
        }
        LookUpdate.thiscall<void>(look);
    }

    void Install()
    {
        TonyRawMouse::InstallSettings(FixSettings());
        auto mouseState = hook::pattern("68 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 6A 14 89 0D ? ? ? ? 89 15 ? ? ? ? 8B 08 50 FF 51 24 8B 0D ? ? ? ? 85 C9");
        static const auto* nativeState = *mouseState.get_first<int32_t*>(1);
        MouseState = safetyhook::create_mid(mouseState.get_first(35), [](SafetyHookContext& regs)
        {
            TonyRawMouse::ReadRelativeState(nativeState, static_cast<HRESULT>(regs.eax));
        });
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
        if (!MouseState || !LookUpdate || !LookLock)
            std::terminate();
        THUG2Device::MouseFrame = Frame;
        THUG2Device::MouseMessage = TonyRawMouse::Message;
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
        }, FixSettings());
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
