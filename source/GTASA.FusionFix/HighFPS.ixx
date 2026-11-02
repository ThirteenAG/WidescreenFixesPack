module;

#include <stdafx.h>

#include "common.h"
#include "FixSettings.hpp"

export module HighFPS;

import Timer;

// The parts of the game that count frames instead of time, made to run as at the game's 30 frames per second when the
// frame limiter is set higher (Display Setup's Frame Limiter):
// - Timers that add a frame's whole milliseconds (the time step rounded down each frame) lost the fractions: mission
//   countdowns, the scripts' timers, HUD fades, stats and many task timers ran slow (4 % at 120 fps, 14 % at 144).
//   They read a time step whose milliseconds carry the fraction over to the next frame.
// - The HUD's flashing (health, armour, breath, radar, wanted stars, mission timers, rampage counter) and the
//   upside-down car check (a flipped car catches fire) count frames of 30 per second.
// - The lightning of storms rolls its dice 30 times per second and measures its bursts in 30 fps frames, so it
//   strikes as often and its thunder follows as late as at 30 fps.
// - The spin damping of vehicles, peds and objects is scaled by the time step.
// - The script-moved objects (gates, doors, lifts of missions) move their step per 30 fps frame.
// Above 30 fps only; at 30 fps or less everything is as the game had it.

namespace
{
    constexpr float FrameStep30 = 50.0f / 30.0f;    // the time step of one frame at 30 fps (1.0 = 1/50 s)
    constexpr float FrameMs30 = 1000.0f / 30.0f;

    GameRef<uint32_t> FrameCounter;                 // CTimer::m_FrameCounter
    GameRef<float> TimeStepNonClipped;              // CTimer::ms_fTimeStepNonClipped

    bool HighRate = false;                          // the average frame rate is above 30 fps
    float AverageStep = FrameStep30;
    double GameMs = 0.0;                            // game time with the fractions of milliseconds

    float WholeMsStep = 0.0f;                       // read as the time steps by the millisecond timers
    float WholeMsStepNonClipped = 0.0f;
    float MsCarry = 0.0f, MsCarryNonClipped = 0.0f;
    uint32_t Frames30 = 0;                          // frames of 30 per second
    uint32_t Frames30Gated = 0;                     // Frames30 on the frame it changed, else all bits set
    uint32_t Frames30Passed = 0;
    float Carry30 = 0.0f;

    float TurnDamping = 0.99f, TurnDampingBase = 0.99f;

    // Whole milliseconds of this frame plus the fractions carried over, as a time step that the timers' truncation
    // of step * 0.02 * 1000 turns into exactly them
    float WholeMs(float step, float& carry)
    {
        carry += step * 20.0f;
        float whole = std::floor(carry);
        carry -= whole;
        return (whole + 0.5f) / 20.0f;
    }

    void UpdateSteps()
    {
        if (!FrameCounter.is_initialized() || !TimeStepNonClipped.is_initialized())
            return;
        float step = CTimer::ms_fTimeStep;
        float stepNonClipped = *TimeStepNonClipped;
        GameMs += step * 20.0;

        // Decide on the average, so that the frame times' jitter at a 30 fps limit doesn't switch every frame
        if (step > 0.011f)
        {
            AverageStep += (step - AverageStep) * 0.1f;
            bool high = HighRate ? AverageStep < 50.0f / 31.0f : AverageStep < 50.0f / 33.0f;
            if (high != HighRate)
            {
                HighRate = high;
                MsCarry = MsCarryNonClipped = Carry30 = 0.0f;
                Frames30 = *FrameCounter;
            }
        }

        if (!HighRate)
        {
            WholeMsStep = step;
            WholeMsStepNonClipped = stepNonClipped;
            Frames30 = Frames30Gated = *FrameCounter;
            Frames30Passed = 1;
            TurnDamping = TurnDampingBase;
            return;
        }

        WholeMsStep = WholeMs(step, MsCarry);
        WholeMsStepNonClipped = WholeMs(stepNonClipped, MsCarryNonClipped);

        Carry30 += step * 20.0f;
        Frames30Passed = uint32_t(Carry30 / FrameMs30);
        Carry30 -= Frames30Passed * FrameMs30;
        Frames30 += Frames30Passed;
        Frames30Gated = Frames30Passed ? Frames30 : UINT32_MAX;

        TurnDamping = std::pow(TurnDampingBase, step / FrameStep30);
    }

    // CTimer::Update, then this frame's steps
    SafetyHookInline shTimerUpdate;
    void __cdecl TimerUpdate()
    {
        shTimerUpdate.unsafe_ccall();
        UpdateSteps();
    }

    // The code section of the game
    std::pair<uint8_t*, uint8_t*> CodeRange()
    {
        auto module = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(module + reinterpret_cast<IMAGE_DOS_HEADER*>(module)->e_lfanew);
        auto section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
        {
            if (section->Characteristics & IMAGE_SCN_CNT_CODE)
                return { module + section->VirtualAddress, module + section->VirtualAddress + section->Misc.VirtualSize };
        }
        return { nullptr, nullptr };
    }

