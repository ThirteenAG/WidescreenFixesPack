module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"

export module HighFPS;

import Timer;

// The parts of the game that count frames instead of time, made to run as at the game's 30 frames per second when the
// frame limiter is set higher (Display Setup's Frame Limiter):
// - Timers that add a frame's whole milliseconds (the time step rounded down each frame) lost the fractions: mission
//   countdowns, the scripts' timers, HUD fades and the upside-down car check ran slow (4 % at 60 fps, 30 % at 700).
//   They read a time step whose milliseconds carry the fraction over to the next frame.
// - Things done every Nth frame (the upside-down car catching fire, the player's money ticking) and the HUD's and
//   rampage counter's flashing count frames of 30 per second.
// - Particles (smoke, sparks, blood, explosions) and their emitters update 30 times per second with the time that
//   passed, so they fade, grow and spawn as at 30 fps.
// - The radio station's name, the clouds' turning, the vehicles' spin damping and the script-moved objects (gates,
//   lifts, doors of missions) are scaled by the time step.
// Above 30 fps only; at 30 fps or less everything is as the game had it.

namespace
{
    constexpr float FrameStep30 = 50.0f / 30.0f;    // the time step of one frame at 30 fps (1.0 = 1/50 s)

    GameRef<uint32_t> FrameCounter;                 // CTimer::m_FrameCounter
    uint32_t LastFrame = UINT32_MAX;

    float WholeMsStep = 0.0f;                       // read as the time step by the millisecond timers
    float MsCarry = 0.0f;
    uint32_t Frames30 = 0;                          // frames of 30 per second
    uint32_t Frames30Gated = UINT32_MAX;            // Frames30 on the frame it changed, else all bits set
    uint32_t Frames30Passed = 0;
    float Carry30 = 0.0f;
    float StepScale = 1.0f;                         // this frame's time step / a 30 fps frame's

    float CloudsTurn = 0.0f, CloudsTurnBase = 0.0f;
    float TurnDamping = 0.99f, TurnDampingBase = 0.99f;

