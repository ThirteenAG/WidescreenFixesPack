#pragma once

#include "rawinput.hpp"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <limits>
#include <mutex>
#include <unordered_map>

namespace PunisherMouse
{
    struct Sample
    {
        int64_t x{}, y{}, wheel{};
        std::vector<DIDEVICEOBJECTDATA> buttons;
    };

    inline std::atomic<HWND> gameWindow{};
    inline std::atomic_bool suspended{ true };
    inline std::atomic_uint generation{};
    inline std::mutex sampleMutex;
    inline Sample pending, current;
    inline HWND reader{};
    inline IDirectInputDevice8A** nativeMouse{};
    inline IDirectInputDevice8A* releasedMouse{};
    inline SafetyHookInline poll;
    inline SafetyHookMid readState, readButtons, checkDevice, flushDevice;
    inline uintptr_t stateRead{}, buttonsRead{}, deviceReady{}, deviceFlushed{};
    inline DWORD buttonsDown{};
    inline size_t nextButton{};
    inline std::atomic_bool registered{};
    inline std::atomic<DWORD> inputThread{};
    inline bool useSample{};
    inline bool (*blocksInput)(){};

    inline void Clear()
    {
        const std::lock_guard lock(sampleMutex);
        ++generation;
        pending = {};
    }

    inline void SetWindow(HWND window)
    {
        if (gameWindow.exchange(window) != window)
            Clear();
    }

    inline void Suspend(bool blocked)
    {
        if (suspended.exchange(blocked) != blocked)
            Clear();
    }

