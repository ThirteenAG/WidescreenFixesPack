module;

#include <stdafx.h>

export module Speedhack;

export float fGameSpeedFactor = 1.0f;

DWORD(WINAPI* pTimeGetTime)() = nullptr;

struct SimpleLock
{
    LONG count = 0;
    DWORD owner = 0;
    LONG recursion = 0;

    void lock()
    {
        DWORD tid = GetCurrentThreadId();
        if (owner == tid)
        {
            ++recursion;
            return;
        }

        while (InterlockedCompareExchange(&count, 1, 0) != 0)
            Sleep(0);

        owner = tid;
        recursion = 1;
    }

    void unlock()
    {
        if (--recursion == 0)
        {
            owner = 0;
            InterlockedExchange(&count, 0);
        }
    }
};

SafetyHookInline shGetTickCount = {};
SafetyHookInline shGetTickCount64 = {};
SafetyHookInline shQueryPerformanceCounter = {};
SafetyHookInline shTimeGetTime = {};

// Synchronization
SimpleLock gtcLock;
SimpleLock tgtLock;
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

// Initial values for timeGetTime
DWORD initialOffsetTGT = 0;
DWORD initialTimeTGT = 0;

// Speedhack state
export float speedMultiplier = 1.0f;

export void SetSpeedhackMultiplier(float multiplier)
{
    if (multiplier > 0.0f)
        speedMultiplier = multiplier;
}

export BOOL QueryRealPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
    if (shQueryPerformanceCounter)
        return shQueryPerformanceCounter.unsafe_stdcall<BOOL>(lpPerformanceCount);

    return QueryPerformanceCounter(lpPerformanceCount);
}

export float GetSpeedhackMultiplier()
{
    return speedMultiplier;
}

// Hooked functions
DWORD WINAPI GetTickCountHook()
{
    float multiplier = GetSpeedhackMultiplier();
    gtcLock.lock();
    DWORD currentTime = shGetTickCount.unsafe_stdcall<DWORD>();
    DWORD result = (DWORD)((currentTime - initialTime) * multiplier) + initialOffset;
    gtcLock.unlock();
    return result;
}

ULONGLONG WINAPI GetTickCount64Hook()
{
    float multiplier = GetSpeedhackMultiplier();
    gtcLock.lock();
    ULONGLONG currentTime = shGetTickCount64.unsafe_stdcall<ULONGLONG>();
    ULONGLONG result = (ULONGLONG)((currentTime - initialTime64) * multiplier) + initialOffset64;
    gtcLock.unlock();
    return result;
}

BOOL WINAPI QueryPerformanceCounterHook(LARGE_INTEGER* lpPerformanceCount)
{
    if (!lpPerformanceCount)
        return FALSE;

    float multiplier = GetSpeedhackMultiplier();
    qpcLock.lock();
    LARGE_INTEGER currentTime;
    BOOL result = shQueryPerformanceCounter.unsafe_stdcall<BOOL>(&currentTime);

    if (result)
    {
        LONGLONG newValue = (LONGLONG)((currentTime.QuadPart - initialTimeQPC) * multiplier) + initialOffsetQPC;
        lpPerformanceCount->QuadPart = newValue;
    }

    qpcLock.unlock();
    return result;
}

DWORD WINAPI timeGetTimeHook()
{
    float multiplier = GetSpeedhackMultiplier();
    tgtLock.lock();
    DWORD currentTime = pTimeGetTime();
    DWORD result = (DWORD)((currentTime - initialTimeTGT) * multiplier) + initialOffsetTGT;
    tgtLock.unlock();
    return result;
}

export void InitSpeedhack()
{
    shGetTickCount = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32"), "GetTickCount"), GetTickCountHook);
    shGetTickCount64 = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32"), "GetTickCount64"), GetTickCount64Hook);
    shQueryPerformanceCounter = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"kernel32"), "QueryPerformanceCounter"), QueryPerformanceCounterHook);

    qpcLock.lock();
    gtcLock.lock();
    tgtLock.lock();

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

    CallbackHandler::RegisterCallback(L"winmm.dll", []()
    {
        pTimeGetTime = (DWORD(WINAPI*)())GetProcAddress(GetModuleHandle(L"winmm.dll"), "timeGetTime");
        if (pTimeGetTime)
        {
            // Gameplay speed
            auto pattern = hook::pattern("E8 ? ? ? ? 85 C0 89 44 24 ? DB 44 24 ? 7D ? D8 05 ? ? ? ? D8 0D");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            pattern = hook::pattern("E8 ? ? ? ? 85 C0 89 44 24 ? DB 44 24 ? 7D ? D8 05 ? ? ? ? 80 3D");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E8 ? ? ? ? 80 3D");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 8B F8");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            pattern = hook::pattern("E8 ? ? ? ? 8D 54 24 ? 52 56 8B F8");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            pattern = hook::pattern("E8 ? ? ? ? 2B C7");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            pattern = hook::pattern("E8 ? ? ? ? 2B 44 24 ? 85 C0 89 44 24 ? DB 44 24 ? 7D ? D8 05 ? ? ? ? 80 3D");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            pattern = hook::pattern("E8 ? ? ? ? DB 05 ? ? ? ? A3");
            injector::MakeCALL(pattern.get_first(), timeGetTimeHook, true);

            // Other xrefs (maybe needed, unknown)
            //injector::MakeCALL(0x496050, timeGetTimeHook, true);
            //injector::MakeCALL(0x4982E9, timeGetTimeHook, true);
            //injector::MakeCALL(0x4988D6, timeGetTimeHook, true);
            //injector::MakeCALL(0x499661, timeGetTimeHook, true);
            //injector::MakeCALL(0x4997FF, timeGetTimeHook, true);
            //injector::MakeCALL(0x49A06E, timeGetTimeHook, true);
            //injector::MakeCALL(0x49A0AE, timeGetTimeHook, true);
            //injector::MakeCALL(0x49A141, timeGetTimeHook, true);
            //injector::MakeCALL(0x49A5D2, timeGetTimeHook, true);
            //injector::MakeCALL(0x49B75B, timeGetTimeHook, true);
            //
            //injector::MakeCALL(0x4A8E66, timeGetTimeHook, true);
            //injector::MakeCALL(0x4A8EDE, timeGetTimeHook, true);
            //injector::MakeCALL(0x4A90ED, timeGetTimeHook, true);
            //injector::MakeCALL(0x5C09B1, timeGetTimeHook, true);
            //injector::MakeCALL(0x5C09FB, timeGetTimeHook, true);
            //injector::MakeCALL(0x5D8552, timeGetTimeHook, true);
            //injector::MakeCALL(0x5D8593, timeGetTimeHook, true);
            //injector::MakeCALL(0x648D1D, timeGetTimeHook, true);
            //injector::MakeCALL(0x649733, timeGetTimeHook, true);

            initialOffsetTGT = pTimeGetTime();
            initialTimeTGT = pTimeGetTime();
        }
    });

    SetSpeedhackMultiplier(fGameSpeedFactor);

    tgtLock.unlock();
    gtcLock.unlock();
    qpcLock.unlock();
}