    // Points every operand in [from, to) that is the address `of` at `replacement` instead; returns how many
    int Redirect(uint8_t* from, uint8_t* to, const void* of, const void* replacement, const std::initializer_list<std::initializer_list<uint8_t>>& opcodes)
    {
        int count = 0;
        for (auto p = from; p + 6 <= to; ++p)
        {
            for (auto& opcode : opcodes)
            {
                if (std::memcmp(p, opcode.begin(), opcode.size()) != 0)
                    continue;
                auto operand = p + opcode.size();
                if (*reinterpret_cast<const void**>(operand) == of)
                {
                    injector::WriteMemory(operand, replacement, true);
                    ++count;
                }
            }
        }
        return count;
    }

    // Millisecond timers: fld step; ...; fmul 0.02; ...; fmul (-)1000.0; ...; call __ftol2. Sites that also use the
    // step otherwise (a float timer, a compare before) are left alone, as is any site whose conversion isn't the
    // stock one (SilentPatch carries the fractions of the scripts' timers itself).
    int RedirectMillisecondTimers(const float* timeStep, float* replacement, uintptr_t ftol)
    {
        auto [begin, end] = CodeRange();
        if (!begin || !timeStep || !ftol)
            return 0;
        auto image = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        auto imageEnd = image + reinterpret_cast<IMAGE_NT_HEADERS*>(image + reinterpret_cast<IMAGE_DOS_HEADER*>(image)->e_lfanew)->OptionalHeader.SizeOfImage;
        int count = 0;
        for (auto p = begin; p + 48 <= end; ++p)
        {
            if (p[0] != 0xD9 || p[1] != 0x05 || *reinterpret_cast<const float* const*>(p + 2) != timeStep)
                continue;
            bool fiftieth = false, thousand = false, other = false;
            uint8_t* call = nullptr;
            for (auto q = p + 6; q < p + 40; ++q)
            {
                if (q[0] == 0xD8 && q[1] == 0x0D)
                {
                    auto constant = *reinterpret_cast<uint8_t**>(q + 2);
                    if (constant >= image && constant + sizeof(float) <= imageEnd)
                    {
                        auto value = *reinterpret_cast<float*>(constant);
                        if (!fiftieth && value == 0.02f)
                        {
                            fiftieth = true;
                            q += 5;
                            continue;
                        }
                        if (fiftieth && !thousand && (value == 1000.0f || value == -1000.0f))
                        {
                            thousand = true;
                            q += 5;
                            continue;
                        }
                    }
                }
                // fnstsw ax, fadd m32, fst m32, fstp m32
                if ((q[0] == 0xDF && q[1] == 0xE0) || (q[0] == 0xD8 && q[1] == 0x05) || (q[0] == 0xD9 && (q[1] == 0x15 || q[1] == 0x1D)))
                    other = true;
                if (thousand && q[0] == 0xE8)
                {
                    call = q;
                    break;
                }
            }
            if (!call || other || injector::GetBranchDestination(call).as_int() != ftol)
                continue;
            injector::WriteMemory(p + 2, replacement, true);
            ++count;
        }
        return count;
    }

    // The scripts' ROTATE_OBJECT and SLIDE_OBJECT move their step each time they run, once a frame in a loop that
    // waits 0: scale it by the time since the same script last moved the same object, up to a 30 fps frame
    struct ScriptMoves
    {
        std::unordered_map<uint64_t, double> last;

        float Scale(uintptr_t script, int32_t handle)
        {
            if (!HighRate)
            {
                last.clear();
                return 1.0f;
            }
            if (last.size() > 256)
                std::erase_if(last, [](const auto& entry) { return GameMs - entry.second > 1000.0; });
            auto key = (uint64_t(script) << 32) | uint32_t(handle);
            auto it = last.find(key);
            float scale = 1.0f;
            if (it != last.end())
                scale = float(std::clamp((GameMs - it->second) / FrameMs30, 0.0, 1.0));
            last[key] = GameMs;
            return scale;
        }
    };
    ScriptMoves RotateMoves, SlideMoves;

    // The lightning's dice: no roll between the 30 fps frames (the high values neither start nor end a burst)
    int(__cdecl* GameRand)() = nullptr;
    int __cdecl LightningRand()
    {
        return Frames30Passed ? GameRand() : 0x7FFF;
    }
}

