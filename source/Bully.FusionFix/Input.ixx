module;

#include <stdafx.h>
#include "Shared/PadInput.hpp"

export module Input;

import Camera;
import Cutscene;
import Overlay;
import ComVars;

export namespace BullyInput
{
    SafetyHookInline PollHook, LookXHook, LookYHook;
    SafetyHookMid StateHook, RumbleHook;
    const int* AutoGamepad = nullptr;
    const float* CameraSpeed = nullptr;
    const float* Deadzone = nullptr;
    DWORD(WINAPI* GetState)(DWORD, XINPUT_STATE*) = nullptr;
    int(__thiscall* Initialize)(void*, void*, bool) = nullptr;
    void(__thiscall* StopVibration)(void*) = nullptr;
    std::array<bool, 4> Connected{};
    std::array<bool, 4> PolledConnected{};
    std::array<ULONGLONG, 4> Activity{};
    std::array<XINPUT_STATE, 4> States{};
    ULONGLONG NextProbe = 0;
    float DeadzoneSquared = 0.0225f;
    double DeadzoneRadius = 0.15, DeadzoneRange = 0.85;

    struct Operand
    {
        uintptr_t* address;
        uintptr_t original;
    };
    std::vector<Operand> DeadzoneOperands;

    int Slot(uintptr_t context)
    {
        if (!InputContexts.is_initialized())
            return -1;
        const auto base = reinterpret_cast<uintptr_t>(InputContexts.get().data());
        if (context < base || context >= base + 4 * 2324 || (context - base) % 2324)
            return -1;
        return int((context - base) / 2324);
    }

    bool Focused()
    {
        return hGameWindow.is_initialized() && hGameWindow && GetForegroundWindow() == hGameWindow;
    }

    void Clear(uint8_t* context, bool previous)
    {
        for (int i = 0; i < 20; ++i)
            reinterpret_cast<float*>(context + 324)[i] = 0.0f;
        for (int i = 0; i < 8; ++i)
            reinterpret_cast<uint32_t*>(context + 468)[i] = 0;
        *reinterpret_cast<uint16_t*>(context + 2236) = 0;
        context[2238] = 0;
        if (previous)
        {
            for (int i = 0; i < 20; ++i)
                reinterpret_cast<float*>(context + 1272)[i] = 0.0f;
            for (int i = 0; i < 8; ++i)
                reinterpret_cast<uint32_t*>(context + 1416)[i] = 0;
            *reinterpret_cast<uint32_t*>(context + 2220) = 0;
            *reinterpret_cast<uint32_t*>(context + 2224) = 0;
        }
    }

    bool HardwareActivity(const XINPUT_STATE& state)
    {
        const auto& pad = state.Gamepad;
        return pad.wButtons || pad.bLeftTrigger >= 30 || pad.bRightTrigger >= 30 ||
            std::abs(int(pad.sThumbLX)) > 10000 || std::abs(int(pad.sThumbLY)) > 10000 ||
            std::abs(int(pad.sThumbRX)) > 10000 || std::abs(int(pad.sThumbRY)) > 10000;
    }

    void RebuildClaims()
    {
        for (int i = 0; i < 4; ++i)
            InputClaims.get()[i] = 0;
        for (int i = 1; i < 4; ++i)
        {
            auto context = InputContexts.get().data() + i * 2324;
            const auto index = *reinterpret_cast<uint32_t*>(context + 316);
            if (*context && index < 4)
                InputClaims.get()[index] = 1;
        }
    }

    void Assign(int slot, DWORD index)
    {
        auto context = InputContexts.get().data() + slot * 2324;
        if (*context && *reinterpret_cast<uint16_t*>(context + 2236) && StopVibration)
            StopVibration(context);
        Clear(context, true);
        for (size_t i = 0; i < sizeof(XINPUT_CAPABILITIES) / 4; ++i)
            reinterpret_cast<uint32_t*>(context + 2304)[i] = 0;
        for (DWORD i = 0; i < 4; ++i)
            InputClaims.get()[i] = i != index;
        *reinterpret_cast<int*>(context + 312) = slot;
        Initialize(context, nullptr, true);
        RebuildClaims();
        PolledConnected[slot] = true;
    }

