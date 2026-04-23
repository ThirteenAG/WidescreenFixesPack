module;

#include <stdafx.h>

export module Speedhack;

float wantedSpeed = 1.0f;

SafetyHookInline shGetTickCount{};
SafetyHookInline shGetTickCount64{};
SafetyHookInline shQueryPerformanceCounter{};
SafetyHookInline shTimeGetTime{};

CRITICAL_SECTION gtcCS;
CRITICAL_SECTION qpcCS;

float     gtc_speed = 1.0f;
ULONGLONG gtc_initialTime = 0;
ULONGLONG gtc_initialOffset = 0;

float    qpc_speed = 1.0f;
LONGLONG qpc_initialTime = 0;
LONGLONG qpc_initialOffset = 0;

// Called AFTER retrieving the real time (original called outside the lock)
static ULONGLONG ComputeTickTime(ULONGLONG currentTime)
{
    EnterCriticalSection(&gtcCS);

    float wanted = wantedSpeed;

    // Lazy init on first call — avoids the init-before-hooks race
    if (gtc_initialTime == 0)
    {
        gtc_initialTime = currentTime;
        gtc_initialOffset = currentTime;
    }

    ULONGLONG newTime = (ULONGLONG)((double)(currentTime - gtc_initialTime) * gtc_speed)
        + gtc_initialOffset;

    // Re-anchor when speed changes (CE does this too) — prevents time discontinuity
    if (gtc_speed != wanted)
    {
        gtc_initialOffset = newTime;
        gtc_initialTime = currentTime;
        gtc_speed = wanted;
    }

    LeaveCriticalSection(&gtcCS);
    return newTime;
}

static LONGLONG ComputeQPCTime(LONGLONG currentQPC)
{
    EnterCriticalSection(&qpcCS);

    float wanted = wantedSpeed;

    if (qpc_initialTime == 0)
    {
        qpc_initialTime = currentQPC;
        qpc_initialOffset = currentQPC;
    }

    LONGLONG newTime = (LONGLONG)((double)(currentQPC - qpc_initialTime) * qpc_speed)
        + qpc_initialOffset;

    if (qpc_speed != wanted)
    {
        qpc_initialOffset = newTime;
        qpc_initialTime = currentQPC;
        qpc_speed = wanted;
    }

    LeaveCriticalSection(&qpcCS);
    return newTime;
}

// All tick-based hooks: call original FIRST (no lock held), then compute
DWORD WINAPI GetTickCountHook()
{
    DWORD current = shGetTickCount.stdcall<DWORD>();
    return (DWORD)ComputeTickTime((ULONGLONG)current);
}

ULONGLONG WINAPI GetTickCount64Hook()
{
    ULONGLONG current = shGetTickCount64.stdcall<ULONGLONG>();
    return ComputeTickTime(current);
}

BOOL WINAPI QueryPerformanceCounterHook(LARGE_INTEGER* lpPerformanceCount)
{
    if (!lpPerformanceCount) return FALSE;

    LARGE_INTEGER current{};
    BOOL res = shQueryPerformanceCounter.stdcall<BOOL>(&current);
    if (!res) return FALSE;

    lpPerformanceCount->QuadPart = ComputeQPCTime(current.QuadPart);
    return TRUE;
}

DWORD WINAPI timeGetTimeHook()
{
    DWORD current = shTimeGetTime.stdcall<DWORD>();
    return (DWORD)ComputeTickTime((ULONGLONG)current);
}

export void InitSpeedhack(float speed = 500.0f)
{
    InitializeCriticalSection(&gtcCS);
    InitializeCriticalSection(&qpcCS);

    wantedSpeed = speed;
    gtc_speed = speed;
    qpc_speed = speed;

    shGetTickCount = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetTickCount"), GetTickCountHook);
    shGetTickCount64 = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetTickCount64"), GetTickCount64Hook);
    shQueryPerformanceCounter = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32.dll"), "QueryPerformanceCounter"), QueryPerformanceCounterHook);

    CallbackHandler::RegisterCallback(L"winmm.dll", []()
    {
        auto pTimeGetTime = (DWORD(WINAPI*)())GetProcAddress(GetModuleHandle(L"winmm.dll"), "timeGetTime");
        if (pTimeGetTime)
        {
            shTimeGetTime = safetyhook::create_inline(pTimeGetTime, timeGetTimeHook);
        }
    });
}

export void SetSpeed(float speed)
{
    wantedSpeed = speed;
}

export void DisableSpeedhack()
{
    wantedSpeed = 1.0f;
}