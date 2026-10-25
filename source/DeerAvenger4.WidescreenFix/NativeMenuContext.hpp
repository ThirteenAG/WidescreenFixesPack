#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace DeerMenu
{
    inline bool Readable(const void* address, size_t size)
    {
        auto begin = reinterpret_cast<uintptr_t>(address);
        if (!begin || size > UINTPTR_MAX - begin)
            return false;
        const auto end = begin + size;
        while (begin < end)
        {
            MEMORY_BASIC_INFORMATION region{};
            if (!VirtualQuery(reinterpret_cast<const void*>(begin), &region, sizeof(region)) ||
                region.State != MEM_COMMIT || (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
                return false;
            const DWORD access = region.Protect & 0xff;
            if (access != PAGE_READONLY && access != PAGE_READWRITE && access != PAGE_WRITECOPY &&
                access != PAGE_EXECUTE_READ && access != PAGE_EXECUTE_READWRITE &&
                access != PAGE_EXECUTE_WRITECOPY)
                return false;
            const auto next = reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize;
            if (next <= begin)
                return false;
            begin = next < end ? next : end;
        }
        return true;
    }

    template<class T> inline T Read(uintptr_t address)
    {
        T value{};
        if (Readable(reinterpret_cast<const void*>(address), sizeof(value)))
            std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
        return value;
    }

    // Globals inside the supported executable's image are always mapped: no VirtualQuery needed.
    template<class T> inline T Global(uintptr_t address)
    {
        return *reinterpret_cast<const volatile T*>(address);
    }

    struct PauseContext
    {
        void* world{};
        void* camera{};
        void* engine{};
        void* raster{};
        int width{};
        int height{};

        bool Alive() const
        {
            return world && camera && engine && raster &&
                Global<void*>(0x546998) == world && Global<void*>(0x54699C) == camera &&
                Global<void*>(0x554934) == engine && Global<int>(0x532B40) == 1 &&
                Read<void*>(reinterpret_cast<uintptr_t>(camera) + 96) == raster &&
                Read<int>(reinterpret_cast<uintptr_t>(raster) + 12) == width &&
                Read<int>(reinterpret_cast<uintptr_t>(raster) + 16) == height;
        }

        bool Current() const
        {
            // Ordinary in-world pause only. Loading/cutscene/exhibition, console,
            // deactivation and stock confirmations are independently excluded.
            return Global<int>(0x532B1C) == 0 && Global<int>(0x532CA0) != 0 &&
                Global<int>(0x532300) == 1 && Global<int>(0x532304) == 1 &&
                Global<int>(0x5322F8) == 5 && Global<int>(0x533228) == 0 &&
                Global<int>(0x52F99C) == 0 && Global<int>(0x4FFA3C) == 1 && Alive();
        }

        bool Rendering() const
        {
            // Font geometry reads the currently begun RenderWare camera. This
            // stronger predicate belongs to the actual draw boundary, not the
            // outer-frame logical queue where EndUpdate may clear that camera.
            const float nearClip = Read<float>(reinterpret_cast<uintptr_t>(camera) + 128);
            const float screenZ = Read<float>(reinterpret_cast<uintptr_t>(engine) + 24);
            return Current() && Read<void*>(reinterpret_cast<uintptr_t>(engine)) == camera &&
                std::isfinite(nearClip) && nearClip > 0.0f && std::isfinite(screenZ);
        }

        // Cheap pre-check before Capture: only the in-world pause menu qualifies.
        static bool PauseShown()
        {
            return Global<int>(0x5322F8) == 5 && Global<int>(0x532300) == 1 && Global<int>(0x532304) == 1 &&
                Global<int>(0x532B40) == 1;
        }

        bool Capture(void* renderWorld, void* renderCamera)
        {
            world = renderWorld;
            camera = renderCamera;
            engine = Read<void*>(0x554934);
            raster = Read<void*>(reinterpret_cast<uintptr_t>(camera) + 96);
            width = Read<int>(reinterpret_cast<uintptr_t>(raster) + 12);
            height = Read<int>(reinterpret_cast<uintptr_t>(raster) + 16);
            const float nearClip = Read<float>(reinterpret_cast<uintptr_t>(camera) + 128);
            const float screenZ = Read<float>(reinterpret_cast<uintptr_t>(engine) + 24);
            return width >= 320 && height >= 240 && width <= 16384 && height <= 16384 &&
                std::isfinite(nearClip) && nearClip > 0.0f && std::isfinite(screenZ) &&
                Rendering();
        }
    };
}
