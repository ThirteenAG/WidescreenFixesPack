#pragma once

#include "NativeMenuContexts.hpp"

namespace THPS2Menu::Unwind
{
    inline unsigned modalDepth = 0;
    inline unsigned worldDepth = 0;
    inline unsigned inputDepth = 0;
    inline unsigned updateDepth = 0;
    inline unsigned drawDepth = 0;
    inline unsigned presentDepth = 0;
    inline unsigned pauseDispatchDepth = 0;
    inline bool servicing = false;
    inline std::function<void()> titleFrame;
    inline std::function<void()> worldFrame;
    inline std::function<void()> modalFinished;
    inline SafetyHookInline modalHook;
    inline SafetyHookInline presentHook;
    inline SafetyHookInline worldHook;

    struct Depth
    {
        unsigned& counter;
        explicit Depth(unsigned& value) : counter(value) { ++counter; }
        ~Depth() { --counter; }
    };

    inline bool OutsideNativeRows()
    {
        return !inputDepth && !updateDepth && !drawDepth && !pauseDispatchDepth;
    }

    inline void Service(const std::function<void()>& action) noexcept
    {
        if (!action || servicing || !OutsideNativeRows())
            return;
        struct Scope { Scope() { servicing = true; } ~Scope() { servicing = false; } } scope;
        try { action(); }
        catch (...) { Diagnose(Diagnostic::Rollback, "foreign action exception; private work rejected"); }
    }

    inline int __cdecl Modal()
    {
        int result;
        {
            Depth scope(modalDepth);
            result = modalHook.unsafe_ccall<int>();
        }
        if (!modalDepth && !worldDepth)
            Service(modalFinished);
        return result;
    }

    inline int __cdecl Present()
    {
        // Called once per title menu iteration after input, drawing and presentation.
        // Queued title actions run only here; the modal return is teardown.
        int result;
        {
            Depth scope(presentDepth);
            result = presentHook.unsafe_ccall<int>();
        }
        if (!presentDepth && modalDepth == 1 && !worldDepth)
            Service(titleFrame);
        return result;
    }

    inline int __cdecl World()
    {
        int result;
        {
            // Covers the game's scene and HUD work; pause callbacks only enqueue.
            Depth scope(worldDepth);
            result = worldHook.unsafe_ccall<int>();
        }
        if (!worldDepth && !modalDepth)
            Service(worldFrame);
        return result;
    }

    inline void Reset()
    {
        worldHook.reset();
        presentHook.reset();
        modalHook.reset();
    }

    inline bool Install()
    {
        // Caller has checked all original contracts and pinned the module.
        // The host commits nativeReady only after all hook creation succeeds.
        modalHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4817E0), reinterpret_cast<void*>(&Modal));
        if (!modalHook)
            return false;
        presentHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4304E0), reinterpret_cast<void*>(&Present));
        if (!presentHook)
        {
            Reset();
            return false;
        }
        worldHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4684B0), reinterpret_cast<void*>(&World));
        if (!worldHook)
        {
            Reset();
            return false;
        }
        return true;
    }
}
