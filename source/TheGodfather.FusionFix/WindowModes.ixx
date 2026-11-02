module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "FloatingPoint.hpp"
#include <d3d9.h>
#include <stdexcept>

export module WindowModes;

import Window;

import ComVars;

export namespace GodfatherWindowModes
{
    SafetyHookInline buildHook;
    unsigned allocationHint{};
    Mode* extended{};
    int originalCount{}, requestedWidth{}, requestedHeight{};
    bool windowed{};
    void(__thiscall* updateRasters)(void*){};
    int(__stdcall* resizeFrontend)(int, int){};
    int framedWidth{}, framedHeight{};

    template <typename T> T Read(T* pointer)
    {
        return *pointer;
    }

    template <typename T> T Read(GameRef<T>& value)
    {
        return value;
    }

    bool Requested()
    {
        return requestedWidth > 0 && requestedHeight > 0 && requestedWidth <= 16384 && requestedHeight <= 16384;
    }

    void Refresh()
    {
        if (!windowed || !Requested() || !extended || Read(active) || Read(modes) != extended
            || Read(count) != originalCount + 2)
            return;
        for (int i = 0; i < 2; ++i)
        {
            auto mode = Read(extended + originalCount + i);
            mode.width = requestedWidth;
            mode.height = requestedHeight;
            extended[originalCount + i] = mode;
        }
    }

    void Requested(bool horizontal, int value)
    {
        (horizontal ? requestedWidth : requestedHeight) = value;
        if (modes.is_initialized() && count.is_initialized() && active.is_initialized())
            Refresh();
    }

    int __cdecl Build()
    {
        Godfather::FloatingPointScope floatingPoint;
        const auto result = buildHook.unsafe_ccall<int>();
        extended = nullptr;
        originalCount = 0;
        if (!windowed)
            return result;
        const auto table = Read(modes);
        const auto size = Read(count);
        const auto api = Read(engine);
        if (!table || size < 1 || size > 8192 || !api)
            return result;
        auto value = Read(table);
        if (value.format != D3DFMT_X8R8G8B8 && value.format != D3DFMT_A8R8G8B8)
            return result;
        using Reallocate = void*(__cdecl*)(void*, size_t, unsigned);
        const auto reallocate = Read(reinterpret_cast<Reallocate*>(static_cast<unsigned char*>(api) + 272));
        if (!reallocate)
            return result;
        const auto enlarged = static_cast<Mode*>(reallocate(table, size_t(size + 2) * sizeof(Mode), allocationHint));
        if (!enlarged)
            return result;
        value.refresh = 0;
        value.flags = 1;
        if (Requested())
        {
            value.width = requestedWidth;
            value.height = requestedHeight;
        }
        modes = enlarged;
        enlarged[size] = value;
        enlarged[size + 1] = value;
        count = size + 2;
        extended = enlarged;
        originalCount = size;
        return reinterpret_cast<uintptr_t>(table) == uint32_t(result) ? int(reinterpret_cast<uintptr_t>(enlarged)) : result;
    }

    int Prepare(int width, int height)
    {
        if (!extended || !selected.is_initialized() || width <= 0 || height <= 0 || width > 16384 || height > 16384
            || Read(modes) != extended || Read(count) != originalCount + 2)
            return -1;
        const auto index = originalCount + (Read(selected) == originalCount ? 1 : 0);
        auto value = Read(extended + index);
        value.width = width;
        value.height = height;
        extended[index] = value;
        return index;
    }

