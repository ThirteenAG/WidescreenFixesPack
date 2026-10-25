module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "FixSettings.hpp"

export module Misc;

import Skeleton;

namespace
{
    // How long the legal screen stays, in milliseconds
    ProtectedGameRef<uint32_t> LegalScreenTime([]() -> uint32_t*
    {
        auto pattern = hook::pattern("81 F9 B8 0B 00 00 72 C9");
        if (!pattern.empty())
            return pattern.get_first<uint32_t>(2);
        return nullptr;
    });

    // Frame rate limit: the game spins until 5 ms have passed since the last frame (200 fps). The limiter waits for
    // the rest of the frame first, sleeping most of it.
    class FrameLimiter
    {
        LARGE_INTEGER frequency = {};
        LARGE_INTEGER next = {};
        HANDLE timer = nullptr;
        int32_t current = 0;
    public:
        FrameLimiter()
        {
            QueryPerformanceFrequency(&frequency);
            timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            if (!timer)
                timer = CreateWaitableTimerW(nullptr, TRUE, nullptr);
        }

        void Wait(int32_t fps)
        {
            if (fps <= 0)
            {
                current = 0;
                return;
            }

            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            const int64_t interval = frequency.QuadPart / fps;
            if (fps != current || now.QuadPart - next.QuadPart > interval * 4)
            {
                // A new limit, or the game was held up (loading): start counting from now
                current = fps;
                next.QuadPart = now.QuadPart + interval;
                return;
            }

            const int64_t remaining = next.QuadPart - now.QuadPart;
            const int64_t spin = frequency.QuadPart / 1000;  // the last millisecond is spun for precision
            if (remaining > spin && timer)
            {
                LARGE_INTEGER due;
                due.QuadPart = -((remaining - spin) * 10000000 / frequency.QuadPart);
                if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, INFINITE);
            }
            do
            {
                QueryPerformanceCounter(&now);
            } while (now.QuadPart < next.QuadPart);
            next.QuadPart += interval;
        }
    };

    int32_t QueryRefreshRate()
    {
        HWND window = nullptr;
        if (auto rs = RsGlobal.get_ptr(); rs && rs->ps)
            window = rs->ps->window;
        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEXW info = {};
        info.cbSize = sizeof(info);
        DEVMODEW mode = {};
        mode.dmSize = sizeof(mode);
        if (GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
            return int32_t(mode.dmDisplayFrequency);
        return 60;
    }

    // The refresh rate of the game's monitor, looked up once a second
    int32_t GetRefreshRate()
    {
        static int32_t rate = 60;
        static ULONGLONG checked = 0;
        const ULONGLONG now = GetTickCount64();
        if (!checked || now - checked >= 1000)
        {
            rate = QueryRefreshRate();
            checked = now;
        }
        return rate;
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            if (FixSettings().Bool("MAIN.SkipIntro"))
            {
                // The legal screen is still drawn, but the game doesn't wait three seconds on it
                if (LegalScreenTime.get_ptr())
                    LegalScreenTime = 0;

                // The opening logo video (TITLES.BIK)
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 79 1C 00");
                if (!pattern.empty())
                    injector::MakeNOP(pattern.get_first(), 5, true);
            }

            // The folder for saves and settings in Documents: the game appends its folder name from a string made by
            // a static constructor. Should it not exist yet, use the same name instead of crashing.
            auto pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 59 59 68 ? ? ? ? FF 15 ? ? ? ? 83 F8 FF");
            if (!pattern.empty())
            {
                static auto UserFilesFolder = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto& folder = *reinterpret_cast<const char**>(regs.esp);
                    if (!folder)
                        folder = "\\Manhunt User Files";
                });
            }

            // Once per frame, in CApp::Idle before the game's own wait
            static auto& limit = FixSettings().Int("MAIN.FPSLimit");
            static FrameLimiter limiter;
            WFP::onGameProcessEvent() += []()
            {
                const int32_t value = limit;
                // 0 leaves the game's own limit; it never runs above 200 fps
                const int32_t fps = value < 0 ? GetRefreshRate() : value;
                limiter.Wait(fps > 0 ? std::clamp(fps, 30, 200) : 0);
            };
        };
    }
} Misc;
