#pragma once

#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

// A mouse report and both of its axes belong to the same game frame. Drain the input
// thread's messages locally, then publish the completed batch in one operation.
class BufferedRawMouse
{
public:
    struct Movement
    {
        int64_t x = 0;
        int64_t y = 0;
        bool relative = false;
    };

    void Clear()
    {
        std::lock_guard lock(mutex);
        ++generation;
        pending = {};
        devices.clear();
        relativeDevices = 0;
    }

    bool HasRelative()
    {
        std::lock_guard lock(mutex);
        return pending.relative;
    }

    Movement Take()
    {
        std::lock_guard lock(mutex);
        const auto result = pending;
        pending.x = pending.y = 0;
        relativeDevices = 0;
        ++interval;
        return result;
    }

    template<typename Active>
    void Read(HRAWINPUT handle, Active active)
    {
        Read(handle, active, [](RAWMOUSE&, unsigned) {});
    }

    template<typename Active, typename Transform>
    void Read(HRAWINPUT handle, Active active, Transform transform)
    {
        const auto start = generation.load();
        if (!active())
            return;
        constexpr size_t bufferSize = 8192;
        struct DeviceBatch
        {
            HANDLE device;
            int64_t x, y;
            bool relative;
            bool reset;
        };
        std::array<DeviceBatch, 1 + bufferSize / (sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE))> batches;
        auto end = batches.begin();
        auto append = [&](HANDLE device, RAWMOUSE mouse)
        {
            if (!(mouse.usFlags & MOUSE_MOVE_ABSOLUTE) && !mouse.lLastX && !mouse.lLastY)
                return;
            transform(mouse, start);
            auto entry = std::find_if(batches.begin(), end, [device](const DeviceBatch& batch) { return batch.device == device; });
            if (entry == end)
                *end++ = { device, 0, 0, false, false };
            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
            {
                entry->x = entry->y = 0;
                entry->relative = false;
                entry->reset = true;
            }
            else
            {
                entry->x += mouse.lLastX;
                entry->y += mouse.lLastY;
                entry->relative = true;
            }
        };
        RAWINPUT input{};
        UINT size = sizeof(input);
        if (GetRawInputData(handle, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            input.header.dwType == RIM_TYPEMOUSE)
            append(input.header.hDevice, input.data.mouse);

        // The dispatched report is no longer queued. Read the following reports in one batch.
        static const bool wow64 = []()
        {
            BOOL result = FALSE;
            return sizeof(void*) == 4 && IsWow64Process(GetCurrentProcess(), &result) && result;
        }();
        const size_t header = wow64 ? 24 : sizeof(RAWINPUTHEADER);
        const size_t alignment = wow64 ? 8 : sizeof(void*);
        alignas(8) uint8_t buffer[bufferSize];
        size = sizeof(buffer);
        UINT count = GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer), &size, sizeof(RAWINPUTHEADER));
        if (count != UINT(-1))
        {
            const uint8_t* packet = buffer;
            for (UINT i = 0; i < count; ++i)
            {
                auto raw = reinterpret_cast<const RAWINPUTHEADER*>(packet);
                if (raw->dwType == RIM_TYPEMOUSE)
                    append(raw->hDevice, *reinterpret_cast<const RAWMOUSE*>(packet + header));
                packet += (raw->dwSize + alignment - 1) & ~(alignment - 1);
            }
        }
        if (end == batches.begin())
            return;

        std::lock_guard lock(mutex);
        if (generation.load() != start)
            return;
        // A device switching to absolute mode only discards its own relative movement.
        for (auto batch = batches.begin(); batch != end; ++batch)
        {
            auto& device = devices[batch->device];
            if (device.interval != interval)
                device = { interval, {} };
            auto& movement = device.movement;
            if (batch->reset && movement.relative)
            {
                pending.x -= movement.x;
                pending.y -= movement.y;
                --relativeDevices;
                movement = {};
            }
            if (batch->relative)
            {
                if (!movement.relative)
                    ++relativeDevices;
                movement.x += batch->x;
                movement.y += batch->y;
                movement.relative = true;
                pending.x += batch->x;
                pending.y += batch->y;
            }
        }
        pending.relative = relativeDevices != 0;
    }

private:
    struct DeviceMovement
    {
        uint64_t interval{};
        Movement movement;
    };

    std::mutex mutex;
    std::atomic<unsigned> generation{ 0 };
    uint64_t interval{};
    size_t relativeDevices{};
    std::unordered_map<HANDLE, DeviceMovement> devices;
    Movement pending;
};
