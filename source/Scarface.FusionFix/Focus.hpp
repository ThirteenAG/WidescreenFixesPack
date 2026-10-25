#pragma once

namespace ScarfaceFocus
{
    inline SafetyHookInline PumpHook;
    inline const int* Pause = nullptr;
    inline HWND* Window = nullptr;
    inline void** Flow = nullptr;
    inline uint32_t(__cdecl* Clock)() = nullptr;
    inline char(__cdecl* SoundActive)(char) = nullptr;

    inline bool Background()
    {
        return Window && *Window && IsWindow(*Window) && GetForegroundWindow() != *Window;
    }

    inline HWND __cdecl Pump()
    {
        auto result = PumpHook.unsafe_call<HWND>();
        if (!Window || !*Window)
            return result;
        bool waited = false;
        while (Pause && *Pause && Background())
        {
            waited = true;
            if (SoundActive)
                SoundActive(0);
            // Keep close, activation and device messages moving while simulation waits.
            if (MsgWaitForMultipleObjectsEx(0, nullptr, 50, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_FAILED)
            {
                OutputDebugStringA("Scarface: background message wait failed.\n");
                break;
            }
            result = PumpHook.unsafe_call<HWND>();
        }
        if (waited && Flow && *Flow && reinterpret_cast<uintptr_t>(*Flow) != 0xDDDDDDDD && Clock)
        {
            // The next FlowTime must exclude the interval spent outside simulation.
            auto previousTime = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(*Flow) + 0x38);
            *previousTime = Clock();
        }
        if (Window && *Window && SoundActive)
            SoundActive(!Background() || (Pause && !*Pause));
        return result;
    }

    inline bool Install(const int& pause, HMODULE module = GetModuleHandle(nullptr))
    {
        Pause = &pause;
        if (PumpHook)
            return true;
        const auto base = reinterpret_cast<uintptr_t>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto begin = base + nt->OptionalHeader.SizeOfHeaders;
        const auto end = base + nt->OptionalHeader.SizeOfImage;
        auto pump = hook::range_pattern(begin, end, "83 C4 04 E8 ? ? ? ? E8 ? ? ? ? 83 F8 12");
        auto clock = hook::range_pattern(begin, end, "E8 ? ? ? ? 8B C8 2B 4D 38 80 7C 24 ? 00 89 45 38");
        auto flow = hook::range_pattern(begin, end, "A1 ? ? ? ? 80 B8 77 01 00 00 00 75 ? B9 80 00 00 00");
        auto sound = hook::range_pattern(begin, end, "6A 01 E8 ? ? ? ? 6A 01 E8 ? ? ? ? 83 C4 08 53 57 56 55");
        auto window = hook::range_pattern(begin, end, "A3 ? ? ? ? 5B 74 0F");
        if (pump.size() != 1 || clock.size() != 1 || flow.size() != 1 || sound.size() != 1 || window.size() != 1)
        {
            OutputDebugStringA("Scarface: background pause patterns unavailable.\n");
            return false;
        }
        Window = *window.get_first<HWND*>(1);
        Flow = *flow.get_first<void**>(1);
        Clock = reinterpret_cast<decltype(Clock)>(injector::GetBranchDestination(clock.get_first()).as_int());
        SoundActive = reinterpret_cast<decltype(SoundActive)>(injector::GetBranchDestination(sound.get_first(2)).as_int());
        PumpHook = safetyhook::create_inline(injector::GetBranchDestination(pump.get_first(3)).as_int(), Pump);
        if (!PumpHook)
            OutputDebugStringA("Scarface: background pause hook unavailable.\n");
        return bool(PumpHook);
    }
}
