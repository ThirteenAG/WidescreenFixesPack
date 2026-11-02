module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"

export module Focus;

import ComVars;

export namespace HD2Focus
{
    inline bool suspended{};
    inline std::atomic<bool> pauseOnFocusLoss{ true };
    inline std::atomic<DWORD> messageThread{};
    inline SafetyHookMid waitBranch;
    inline SafetyHookMid activationBranches[2];
    inline bool pausedTitle{};

    inline DWORD WINAPI SuspendWorker(HANDLE thread)
    {
        if (!pauseOnFocusLoss.load(std::memory_order_relaxed))
            return 0;
        const auto result = SuspendThread(thread);
        suspended = result != DWORD(-1);
        return result;
    }

    inline DWORD WINAPI ResumeWorker(HANDLE thread)
    {
        if (!suspended)
            return 0;
        suspended = false;
        return ResumeThread(thread);
    }

    inline void Init(HMODULE graphics)
    {
        WindowCursor::Initialize(FixSettings());
        auto suspend = hook::module_pattern(graphics, "A1 ? ? ? ? 85 C0 74 07 50 FF 15 ? ? ? ? C6 05 ? ? ? ? 01");
        FocusWorker.SetAddress(*suspend.get_first<HANDLE*>(1));
        auto resume = hook::module_pattern(graphics, "A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 50 FF 15 ? ? ? ? E9");
        auto title = hook::module_pattern(graphics, "88 88 ? ? ? ? 40 84 C9 75 EF F6 05 ? ? ? ? 20 74 ? 83 C9 FF 33 C0");
        FocusTitle.SetAddress(*title.get_first<char*>(2));
        auto window = hook::module_pattern(graphics, "8B 15 ? ? ? ? 68 ? ? ? ? 52 FF 15 ? ? ? ? A1 ? ? ? ? 85 C0 74 07 50");
        FocusWindow.SetAddress(*window.get_first<HWND*>(2));
        injector::MakeCALL(suspend.get_first(10), SuspendWorker, true);
        injector::MakeNOP(suspend.get_first(15), 1, true);
        injector::MakeCALL(resume.get_first(14), ResumeWorker, true);
        injector::MakeNOP(resume.get_first(19), 1, true);

        auto shortBranch = hook::module_pattern(graphics, "F6 05 ? ? ? ? 20 74 ? 83 C9 FF 33 C0 BF ? ? ? ? F2 AE F7 D1 49 81 F9 F6 00 00 00");
        auto longBranch = hook::module_pattern(graphics, "F6 05 ? ? ? ? 20 0F 84 ? ? ? ? 83 C9 FF 33 C0 BF ? ? ? ? F2 AE F7 D1 49 81 F9 F6 00 00 00");
        const auto activation = [](SafetyHookContext& context)
        {
            const bool pause = !(context.eflags & 0x40) && pauseOnFocusLoss.load(std::memory_order_relaxed);
            pausedTitle = pause;
            if (!pause)
                context.eflags |= 0x40;
        };
        activationBranches[0] = safetyhook::create_mid(shortBranch.get_first(7), activation);
        activationBranches[1] = safetyhook::create_mid(longBranch.get_first(7), activation);

        auto loop = hook::module_pattern(graphics, "38 1D ? ? ? ? 0F 84 ? ? ? ? 38 1D ? ? ? ? 0F 85 ? ? ? ? F6 05 ? ? ? ? 20 0F 84 ? ? ? ? FF 15 ? ? ? ? EB");
        waitBranch = safetyhook::create_mid(loop.get_first(31), [](SafetyHookContext& context)
        {
            messageThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
            if (context.eflags & 0x40)
                return;
            const bool pause = pauseOnFocusLoss.load(std::memory_order_relaxed);
            if (FocusWorker.get() && pause && !suspended)
                SuspendWorker(FocusWorker.get());
            else if (FocusWorker.get() && !pause && suspended)
                ResumeWorker(FocusWorker.get());
            if (!pause)
            {
                if (pausedTitle && FocusWindow.get())
                    SetWindowTextA(FocusWindow.get(), FocusTitle.get_ptr());
                pausedTitle = false;
                context.eflags |= 0x40;
            }
        });
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            pauseOnFocusLoss.store(FixSettings().Get("MAIN.PauseOnFocusLoss") != "0", std::memory_order_relaxed);
            const auto thread = messageThread.load(std::memory_order_relaxed);
            if (thread)
                PostThreadMessageW(thread, WM_NULL, 0, 0);
        });
        pauseOnFocusLoss.store(FixSettings().Get("MAIN.PauseOnFocusLoss") != "0", std::memory_order_relaxed);
    }
}
