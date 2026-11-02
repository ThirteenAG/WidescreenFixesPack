module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/Display.hpp"
#include <intrin.h>

export module Window;

import ComVars;

export namespace THPS3Display
{
    SafetyHookInline LogoHook;
    inline std::atomic<int> FrameLimit{ 60 };

    inline void UpdateFrameLimit()
    {
        int limit = FixSettings().Int("MAIN.FPSLimit");
        if (limit == -1)
        {
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            limit = EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1
                ? static_cast<int>(mode.dmDisplayFrequency) : 60;
        }
        FrameLimit.store(limit > 0 ? std::clamp(limit, 30, 1000) : 0, std::memory_order_relaxed);
    }

    inline void LimitFrameRate()
    {
        struct Pacer
        {
            HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            double frequency = []() { LARGE_INTEGER value{}; QueryPerformanceFrequency(&value); return static_cast<double>(value.QuadPart); }();
            double next = 0.0;
            int previous = 0;
            ~Pacer() { if (timer) CloseHandle(timer); }
            double Now() const { LARGE_INTEGER value{}; QueryPerformanceCounter(&value); return static_cast<double>(value.QuadPart); }
        };
        static Pacer pacer;
        const int limit = FrameLimit.load(std::memory_order_relaxed);
        if (limit <= 0 || pacer.frequency <= 0.0)
        {
            pacer.next = 0.0;
            pacer.previous = 0;
            return;
        }
        if (pacer.previous != limit)
        {
            pacer.next = 0.0;
            pacer.previous = limit;
        }
        const double period = pacer.frequency / limit;
        const double now = pacer.Now();
        if (pacer.next == 0.0 || now > pacer.next + period)
            pacer.next = now + period;
        const double seconds = (pacer.next - now) / pacer.frequency;
        constexpr double spinTime = 0.0015;
        if (seconds > spinTime)
        {
            LARGE_INTEGER due{};
            due.QuadPart = -static_cast<LONGLONG>((seconds - spinTime) * 10000000.0);
            if (pacer.timer && SetWaitableTimer(pacer.timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(pacer.timer, INFINITE);
            else
                Sleep(static_cast<DWORD>((seconds - spinTime) * 1000.0));
        }
        while (pacer.Now() < pacer.next)
            YieldProcessor();
        pacer.next += period;
    }

    // Startup logos and the intro movie (the intro stays in Options > Movies)
    std::array<uintptr_t, 4> StartupLogoReturns{};
    char __cdecl Logo(const char* filename, int mode)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        if (std::find(StartupLogoReturns.begin(), StartupLogoReturns.end(), caller) != StartupLogoReturns.end())
            return 1;
        return LogoHook.ccall<char>(filename, mode);
    }

    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        UpdateFrameLimit();
        settings.OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        auto present = hook::pattern("8B 44 24 04 8B 0D ? ? ? ? 55 33 ED 8B 44 01 24 56 55 55 55 3B C5 55");
        static auto presentHook = safetyhook::create_mid(present.get_first(), [](SafetyHookContext&)
        {
            LimitFrameRate();
        });
        WindowedModeWrapper::EnableDpiAwareness();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        TonyDisplay::Window(0, width, height);
        if (mode != 0)
        {
            // The game chooses render dimensions from either the requested
            // client dimensions or a native display-mode record according to
            // this flag. Clear it before mode selection, not just when the
            // renderer later creates the windowed device: custom window sizes
            // have no matching display-mode record.
            auto selection = hook::pattern("8B 43 10 83 E0 01 84 C0 75 ? 6A 00 E8 ? ? ? ? 83 C4 04");
            static auto selectionHook = safetyhook::create_mid(selection.get_first(), [](SafetyHookContext& regs)
            {
                *reinterpret_cast<uint32_t*>(regs.ebx + 0x10) &= ~1u;
            });
            auto pattern = hook::pattern("A0 ? ? ? ? 8B 4C 24 20 84 C3 89 1D ? ? ? ? 74 47 8B 15 ? ? ? ? A1");
            ModeFlags.SetAddress(*pattern.get_first<uint8_t*>(1));
            static auto windowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                ModeFlags = ModeFlags.get() & ~1;
            });
            TonyDisplay::Window(mode, width, height);
        }
        if (settings.Bool("MAIN.SkipIntro"))
        {
            const char* callers[] = {
                "56 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 84 C0 0F 85 ? ? ? ? 89 7C 24 1C C6 84 24 ? ? ? ? 0D",
                "56 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 84 C0 0F 85 ? ? ? ? 89 7C 24 1C C6 84 24 ? ? ? ? 19",
                "56 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 84 C0 0F 85 ? ? ? ? 89 7C 24 1C C6 84 24 ? ? ? ? 25",
                "56 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 84 C0 0F 85 ? ? ? ? 89 7C 24 1C C6 84 24 ? ? ? ? 31"
            };
            std::array<uint8_t*, 4> sites{};
            for (size_t i = 0; i < sites.size(); ++i)
            {
                sites[i] = hook::pattern(callers[i]).get_first<uint8_t>();
                StartupLogoReturns[i] = reinterpret_cast<uintptr_t>(sites[i] + 11);
            }
            auto* playback = injector::GetBranchDestination(sites[2] + 6).get<void>();
            LogoHook = safetyhook::create_inline(playback, Logo);
        }
    }
}

export void InstallResolution()
{
    hook::pattern pattern("");
    //Resolution
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? B0 01 5F 5E"); //40B349
    dword_851084.SetAddress(*pattern.get_first<int32_t*>(2));
    dword_851088.SetAddress(*pattern.get_first<int32_t*>(12));
    injector::MakeNOP(pattern.get_first(-12), 2, true);

    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_851084 = Screen.Width;
            dword_851088 = Screen.Height;
        }
    };
    injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(20));
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 2D"); //40B5B8
    injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(20));
    auto firstMode = hook::pattern("A3 ? ? ? ? 8B 04 AE 85 C0 74 0F").get_first();
    auto secondMode = hook::pattern("A3 ? ? ? ? 8B 04 AE 85 C0 74 0B").get_first();
    injector::MakeInline<SetResHook>(firstMode);
    injector::MakeInline<SetResHook>(secondMode);
}
