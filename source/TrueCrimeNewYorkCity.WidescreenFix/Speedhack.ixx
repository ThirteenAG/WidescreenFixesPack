module;

#include <stdafx.h>

export module Speedhack;

export bool* bPause = nullptr;
export bool* bCutscene = nullptr;
export float fGameSpeedFactor = 1.0f;

struct SimpleLock
{
    LONG count = 0;
    DWORD owner = 0;

    void lock()
    {
        DWORD tid = GetCurrentThreadId();
        if (owner != tid)
        {
            while (InterlockedExchange(&count, 1) != 0)
                Sleep(0);
            owner = tid;
        }
        else
            InterlockedIncrement(&count);
    }

    void unlock()
    {
        if (count == 1)
            owner = 0;
        InterlockedDecrement(&count);
    }
};

SafetyHookInline shGetTickCount = {};
SafetyHookInline shGetTickCount64 = {};
SafetyHookInline shQueryPerformanceCounter = {};

// Speedhack state
export float speedMultiplier = 1.0f;

export void SetSpeedhackMultiplier(float multiplier)
{
    if (multiplier > 0.0f)
        speedMultiplier = multiplier;
}

export float GetSpeedhackMultiplier()
{
    if (*bPause || *bCutscene)
        return 1.0f;

    return speedMultiplier;
}

// Synchronization
SimpleLock gtcLock;
export SimpleLock qpcLock;

// Initial values for GetTickCount
DWORD initialOffset = 0;
DWORD initialTime = 0;

// Initial values for GetTickCount64
ULONGLONG initialOffset64 = 0;
ULONGLONG initialTime64 = 0;

// Initial values for QueryPerformanceCounter
LONGLONG initialOffsetQPC = 0;
LONGLONG initialTimeQPC = 0;

// Hooked functions
DWORD WINAPI GetTickCountHook()
{
    gtcLock.lock();
    DWORD currentTime = shGetTickCount.unsafe_stdcall<DWORD>();
    DWORD result = (DWORD)((currentTime - initialTime) * GetSpeedhackMultiplier()) + initialOffset;
    gtcLock.unlock();
    return result;
}

ULONGLONG WINAPI GetTickCount64Hook()
{
    gtcLock.lock();
    ULONGLONG currentTime = shGetTickCount64.unsafe_stdcall<ULONGLONG>();
    ULONGLONG result = (ULONGLONG)((currentTime - initialTime64) * GetSpeedhackMultiplier()) + initialOffset64;
    gtcLock.unlock();
    return result;
}

BOOL WINAPI QueryPerformanceCounterHook(LARGE_INTEGER* lpPerformanceCount)
{
    if (!lpPerformanceCount)
        return FALSE;

    qpcLock.lock();
    LARGE_INTEGER currentTime;
    BOOL result = shQueryPerformanceCounter.unsafe_stdcall<BOOL>(&currentTime);

    if (result)
    {
        LONGLONG newValue = (LONGLONG)((currentTime.QuadPart - initialTimeQPC) * GetSpeedhackMultiplier()) + initialOffsetQPC;
        lpPerformanceCount->QuadPart = newValue;
    }

    qpcLock.unlock();
    return result;
}

export void InitSpeedhack()
{
    auto pattern = hook::pattern("88 15 ? ? ? ? 8D 45");
    bPause = *pattern.get_first<bool*>(2);

    pattern = hook::pattern("88 1D ? ? ? ? E8 ? ? ? ? 85 C0");
    bCutscene = *pattern.get_first<bool*>(2);

    shGetTickCount = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32"), "GetTickCount"), GetTickCountHook);
    shGetTickCount64 = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32"), "GetTickCount64"), GetTickCount64Hook);
    shQueryPerformanceCounter = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32"), "QueryPerformanceCounter"), QueryPerformanceCounterHook);

    qpcLock.lock();
    gtcLock.lock();

    initialOffset = GetTickCount();
    initialTime = shGetTickCount ? shGetTickCount.unsafe_stdcall<DWORD>() : GetTickCount();

    QueryPerformanceCounter((LARGE_INTEGER*)&initialOffsetQPC);
    if (shQueryPerformanceCounter)
        shQueryPerformanceCounter.unsafe_stdcall<BOOL>(&initialTimeQPC);

    if (shGetTickCount64)
    {
        initialOffset64 = shGetTickCount64.unsafe_stdcall<ULONGLONG>();
        initialTime64 = shGetTickCount64.unsafe_stdcall<ULONGLONG>();
    }

    SetSpeedhackMultiplier(fGameSpeedFactor);

    gtcLock.unlock();
    qpcLock.unlock();
}