    void Refresh()
    {
        if (!GetState || !Initialize || !AutoGamepad || !*AutoGamepad || !Focused())
            return;
        const auto now = GetTickCount64();
        if (now < NextProbe)
            return;
        NextProbe = now + 500;
        for (DWORD i = 0; i < 4; ++i)
        {
            States[i] = {};
            Connected[i] = GetState(i, &States[i]) == ERROR_SUCCESS;
        }
        std::array<bool, 4> assigned{};
        for (int i = 1; i < 4; ++i)
        {
            auto context = InputContexts.get().data() + i * 2324;
            const auto index = *reinterpret_cast<uint32_t*>(context + 316);
            if (*context && index < 4 && Connected[index])
                assigned[index] = true;
        }
        for (DWORD index = 0; index < 4; ++index)
        {
            if (!Connected[index] || assigned[index])
                continue;
            int slot = 0;
            for (int i = 1; i < 4; ++i)
            {
                auto context = InputContexts.get().data() + i * 2324;
                const auto old = *reinterpret_cast<uint32_t*>(context + 316);
                if (!*context || old >= 4 || !Connected[old])
                {
                    slot = i;
                    break;
                }
            }
            if (!slot && HardwareActivity(States[index]))
            {
                slot = 1;
                for (int i = 2; i < 4; ++i)
                {
                    if (Activity[i] < Activity[slot])
                        slot = i;
                }
            }
            if (slot)
            {
                Assign(slot, index);
                assigned[index] = true;
            }
        }
    }

    bool Changed(uint8_t* context)
    {
        if (*context)
        {
            if (*reinterpret_cast<uint16_t*>(context + 488) != *reinterpret_cast<uint16_t*>(context + 1436))
                return true;
            for (int i = 0; i < 4; ++i)
            {
                const float value = *reinterpret_cast<float*>(context + 388 + i * 4);
                const float old = *reinterpret_cast<float*>(context + 1336 + i * 4);
                if (std::isfinite(value) && std::abs(value) > 0.1f && std::abs(value - old) > 0.01f)
                    return true;
            }
            return false;
        }
        return std::memcmp(context + 516, context + 1464, 256) != 0 ||
            std::memcmp(context + 512, context + 1460, 4) != 0 ||
            *reinterpret_cast<int*>(context + 500) || *reinterpret_cast<int*>(context + 504) || *reinterpret_cast<int*>(context + 508);
    }

    bool UsingPad()
    {
        return InputContexts.is_initialized() && PrimaryController.is_initialized() && PrimaryController > 0 && PrimaryController < 4 && InputContexts.get()[PrimaryController * 2324] != 0;
    }

    bool HelpPressed()
    {
        if (!UsingPad())
            return false;
        const auto context = InputContexts.get().data() + PrimaryController * 2324;
        return (*reinterpret_cast<uint16_t*>(context + 488) & XINPUT_GAMEPAD_Y) &&
            !(*reinterpret_cast<uint16_t*>(context + 1436) & XINPUT_GAMEPAD_Y);
    }

    char __fastcall Poll(uint8_t* context, void*)
    {
        const int slot = Slot(reinterpret_cast<uintptr_t>(context));
        if (slot >= 0 && hGameWindow.is_initialized() && hGameWindow && (!Focused() || BullyOverlay::BlocksInput()))
        {
            Clear(context, true);
            if (!*context)
            {
                for (int i = 0; i < 68; ++i)
                {
                    reinterpret_cast<uint32_t*>(context + 500)[i] = 0;
                    reinterpret_cast<uint32_t*>(context + 1448)[i] = 0;
                }
            }
            return 0;
        }
        if (slot == 0)
            Refresh();
        const float deadzone = Deadzone && std::isfinite(*Deadzone) ? std::clamp(*Deadzone, 0.0f, 0.5f) : 0.15f;
        DeadzoneSquared = deadzone * deadzone;
        DeadzoneRadius = deadzone;
        DeadzoneRange = 1.0 - DeadzoneRadius;
        const bool automatic = AutoGamepad && *AutoGamepad;
        const bool pad = slot > 0 && *context != 0;
        const auto index = pad ? *reinterpret_cast<uint32_t*>(context + 316) : 4;
        const bool wasConnected = slot >= 0 && PolledConnected[slot];
        if (automatic && GetState && pad && !wasConnected && (index >= 4 || !Connected[index]))
        {
            Clear(context, true);
            return 0;
        }
        char result = PollHook.thiscall<char>(context);
        if (slot >= 0 && pad && wasConnected && !PolledConnected[slot])
        {
            if (PrimaryController.is_initialized() && PrimaryController == slot)
                PrimaryController = 0;
            result = 1;
        }
        if (slot >= 0 && automatic && result && Focused() && (!pad || PolledConnected[slot]) && Changed(context))
        {
            Activity[slot] = GetTickCount64();
            PrimaryController = slot;
        }
        return result;
    }