    void UpdateSteps()
    {
        if (*FrameCounter == LastFrame)
            return;
        LastFrame = *FrameCounter;
        float step = CTimer::ms_fTimeStep;
        float ms = step * 20.0f;

        // The timers truncate step * 0.02 * 1000 to whole milliseconds: give them a step that comes out as this
        // frame's whole milliseconds plus the fractions carried over
        MsCarry += ms;
        float whole = std::floor(MsCarry);
        MsCarry -= whole;
        WholeMsStep = (whole + 0.5f) / 20.0f;

        Carry30 += ms;
        Frames30Passed = uint32_t(Carry30 / (1000.0f / 30.0f));
        Carry30 -= Frames30Passed * (1000.0f / 30.0f);
        Frames30 += Frames30Passed;
        Frames30Gated = Frames30Passed ? Frames30 : UINT32_MAX;

        StepScale = step / FrameStep30;
        CloudsTurn = CloudsTurnBase * StepScale;
        TurnDamping = std::pow(TurnDampingBase, StepScale);
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

    // Points every operand in [from, to) that is the address `of` at `to` instead; returns how many
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

    // Millisecond timers: fld ms_fTimeStep; ...; fmul 0.02; ...; fmul 1000.0
    int RedirectMillisecondTimers()
    {
        auto [begin, end] = CodeRange();
        auto timeStep = CTimer::ms_fTimeStep.get_ptr();
        if (!begin || !timeStep)
            return 0;
        int count = 0;
        for (auto p = begin; p + 32 <= end; ++p)
        {
            if (p[0] != 0xD9 || p[1] != 0x05 || *reinterpret_cast<float**>(p + 2) != timeStep)
                continue;
            bool fiftieth = false, thousand = false;
            for (auto q = p + 6; q < p + 32 && !(fiftieth && thousand); ++q)
            {
                if (q[0] != 0xD8 || q[1] != 0x0D)
                    continue;
                auto value = **reinterpret_cast<float**>(q + 2);
                fiftieth |= value == 0.02f;
                thousand |= fiftieth && value == 1000.0f;
            }
            if (fiftieth && thousand)
            {
                injector::WriteMemory(p + 2, &WholeMsStep, true);
                ++count;
            }
        }
        return count;
    }

    // CParticle::Update at 30 per second, with the time that passed
    injector::hook_back<void(__cdecl*)()> hbParticleUpdate;
    float ParticleStep = 0.0f;
    void __cdecl ParticleUpdate()
    {
        float step = CTimer::ms_fTimeStep;
        ParticleStep += step;
        if (step >= FrameStep30 || ParticleStep >= FrameStep30)
        {
            CTimer::ms_fTimeStep = ParticleStep;
            hbParticleUpdate.fun();
            CTimer::ms_fTimeStep = step;
            ParticleStep = 0.0f;
        }
    }
}

class HighFPS
{
public:
    HighFPS()
    {
        WFP::onGameInitEvent() += []()
        {
            // CPlayerInfo::Process: every 16th frame (money, wanted level) and every 32nd (the upside-down car)
            auto every16 = hook::pattern("A1 ? ? ? ? 83 E0 0F 75");
            auto every32 = hook::pattern("A1 ? ? ? ? 83 E0 1F D9 EE 0F 85");
            FrameCounter.SetAddress(*every32.get_first<uint32_t*>(1));
            injector::WriteMemory(every32.get_first(1), &Frames30Gated, true);
            injector::WriteMemory(every16.get_first(1), &Frames30Gated, true);

            RedirectMillisecondTimers();

            // CHud::Draw and CDarkel::DrawMessages: the flashing of the health, armour, wanted level and rampage
            auto hud = hook::pattern("53 56 57 55 81 EC 10 05");
            Redirect(hud.get_first<uint8_t>(), hud.get_first<uint8_t>(0x4A00), FrameCounter.get_ptr(), &Frames30, { { 0xA1 } });
            auto darkel = hook::pattern("53 55 83 EC 28 80 3D ? ? ? ? 01");
            Redirect(darkel.get_first<uint8_t>(), darkel.get_first<uint8_t>(0x665), FrameCounter.get_ptr(), &Frames30, { { 0xA1 } });

            // CGame::Process: CParticle::Update
            auto particles = hook::pattern("E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 83 FB 02");
            hbParticleUpdate.fun = injector::MakeCALL(particles.get_first(), ParticleUpdate, true).get();

            // cMusicManager::DisplayRadioStationName: the name's frames left, one off per frame
            auto radio = hook::pattern("FE 0D ? ? ? ? EB");
            static GameRef<uint8_t> framesLeft(*radio.get_first<uint8_t*>(2));
            static auto RadioNameHook = safetyhook::create_mid(radio.get_first(), [](SafetyHookContext&)
            {
                // The game takes one off right after: take off the 30 fps frames that passed instead
                int left = *framesLeft + 1 - int(Frames30Passed);
                *framesLeft = uint8_t(std::clamp(left, 1, 255));
            });

            // CClouds::Update: the clouds' turn per frame
            auto clouds = hook::pattern("D9 05 ? ? ? ? D8 C9 D8 0D ? ? ? ? A1");
            CloudsTurnBase = CloudsTurn = **clouds.get_first<float*>(2);
            injector::WriteMemory(clouds.get_first(2), &CloudsTurn, true);

            // CPhysical::ApplyAirResistance: the spin's damping per frame
            auto damping = hook::pattern("D8 0D ? ? ? ? D9 5B 7C");
            auto constant = *damping.get_first<float*>(2);
            TurnDampingBase = TurnDamping = *constant;
            Redirect(damping.get_first<uint8_t>(), damping.get_first<uint8_t>(0x30), constant, &TurnDamping, { { 0xD8, 0x0D } });

            // ROTATE_OBJECT and SLIDE_OBJECT of the mission scripts: their step per frame
            auto rotate = hook::pattern("6A 04 50 E8 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 89 C7 D9 EE D9 EE D9 EE D9 47 14");
            auto slide = hook::pattern("6A 08 50 E8 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 89 C6 8D 46 34");
            static float* rotateParams = *rotate.get_first<float*>(9);
            static auto RotateHook = safetyhook::create_mid(rotate.get_first(8), [](SafetyHookContext&)
            {
                rotateParams[2] *= StepScale;
            });
            static float* slideParams = *slide.get_first<float*>(9);
            static auto SlideHook = safetyhook::create_mid(slide.get_first(8), [](SafetyHookContext&)
            {
                slideParams[4] *= StepScale;
                slideParams[5] *= StepScale;
                slideParams[6] *= StepScale;
            });

            WFP::onGameProcessEvent() += []()
            {
                UpdateSteps();
            };
        };
    }
} HighFPS;
