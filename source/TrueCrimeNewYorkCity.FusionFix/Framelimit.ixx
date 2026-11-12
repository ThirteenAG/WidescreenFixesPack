module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

export module Framelimit;

import ComVars;

// The game's clock is a 60 Hz tick, counted by its tick thread. A frame waits a number of ticks since the last one
// (2 = 30 FPS, 1 = 60 FPS) and the simulation runs one step per tick.
GameRef<uint32_t> nTickCount;
GameRef<uint32_t> nLastWaitTick;
void (__cdecl* Tick)() = nullptr;

HANDLE hTickEvent = nullptr;
// Read by the game in place of its own frame interval, which stays as saved in its preferences
int32_t nFrameInterval = 2;
uint32_t nSteppedTick = 0;
int32_t nFrameSteps = 2;

// Replaces the game's tick thread, which measured its 17 ms with a millisecond timer and ran slow and unevenly
DWORD WINAPI TickThread(LPVOID)
{
    LARGE_INTEGER frequency, now;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&now);

    HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!timer)
        timer = CreateWaitableTimerW(nullptr, TRUE, nullptr);

    const double period = double(frequency.QuadPart) / 60.0;
    double next = double(now.QuadPart);

    while (true)
    {
        next += period;
        QueryPerformanceCounter(&now);

        // After a stall the ticks start over instead of arriving all at once
        if (double(now.QuadPart) - next > period * 4.0)
            next = double(now.QuadPart);

        while (double(now.QuadPart) < next)
        {
            LARGE_INTEGER due;
            due.QuadPart = -std::max(1LL, LONGLONG((next - double(now.QuadPart)) * 10000000.0 / double(frequency.QuadPart)));
            SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
            WaitForSingleObject(timer, INFINITE);
            QueryPerformanceCounter(&now);
        }

        Tick();
        SetEvent(hTickEvent);
    }
}

SafetyHookInline shFrameWait = {};
// The game's frame wait spins until the tick arrives and handles one window message per turn; it is also the only
// message pump of the movie player. Here the messages are handled and the thread sleeps until the tick, so the
// game's own wait returns at once.
void __cdecl FrameWait(int ticks)
{
    while (true)
    {
        bool quit = false;
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            // Left for the game's own loop
            if (msg.message == WM_QUIT)
            {
                PostQuitMessage(int(msg.wParam));
                quit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (quit || int32_t(nTickCount - nLastWaitTick) >= ticks)
            break;

        MsgWaitForMultipleObjects(1, &hTickEvent, FALSE, 100, QS_ALLINPUT);
    }
    return shFrameWait.unsafe_ccall(ticks);
}

bool GameNeeds30FPS()
{
    if (nLoading != 0 || bCutscene || bPause)
        return false;

    static std::vector<std::pair<eCaseNames, eMissionNames_BC03>> missionList =
    {
        { BC03, BC3_M6 },
    };

    if (std::any_of(missionList.begin(), missionList.end(), [](const auto& p) { return CurrentCaseName == p.first && CurrentMissionName == p.second; }))
        return true;

    return false;
}

void InitFrameLimiter()
{
    timeBeginPeriod(1);
    hTickEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    auto pattern = hook::pattern("A1 ? ? ? ? 83 C0 01 A3 ? ? ? ? A1");
    Tick = (decltype(Tick))pattern.get_first();
    nTickCount.SetAddress(*pattern.get_first<uint32_t*>(1));

    pattern = hook::pattern("56 8B 35 ? ? ? ? 57 8B 3D ? ? ? ? 8B FF");
    injector::MakeJMP(pattern.get_first(), TickThread, true);

    pattern = hook::pattern("A1 ? ? ? ? 83 EC 1C");
    nLastWaitTick.SetAddress(*pattern.get_first<uint32_t*>(1));
    shFrameWait = safetyhook::create_inline(pattern.get_first(0), FrameWait);

    // Frame wait of the main loop, least steps of a paused frame, frame wait of the map screen
    pattern = hook::pattern("83 38 13 B8 02 00 00 00 74 05 A1 ? ? ? ?");
    injector::WriteMemory(pattern.get_first(11), &nFrameInterval, true);
    pattern = hook::pattern("E8 ? ? ? ? 2B 05 ? ? ? ? 8B 0D ? ? ? ? 89 44 24");
    injector::WriteMemory(pattern.get_first(13), &nFrameInterval, true);
    pattern = hook::pattern("B8 02 00 00 00 2B 05 ? ? ? ? 50 E8");
    injector::WriteMemory(pattern.get_first(7), &nFrameInterval, true);

    // Each turn of the main loop: the frame rate, and the simulation steps of this frame = the ticks since the last
    // one. The game guessed the steps from the last frame's duration instead, starting at 2, which tied the game
    // speed to the frame rate. At most 4 steps, like the game; ticks beyond that are dropped.
    pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E8 ? ? ? ? 80 3D");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static const int& nFPSLimit = FixSettings().Int("MAIN.FPSLimit");
        nFrameInterval = (nFPSLimit == 60 && !GameNeeds30FPS()) ? 1 : 2;

        const uint32_t tick = nTickCount;
        int32_t steps = int32_t(tick - nSteppedTick);
        if (steps > 4)
        {
            steps = 4;
            nSteppedTick = tick;
        }
        else
        {
            steps = std::max(steps, 1);
            nSteppedTick += steps;
        }
        nFrameSteps = steps;
    });

    pattern = hook::pattern("8A 0D ? ? ? ? 8A 5C 24 ? 88 0D ? ? ? ?");
    static auto pStepsDone = (uintptr_t)pattern.get_first();

    pattern = hook::pattern("F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 5C C8 F3 0F 5E 44 24 ? BE 02 00 00 00");
    static auto StepsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        regs.esi = nFrameSteps;
        regs.eip = pStepsDone;
    });

    // improve camera rotation with gamepad
    pattern = hook::pattern("51 F3 0F 11 04 24 68 ? ? ? ? 8D 44 24 ? 68 ? ? ? ? 50 E8 ? ? ? ? 0F 28 08 0F 28 C1 0F 59 C1 0F 28 D0 0F C6 D0 FF 0F 28 D8 0F C6 D8 AA");
    static auto slerpHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (CurrentCameraMode == CameraOnFoot)
            regs.xmm0.f32[0] *= 1.5f;
    });
}

class Framelimit
{
public:
    Framelimit()
    {
        WFP::onInitEvent() += []()
        {
            InitFrameLimiter();
        };
    }
} Framelimit;