    void State(SafetyHookContext& regs)
    {
        const int slot = Slot(regs.ebp);
        if (slot < 0)
            return;
        auto context = reinterpret_cast<uint8_t*>(regs.ebp);
        PolledConnected[slot] = regs.eax == ERROR_SUCCESS;
        if (regs.eax != ERROR_SUCCESS)
        {
            const auto index = *reinterpret_cast<uint32_t*>(context + 316);
            if (index < 4)
                Connected[index] = false;
            Clear(context, false);
        }
    }

    void LimitRumble(SafetyHookContext& regs)
    {
        regs.ebp = std::min<uint32_t>(regs.ebp, 5);
        if (regs.eax == ERROR_DEVICE_NOT_CONNECTED)
            regs.edi = regs.ebp;
    }

    double Speed(bool vertical)
    {
        if (!PrimaryController.is_initialized() || !InputContexts.is_initialized() || PrimaryController < 0 || PrimaryController >= 4 || InputContexts.get()[PrimaryController * 2324] == 0)
            return 1.0;
        const double speed = CameraSpeed && std::isfinite(*CameraSpeed) ? std::clamp(*CameraSpeed, 0.1f, 5.0f) : 1.0f;
        const float native = CutsceneTimeStep.is_initialized() ? CutsceneTimeStep.get() : 1.0f;
        const float actual = BullyCutscene::HaveStep ? BullyCutscene::UnclampedStep : native;
        if (!std::isfinite(native) || !std::isfinite(actual) || native <= 0.0f || actual <= 0.0f)
            return 0.0;
        const double rate = actual / native;
        return speed * rate * (vertical ? (50.0 / 30.0) / native : 1.0);
    }

    double __fastcall LookX(void* camera, void*)
    {
        return LookXHook.thiscall<double>(camera) * Speed(false);
    }

    double __fastcall LookY(void* camera, void*)
    {
        return LookYHook.thiscall<double>(camera) * Speed(true);
    }

    void ResetHooks()
    {
        PollHook.reset();
        StateHook.reset();
        RumbleHook.reset();
        LookXHook.reset();
        LookYHook.reset();
        for (const auto& operand : DeadzoneOperands)
            injector::WriteMemory<uintptr_t>(operand.address, operand.original, true);
        DeadzoneOperands.clear();
    }