    void Service()
    {
        if (!updateRasters || !resizeFrontend || !frontend.is_initialized() || !Read(frontend) || !display.is_initialized() || !extended || !selected.is_initialized()
            || !Read(active) || Read(modes) != extended || Read(count) != originalCount + 2 || !GodfatherWindow::device
            || !GodfatherWindow::mode || GodfatherWindow::sizing.load(std::memory_order_relaxed))
            return;
        const auto manager = static_cast<unsigned char*>(Read(display));
        if (!manager || Read(manager + 12) || !Read(manager + 14))
            return;
        if (GodfatherWindow::toggleRequested.exchange(false, std::memory_order_relaxed))
        {
            // The new client size reaches the device through the renderer's own window-size check, which releases,
            // resets and restores the device at the start of the next frame.
            if (GodfatherWindow::mode == 1)
            {
                framedWidth = GodfatherWindow::width;
                framedHeight = GodfatherWindow::height;
                GodfatherWindow::mode = 2;
            }
            else
            {
                GodfatherWindow::mode = 1;
                GodfatherWindow::width = framedWidth > 0 ? framedWidth : requestedWidth > 0 ? requestedWidth : GodfatherWindow::width;
                GodfatherWindow::height = framedHeight > 0 ? framedHeight : requestedHeight > 0 ? requestedHeight : GodfatherWindow::height;
            }
            GodfatherWindow::Place();
            return;
        }
        // Once the device has the new size, update the game's display state, frontend and camera rasters to match.
        const auto width = GodfatherWindow::width, height = GodfatherWindow::height;
        if (Read(reinterpret_cast<int*>(manager + 4)) == width && Read(reinterpret_cast<int*>(manager + 8)) == height)
            return;
        const auto index = Prepare(width, height);
        if (index < 0)
            return;
        Godfather::FloatingPointScope floatingPoint;
        selected = index;
        *reinterpret_cast<int*>(manager + 4) = width;
        *reinterpret_cast<int*>(manager + 8) = height;
        resizeFrontend(width, height);
        updateRasters(manager);
    }

    void Init()
    {
        windowed = FixSettings().Int("MAIN.WindowMode") != 0;
        if (!windowed)
            return;
        auto build = hook::pattern("8B 0D ? ? ? ? A1 ? ? ? ? 83 EC 0C 53 55 56 33 F6 3B CE 57 68 ? ? ? ? 74 1A "
            "8D 44 80 05 C1 E0 02 50 51 8B 0D ? ? ? ? FF 91 10 01 00 00");
        auto close = hook::pattern("A1 ? ? ? ? 85 C0 74 23 50 A1 ? ? ? ? FF 90 0C 01 00 00 83 C4 04 C7 05 ? ? ? ? "
            "00 00 00 00 C7 05 ? ? ? ? 00 00 00 00");
        auto stop = hook::pattern("51 A1 ? ? ? ? 53 33 DB 3B C3 89 1D ? ? ? ? 74 1B 50 A1 ? ? ? ? FF 90 0C 01 00 00");
        modes.SetAddress(*build.get_first<Mode**>(2));
        count.SetAddress(*close.get_first<int*>(36));
        active.SetAddress(*stop.get_first<int*>(13));
        engine.SetAddress(*close.get_first<void**>(11));
        allocationHint = *build.get_first<unsigned>(23);
        buildHook = safetyhook::create_inline(build.get_first(), Build);
        if (!buildHook)
            throw std::runtime_error("Window resolution hook unavailable");
        auto change = hook::pattern("83 EC 18 56 8B F1 8A 46 0C 84 C0 75 76 8B 4E 04 8B 44 24 20 3B C1 8B 4C 24 24");
        auto manager = hook::pattern("A1 ? ? ? ? 85 C0 75 3E 68 40 06 00 00 E8 ? ? ? ? 83 C4 04 89 04 24 85 C0");
        auto frontendSize = hook::pattern("A1 ? ? ? ? 8B 4C 24 04 89 48 18 8B 15 ? ? ? ? 8B 44 24 08 89 42 1C "
            "8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? C2 08 00");
        auto selection = hook::pattern("A1 ? ? ? ? 83 EC 48 53 55 33 ED 3B C5 56 57 0F 84 ? ? ? ? 8B 5C 24 5C 3B DD "
            "0F 8C ? ? ? ? 3B 1D ? ? ? ? 0F 8D ? ? ? ? 39 1D ? ? ? ?");
        if (buildHook)
        {
            updateRasters = reinterpret_cast<decltype(updateRasters)>(injector::GetBranchDestination(change.get_first(0x6A)).as_int());
            display.SetAddress(*manager.get_first<void**>(1));
            frontend.SetAddress(*frontendSize.get_first<void**>(1));
            resizeFrontend = reinterpret_cast<decltype(resizeFrontend)>(frontendSize.get_first());
            selected.SetAddress(*selection.get_first<int*>(48));
            GodfatherWindow::service = Service;
        }
    }
}
