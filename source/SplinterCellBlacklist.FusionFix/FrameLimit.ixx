module;

#include <stdafx.h>

export module FrameLimit;

import ComVars;

// The game has no frame rate limit other than vsync, movement and some animations (guards) don't hold up at high frame rates.
// FPSLimit limits the frame rate.
namespace FrameLimit
{
    int limit = 0;

    HANDLE timer = nullptr;
    LARGE_INTEGER frequency = {};
    LONGLONG next = 0;

    LONGLONG Now()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return counter.QuadPart;
    }

    void Wait(int fps)
    {
        auto period = frequency.QuadPart / fps;
        auto now = Now();
        // a late frame starts the next period from now instead of making up for it with short frames
        if (next == 0 || now - next > period)
            next = now;
        else
        {
            // sleep until about 1 ms before the target, then spin
            auto remaining = next - now;
            auto ms = remaining * 1000 / frequency.QuadPart;
            if (ms > 1)
            {
                if (timer)
                {
                    LARGE_INTEGER due;
                    due.QuadPart = -(ms - 1) * 10000;
                    if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                        WaitForSingleObject(timer, INFINITE);
                }
                else
                    Sleep(DWORD(ms - 1));
            }
            while (Now() < next)
                YieldProcessor();
        }
        next += period;
    }
}

// The SMI grid (UI::SMIGridReveal, the tiles covering the screen between the game and the SMI screens) is shown and hidden with the
// GridShow signal (339C2AC, 1 = show, 0 = hide: sub_1971470, no automatic reveal) and the GridTransition signal (339C264: sub_19AC740,
// reveals by itself once it's in, ignored while the grid is moving). The SMI state (dword_344DB44) has +347h set when the grid is in and
// +348h when a GridTransition started.
// After the last mission's stats the grid is shown, then UI::SMISvMManual (sub_199AF90) hides it if it's in already, otherwise it starts
// a GridTransition. The grid takes about 0.6 seconds to come in; above 30 FPS SMISvMManual gets there earlier, its GridTransition is
// ignored and nothing hides the grid, it stays over the ending cutscene and the credits (#1600). Its hide is sent once the grid is in.
namespace GridFix
{
    uint8_t** pSMIState = nullptr;
    uintptr_t pGridShow = 0;
    char(__fastcall* SendSignal)(uintptr_t signal, void* edx, int value, char a3) = nullptr;

    bool pendingHide = false;
    ULONGLONG pendingTime = 0;

    uint8_t* State() { return pSMIState ? *pSMIState : nullptr; }

    SafetyHookInline shSvMManual{};
    char __fastcall SvMManual(uintptr_t scene, void* edx)
    {
        auto state = State();
        auto transitionStarted = state ? state[0x348] : 1;
        auto result = shSvMManual.thiscall<char>(scene);
        state = State();
        // the GridTransition it started was ignored, the grid is still coming in
        if (state && state[0x347] == 0 && transitionStarted == 0 && state[0x348] == 0 && result == 0)
        {
            pendingHide = true;
            pendingTime = GetTickCount64();
        }
        return result;
    }

    void Tick()
    {
        if (!pendingHide)
            return;
        auto state = State();
        if (state && state[0x347] != 0)
        {
            pendingHide = false;
            SendSignal(pGridShow, nullptr, 0, 0);
        }
        else if (GetTickCount64() - pendingTime > 5000)
            pendingHide = false;
    }
}

export void InitFrameLimit()
{
    using namespace FrameLimit;

    CIniReader iniReader("");
    limit = iniReader.ReadInteger("MAIN", "FPSLimit", 0);
    if (limit > 0)
        limit = std::clamp(limit, 30, 1000);

    QueryPerformanceFrequency(&frequency);
    timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

    // UI::SMISvMManual (sub_199AF90), the function starts 28h before the pattern; the SMI state pointer is read right after it
    {
        auto pattern = hook::pattern("8B F1 8A 86 9F 01 00 00 83 CB FF 84 C0 A1");
        if (!pattern.empty())
            GridFix::pSMIState = *pattern.get_first<uint8_t**>(14);
        else
        {
            pattern = hook::pattern("8B F1 8A 86 9F 01 00 00 8B 0D ? ? ? ? 84 C0");
            GridFix::pSMIState = *pattern.get_first<uint8_t**>(10);
        }
        GridFix::shSvMManual = safetyhook::create_inline(pattern.get_first(-0x28), GridFix::SvMManual);

        // its hide: GridShow(0)
        pattern = hook::pattern("80 B8 4A 03 00 00 00 74 0E 6A 00 6A 00 B9 ? ? ? ? E8");
        GridFix::pGridShow = *pattern.get_first<uintptr_t>(14);
        GridFix::SendSignal = reinterpret_cast<decltype(GridFix::SendSignal)>(injector::GetBranchDestination(pattern.get_first(18)).as_int());
    }

    // UGameEngine::Tick, GFrameCounter is incremented once a frame
    auto pattern = hook::pattern("BB 01 00 00 00 01 1D ? ? ? ? 11 3D ? ? ? ? F7 86 DC 00 00 00 00 04 00 00");
    static auto EngineTick = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        GridFix::Tick();
        if (limit > 0)
            Wait(limit);
        else
            next = 0;
    });
}
