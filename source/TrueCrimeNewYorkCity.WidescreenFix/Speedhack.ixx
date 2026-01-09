module;

#include <Windows.h>
#include <winternl.h>
#include <atomic>
#include <thread>
#include "MinHook.h"

export module Speedhack;

// =======================================================
// Globals
// =======================================================

export float fGameSpeedFactor = 0.5f;
static std::atomic<float> speedMultiplier{ 1.0f };
static float lastMultiplier = 1.0f;

static std::atomic<bool> isRU{ false };
static bool versionDetected = false;

// =======================================================
// US / RU pointers
// =======================================================

//static volatile uint32_t* bUSPause = nullptr;
static volatile uint32_t* bUSCutscene = nullptr;
static volatile uint32_t* bUSLoading = nullptr;

//static volatile uint32_t* bRUPause = nullptr;
static volatile uint32_t* bRUCutscene = nullptr;
static volatile uint32_t* bRULoading = nullptr;

//static volatile uint32_t* bPauseCurrent = nullptr;
static volatile uint32_t* bCutsceneCurrent = nullptr;
static volatile uint32_t* bLoadingCurrent = nullptr;

// =======================================================
// CE-style spinlock
// =======================================================

struct SimpleLock {
    LONG count = 0;
    DWORD owner = 0;

    void lock() {
        DWORD tid = GetCurrentThreadId();
        if (owner != tid) {
            while (InterlockedExchange(&count, 1) != 0)
                Sleep(0);
            owner = tid;
        }
        else {
            InterlockedIncrement(&count);
        }
    }

    void unlock() {
        if (count == 1)
            owner = 0;
        InterlockedDecrement(&count);
    }
};

static SimpleLock GTCLock;
static SimpleLock QPCLock;

// =======================================================
// Anchors
// =======================================================

static DWORD      initialOffset32 = 0;
static DWORD      initialTime32 = 0;

static ULONGLONG  initialOffset64 = 0;
static ULONGLONG  initialTime64 = 0;

static LONGLONG   initialOffsetQPC = 0;
static LONGLONG   initialTimeQPC = 0;

// =======================================================
// Original functions
// =======================================================

using FnGetTickCount = DWORD(WINAPI*)();
using FnGetTickCount64 = ULONGLONG(WINAPI*)();
using FnTimeGetTime = DWORD(WINAPI*)();
using FnQPC = BOOL(WINAPI*)(LARGE_INTEGER*);

using FnNtQuerySystemTime = NTSTATUS(NTAPI*)(PLARGE_INTEGER);
using FnNtQueryPerformanceCounter = NTSTATUS(NTAPI*)(PLARGE_INTEGER, PLARGE_INTEGER);

static FnGetTickCount realGetTickCount;
static FnGetTickCount64 realGetTickCount64;
static FnTimeGetTime realTimeGetTime;
static FnQPC realQPC;

static FnNtQuerySystemTime realNtQuerySystemTime;
static FnNtQueryPerformanceCounter realNtQueryPerformanceCounter;

// =======================================================
// Re-anchor (CRITICAL)
// =======================================================

static void Reanchor(float newMultiplier)
{
    GTCLock.lock();
    QPCLock.lock();

    float old = speedMultiplier.load();

    DWORD now32 = realGetTickCount();
    ULONGLONG now64 = realGetTickCount64();

    LARGE_INTEGER qpcNow{};
    realQPC(&qpcNow);

    initialOffset32 += DWORD((now32 - initialTime32) * old);
    initialOffset64 += ULONGLONG((now64 - initialTime64) * old);
    initialOffsetQPC += LONGLONG((qpcNow.QuadPart - initialTimeQPC) * old);

    initialTime32 = now32;
    initialTime64 = now64;
    initialTimeQPC = qpcNow.QuadPart;

    speedMultiplier.store(newMultiplier);

    QPCLock.unlock();
    GTCLock.unlock();
}

// =======================================================
// Hooks
// =======================================================

DWORD WINAPI GetTickCount_Hook()
{
    GTCLock.lock();
    DWORD t = realGetTickCount();
    DWORD r = initialOffset32 + DWORD((t - initialTime32) * speedMultiplier.load());
    GTCLock.unlock();
    return r;
}

ULONGLONG WINAPI GetTickCount64_Hook()
{
    GTCLock.lock();
    ULONGLONG t = realGetTickCount64();
    ULONGLONG r = initialOffset64 + ULONGLONG((t - initialTime64) * speedMultiplier.load());
    GTCLock.unlock();
    return r;
}