class HighFPS
{
public:
    HighFPS()
    {
        WFP::onGameInitEvent() += []()
        {
            // CPlayerInfo::Process: every 32nd frame, the upside-down car check (on the 7th the car catches fire)
            auto upsideDown = hook::pattern("F6 05 ? ? ? ? 1F 0F 85 ? ? ? ? 0F B6 3D");
            // CTimer::Update
            auto timerUpdate = hook::pattern("8B 0D ? ? ? ? 83 EC 0C 85 C9 0F");
            // The steps in milliseconds: fld; fmul 0.02; fmul 1000.0; jmp __ftol2
            auto msGetters = hook::pattern("D9 05 ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? E9").count(2);
            FrameCounter.SetAddress(*upsideDown.get_first<uint32_t*>(2));
            Frames30 = Frames30Gated = *FrameCounter;
            uintptr_t ftol = 0;
            for (size_t i = 0; i < msGetters.size(); ++i)
            {
                auto variable = *msGetters.get(i).get<float*>(2);
                if (variable != CTimer::ms_fTimeStep.get_ptr())
                    TimeStepNonClipped.SetAddress(variable);
                ftol = injector::GetBranchDestination(msGetters.get(i).get<void>(18)).as_int();
            }
            WholeMsStep = CTimer::ms_fTimeStep;
            WholeMsStepNonClipped = *TimeStepNonClipped;
            shTimerUpdate = safetyhook::create_inline(timerUpdate.get_first(), TimerUpdate);

            injector::WriteMemory(upsideDown.get_first(2), &Frames30Gated, true);

            RedirectMillisecondTimers(CTimer::ms_fTimeStep.get_ptr(), &WholeMsStep, ftol);
            RedirectMillisecondTimers(TimeStepNonClipped.get_ptr(), &WholeMsStepNonClipped, ftol);

            // CHud::RenderArmorBar up to CHud::DrawWanted: the flashing of the armour, breath, health, radar,
            // mission timers and wanted level
            auto hud = hook::pattern("83 EC 0C 66 83 3D ? ? ? ? 03");
            Redirect(hud.get_first<uint8_t>(), hud.get_first<uint8_t>(0x4E00), FrameCounter.get_ptr(), &Frames30, { { 0xF6, 0x05 }, { 0x8A, 0x1D }, { 0xA0 } });

            // CDarkel::DrawMessages: the rampage counter's flashing
            auto darkel = hook::pattern("F6 05 ? ? ? ? 01 74 2E");
            injector::WriteMemory(darkel.get_first(2), &Frames30, true);

            // CWeather::Update: the lightning's burst end and start, and its length in frames
            auto lightningEnd = hook::pattern("E8 ? ? ? ? 25 FF 00 00 00 83 F8 18 7D ? A1 ? ? ? ? 2B 05");
            auto lightningStart = hook::pattern("E8 ? ? ? ? 8B 35 ? ? ? ? 66 8B 15 ? ? ? ? 25 FF FF 00 00 3D C8 00 00 00 7D ? A1");
            {
                GameRand = reinterpret_cast<int(__cdecl*)()>(injector::GetBranchDestination(lightningEnd.get_first()).as_int());
                injector::MakeCALL(lightningEnd.get_first(), LightningRand, true);
                injector::MakeCALL(lightningStart.get_first(), LightningRand, true);
                injector::WriteMemory(lightningEnd.get_first(16), &Frames30, true);
                injector::WriteMemory(lightningStart.get_first(31), &Frames30, true);
            }

            // CPhysical::ApplyAirResistance: the spin's damping per frame
            auto damping = hook::pattern("D8 4E 4C D9 5E 4C D9 46 50 D8 0D ? ? ? ? D9 5E 50 D9 46 54 D8 0D ? ? ? ? D9 5E 54 D9 46 58 D8 0D ? ? ? ? D9 5E 58");
            {
                auto constant = *damping.get_first<float*>(11);
                TurnDampingBase = TurnDamping = *constant;
                Redirect(damping.get_first<uint8_t>(), damping.get_first<uint8_t>(0x2A), constant, &TurnDamping, { { 0xD8, 0x0D } });
            }

            // ROTATE_OBJECT (step: the 3rd parameter) and SLIDE_OBJECT (steps: the 5th to 7th), right after their
            // parameters are read; the script is in esi
            auto rotate = hook::pattern("6A 04 8B CE E8 ? ? ? ? 8B 0D ? ? ? ? 51 8B 0D ? ? ? ? E8 ? ? ? ? 8B F8 8B CF E8 ? ? ? ? D8 0D");
            static auto rotateParams = *rotate.get_first<float*>(11);
            static auto RotateHook = safetyhook::create_mid(rotate.get_first(9), [](SafetyHookContext& regs)
            {
                rotateParams[2] *= RotateMoves.Scale(regs.esi, *reinterpret_cast<int32_t*>(&rotateParams[0]));
            });

            auto slide = hook::pattern("6A 08 8B CE E8 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B F8 8B 47 14 85 C0 74 05 83 C0 30");
            static auto slideParams = *slide.get_first<float*>(10);
            static auto SlideHook = safetyhook::create_mid(slide.get_first(9), [](SafetyHookContext& regs)
            {
                float scale = SlideMoves.Scale(regs.esi, *reinterpret_cast<int32_t*>(&slideParams[0]));
                slideParams[4] *= scale;
                slideParams[5] *= scale;
                slideParams[6] *= scale;
            });
        };
    }
} HighFPS;
