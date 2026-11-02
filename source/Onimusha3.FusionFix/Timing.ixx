module;
#include <stdafx.h>
#include <chrono>
#include <thread>
#include "FixSettings.hpp"

export module Timing;
import ComVars;
import Window;
import Focus;


export namespace OnimushaTiming
{
    using Clock = std::chrono::steady_clock;
    inline SafetyHookInline UpdateHook;
    inline SafetyHookMid PaceHook;
    inline std::atomic<int> Limit{ 60 };
    inline Clock::time_point Previous{}, Deadline{};
    inline double Accumulator = 0.0;

    inline void __stdcall Update(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
    {
        const auto now = Clock::now();
        if (OnimushaFocus::Paused || !Ready)
        {
            Previous = {};
            Accumulator = 0.0;
            return;
        }
        constexpr double step = 1.0 / 60.0;
        Accumulator += Previous == Clock::time_point{} ? step :
            std::clamp(std::chrono::duration<double>(now - Previous).count(), 0.0, 0.125);
        Previous = now;
        // Each native tick finishes its task-event rendezvous and builds one complete command buffer.
        // Drawing can replay that buffer between ticks without advancing game logic or reading input twice.
        while (Accumulator + 1.0e-9 >= step)
        {
            static std::once_flag gameInit;
            std::call_once(gameInit, []()
            {
                WFP::onGameInitEvent().executeAll();
            });
            WFP::onGameProcessEvent().executeAll();
            UpdateHook.unsafe_stdcall<void>(a, b, c, d, e);
            Accumulator -= step;
        }
    }

    inline void Pace()
    {
        int limit = Limit.load(std::memory_order_relaxed);
        if (limit == -1)
        {
            MONITORINFOEXW monitor{};
            monitor.cbSize = sizeof(monitor);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            limit = GetMonitorInfoW(MonitorFromWindow(OnimushaWindow::Window, MONITOR_DEFAULTTONEAREST), &monitor) &&
                EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode) ? int(mode.dmDisplayFrequency) : 60;
            limit = std::clamp(limit, 30, 1000);
        }
        if (limit == 0)
        {
            Deadline = {};
            return;
        }
        limit = std::clamp(limit, 30, 1000);
        const auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / limit));
        const auto now = Clock::now();
        if (Deadline == Clock::time_point{} || now - Deadline > std::chrono::milliseconds(125) || Deadline - now > period)
            Deadline = now;
        Deadline += period;
        const auto remaining = Deadline - Clock::now();
        if (remaining > std::chrono::milliseconds(2))
            std::this_thread::sleep_for(remaining - std::chrono::milliseconds(1));
        while (Clock::now() < Deadline)
            YieldProcessor();
    }

    inline void Init()
    {
        const auto refresh = []() { Limit = std::clamp(FixSettings().Int("MAIN.FPSLimit"), -1, 1000); };
        refresh();
        FixSettings().OnChange("MAIN.FPSLimit", refresh);
        auto update = hook::pattern("51 53 33 DB 80 3D ? ? ? ? 01 75 13 6A 05 E8");
        Ready.SetAddress(*hook::pattern("38 1D ? ? ? ? 0F 84 ? ? ? ? A0 ? ? ? ? 3C 01").get_first<uint8_t*>(2));
        auto skip = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? 04 7D 40 A1 ? ? ? ? 83 C0 01");
        auto buffers = hook::pattern("A0 ? ? ? ? 8B 0D ? ? ? ? 0F B6 D0 34 01 89 15");
        injector::MakeJMP(skip.get_first(), buffers.get_first(), true);
        auto wait = hook::pattern("80 3D ? ? ? ? 00 75 63 E8 ? ? ? ? DB 05");
        auto finish = hook::pattern("80 3D ? ? ? ? 01 75 3E E8 ? ? ? ? 84 C0 75 2E");
        injector::MakeJMP(wait.get_first(), finish.get_first(), true);
        UpdateHook = safetyhook::create_inline(update.get_first(), Update);
        PaceHook = safetyhook::create_mid(finish.get_first(), [](SafetyHookContext&) { Pace(); });
    }
}
