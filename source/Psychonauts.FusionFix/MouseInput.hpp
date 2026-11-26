#pragma once

#include "rawinput.hpp"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <array>
#include <bit>
#include <limits>
#include <mutex>
#include <unordered_map>

namespace PsychonautsMouse
{
    constexpr DWORD BufferSize = 4096;
    constexpr unsigned ButtonCount = 5;
    constexpr int FirstButtonAction = 258;

    struct Batch
    {
        int64_t x{}, y{};
        std::vector<uint8_t> buttons;
    };

    inline std::mutex pendingMutex;
    inline Batch pending;
    inline std::atomic<unsigned> generation{};
    inline std::atomic<HWND> gameWindow{};
    inline std::atomic_bool suspended{};
    inline HWND reader{};
    inline bool rawRegistered{};
    inline SafetyHookInline acquireMouse;
    inline uint8_t* nativeMouseAcquired{};
    inline int(__cdecl* setButton)(int, uint8_t){};

    inline void Clear()
    {
        const std::lock_guard lock(pendingMutex);
        pending.x = pending.y = 0;
        pending.buttons.clear();
        generation.fetch_add(1, std::memory_order_relaxed);
    }

    inline void SetWindow(HWND window)
    {
        if (gameWindow.exchange(window) != window)
            Clear();
    }

    inline void Suspend(bool blocked)
    {
        if (suspended.exchange(blocked, std::memory_order_relaxed) != blocked)
            Clear();
    }

