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

export void InitGameSpeedFix()
{
    init_qpc_timing();

    auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05");
    shTIM_Clock_Reset = safetyhook::create_inline(pattern.get_first(), TIM_Clock_Reset);

    pattern = hook::pattern("83 EC 0C 8D 4C 24 ? E8 ? ? ? ? DB 44 24 ? 8B 44 24 ? 85 C0 7D ? D8 05 ? ? ? ? 8B 0D ? ? ? ? DB 05 ? ? ? ? 85 C9 7D ? D8 05 ? ? ? ? 8B 54 24 ? 85 D2 DE E9 D8 0D ? ? ? ? DB 44 24 ? 7D ? D8 05 ? ? ? ? A1 ? ? ? ? DB 05 ? ? ? ? 85 C0 7D ? D8 05 ? ? ? ? 8B 0D");
    shTIM_f_Clock_TrueRead = safetyhook::create_inline(pattern.get_first(), TIM_f_Clock_TrueRead);

    pattern = hook::pattern("A0 ? ? ? ? 84 C0 74 ? E8");
    shTIM_UpdateCPUClockFrequency = safetyhook::create_inline(pattern.get_first(), TIM_UpdateCPUClockFrequency);

    pattern = hook::pattern("7A ? ? ? ? ? ? ? ? ? EB ? ? ? ? ? ? ? DF E0 F6 C4 ? 75 ? ? ? ? ? ? ? ? ? A1");
    injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
}