DWORD WINAPI timeGetTime_Hook()
{
    return GetTickCount_Hook();
}

BOOL WINAPI QPC_Hook(LARGE_INTEGER* out)
{
    QPCLock.lock();
    LARGE_INTEGER t{};
    realQPC(&t);
    out->QuadPart = initialOffsetQPC + LONGLONG((t.QuadPart - initialTimeQPC) * speedMultiplier.load());
    QPCLock.unlock();
    return TRUE;
}

// ---------------- NT ----------------

NTSTATUS NTAPI NtQuerySystemTime_Hook(PLARGE_INTEGER out)
{
    NTSTATUS s = realNtQuerySystemTime(out);
    if (NT_SUCCESS(s)) {
        QPCLock.lock();
        out->QuadPart = initialOffsetQPC +
            LONGLONG((out->QuadPart - initialTimeQPC) * speedMultiplier.load());
        QPCLock.unlock();
    }
    return s;
}

NTSTATUS NTAPI NtQueryPerformanceCounter_Hook(PLARGE_INTEGER out, PLARGE_INTEGER freq)
{
    NTSTATUS s = realNtQueryPerformanceCounter(out, freq);
    if (NT_SUCCESS(s)) {
        QPCLock.lock();
        out->QuadPart = initialOffsetQPC +
            LONGLONG((out->QuadPart - initialTimeQPC) * speedMultiplier.load());
        QPCLock.unlock();
    }
    return s;
}

// =======================================================
// Watcher thread
// =======================================================

static void Watcher()
{
    while (true) {
        //int pause = bPauseCurrent ? *bPauseCurrent : 0;
        int cut = bCutsceneCurrent ? *bCutsceneCurrent : 0;
        int load = bLoadingCurrent ? *bLoadingCurrent : 0;

        float wanted = (cut || load) ? 1.0f : fGameSpeedFactor;

        if (wanted != lastMultiplier) {
            Reanchor(wanted);
            lastMultiplier = wanted;
        }

        Sleep(50);
    }
}

// =======================================================
// Init
// =======================================================

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
        //bPauseCurrent = (uint32_t*)(base + 0x3A0F5C);
        bCutsceneCurrent = (uint32_t*)(base + 0x387B78);
        bLoadingCurrent = (uint32_t*)(base + 0x39339C);
    }
    else {
        //bPauseCurrent = (uint32_t*)(base + 0x3A1F1C);
        bCutsceneCurrent = (uint32_t*)(base + 0x388B38);
        bLoadingCurrent = (uint32_t*)(base + 0x394360);
    }

    // ---------- resolve ----------
    realGetTickCount = GetTickCount;
    realGetTickCount64 = GetTickCount64;
    realTimeGetTime = (FnTimeGetTime)GetProcAddress(GetModuleHandleW(L"winmm"), "timeGetTime");
    realQPC = QueryPerformanceCounter;

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    realNtQuerySystemTime = (FnNtQuerySystemTime)GetProcAddress(ntdll, "NtQuerySystemTime");
    realNtQueryPerformanceCounter = (FnNtQueryPerformanceCounter)GetProcAddress(ntdll, "NtQueryPerformanceCounter");

    // ---------- anchors ----------
    initialTime32 = realGetTickCount();
    initialOffset32 = initialTime32;

    initialTime64 = realGetTickCount64();
    initialOffset64 = initialTime64;

    LARGE_INTEGER q{};
    realQPC(&q);
    initialTimeQPC = q.QuadPart;
    initialOffsetQPC = q.QuadPart;

    // ---------- hooks ----------
    MH_Initialize();

    MH_CreateHook(realGetTickCount, GetTickCount_Hook, (void**)&realGetTickCount);
    MH_CreateHook(realGetTickCount64, GetTickCount64_Hook, (void**)&realGetTickCount64);
    MH_CreateHook(realTimeGetTime, timeGetTime_Hook, (void**)&realTimeGetTime);
    MH_CreateHook(realQPC, QPC_Hook, (void**)&realQPC);

    MH_CreateHook(realNtQuerySystemTime, NtQuerySystemTime_Hook, (void**)&realNtQuerySystemTime);
    MH_CreateHook(realNtQueryPerformanceCounter, NtQueryPerformanceCounter_Hook, (void**)&realNtQueryPerformanceCounter);

    MH_EnableHook(MH_ALL_HOOKS);

    // ---------- watcher ----------
    std::thread(Watcher).detach();
}