    void Install(const int& automatic, const float& speed, const float& deadzone, HMODULE module = GetModuleHandle(nullptr), bool resolveApi = true)
    {
        auto poll = hook::module_pattern(module, "83 EC 24 53 55 8B E9 56 33 DB 38 5D 00 57 8D B5 44 01 00 00 8D BD F8 04 00 00 B9 ED 00 00 00");
        auto end = hook::module_pattern(module, "5F 5E C6 85 B4 08 00 00 00 5D 8A C3 5B 83 C4 24 C3");
        auto initialize = hook::module_pattern(module, "83 EC 5C 8A 44 24 64 84 C0 53 8B D9 56 89 5C 24 08 88 03 74 67 33 F6 80 BE ? ? ? ? 00 75 0F 8D 44 24 0C 50 56 E8");
        auto contexts = hook::module_pattern(module, "8B 44 24 04 69 C0 14 09 00 00 8A 80 ? ? ? ? C3");
        auto primary = hook::module_pattern(module, "A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 56 57 33 FF BE");
        auto state = hook::module_pattern(module, "8B 8D 3C 01 00 00 8D 85 E4 01 00 00 50 51 E8 ? ? ? ? B1 1E 38 8D EA 01 00 00");
        auto rumble = hook::module_pattern(module, "3B FD 0F 8D ? ? ? ? 8B 8E 3C 01 00 00 8D 44 24 60 50 51 83 C7 01 E8 ? ? ? ? 85 C0 75");
        auto window = hook::module_pattern(module, "8B 15 ? ? ? ? 8B 08 52 50 8B 41 28 FF D0");
        auto lookX = hook::module_pattern(module, "51 56 8B F1 B9 ? ? ? ? E8 ? ? ? ? 84 C0 75 05 D9 EE 5E 59 C3 80 3D ? ? ? ? 00 74 09 80 3D ? ? ? ? 00 74 E9 D9 EE 8B 86 08 01 00 00 83 F8 03 D9 5C 24 04 75 1F 6A 00 6A 10");
        auto lookY = hook::module_pattern(module, "51 56 8B F1 B9 ? ? ? ? E8 ? ? ? ? 84 C0 75 05 D9 EE 5E 59 C3 80 3D ? ? ? ? 00 74 09 80 3D ? ? ? ? 00 74 E9 D9 EE 8B 86 08 01 00 00 83 F8 03 D9 5C 24 04 75 1F 6A 00 6A 11");
        const auto begin = reinterpret_cast<uintptr_t>(poll.get_first());
        const auto finish = reinterpret_cast<uintptr_t>(end.get_first(17));
        std::array<uint8_t*, 2> radial = {
            hook::range_pattern(begin, finish, "DD 05 ? ? ? ? D8 E9 D9 C9 DC 0D ? ? ? ? DE F9 D9 5C 24 18 D9 44 24 18 D9 C0 D8 4C 24 1C").get_first<uint8_t>(),
            hook::range_pattern(begin, finish, "DD 05 ? ? ? ? D8 E9 D9 C9 DC 0D ? ? ? ? DE F9 D9 5C 24 18 D9 44 24 18 D9 C0 D8 4C 24 24").get_first<uint8_t>()
        };
        std::array<uint8_t*, 2> squaredLoads = {
            hook::range_pattern(begin, finish, "D9 05 ? ? ? ? D8 D9 DF E0 F6 C4 41 75 18").get_first<uint8_t>(),
            hook::range_pattern(begin, finish, "D9 05 ? ? ? ? D8 DB").get_first<uint8_t>()
        };
        AutoGamepad = &automatic;
        CameraSpeed = &speed;
        Deadzone = &deadzone;
        InputContexts.SetAddress(*contexts.get_first<uint8_t*>(12));
        InputClaims.SetAddress(*initialize.get_first<uint8_t*>(25));
        PrimaryController.SetAddress(*primary.get_first<int*>(1));
        (void)*window.get_first<HWND*>(2);
        Initialize = reinterpret_cast<decltype(Initialize)>(initialize.get_first());
        StopVibration = reinterpret_cast<decltype(StopVibration)>(injector::GetBranchDestination(end.get_first(-5)).as_int());
        if (resolveApi)
        {
            GetState = PadInput::XInput().getState;
            if (!GetState)
                OutputDebugStringA("Bully: controller hotplug API unavailable.\n");
        }
        PollHook = safetyhook::create_inline(poll.get_first(), Poll);
        StateHook = safetyhook::create_mid(state.get_first(19), State);
        RumbleHook = safetyhook::create_mid(rumble.get_first(), LimitRumble);
        LookXHook = safetyhook::create_inline(lookX.get_first(), LookX);
        LookYHook = safetyhook::create_inline(lookY.get_first(), LookY);
        auto replace = [&](uintptr_t* address, uintptr_t value)
        {
            DeadzoneOperands.push_back({ address, *address });
            injector::WriteMemory<uintptr_t>(address, value, true);
        };
        for (size_t i = 0; i < 2; ++i)
        {
            replace(reinterpret_cast<uintptr_t*>(squaredLoads[i] + 2), reinterpret_cast<uintptr_t>(&DeadzoneSquared));
            replace(reinterpret_cast<uintptr_t*>(radial[i] + 2), reinterpret_cast<uintptr_t>(&DeadzoneRadius));
            replace(reinterpret_cast<uintptr_t*>(radial[i] + 12), reinterpret_cast<uintptr_t>(&DeadzoneRange));
        }
    }
}
