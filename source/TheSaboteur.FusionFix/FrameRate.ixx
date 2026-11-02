module;

#include "stdafx.h"
#include <atomic>
#include <mmsystem.h>

#include "FixSettings.hpp"

#pragma comment(lib, "winmm.lib")

export module FrameRate;

export namespace SaboteurFrameRate
{
    inline std::atomic<int> Limit{ 60 };
    inline SafetyHookMid TickHook;
    inline uintptr_t Clock = 0;
    inline bool TimerRequested = false;

    inline void SetTimingResolution(bool enabled)
    {
        if (enabled && !TimerRequested)
            TimerRequested = timeBeginPeriod(1) == TIMERR_NOERROR;
        else if (!enabled && TimerRequested)
        {
            timeEndPeriod(1);
            TimerRequested = false;
        }
    }

    inline int MonitorRefresh()
    {
        static ULONGLONG nextCheck = 0;
        static int refresh = 60;
        const auto now = GetTickCount64();
        if (now >= nextCheck)
        {
            nextCheck = now + 1000;
            const auto monitor = WindowedModeWrapper::WindowMonitor();
            DEVMODE mode{ .dmSize = sizeof(mode) };
            if (EnumDisplaySettingsEx(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode, 0) && mode.dmDisplayFrequency > 1)
                refresh = std::clamp(static_cast<int>(mode.dmDisplayFrequency), 30, 1000);
        }
        return refresh;
    }

    inline void Tick(SafetyHookContext& regs)
    {
        if (regs.ecx != Clock)
            return;
        auto fps = Limit.load(std::memory_order_relaxed);
        if (fps == -1)
            fps = MonitorRefresh();
        SetTimingResolution(fps != 0);
        const float interval = fps == 0 ? 0.0f : 1.0f / static_cast<float>(std::clamp(fps, 30, 1000));
        ProtectedGameRef<float> nativeInterval{ reinterpret_cast<float*>(regs.ecx + 0x4C) };
        if (nativeInterval.get() != interval)
            nativeInterval = interval;
    }

    inline void Install()
    {
        Limit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
        FixSettings().OnChange("MAIN.FPSLimit", []()
        {
            Limit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
        });
        auto pattern = hook::pattern("D9 05 ? ? ? ? A2 ? ? ? ? D9 1D ? ? ? ? E8");
        Clock = reinterpret_cast<uintptr_t>(*pattern.get_first<float*>(13)) - 0x4C;
        pattern = hook::pattern("83 EC 10 56 8B F1 D9 46 18 0F B6 46 54 D9 5E 50 51 D9 46 4C");
        TickHook = safetyhook::create_mid(pattern.get_first(), Tick);
        WFP::onShutdownEvent() += []()
        {
            SetTimingResolution(false);
        };
    }
}
