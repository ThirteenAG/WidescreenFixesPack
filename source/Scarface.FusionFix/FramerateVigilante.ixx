module;
#include "stdafx.h"
#include "FixSettings.hpp"
#include "RTTI.h"
#include "VehicleCamera.hpp"

export module FramerateVigilante;
import ComVars;

namespace
{
    constexpr float referenceFPS = 30.0f;
    SafetyHookInline rageHook;
    injector::hook_back<void(__thiscall*)(void*, const void*)> hbRagePreSim;
    SafetyHookMid mortarHook;
    uintptr_t mortarContinue;
    struct RageFrame
    {
        void* state = nullptr;
        float seconds = 0.0f;
    };
    thread_local RageFrame rageFrame;

    void __fastcall UpdateRagePreSim(void* rage, void*, const void* time)
    {
        // Capture the actual per-object FlowTime through the virtual entry, not
        // through registers or instructions inside the DRM-protected caller.
        const auto previous = rageFrame;
        struct RestoreFrame
        {
            RageFrame previous;
            ~RestoreFrame() { rageFrame = previous; }
        } restore{ previous };
        rageFrame = { rage, *reinterpret_cast<const float*>(static_cast<const uint8_t*>(time) + 0x0C) };
        hbRagePreSim.fun(rage, time);
    }

    void __fastcall UpdateRageBar(void* rage, void*)
    {
        // Calls outside UpdatePreSim have no associated FlowTime.
        if (rageFrame.state != rage)
            return rageHook.unsafe_fastcall(rage, nullptr);
        auto& activeRate = Game::Field<float>(rage, 0x44);
        auto& idleRate = Game::Field<float>(rage, 0x48);
        const auto active = activeRate;
        const auto idle = idleRate;
        if (!std::isfinite(rageFrame.seconds) || rageFrame.seconds < 0.0f)
            return rageHook.unsafe_fastcall(rage, nullptr);
        const auto factor = rageFrame.seconds * referenceFPS;
        activeRate = active * factor;
        idleRate = idle * factor;
        rageHook.unsafe_fastcall(rage, nullptr);
        activeRate = active;
        idleRate = idle;
    }

    void UpdateMortarForce(SafetyHookContext& regs)
    {
        auto object = reinterpret_cast<void*>(regs.esi);
        auto weaponTemplate = reinterpret_cast<void*>(regs.eax);
        auto time = reinterpret_cast<void*>(regs.ebp);
        auto& upForce = Game::Field<float>(object, 0x140);
        upForce = upForce - Game::Field<float>(weaponTemplate, 0x188) *
            Game::Field<float>(time, 0x0C) * referenceFPS;
        // The preceding FLD is removed, so there is no x87 value to pop here.
        // Skip FSUB/FSTP; SafetyHook restores TEST BL,BL's flags for the JNZ.
        return_to(mortarContinue);
    }
}

class FramerateVigilante
{
public:
    FramerateVigilante()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            auto bFixAnimationDelta = settings.Bool("FRAMERATE.FixAnimationDelta");
            auto bFixRageDepletion = settings.Bool("FRAMERATE.FixRageDepletion");
            auto bFixMortarForce = settings.Bool("FRAMERATE.FixMortarForce");

            if (settings.Bool("FRAMERATE.FixVehicleCameraSmoothing"))
                ScarfaceVehicleCamera::Install();

            if (bFixAnimationDelta)
            {
                // Animation update clamps milliseconds to [16,100]. Remove only the floor.
                static float animationMinDelta = 0.0f;
                auto pattern = hook::pattern("68 ? ? ? ? 52 89 4C 24 ? E8 ? ? ? ? ? ? ? ? A0");
                if (pattern.size() == 1)
                    ProtectedGameRef<float*>{ pattern.get_first<float*>(1) } = &animationMinDelta;
                else
                    OutputDebugStringA("Scarface: animation timing pattern unavailable.\n");
            }

            if (bFixRageDepletion)
            {
                const auto rageVtable = ScarfaceRTTI::FindVtable(".?AVRageState@@");
                if (rageVtable)
                {
                    auto pattern = hook::pattern("56 8B F1 8B 46 ? 83 F8 ? 75 ? 8B 0D");
                    if (pattern.size() == 1)
                        rageHook = safetyhook::create_inline(pattern.get_first(), UpdateRageBar);
                    if (rageHook)
                    {
                        // RageState::UpdatePreSim(const FlowTime&) is vtable slot 8.
                        const auto updateSlot = rageVtable + 8 * sizeof(void*);
                        hbRagePreSim.fun = injector::ReadMemory<decltype(hbRagePreSim.fun)>(updateSlot, true);
                        ProtectedGameRef<decltype(&UpdateRagePreSim)>{reinterpret_cast<decltype(&UpdateRagePreSim)*>(updateSlot)} = &UpdateRagePreSim;
                    }
                    else
                        OutputDebugStringA("Scarface: rage timing hook unavailable.\n");
                }
                else
                    OutputDebugStringA("Scarface: rage timing table unavailable.\n");
            }

            if (bFixMortarForce)
            {
                auto pattern = hook::pattern("D8 A0 ? ? ? ? D9 9E");
                if (pattern.size() != 1)
                {
                    OutputDebugStringA("Scarface: mortar timing pattern unavailable.\n");
                    return;
                }
                auto decay = pattern.get_first<uint8_t>();
                mortarContinue = reinterpret_cast<uintptr_t>(decay + 12);
                mortarHook = safetyhook::create_mid(decay, UpdateMortarForce);
                if (mortarHook)
                {
                    // FLD [ESI+140h] precedes ADD ESP,4 / TEST BL,BL. Preserve both
                    // integer instructions and replace the entire x87 operation.
                    injector::MakeNOP(decay - 11, 6);
                }
                else
                    OutputDebugStringA("Scarface: mortar timing hook unavailable.\n");
            }
        };
    }
} FramerateVigilante;
