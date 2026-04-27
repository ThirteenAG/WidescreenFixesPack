#pragma once
#include <tuple>
#include <cstdint>
#include "safetyhook.hpp"

// Returns a tuple of references directly into regs/stack slots.
// Structured bindings on the result let you read AND modify args before the call proceeds.
// Place safetyhook::create_mid on the call instruction — args are already pushed at that point.

namespace detail
{
    template<typename T>
    T& stack_ref(SafetyHookContext& regs, size_t byte_offset)
    {
        return *reinterpret_cast<T*>(regs.esp + byte_offset);
    }

    // ── __cdecl / __stdcall  (all args on stack, left-to-right at esp+0,+4,+8…) ─────
    template<typename FuncPtr> struct ArgsDeducer;

    template<typename Ret, typename... Args>
    struct ArgsDeducer<Ret(__cdecl*)(Args...)>
    {
        template<size_t... I>
        static std::tuple<Args&...> get(SafetyHookContext& regs, std::index_sequence<I...>)
        {
            return std::tuple<Args&...>(stack_ref<Args>(regs, I * 4)...);
        }
        static std::tuple<Args&...> get(SafetyHookContext& regs)
        {
            return get(regs, std::index_sequence_for<Args...>{});
        }
    };

    template<typename Ret, typename... Args>
    struct ArgsDeducer<Ret(__stdcall*)(Args...)>
    {
        template<size_t... I>
        static std::tuple<Args&...> get(SafetyHookContext& regs, std::index_sequence<I...>)
        {
            return std::tuple<Args&...>(stack_ref<Args>(regs, I * 4)...);
        }
        static std::tuple<Args&...> get(SafetyHookContext& regs)
        {
            return get(regs, std::index_sequence_for<Args...>{});
        }
    };

    // ── __fastcall  (arg0->ecx, arg1->edx, arg2+->stack) ────────────────────────────
    template<typename Ret, typename Arg0, typename... Rest>
    struct ArgsDeducer<Ret(__fastcall*)(Arg0, Rest...)>
    {
        template<size_t... I>
        static std::tuple<Arg0&, Rest&...> get(SafetyHookContext& regs, std::index_sequence<I...>)
        {
            return std::tuple<Arg0&, Rest&...>(
                reinterpret_cast<Arg0&>(regs.ecx),
                stack_ref<Rest>(regs, I * 4)...   // I starts at 0 for Rest
            );
        }
        static std::tuple<Arg0&, Rest&...> get(SafetyHookContext& regs)
        {
            return get(regs, std::index_sequence_for<Rest...>{});
        }
    };

    // ── __thiscall  (arg0/this->ecx, arg1+->stack) ───────────────────────────────────
    template<typename Ret, typename This, typename... Rest>
    struct ArgsDeducer<Ret(__thiscall*)(This, Rest...)>
    {
        template<size_t... I>
        static std::tuple<This&, Rest&...> get(SafetyHookContext& regs, std::index_sequence<I...>)
        {
            return std::tuple<This&, Rest&...>(
                reinterpret_cast<This&>(regs.ecx),
                stack_ref<Rest>(regs, I * 4)...
            );
        }
        static std::tuple<This&, Rest&...> get(SafetyHookContext& regs)
        {
            return get(regs, std::index_sequence_for<Rest...>{});
        }
    };
}

template<typename FuncPtr>
auto deduce_args(SafetyHookContext& regs)
{
    return detail::ArgsDeducer<FuncPtr>::get(regs);
}