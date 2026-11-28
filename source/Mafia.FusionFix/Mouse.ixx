module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/DirectInputRawMouse.hpp"
#include "rawinput.hpp"
#include <dinput.h>
#include <intrin.h>

export module Mouse;

import ComVars;
import Camera;
import Controller;
import DeveloperOverlay;

// Gameplay input: the modern pad layout, no input while the overlay is open or the game is in the background, mouse
// look without the per-frame limit
namespace MafiaMouse
{
    SafetyHookInline inputHook;
    SafetyHookInline registerHook;
    DIMOUSESTATE* nativeMouse;
    std::array<uintptr_t, 2> resume{};

    BOOL WINAPI RegisterMouse(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        HMODULE caller = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(_ReturnAddress()), &caller);
        if (!caller ||
            (caller != GetModuleHandleW(L"dinput8.dll") && caller != GetModuleHandleW(L"dinput.dll")))
            return registerHook.stdcall<BOOL>(devices, count, size);
        return DirectInputRawMouse::RegisterDevices(devices, count, size, [](PCRAWINPUTDEVICE list, UINT entries, UINT bytes)
        {
            return registerHook.stdcall<BOOL>(list, entries, bytes);
        });
    }

    void ReadMouse(SafetyHookContext&)
    {
        if (!DirectInputRawMouse::Active())
        {
            MenuWheel += nativeMouse->lZ;
            return;
        }
        const auto state = DirectInputRawMouse::Take();
        nativeMouse->lX = state.x;
        nativeMouse->lY = state.y;
        nativeMouse->lZ = state.wheel;
        MenuWheel += state.wheel;
        for (size_t i = 0; i < std::size(nativeMouse->rgbButtons); ++i)
            nativeMouse->rgbButtons[i] = ((state.down | state.pressed) & (1u << i)) ? 0x80 : 0;
    }

    template<size_t Axis> void Uncap(SafetyHookContext& regs)
    {
        if (RawMouseEnabled())
            regs.eip = resume[Axis];
    }

    uint16_t __fastcall Input(uintptr_t object, void*, uint32_t poll)
    {
        const auto result = MafiaPad::Update(object, [&]() { return inputHook.unsafe_thiscall<uint16_t>(object, poll); });
        const auto window = WindowedModeWrapper::GameHWND;
        if (MafiaOverlay::BlocksInput() || (window && GetForegroundWindow() != window))
        {
            const size_t actions = InputBindingsOffset + 16;
            for (size_t offset : { actions, actions + 16 })
            {
                auto begin = Field<uintptr_t>(object, offset);
                auto end = Field<uintptr_t>(object, offset + 4);
                if (!begin || end < begin || end - begin > 256 * sizeof(float) || (end - begin) % sizeof(float)) continue;
                std::fill(reinterpret_cast<float*>(begin), reinterpret_cast<float*>(end), 0.0f);
            }
        }
        CaptureCameraInput(object);
        return result;
    }

    void Install()
    {
        auto engine = GetLS3DF();
        auto reads = hook::module_pattern(engine, "68 ? ? ? ? 6A 10 50 FF 51 24 85 C0 7D ? A1");
        auto* read = reads.get_first<uint8_t>();
        nativeMouse = *reads.get_first<DIMOUSESTATE*>(1);
        // Both successful GetDeviceState paths join before the engine swaps buttons and integrates its cursor.
        static auto pollHook = safetyhook::create_mid(read + 15 + static_cast<int8_t>(read[14]), ReadMouse);
        DirectInputRawMouse::Start();
        registerHook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterMouse);
        MessagePumpGuard::Install(engine);
        MessagePumpGuard::Install(GetModuleHandleW(nullptr));
        auto firstAxis = hook::pattern("D8 1D ? ? ? ? D9 85 A0 00 00 00 DF E0 25 00 41 00 00 74 02 D9 E0 D8 B5 A0 00 00 00");
        if (!firstAxis.empty()) // 1.0 input layout
        {
            firstAxis.get_first();
            InputBindingsOffset = 176;
        }
        else // 1.1/1.2 input layout
        {
            firstAxis = hook::pattern("D8 1D ? ? ? ? D9 85 A4 00 00 00 DF E0 25 00 41 00 00 74 02 D9 E0 D8 B5 A4 00 00 00");
            firstAxis.get_first();
            InputBindingsOffset = 180;
        }
        auto input = hook::pattern("83 EC ? 53 8A 5C 24 ? 55 8B E9 56 57 8B 45 08 85 C0 74 ? 8B 4D 0C 2B C8 B8 EB A0 0E EA");
        inputHook = safetyhook::create_inline(input.get_first(), Input);
        static std::vector<SafetyHookMid> hooks;
        constexpr std::array callbacks{ &Uncap<0>, &Uncap<1> };
        const uint32_t first = static_cast<uint32_t>(InputBindingsOffset - 16);
        for (size_t axis = 0; axis < callbacks.size(); ++axis)
        {
            auto field = pattern_str(to_bytes(first + uint32_t(axis) * 4));
            auto clamp = hook::pattern("D8 1D ? ? ? ? D9 85 " + field +
                "DF E0 25 00 41 00 00 74 02 D9 E0 D8 B5 " + field);
            auto site = clamp.get_first<uint8_t>();
            // DirectInput already supplies accumulated relative mouse counts.
            // Keep native sensitivity, but don't discard fast movement at the
            // per-frame +/-1 normalization limit. The wheel retains its cap.
            resume[axis] = reinterpret_cast<uintptr_t>(site + 23);
            hooks.emplace_back(safetyhook::create_mid(site, callbacks[axis]));
        }
    }
}

class Mouse
{
public:
    Mouse()
    {
        WFP::onInitEvent() += []()
        {
            MafiaMouse::Install();
            InitCamera();
        };
    }
} Mouse;
