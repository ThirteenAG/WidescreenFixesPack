module;

#include <stdafx.h>
#include "Shared/TonyHawk/Camera.hpp"
#include "Shared/TonyHawk/RawMouse.hpp"

export module RawMouse;

import Frontend;
import Controller;

export namespace THAWRawMouse
{
    inline SafetyHookInline LookUpdate, LookLock;
    inline SafetyHookMid MouseState;
    inline float (__cdecl* NativeMouseAxis)(const int32_t*, int, int) = nullptr;
    inline uintptr_t LookOwner = 0;
    inline size_t LookOffset = 0;
    inline ULONGLONG LastLook = 0;

    inline void Frame(bool blocked)
    {
        TonyRawMouse::Frame(WindowedModeWrapper::GameHWND, blocked, L"THAWRawMouse", []()
        {
            return std::any_of(TonyCamera::Cameras.begin(), TonyCamera::Cameras.end(),
                [](const auto& entry) { return entry.second.gameplay; }) && !THAWMenu::Lookup("current_menu_anchor");
        });
    }

    inline void Message(HWND window, UINT message, LPARAM lParam)
    {
        TonyRawMouse::Message(window, message, lParam);
    }

    inline float __cdecl CameraMouseAxis(const int32_t* state, int negative, int positive)
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
        std::array<int64_t, 2> delta{};
        if (active)
            delta = TonyRawMouse::TakeMotion();
        const auto [x, y] = delta;
        const bool allowed = active && TonyRawMouse::Gameplay.load(std::memory_order_relaxed) &&
            TonyRawMouse::Enabled.load(std::memory_order_relaxed) && !*animated;
        if (allowed && (x || y))
            THAWController::UsingPad.store(false, std::memory_order_relaxed);
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
        if (LookOwner == look && (!allowed || !TonyRawMouse::Seen.load(std::memory_order_acquire) ||
            THAWController::UsingPad.load(std::memory_order_relaxed) || now - LastLook >= 1500))
        {
            if (!*animated) *locked = 0;
            LookOwner = 0;
        }
        LookUpdate.thiscall<void>(look);
    }

    inline void Install(asi_settings::Plugin& settings)
    {
        TonyRawMouse::InstallSettings(settings);
        auto mouse = hook::pattern("68 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 6A 14 89 0D ? ? ? ? 89 15 ? ? ? ? 8B 08 50 FF 51 24 8B 0D ? ? ? ? 85 C9");
        static const auto* nativeState = *mouse.get_first<int32_t*>(1);
        MouseState = safetyhook::create_mid(mouse.get_first(35), [](SafetyHookContext& regs)
        {
            TonyRawMouse::ReadRelativeState(nativeState, static_cast<HRESULT>(regs.eax));
        });
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