    inline void Add(Sample& sample, const RAWINPUTHEADER& header, const RAWMOUSE& mouse, unsigned epoch, DWORD time)
    {
        struct Position { POINT point; RECT screen; bool virtualDesktop; };
        static std::unordered_map<HANDLE, Position> positions;
        static unsigned lastEpoch{};
        if (lastEpoch != epoch)
        {
            lastEpoch = epoch;
            positions.clear();
        }
        if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            const bool virtualDesktop = (mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
            const int width = GetSystemMetrics(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int height = GetSystemMetrics(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            const int left = virtualDesktop ? GetSystemMetrics(SM_XVIRTUALSCREEN) : 0;
            const int top = virtualDesktop ? GetSystemMetrics(SM_YVIRTUALSCREEN) : 0;
            const Position next{ { MulDiv(mouse.lLastX, width, 65535), MulDiv(mouse.lLastY, height, 65535) },
                { left, top, left + width, top + height }, virtualDesktop };
            auto [it, inserted] = positions.try_emplace(header.hDevice, next);
            if (!inserted)
            {
                auto& previous = it->second;
                if (previous.virtualDesktop == next.virtualDesktop && EqualRect(&previous.screen, &next.screen))
                {
                    sample.x += next.point.x - previous.point.x;
                    sample.y += next.point.y - previous.point.y;
                }
                previous = next;
            }
        }
        else
        {
            if (!positions.empty())
                positions.erase(header.hDevice);
            sample.x += mouse.lLastX;
            sample.y += mouse.lLastY;
        }

        const auto flags = mouse.usButtonFlags;
        if (flags & RI_MOUSE_WHEEL)
            sample.wheel += static_cast<SHORT>(mouse.usButtonData);
        constexpr USHORT buttonFlags = RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_1_UP | RI_MOUSE_BUTTON_2_DOWN |
            RI_MOUSE_BUTTON_2_UP | RI_MOUSE_BUTTON_3_DOWN | RI_MOUSE_BUTTON_3_UP | RI_MOUSE_BUTTON_4_DOWN |
            RI_MOUSE_BUTTON_4_UP | RI_MOUSE_BUTTON_5_DOWN | RI_MOUSE_BUTTON_5_UP;
        for (DWORD i = 0; (flags & buttonFlags) && i < 5; ++i)
        {
            if (flags & (1u << (i * 2)))
                sample.buttons.push_back({ DIMOFS_BUTTON0 + i, 0x80, time });
            if (flags & (2u << (i * 2)))
                sample.buttons.push_back({ DIMOFS_BUTTON0 + i, 0, time });
        }
    }

    inline LRESULT CALLBACK ReadInput(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message != WM_INPUT)
            return DefWindowProcW(hwnd, message, wParam, lParam);

        const auto window = gameWindow.load();
        const auto epoch = generation.load();
        const bool active = window && !suspended.load() && GetForegroundWindow() == window;
        Sample batch;
        RAWINPUT first{};
        UINT size = sizeof(first);
        const auto time = static_cast<DWORD>(GetMessageTime());
        if (active && GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &first, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            first.header.dwType == RIM_TYPEMOUSE)
            Add(batch, first.header, first.data.mouse, epoch, time);

        static const bool wow64 = []()
        {
            BOOL result = FALSE;
            return IsWow64Process(GetCurrentProcess(), &result) && result;
        }();
        const size_t headerSize = sizeof(RAWINPUTHEADER) + (wow64 ? 8 : 0);
        const size_t alignment = wow64 ? 8 : alignof(RAWINPUT);
        alignas(8) uint8_t buffer[16384];
        size = sizeof(buffer);
        const UINT count = GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer), &size, sizeof(RAWINPUTHEADER));
        if (active && count != UINT(-1))
        {
            const uint8_t* packet = buffer;
            for (UINT i = 0; i < count; ++i)
            {
                const auto& header = *reinterpret_cast<const RAWINPUTHEADER*>(packet);
                if (header.dwType == RIM_TYPEMOUSE)
                    Add(batch, header, *reinterpret_cast<const RAWMOUSE*>(packet + headerSize), epoch, time);
                packet += (header.dwSize + alignment - 1) & ~(alignment - 1);
            }
        }
        if (batch.x || batch.y || batch.wheel || !batch.buttons.empty())
        {
            const std::lock_guard lock(sampleMutex);
            if (epoch == generation.load() && window == gameWindow.load())
            {
                pending.x += batch.x;
                pending.y += batch.y;
                pending.wheel += batch.wheel;
                pending.buttons.insert(pending.buttons.end(), batch.buttons.begin(), batch.buttons.end());
            }
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    inline void UpdateRegistration()
    {
        if (!reader)
            return;
        static ULONGLONG nextCheck{};
        const auto now = GetTickCount64();
        if (now < nextCheck)
            return;
        nextCheck = now + 250;

        UINT count{};
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1) &&
            (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !count))
            return;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return;
        const auto mouse = std::find_if(devices.begin(), devices.end(), [](const RAWINPUTDEVICE& device)
        {
            return device.usUsagePage == HID_USAGE_PAGE_GENERIC && device.usUsage == HID_USAGE_GENERIC_MOUSE;
        });
        bool ownsMouse;
        if (mouse == devices.end())
        {
            const RAWINPUTDEVICE device{ HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_INPUTSINK, reader };
            ownsMouse = RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;
        }
        else
            ownsMouse = mouse->hwndTarget == reader;
        if (registered != ownsMouse)
        {
            registered = ownsMouse;
            Clear();
        }
    }

    inline bool UseRaw()
    {
        const auto window = gameWindow.load();
        Suspend(!window || GetForegroundWindow() != window || blocksInput());
        UpdateRegistration();
        auto* device = *nativeMouse;
        if (device && registered && releasedMouse != device)
        {
            device->Unacquire();
            releasedMouse = device;
        }
        else if (device && !registered && releasedMouse == device && !suspended && SUCCEEDED(device->Acquire()))
            releasedMouse = nullptr;
        if (!device)
            releasedMouse = nullptr;
        return registered;
    }

    inline char __cdecl Poll()
    {
        inputThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
        const bool raw = UseRaw();
        static unsigned lastEpoch{};
        {
            const std::lock_guard lock(sampleMutex);
            const auto epoch = generation.load();
            current = raw && !suspended ? std::exchange(pending, {}) : Sample{};
            if (lastEpoch != epoch)
            {
                lastEpoch = epoch;
                auto first = current.buttons.begin();
                for (DWORD i = 0; buttonsDown && i < 5; ++i)
                {
                    if (buttonsDown & (1u << i))
                        first = current.buttons.insert(first, { DIMOFS_BUTTON0 + i, 0, GetTickCount() }) + 1;
                }
            }
        }
        nextButton = 0;
        useSample = raw || suspended || !current.buttons.empty();
        const auto result = poll.ccall<char>();
        useSample = false;
        return result;
    }

    inline void Install(bool (*overlayBlocksInput)())
    {
        blocksInput = overlayBlocksInput;
        auto pollEntry = hook::pattern("81 EC 9C 02 00 00 A1 ? ? ? ? 89 84 24 98 02 00 00 A0 ? ? ? ? 53 33 DB");
        auto state = hook::pattern("8D 94 24 94 02 00 00 52 89 84 24 A8 02 00 00 A1 ? ? ? ? 8B 08 6A 14 50 89 1D ? ? ? ? 89 1D ? ? ? ? 89 1D ? ? ? ? FF 51 24 85 C0");
        auto buttons = hook::pattern("A1 ? ? ? ? 53 8D 4C 24 14 51 8D 4C 24 1C 51 6A 14 C7 44 24 20 20 00 00 00 8B 10 50 FF 52 28 85 C0");
        auto check = hook::pattern("A1 ? ? ? ? 6A 01 8D 54 24 08 52 6A 00 6A 14 C7 44 24 14 00 00 00 00 8B 08 50 FF 51 28 3D 1E 00 07 80");
        auto flush = hook::pattern("A1 ? ? ? ? 3B C3 74 17 53 8D 54 24 08 52 53 6A 14 C7 44 24 14 FF FF FF FF 8B 08 50 FF 51 28 5B 59 C3");

        nativeMouse = *reinterpret_cast<IDirectInputDevice8A***>(state.get_first<uint8_t>(16));
        stateRead = reinterpret_cast<uintptr_t>(state.get_first(46));
        buttonsRead = reinterpret_cast<uintptr_t>(buttons.get_first(32));
        deviceReady = reinterpret_cast<uintptr_t>(check.get_first(101));
        deviceFlushed = reinterpret_cast<uintptr_t>(flush.get_first(32));

        // The native poll still applies sensitivity, camera and cursor coordinates, wheel timing, and button state.
        readState = safetyhook::create_mid(state.get_first(), [](SafetyHookContext& regs)
        {
            if (!useSample)
                return;
            auto& input = *reinterpret_cast<DIMOUSESTATE2*>(regs.esp + 0x294);
            const auto axis = [](int64_t value)
            {
                return static_cast<LONG>(std::clamp<int64_t>(value, std::numeric_limits<LONG>::min(), std::numeric_limits<LONG>::max()));
            };
            input = {};
            input.lX = axis(current.x);
            input.lY = axis(current.y);
            input.lZ = axis(current.wheel);
            regs.eax = DI_OK;
            regs.eip = stateRead;
        });
        readButtons = safetyhook::create_mid(buttons.get_first(), [](SafetyHookContext& regs)
        {
            if (!useSample)
                return;
            const auto count = std::min<size_t>(32, current.buttons.size() - nextButton);
            static_assert(sizeof(DIDEVICEOBJECTDATA) == 20);
            auto* events = reinterpret_cast<DIDEVICEOBJECTDATA*>(regs.esp + 0x14);
            for (size_t i = 0; i < count; ++i)
            {
                events[i] = current.buttons[nextButton++];
                const DWORD bit = 1u << (events[i].dwOfs - DIMOFS_BUTTON0);
                buttonsDown = (buttonsDown & ~bit) | ((events[i].dwData & 0x80) ? bit : 0);
            }
            *reinterpret_cast<DWORD*>(regs.esp + 0x10) = static_cast<DWORD>(count);
            regs.eax = DI_OK;
            regs.eip = buttonsRead;
        });
        // The loading screen presents from a timer thread, which also runs this check: only the input thread
        // touches the devices and the raw input registration
        checkDevice = safetyhook::create_mid(check.get_first(), [](SafetyHookContext& regs)
        {
            const auto thread = inputThread.load(std::memory_order_relaxed);
            if (thread && thread != GetCurrentThreadId())
            {
                if (registered.load())
                    regs.eip = deviceReady;
                return;
            }
            if (!UseRaw())
                return;
            if (*reinterpret_cast<const uint8_t*>(regs.esp + 0x10))
                Clear();
            regs.eip = deviceReady;
        });
        flushDevice = safetyhook::create_mid(flush.get_first(), [](SafetyHookContext& regs)
        {
            buttonsDown = 0;
            if (registered)
            {
                Clear();
                regs.eax = DI_OK;
                regs.eip = deviceFlushed;
            }
        });
        poll = safetyhook::create_inline(pollEntry.get_first(), Poll);
        reader = CreateRawInputWindow(L"ThePunisherRawMouse", ReadInput);
        UpdateRegistration();
        MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    }
}
