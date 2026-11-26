#pragma once

#include "rawinput.hpp"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <limits>
#include <mutex>
#include <unordered_map>

namespace OverdoseMouse
{
    struct Movement
    {
        int64_t x{}, y{}, wheel{};
        unsigned changed{}, down{}, pressed{}, released{}, wheelDirections{};

        void Add(const Movement& next)
        {
            x += next.x;
            y += next.y;
            wheel += next.wheel;
            down = (down & ~next.changed) | next.down;
            changed |= next.changed;
            pressed |= next.pressed;
            released |= next.released;
            wheelDirections |= next.wheelDirections;
        }

        void ApplyEdges(uint32_t* input) const
        {
            for (unsigned i = 0; changed && i < 5; ++i)
            {
                const unsigned bit = 1u << i;
                if (changed & bit)
                    input[i] = (input[i] & ~1u) | ((down & bit) ? 1u : 0u) |
                        ((pressed & bit) ? 2u : 0u) | ((released & bit) ? 4u : 0u);
            }
            if (wheelDirections & 1u)
                input[8] |= 6u;
            if (wheelDirections & 2u)
                input[9] |= 6u;
        }
    };

    inline SafetyHookInline pollMouse;
    inline SafetyHookMid acquireMouse, readState, readButtons;
    inline uintptr_t acquireDone{}, stateDone{}, buttonsDone{};
    inline std::atomic<HWND> window{};
    inline std::atomic_bool suspended{ true };
    inline std::atomic<unsigned> generation{};
    inline std::mutex inputMutex;
    inline Movement pending, sample;
    inline HWND reader{};
    inline bool registered{}, useSample{};
    inline bool (*blocksInput)(){};

    inline void Clear()
    {
        const std::lock_guard lock(inputMutex);
        pending = {};
        ++generation;
    }

    inline void SetWindow(HWND next)
    {
        if (window.exchange(next) != next)
            Clear();
    }

    inline void Suspend(bool blocked)
    {
        if (suspended.exchange(blocked) != blocked)
            Clear();
    }

