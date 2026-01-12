module;

#include <stdafx.h>

export module Speedhack;

export bool* bPause = nullptr;
export bool* bCutscene = nullptr;
export uint32_t* nLoading = nullptr;
export float fGameSpeedFactor = 1.0f;
static float lastMultiplier = 1.0f;

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

void SynchronizeTimeBase(float newMultiplier)
{
    gtcLock.lock();
    qpcLock.lock();
    tgtLock.lock();

    float oldMultiplier = lastMultiplier;

    // Snapshot current real times
    DWORD now32 = shGetTickCount ? shGetTickCount.unsafe_stdcall<DWORD>() : GetTickCount();
    ULONGLONG now64 = shGetTickCount64 ? shGetTickCount64.unsafe_stdcall<ULONGLONG>() : GetTickCount64();
    LARGE_INTEGER qpcNow{};
    if (shQueryPerformanceCounter)
        shQueryPerformanceCounter.unsafe_stdcall<BOOL>(&qpcNow);
    else
        QueryPerformanceCounter(&qpcNow);
    DWORD tgtNow = shTimeGetTime ? shTimeGetTime.unsafe_stdcall<DWORD>() : now32;

    // Calculate elapsed game time with the old multiplier and bake it into the offset
    initialOffset += DWORD((now32 - initialTime) * oldMultiplier);
    initialOffset64 += ULONGLONG((now64 - initialTime64) * oldMultiplier);
    initialOffsetQPC += LONGLONG((qpcNow.QuadPart - initialTimeQPC) * oldMultiplier);
    initialOffsetTGT += DWORD((tgtNow - initialTimeTGT) * oldMultiplier);

    // Reset the start time anchor to "now"
    initialTime = now32;
    initialTime64 = now64;
    initialTimeQPC = qpcNow.QuadPart;
    initialTimeTGT = tgtNow;

    lastMultiplier = newMultiplier;

    tgtLock.unlock();
    qpcLock.unlock();
    gtcLock.unlock();
}

export float GetSpeedhackMultiplier()
{
    float wanted = speedMultiplier;

    if (/*(bPause && *bPause) ||*/(bCutscene && *bCutscene) || (nLoading && *nLoading))
        wanted = 1.0f;

    if (wanted != lastMultiplier)
        SynchronizeTimeBase(wanted);

    return wanted;
}

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

DWORD WINAPI timeGetTimeHook()
{
    tgtLock.lock();
    DWORD currentTime = shTimeGetTime.unsafe_stdcall<DWORD>();
    DWORD result = (DWORD)((currentTime - initialTimeTGT) * GetSpeedhackMultiplier()) + initialOffsetTGT;
    tgtLock.unlock();
    return result;
}

export void InitSpeedhack()
{
    auto pattern = hook::pattern("88 15 ? ? ? ? 8D 45");
    bPause = *pattern.get_first<bool*>(2);

    pattern = hook::pattern("32 C0 88 81 ? ? ? ? A2 ? ? ? ? E8 ? ? ? ? 33 C0 C3");
    bCutscene = *pattern.get_first<bool*>(9);

    pattern = hook::pattern("83 3D ? ? ? ? ? 74 ? 84 DB");
    nLoading = *pattern.get_first<uint32_t*>(2);

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
        auto pTimeGetTime = (DWORD(WINAPI*)())GetProcAddress(GetModuleHandle(L"winmm.dll"), "timeGetTime");
        if (pTimeGetTime)
        {
            shTimeGetTime = safetyhook::create_inline(pTimeGetTime, timeGetTimeHook);
            initialOffsetTGT = pTimeGetTime();
            initialTimeTGT = shTimeGetTime ? shTimeGetTime.unsafe_stdcall<DWORD>() : pTimeGetTime();
        }
    });

    SetSpeedhackMultiplier(fGameSpeedFactor);
    lastMultiplier = speedMultiplier;

    tgtLock.unlock();
    gtcLock.unlock();
    qpcLock.unlock();
}