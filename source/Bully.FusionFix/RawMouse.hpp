#pragma once

#include <atomic>
#include <limits>

namespace BullyRawMouse
{
    inline SafetyHookMid MouseHook, SampleHook;
    inline const int* Enabled = nullptr;
    inline uintptr_t Resume = 0;
    inline std::atomic<int64_t> DeltaX{ 0 }, DeltaY{ 0 };
    inline std::atomic<HWND> InputWindow{ nullptr };
    inline std::atomic<bool> SeenRelativeMovement{ false };

    inline void Clear()
    {
        DeltaX.exchange(0, std::memory_order_relaxed);
        DeltaY.exchange(0, std::memory_order_relaxed);
        SeenRelativeMovement.store(false, std::memory_order_release);
    }

    inline void Attach(HWND window)
    {
        if (!window || !Enabled || !*Enabled)
        {
            InputWindow.store(nullptr, std::memory_order_relaxed);
            Clear();
            return;
        }
        static ULONGLONG nextCheck = 0;
        const auto now = GetTickCount64();
        if (now < nextCheck)
            return;
        nextCheck = now + 250;
        UINT count = 0;
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
        {
            InputWindow.store(nullptr, std::memory_order_relaxed);
            Clear();
            return;
        }
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
        {
            InputWindow.store(nullptr, std::memory_order_relaxed);
            Clear();
            return;
        }
        for (UINT i = 0; i < count; ++i)
        {
            if (devices[i].usUsagePage == 1 && devices[i].usUsage == 2)
            {
                const auto target = devices[i].hwndTarget ? devices[i].hwndTarget : window;
                if (target != window)
                {
                    InputWindow.store(nullptr, std::memory_order_relaxed);
                    Clear();
                }
                else if (InputWindow.exchange(window, std::memory_order_relaxed) != window)
                    Clear();
                return;
            }
        }
        RAWINPUTDEVICE device{ 1, 2, 0, window };
        InputWindow.store(RegisterRawInputDevices(&device, 1, sizeof(device)) ? window : nullptr, std::memory_order_relaxed);
        Clear();
    }

    inline void Message(HWND window, UINT message, LPARAM lParam, bool blocked)
    {
        if (message == WM_KILLFOCUS || message == WM_NCDESTROY)
        {
            Clear();
            if (message == WM_NCDESTROY)
                InputWindow.store(nullptr, std::memory_order_relaxed);
        }
        if (blocked)
            Clear();
        if (message != WM_INPUT || window != InputWindow.load(std::memory_order_relaxed) || blocked ||
            GetForegroundWindow() != window || !Enabled || !*Enabled)
            return;
        RAWINPUT input{};
        UINT size = sizeof(input);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == UINT(-1) ||
            input.header.dwType != RIM_TYPEMOUSE || (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
            return;
        DeltaX.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
        DeltaY.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
        if (input.data.mouse.lLastX || input.data.mouse.lLastY)
            SeenRelativeMovement.store(true, std::memory_order_release);
    }

    inline void Install(const int& enabled)
    {
        auto mouse = hook::pattern("8B 06 3B C3 C6 44 24 14 01 74 05 83 C0 FF 89 06 DB 06 D9 E1");
        Enabled = &enabled;
        Resume = reinterpret_cast<uintptr_t>(mouse.get_first(16));
        SampleHook = safetyhook::create_mid(mouse.get_first(), [](SafetyHookContext& regs)
        {
            const auto window = InputWindow.load(std::memory_order_relaxed);
            if (!Enabled || !*Enabled || !window || GetForegroundWindow() != window)
            {
                Clear();
                return;
            }
            if (!SeenRelativeMovement.load(std::memory_order_acquire))
                return;
            auto* axes = reinterpret_cast<int32_t*>(regs.esi);
            constexpr auto low = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
            constexpr auto high = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
            axes[0] = static_cast<int32_t>(std::clamp(DeltaX.exchange(0, std::memory_order_relaxed), low, high));
            axes[1] = static_cast<int32_t>(std::clamp(DeltaY.exchange(0, std::memory_order_relaxed), low, high));
        });
        MouseHook = safetyhook::create_mid(mouse.get_first(11), [](SafetyHookContext& regs)
        {
            if (Enabled && *Enabled)
            {
                *reinterpret_cast<int32_t*>(regs.esi) = int32_t(regs.eax);
                regs.eip = Resume;
            }
        });
        if (!SampleHook || !MouseHook)
            std::terminate();
    }
}
