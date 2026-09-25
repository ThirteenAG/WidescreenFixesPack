module;
#include "stdafx.h"
#include "RTTI.h"

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
        const auto factor = rageFrame.seconds * referenceFPS;
        activeRate *= factor;
        idleRate *= factor;
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
        upForce -= Game::Field<float>(weaponTemplate, 0x188) *
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
            CIniReader iniReader("");
            auto bFixAnimationDelta = iniReader.ReadInteger("FRAMERATE", "FixAnimationDelta", 1) != 0;
            auto bFixRageDepletion = iniReader.ReadInteger("FRAMERATE", "FixRageDepletion", 1) != 0;
            auto bFixMortarForce = iniReader.ReadInteger("FRAMERATE", "FixMortarForce", 1) != 0;

            if (bFixAnimationDelta)
            {
                // Animation update clamps milliseconds to [16,100]. Remove only the floor.
                static float animationMinDelta = 0.0f;
                auto pattern = hook::pattern("68 ? ? ? ? 52 89 4C 24 ? E8 ? ? ? ? ? ? ? ? A0");
                injector::WriteMemory(pattern.get_first(1), &animationMinDelta, true);
            }

            if (bFixRageDepletion)
            {
                const auto rageVtable = ScarfaceRTTI::FindVtable(".?AVRageState@@");
                if (rageVtable)
                {
                    auto pattern = hook::pattern("56 8B F1 8B 46 ? 83 F8 ? 75 ? 8B 0D");
                    rageHook = safetyhook::create_inline(pattern.get_first(), UpdateRageBar);
                    if (rageHook)
                    {
                        // RageState::UpdatePreSim(const FlowTime&) is vtable slot 8.
                        const auto updateSlot = rageVtable + 8 * sizeof(void*);
                        hbRagePreSim.fun = injector::ReadMemory<decltype(hbRagePreSim.fun)>(updateSlot, true);
                        injector::WriteMemory(updateSlot, &UpdateRagePreSim, true);
                    }
                }
            }

            if (bFixMortarForce)
            {
                auto pattern = hook::pattern("D8 A0 ? ? ? ? D9 9E");
                auto decay = pattern.get_first<uint8_t>();
                mortarContinue = reinterpret_cast<uintptr_t>(decay + 12);
                mortarHook = safetyhook::create_mid(decay, UpdateMortarForce);
                if (mortarHook)
                {
                    // FLD [ESI+140h] precedes ADD ESP,4 / TEST BL,BL. Preserve both
                    // integer instructions and replace the entire x87 operation.
                    injector::MakeNOP(decay - 11, 6);
                }
            }
        };
    }
} FramerateVigilante;