    inline void Append(Batch& batch, const RAWMOUSE& mouse, HANDLE device, unsigned epoch)
    {
        struct AbsoluteSample
        {
            POINT position;
            RECT desktop;
            bool virtualDesktop;
        };
        static std::unordered_map<HANDLE, AbsoluteSample> lastPositions;
        static unsigned previousEpoch{};
        if (previousEpoch != epoch)
        {
            lastPositions.clear();
            previousEpoch = epoch;
        }
        if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            const bool virtualDesktop = (mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
            const int width = GetSystemMetrics(virtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int height = GetSystemMetrics(virtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            const int left = virtualDesktop ? GetSystemMetrics(SM_XVIRTUALSCREEN) : 0;
            const int top = virtualDesktop ? GetSystemMetrics(SM_YVIRTUALSCREEN) : 0;
            const AbsoluteSample sample{ { MulDiv(mouse.lLastX, width, 65535), MulDiv(mouse.lLastY, height, 65535) },
                { left, top, left + width, top + height }, virtualDesktop };
            auto [it, inserted] = lastPositions.try_emplace(device, sample);
            if (!inserted)
            {
                auto& previous = it->second;
                if (previous.virtualDesktop == sample.virtualDesktop && EqualRect(&previous.desktop, &sample.desktop))
                {
                    batch.x += sample.position.x - previous.position.x;
                    batch.y += sample.position.y - previous.position.y;
                }
                previous = sample;
            }
        }
        else
        {
            if (!lastPositions.empty())
                lastPositions.erase(device);
            batch.x += mouse.lLastX;
            batch.y += mouse.lLastY;
        }
        if (mouse.usButtonFlags)
        {
            for (unsigned i = 0; i < ButtonCount * 2; ++i)
                if (mouse.usButtonFlags & (1u << i))
                    batch.buttons.push_back(static_cast<uint8_t>(i));
        }
    }

    inline LRESULT CALLBACK RawInputProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message != WM_INPUT)
            return DefWindowProcW(hwnd, message, wParam, lParam);

        const auto window = gameWindow.load();
        const auto epoch = generation.load(std::memory_order_relaxed);
        const bool active = window && !suspended.load(std::memory_order_relaxed) && GetForegroundWindow() == window;

        Batch batch;
        RAWINPUT first{};
        UINT size = sizeof(first);
        if (active &&
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &first, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            first.header.dwType == RIM_TYPEMOUSE)
            Append(batch, first.data.mouse, first.header.hDevice, epoch);

        static const bool wow64 = []()
        {
            BOOL result = FALSE;
            return sizeof(void*) == 4 && IsWow64Process(GetCurrentProcess(), &result) && result;
        }();
        const size_t header = wow64 ? 24 : sizeof(RAWINPUTHEADER);
        const size_t alignment = wow64 ? 8 : sizeof(void*);
        alignas(8) uint8_t buffer[8192];
        size = sizeof(buffer);
        const UINT count = GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer), &size, sizeof(RAWINPUTHEADER));
        if (active && count != UINT(-1))
        {
            const uint8_t* packet = buffer;
            for (UINT i = 0; i < count; ++i)
            {
                const auto& input = *reinterpret_cast<const RAWINPUTHEADER*>(packet);
                if (input.dwType == RIM_TYPEMOUSE)
                    Append(batch, *reinterpret_cast<const RAWMOUSE*>(packet + header), input.hDevice, epoch);
                packet += (input.dwSize + alignment - 1) & ~(alignment - 1);
            }
        }
        if (batch.x || batch.y || !batch.buttons.empty())
        {
            const std::lock_guard lock(pendingMutex);
            if (epoch == generation.load(std::memory_order_relaxed) && window == gameWindow.load())
            {
                pending.x += batch.x;
                pending.y += batch.y;
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
        nextCheck = now + 1000;

        UINT count{};
        if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return;
        std::vector<RAWINPUTDEVICE> devices(count);
        if (count && GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
            return;
        const auto mouse = std::find_if(devices.begin(), devices.begin() + count, [](const RAWINPUTDEVICE& device)
        {
            return device.usUsagePage == HID_USAGE_PAGE_GENERIC && device.usUsage == HID_USAGE_GENERIC_MOUSE;
        });
        bool registered = false;
        if (mouse == devices.begin() + count)
        {
            const RAWINPUTDEVICE device{ HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_INPUTSINK, reader };
            registered = RegisterRawInputDevices(&device, 1, sizeof(device)) != FALSE;
        }
        else
            registered = mouse->hwndTarget == reader;
        if (registered != rawRegistered)
        {
            Clear();
            rawRegistered = registered;
            *nativeMouseAcquired = 0;
        }
    }

    inline void __cdecl Acquire(char acquire)
    {
        UpdateRegistration();
        if (!rawRegistered)
        {
            acquireMouse.ccall<void>(acquire);
            return;
        }
        if (!acquire)
        {
            *nativeMouseAcquired = 0;
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        }
        else if (const auto window = gameWindow.load(); window && GetForegroundWindow() == window && !*nativeMouseAcquired)
        {
            *nativeMouseAcquired = 1;
            SetCursor(nullptr);
        }
    }

    inline HRESULT WINAPI Read(IDirectInputDevice8A* device, DWORD eventSize, DIDEVICEOBJECTDATA* events,
        DWORD* eventCount, DWORD flags)
    {
        UpdateRegistration();
        static unsigned previousGeneration{};
        const auto epoch = generation.load(std::memory_order_relaxed);
        if (previousGeneration != epoch)
        {
            previousGeneration = epoch;
            for (unsigned button = 0; button < ButtonCount; ++button)
                setButton(FirstButtonAction + button, 0);
        }
        Batch batch;
        HRESULT result = DI_OK;
        if (rawRegistered)
        {
            const std::lock_guard lock(pendingMutex);
            std::swap(batch, pending);
        }
        else
        {
            if (!*nativeMouseAcquired)
                Acquire(1);
            static thread_local std::array<DIDEVICEOBJECTDATA, BufferSize> records;
            DWORD count = static_cast<DWORD>(records.size());
            result = device->GetDeviceData(eventSize, records.data(), &count, flags);
            if (SUCCEEDED(result))
            {
                for (DWORD i = 0; i < count; ++i)
                {
                    const auto& record = records[i];
                    const auto offset = record.dwOfs;
                    const auto value = record.dwData;
                    if (offset == DIMOFS_X)
                        batch.x += std::bit_cast<int32_t>(value);
                    else if (offset == DIMOFS_Y)
                        batch.y += std::bit_cast<int32_t>(value);
                    else if (offset >= DIMOFS_BUTTON0 && offset < DIMOFS_BUTTON0 + ButtonCount)
                        batch.buttons.push_back(static_cast<uint8_t>(2 * (offset - DIMOFS_BUTTON0) + ((value & 0x80) == 0)));
                }
            }
        }

        *eventCount = 0;
        if (FAILED(result) || suspended.load(std::memory_order_relaxed))
            return result;
        for (const uint8_t button : batch.buttons)
            setButton(FirstButtonAction + button / 2, (button & 1) == 0);

        const auto addAxis = [&](DWORD offset, int64_t delta)
        {
            if (!delta)
                return;
            const auto amount = static_cast<int32_t>(std::clamp(delta,
                static_cast<int64_t>(std::numeric_limits<int32_t>::min()),
                static_cast<int64_t>(std::numeric_limits<int32_t>::max())));
            events[(*eventCount)++] = { offset, std::bit_cast<DWORD>(amount) };
        };
        addAxis(DIMOFS_X, batch.x);
        addAxis(DIMOFS_Y, batch.y);
        return result;
    }

    inline void Install()
    {
        setButton = reinterpret_cast<decltype(setButton)>(hook::pattern(
            "55 8B EC 0F B6 45 0C 85 C0 74 ? 8B 4D 08 0F B6 91 ? ? ? ? 83 E2 01").get_first());
        auto buffer = hook::pattern("C7 45 F8 00 00 00 00 C7 45 FC 10 00 00 00 8D 4D EC 51 6A 01 8B 15");
        injector::WriteMemory<DWORD>(buffer.get_first(10), BufferSize, true);
        auto read = hook::pattern("52 8B 41 28 FF D0 89 45 F0 C7 45 F4 64 00 00 00 C6 45 FB 80");
        injector::MakeCALL(read.get_first(1), Read, true);
        auto acquire = hook::pattern("55 8B EC 51 83 3D ? ? ? ? 00 75 05 E9 ? ? ? ? 0F B6 45 08 85 C0 74 0D 0F B6 0D");
        nativeMouseAcquired = *acquire.get_first<uint8_t*>(29);
        // The exclusive DirectInput mouse would replace the process's mouse registration on focus gain.
        acquireMouse = safetyhook::create_inline(acquire.get_first(), Acquire);
        reader = CreateRawInputWindow(L"PsychonautsFusionFixMouse", RawInputProc);
        UpdateRegistration();
    }
}
