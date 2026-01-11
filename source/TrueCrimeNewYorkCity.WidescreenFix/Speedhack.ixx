module;

#include <stdafx.h>

export module Speedhack;

export float fGameSpeedFactor = 1.0f;
static float lastMultiplier = 1.0f;

static volatile uint32_t* bPause = nullptr;
static volatile uint32_t* bCutscene = nullptr;
static volatile uint32_t* bLoading = nullptr;

static std::atomic<bool> isRU{ false };
static bool versionDetected = false;
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

// Speedhack state
export float speedMultiplier = 1.0f;

export void SetSpeedhackMultiplier(float multiplier)
{
    if (multiplier > 0.0f)
        speedMultiplier = multiplier;
}

static void Reanchor(float newMultiplier);

export float GetSpeedhackMultiplier()
{
    float wanted;

    if ((bLoading && *bLoading) || (bCutscene && *bCutscene))
        wanted = 1.0f;
    else
        wanted = speedMultiplier;

    if (wanted != lastMultiplier)
        Reanchor(wanted);

    return wanted;
}

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

static void Reanchor(float newMultiplier)
{
    gtcLock.lock();
    qpcLock.lock();
    tgtLock.lock();

    float old = lastMultiplier;

    DWORD now32 = shGetTickCount
        ? shGetTickCount.unsafe_stdcall<DWORD>()
        : GetTickCount();

    ULONGLONG now64 = shGetTickCount64
        ? shGetTickCount64.unsafe_stdcall<ULONGLONG>()
        : GetTickCount64();

    LARGE_INTEGER qpcNow{};
    if (shQueryPerformanceCounter)
        shQueryPerformanceCounter.unsafe_stdcall<BOOL>(&qpcNow);
    else
        QueryPerformanceCounter(&qpcNow);

    DWORD tgtNow = shTimeGetTime
        ? shTimeGetTime.unsafe_stdcall<DWORD>()
        : now32;

    initialOffset += DWORD((now32 - initialTime) * old);
    initialOffset64 += ULONGLONG((now64 - initialTime64) * old);
    initialOffsetQPC += LONGLONG((qpcNow.QuadPart - initialTimeQPC) * old);
    initialOffsetTGT += DWORD((tgtNow - initialTimeTGT) * old);

    initialTime = now32;
    initialTime64 = now64;
    initialTimeQPC = qpcNow.QuadPart;
    initialTimeTGT = tgtNow;

    lastMultiplier = newMultiplier;

    tgtLock.unlock();
    qpcLock.unlock();
    gtcLock.unlock();
}

export void InitSpeedhack()
{
    HMODULE hGame = GetModuleHandle(nullptr);
    uintptr_t base = (uintptr_t)hGame;
    // ---------- version detection ----------
    if (!versionDetected) {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);

        WIN32_FILE_ATTRIBUTE_DATA info{};
        GetFileAttributesExW(path, GetFileExInfoStandard, &info);

        LARGE_INTEGER sz{};
        sz.HighPart = info.nFileSizeHigh;
        sz.LowPart = info.nFileSizeLow;

        if (sz.QuadPart == 20135936) {
            isRU = false;
        }
        else if (sz.QuadPart == 5509120) {
            isRU = true;
        }
        versionDetected = true;
    }

    if (!isRU) {
        //bPause = (uint32_t*)(base + 0x3A0F5C);
        bCutscene = (uint32_t*)(base + 0x387B78);
        bLoading = (uint32_t*)(base + 0x39339C);
    }
    else {
        //bPause = (uint32_t*)(base + 0x3A1F1C);
        bCutscene = (uint32_t*)(base + 0x388B38);
        bLoading = (uint32_t*)(base + 0x394360);
    }

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