    inline void AddReport(Movement& batch, const RAWMOUSE& mouse, HANDLE device, unsigned epoch)
    {
        struct AbsolutePosition
        {
            POINT point;
            RECT desktop;
            bool virtualDesktop;
        };
        static std::unordered_map<HANDLE, AbsolutePosition> positions;
        static unsigned previousEpoch{};
        if (epoch != previousEpoch)
        {
            positions.clear();
            previousEpoch = epoch;
        }
        if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            const bool virtualDesktop = (mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
            const int width = GetSystemMetrics(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int height = GetSystemMetrics(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            const int left = virtualDesktop ? GetSystemMetrics(SM_XVIRTUALSCREEN) : 0;
            const int top = virtualDesktop ? GetSystemMetrics(SM_YVIRTUALSCREEN) : 0;
            const AbsolutePosition position{ { MulDiv(mouse.lLastX, width, 65535), MulDiv(mouse.lLastY, height, 65535) },
                { left, top, left + width, top + height }, virtualDesktop };
            auto [it, inserted] = positions.try_emplace(device, position);
            if (!inserted)
            {
                auto& last = it->second;
                if (last.virtualDesktop == position.virtualDesktop && EqualRect(&last.desktop, &position.desktop))
                {
                    batch.x += position.point.x - last.point.x;
                    batch.y += position.point.y - last.point.y;
                }
                last = position;
            }
        }
        else
        {
            if (!positions.empty())
                positions.erase(device);
            batch.x += mouse.lLastX;
            batch.y += mouse.lLastY;
        }

        const auto flags = mouse.usButtonFlags;
        for (unsigned i = 0; flags && i < 5; ++i)
        {
            const unsigned button = 1u << i;
            if (flags & (1u << (i * 2)))
            {
                batch.changed |= button;
                batch.down |= button;
                batch.pressed |= button;
            }
            if (flags & (2u << (i * 2)))
            {
                batch.changed |= button;
                batch.down &= ~button;
                batch.released |= button;
            }
        }
        if (flags & RI_MOUSE_WHEEL)
        {
            const auto wheel = static_cast<SHORT>(mouse.usButtonData);
            batch.wheel += wheel;
            batch.wheelDirections |= wheel > 0 ? 1u : wheel < 0 ? 2u : 0u;
        }
    }

    inline LRESULT CALLBACK ReadInput(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message != WM_INPUT)
            return DefWindowProcW(hwnd, message, wParam, lParam);

        const auto game = window.load();
        const auto epoch = generation.load();
        const bool active = game && !suspended.load() && GetForegroundWindow() == game;
        Movement batch;
        RAWINPUT first{};
        UINT size = sizeof(first);
        if (active && GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &first, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            first.header.dwType == RIM_TYPEMOUSE)
            AddReport(batch, first.data.mouse, first.header.hDevice, epoch);

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
                const auto& input = *reinterpret_cast<const RAWINPUTHEADER*>(packet);
                if (input.dwType == RIM_TYPEMOUSE)
                    AddReport(batch, *reinterpret_cast<const RAWMOUSE*>(packet + headerSize), input.hDevice, epoch);
                packet += (input.dwSize + alignment - 1) & ~(alignment - 1);
            }
        }
        if (batch.x || batch.y || batch.changed || batch.wheelDirections)
        {
            const std::lock_guard lock(inputMutex);
            if (epoch == generation.load() && game == window.load())
                pending.Add(batch);
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
        nextCheck = now + 500;

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
        bool ownsMouse = false;
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

    inline char __fastcall Poll(uint8_t* mouse, void*)
    {
        const auto game = window.load();
        Suspend(!game || GetForegroundWindow() != game || blocksInput());
        UpdateRegistration();
        auto& acquired = mouse[0x54];
        useSample = registered || suspended.load();
        if (useSample && acquired)
        {
            auto device = *reinterpret_cast<IDirectInputDevice8A**>(mouse + 0x50);
            device->Unacquire();
            acquired = 0;
        }
        auto buttons = reinterpret_cast<uint32_t*>(mouse);
        static unsigned lastEpoch{};
        const auto epoch = generation.load();
        if (epoch != lastEpoch)
        {
            lastEpoch = epoch;
            for (unsigned i = 0; i < 8; ++i)
                buttons[i] = buttons[i] & 1 ? 4u : 0u;
        }
        if (useSample)
        {
            const std::lock_guard lock(inputMutex);
            sample = std::exchange(pending, {});
            acquired = 1;
        }
        const auto result = pollMouse.unsafe_thiscall<char>(mouse);
        if (useSample)
            acquired = 0;
        return result;
    }

    inline void Install(bool (*overlayBlocksInput)())
    {
        blocksInput = overlayBlocksInput;
        auto acquire = hook::pattern("8B 45 00 55 FF 50 1C 85 C0 7C 04 C6 46 54 01 8D 4C 24 10 51");
        auto poll = hook::pattern("83 EC 28 56 8B F1 C6 46 48 01 A1 ? ? ? ? 8A 48 2C 84 C9");
        auto state = hook::pattern("8B 46 50 8B 10 8D 4C 24 18 51 6A 14 50 FF 52 24 85 C0");
        auto buttons = hook::pattern("8B 46 50 55 8D 4C 24 14 51 8B 4E 58 51 6A 14 C7 44 24 20 1E 00 00 00 8B 10 50 FF 52 28 3B C5");

        acquireDone = reinterpret_cast<uintptr_t>(acquire.get_first(15));
        stateDone = reinterpret_cast<uintptr_t>(state.get_first(16));
        buttonsDone = reinterpret_cast<uintptr_t>(buttons.get_first(29));

        // Keep the game's own focus state, cursor integration and bounds. Feed its mouse state and button flags here.
        acquireMouse = safetyhook::create_mid(acquire.get_first(), [](SafetyHookContext& regs)
        {
            UpdateRegistration();
            if (registered)
                regs.eip = acquireDone;
        });
        readState = safetyhook::create_mid(state.get_first(), [](SafetyHookContext& regs)
        {
            if (!useSample)
                return;
            auto& input = *reinterpret_cast<DIMOUSESTATE2*>(regs.esp + 0x18);
            const auto axis = [](int64_t value)
            {
                return static_cast<LONG>(std::clamp<int64_t>(value, std::numeric_limits<LONG>::min(), std::numeric_limits<LONG>::max()));
            };
            input = {};
            input.lX = axis(sample.x);
            input.lY = axis(sample.y);
            input.lZ = axis(sample.wheel);
            regs.eax = DI_OK;
            regs.eip = stateDone;
        });
        readButtons = safetyhook::create_mid(buttons.get_first(), [](SafetyHookContext& regs)
        {
            if (!useSample)
                return;
            sample.ApplyEdges(reinterpret_cast<uint32_t*>(regs.esi));
            // There are no DirectInput events to process for this sample.
            *reinterpret_cast<uint32_t*>(regs.esp + 0x10) = 0;
            regs.eax = DI_OK;
            regs.eip = buttonsDone;
        });
        pollMouse = safetyhook::create_inline(poll.get_first(), Poll);

        reader = CreateRawInputWindow(L"TotalOverdoseRawMouse", ReadInput);
        UpdateRegistration();
        MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    }
}
