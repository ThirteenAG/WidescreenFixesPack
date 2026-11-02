module;

#include <stdafx.h>

export module GameSpeedFix;

import ComVars;

LARGE_INTEGER g_qpcStart;
double        g_qpcFreqInv;
bool          g_qpcInitialized = false;

void init_qpc_timing()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    g_qpcFreqInv = 1.0 / (double)freq.QuadPart;
    QueryPerformanceCounter(&g_qpcStart);
    g_qpcInitialized = true;
}

SafetyHookInline shTIM_f_Clock_TrueRead = {};
float TIM_f_Clock_TrueRead()
{
    if (!g_qpcInitialized) init_qpc_timing();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (float)((double)(now.QuadPart - g_qpcStart.QuadPart) * g_qpcFreqInv);
}

SafetyHookInline shTIM_UpdateCPUClockFrequency = {};
void TIM_UpdateCPUClockFrequency()
{
    //return shTIM_UpdateCPUClockFrequency.unsafe_call();
}

SafetyHookInline shTIM_Clock_Reset = {};
void TIM_Clock_Reset()
{
    shTIM_Clock_Reset.unsafe_call();
    QueryPerformanceCounter(&g_qpcStart);
}

// Physics (ODE): the game stepped its world once per frame with the whole frame time, while the contact correction
// and the "fall asleep after 10 slow steps" rule are tuned for 1/60 s steps. At high frame rates contacts popped
// and props froze early. The world now advances in fixed 1/60 s steps, the remainder carried to the next frame.
using QuickStepFn = void(__cdecl*)(void* world, float step);
QuickStepFn QuickStep = nullptr;
constexpr double PhysicsStep = 1.0 / 60.0;
constexpr int MaxPhysicsSteps = 8;

void __cdecl FixedQuickStep(void* world, float frameTime)
{
    static void* lastWorld = nullptr;
    static double remainder = 0.0;
    if (world != lastWorld)
    {
        lastWorld = world;
        remainder = 0.0;
    }
    if (!std::isfinite(frameTime) || frameTime <= 0.0f)
        return;
    remainder = std::min(remainder + frameTime, PhysicsStep * MaxPhysicsSteps);
    while (remainder >= PhysicsStep)
    {
        QuickStep(world, static_cast<float>(PhysicsStep));
        remainder -= PhysicsStep;
    }
}

void InitPhysicsStep()
{
    // Each edition calls dWorldQuickStep immediately after setting its iteration count.
    uint8_t* step = nullptr;
    auto pattern = hook::pattern("6A 0A 52 89 0D ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 8E ? ? ? ? 50 51 E8");
    if (!Screen.bSignatureEdition) // Gamer's Edition
        step = pattern.get_first<uint8_t>(27);
    else // Signature Edition
        step = hook::pattern("6A 0A 50 89 15 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 8B 96 ? ? ? ? 51 52 E8").get_first<uint8_t>(28);
    QuickStep = reinterpret_cast<QuickStepFn>(reinterpret_cast<uintptr_t>(step) + 5 + *reinterpret_cast<int32_t*>(step + 1));
    injector::MakeCALL(step, FixedQuickStep, true);
}

// After a world reload the game waits 30 frames (half a second at 60 fps) before its scheduled pause; it now counts
// 1/60 s ticks of real time, so the wait is the same at any frame rate
void InitReloadPauseDelay()
{
    if (Screen.bSignatureEdition)
        return;
    auto pattern = hook::pattern("83 3D ? ? ? ? 00 7E ? 8B 15 ? ? ? ? 83 EA 01 89 15 ? ? ? ? 83 3D ? ? ? ? 00 74 ? 83 3D");
    nReloadPauseCounter.SetAddress(*pattern.get_first<int32_t*>(11)); //0xB0DA28
    static auto Countdown = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext&)
    {
        static LARGE_INTEGER last{};
        static double ticks = 0.0;
        LARGE_INTEGER now, frequency;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&frequency);
        const double elapsed = last.QuadPart ? double(now.QuadPart - last.QuadPart) / double(frequency.QuadPart) : 1.0 / 60.0;
        last = now;
        ticks = std::min(ticks + elapsed * 60.0, 64.0);
        // The game subtracts 1 right after this; subtract the remaining whole ticks first, keeping at least 1
        const int whole = int(ticks);
        ticks -= whole;
        nReloadPauseCounter = std::max(nReloadPauseCounter - std::max(whole - 1, 0), whole > 0 ? 1 : nReloadPauseCounter + 1);
    }); //0x8FF0DE
}

// The water and mud surfaces write their grid straight into a locked vertex buffer and read two of the written values
// back for the last one. Reads from that memory are uncached on many drivers, which drops swamp levels to single-digit
// frame rates; the value is kept on the FPU stack instead (the read-back divisor is the 1.0 just stored)
void InitSurfaceGridReadback()
{
    if (Screen.bSignatureEdition)
        return;
    auto pattern = hook::pattern("8B 45 C0 D9 18 8B 4D C4 D9 41 FC 8B 55 C4 D8 62 04 D8 0D ? ? ? ? 8B 45 C0 D9 58 04 8B 4D C0 C7 41 08 00 00 80 3F 8B 55 C0 C7 42 0C 00 00 80 3F 8B 45 C0 C7 40 10 00 00 00 00 8B 4D C0 D9 01 D9 E0 8B 55 C0 D8 72 08");
    auto code = pattern.get_first<uint8_t>(); //0x972F34
    injector::WriteMemory<uint8_t>(code + 4, 0x10, true); // fstp [eax] -> fst [eax]
    for (auto offset : { 0x3E, 0x3F, 0x45, 0x46, 0x47 }) // fld [ecx]; fdiv [edx+8]
        injector::WriteMemory<uint8_t>(code + offset, 0x90, true);
}

class GameSpeedFix
{
public:
    GameSpeedFix()
    {
        WFP::onInitEvent() += []()
        {
            InitPhysicsStep();
            InitReloadPauseDelay();
            InitSurfaceGridReadback();

            init_qpc_timing();

            auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05");
            shTIM_Clock_Reset = safetyhook::create_inline(pattern.get_first(), TIM_Clock_Reset);

            pattern = hook::pattern("83 EC 0C 8D 4C 24 ? E8 ? ? ? ? DB 44 24 ? 8B 44 24 ? 85 C0 7D ? D8 05 ? ? ? ? 8B 0D ? ? ? ? DB 05 ? ? ? ? 85 C9 7D ? D8 05 ? ? ? ? 8B 54 24 ? 85 D2 DE E9 D8 0D ? ? ? ? DB 44 24 ? 7D ? D8 05 ? ? ? ? A1 ? ? ? ? DB 05 ? ? ? ? 85 C0 7D ? D8 05 ? ? ? ? 8B 0D");
            shTIM_f_Clock_TrueRead = safetyhook::create_inline(pattern.get_first(), TIM_f_Clock_TrueRead);

            pattern = hook::pattern("A0 ? ? ? ? 84 C0 74 ? E8");
            shTIM_UpdateCPUClockFrequency = safetyhook::create_inline(pattern.get_first(), TIM_UpdateCPUClockFrequency);

            pattern = hook::pattern("7A ? ? ? ? ? ? ? ? ? EB ? ? ? ? ? ? ? DF E0 F6 C4 ? 75 ? ? ? ? ? ? ? ? ? A1");
            injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true);
        };
    }
} GameSpeedFix;
