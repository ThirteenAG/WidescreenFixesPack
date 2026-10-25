#pragma once
#include <safetyhook.hpp>
#include "NativeMenuController.hpp"

namespace THPS3Menu
{
    // Every native caller below keeps touching its node/list after controller
    // Execute returns. Actions and storage reclamation run only after the full
    // recursive/parent event, input and frontend task call has unwound.
    struct Unwind
    {
        inline static SafetyHookInline recursive, parents, input, task, destroy, remove, removeControllers, removeTree;
        inline static uint32_t depth = 0;
        inline static bool draining = false;
        inline static void (*afterFrame)() = nullptr;
        inline static void (*beforeDestroy)(void*) = nullptr;
        inline static void (*observe)() = nullptr;

        struct Scope
        {
            Scope() { ++depth; }
            ~Scope() { --depth; }
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;
        };
        static bool Idle() { return depth == 0 && !draining; }
        static uint8_t __fastcall Recursive(void* node, void*, Event* event, uint8_t skip)
        {
            Scope scope;
            auto result = recursive.unsafe_thiscall<uint8_t>(node, event, skip);
            if (observe) observe();
            return result;
        }
        static uintptr_t __fastcall Parents(void* node, void*, Event* event)
        {
            Scope scope;
            auto result = parents.unsafe_thiscall<uintptr_t>(node, event);
            if (observe) observe();
            return result;
        }
        static uintptr_t __cdecl Input(void* data)
        {
            Scope scope;
            auto result = input.unsafe_ccall<uintptr_t>(data);
            if (observe) observe();
            return result;
        }
        static uintptr_t __cdecl Task(void* data)
        {
            uintptr_t result;
            {
                Scope scope;
                result = task.unsafe_ccall<uintptr_t>(data);
                if (observe) observe();
            }
            if (Idle() && afterFrame)
            {
                struct DrainScope
                {
                    bool previous = draining;
                    DrainScope() { draining = true; }
                    ~DrainScope() { draining = previous; }
                } drain;
                afterFrame();
            }
            return result;
        }
        static void* __fastcall Destroy(void* node, void*)
        {
            Scope scope;
            if (beforeDestroy) beforeDestroy(node);
            return destroy.unsafe_thiscall<void*>(node);
        }
        static uintptr_t __fastcall Remove(void* manager, void*, uint32_t id)
        {
            Scope scope;
            return remove.unsafe_thiscall<uintptr_t>(manager, id);
        }
        static uintptr_t __fastcall RemoveControllers(void* node, void*)
        {
            Scope scope;
            return removeControllers.unsafe_thiscall<uintptr_t>(node);
        }
        static uintptr_t __fastcall RemoveTree(void* node, void*)
        {
            Scope scope;
            return removeTree.unsafe_thiscall<uintptr_t>(node);
        }
        static void Reset()
        {
            // Only startup rollback, before any native retained controllers.
            removeTree.reset(); removeControllers.reset(); remove.reset();
            destroy.reset(); task.reset(); input.reset(); parents.reset(); recursive.reset();
            afterFrame = nullptr;
            beforeDestroy = nullptr;
            observe = nullptr;
        }
        static bool Install(void (*frame)(), void (*retire)(void*), void (*check)())
        {
            afterFrame = frame;
            beforeDestroy = retire;
            observe = check;
            recursive = safetyhook::create_inline(reinterpret_cast<void*>(0x4CD460), Recursive);
            parents = safetyhook::create_inline(reinterpret_cast<void*>(0x4CD4B0), Parents);
            input = safetyhook::create_inline(reinterpret_cast<void*>(0x44A160), Input);
            task = safetyhook::create_inline(reinterpret_cast<void*>(0x44A1A0), Task);
            destroy = safetyhook::create_inline(reinterpret_cast<void*>(0x4CD100), Destroy);
            remove = safetyhook::create_inline(reinterpret_cast<void*>(0x4D2280), Remove);
            removeControllers = safetyhook::create_inline(reinterpret_cast<void*>(0x4CD650), RemoveControllers);
            removeTree = safetyhook::create_inline(reinterpret_cast<void*>(0x4C7770), RemoveTree);
            if (!recursive || !parents || !input || !task || !destroy || !remove || !removeControllers || !removeTree)
            {
                Reset();
                return false;
            }
            return true;
        }
    };
}
