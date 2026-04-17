#pragma once

#include <type_traits>
#include <cstdint>
#include "injector\injector.hpp"
#include <safetyhook.hpp>

namespace usercall
{
    template<class...> inline constexpr bool always_false = false;

    enum class reg { none, eax, ebx, ecx, edx, esi, edi, ebp, stack };

    template<typename T>
    inline void* to_void_ptr(T val)
    {
        if constexpr (std::is_pointer_v<T>)
            return const_cast<void*>(reinterpret_cast<const void*>(val));
        else if constexpr (std::is_integral_v<T>)
            return reinterpret_cast<void*>(static_cast<uintptr_t>(val));
        else
            return const_cast<void*>(reinterpret_cast<const void*>(val));
    }

    // Single register (no stack)
    __declspec(naked) inline void Call_A(void*, void*)
    {
        __asm
        {
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline void Call_C(void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_D(void*, void*)
    {
        __asm
        {
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_S(void*, void*)
    {
        __asm
        {
            mov esi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_I(void*, void*)
    {
        __asm
        {
            mov edi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_B(void*, void*)
    {
        __asm
        {
            mov ebx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    // Two registers (no stack)
    __declspec(naked) inline void Call_AC(void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            mov edx, [esp + 4]
            call edx
            ret
        }
    }

    __declspec(naked) inline void Call_AD(void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline void Call_AS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline void Call_AI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline void Call_CD(void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_CS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_CI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_DS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_DI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_IS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov edi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_SI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov esi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_DC(void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    // Three registers (no stack)
    __declspec(naked) inline void Call_ACD(void*, void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 16]
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            mov ebx, [esp + 4]
            call ebx
            ret
        }
    }

    __declspec(naked) inline void Call_ACS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            mov edx, [esp + 4]
            call edx
            ret
        }
    }

    __declspec(naked) inline void Call_ADS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline void Call_CDS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edx, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline void Call_DIS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edi, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    // Single register + stack
    __declspec(naked) inline void Call_A_ST(void*, void*, void*)
    {
        __asm
        {
            mov eax, [esp + 8]
            push dword ptr[esp + 12]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_C_ST(void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_D_ST(void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_S_ST(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_I_ST(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    // Two registers + stack
    __declspec(naked) inline void Call_AC_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 16]
            mov edx, [esp + 8]
            call edx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_AD_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 16]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_CD_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov ecx, [esp + 8]
            push dword ptr[esp + 16]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_DI_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov edx, [esp + 8]
            push dword ptr[esp + 16]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_IS_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov edi, [esp + 8]
            push dword ptr[esp + 16]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    // Three registers + stack
    __declspec(naked) inline void Call_ADS_ST(void*, void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 20]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline void Call_AIS_ST(void*, void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edi, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 20]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    // Single register (no stack)
    __declspec(naked) inline uintptr_t RetCall_A(void*, void*)
    {
        __asm
        {
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_C(void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_D(void*, void*)
    {
        __asm
        {
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_S(void*, void*)
    {
        __asm
        {
            mov esi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_I(void*, void*)
    {
        __asm
        {
            mov edi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_B(void*, void*)
    {
        __asm
        {
            mov ebx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    // Two registers (no stack)
    __declspec(naked) inline uintptr_t RetCall_AC(void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            mov edx, [esp + 4]
            call edx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_AD(void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_AS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_AI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_CD(void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_CS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_CI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_DS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_DI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_IS(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov edi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_SI(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov esi, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_DC(void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    // Three registers (no stack)
    __declspec(naked) inline uintptr_t RetCall_ACD(void*, void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 16]
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            mov ebx, [esp + 4]
            call ebx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_ACS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            mov edx, [esp + 4]
            call edx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_ADS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            mov ecx, [esp + 4]
            call ecx
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_CDS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edx, [esp + 12]
            mov ecx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_DIS(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edi, [esp + 12]
            mov edx, [esp + 8]
            mov eax, [esp + 4]
            call eax
            ret
        }
    }

    // Single register + stack
    __declspec(naked) inline uintptr_t RetCall_A_ST(void*, void*, void*)
    {
        __asm
        {
            mov eax, [esp + 8]
            push dword ptr[esp + 12]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_C_ST(void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_D_ST(void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_S_ST(void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_I_ST(void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 8]
            push dword ptr[esp + 12]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    // Two registers + stack
    __declspec(naked) inline uintptr_t RetCall_AC_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov ecx, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 16]
            mov edx, [esp + 8]
            call edx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_AD_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 16]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_CD_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov edx, [esp + 12]
            mov ecx, [esp + 8]
            push dword ptr[esp + 16]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_DI_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov edi, [esp + 12]
            mov edx, [esp + 8]
            push dword ptr[esp + 16]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_IS_ST(void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 12]
            mov edi, [esp + 8]
            push dword ptr[esp + 16]
            mov eax, [esp + 8]
            call eax
            add esp, 4
            ret
        }
    }

    // Three registers + stack
    __declspec(naked) inline uintptr_t RetCall_ADS_ST(void*, void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edx, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 20]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    __declspec(naked) inline uintptr_t RetCall_AIS_ST(void*, void*, void*, void*, void*)
    {
        __asm
        {
            mov esi, [esp + 16]
            mov edi, [esp + 12]
            mov eax, [esp + 8]
            push dword ptr[esp + 20]
            mov ecx, [esp + 8]
            call ecx
            add esp, 4
            ret
        }
    }

    // 1 arg (register only)
    template<reg R1>
    inline void Call(auto func, auto a1)
    {
        if constexpr (R1 == reg::eax) Call_A(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::ecx) Call_C(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::edx) Call_D(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::esi) Call_S(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::edi) Call_I(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::ebx) Call_B(to_void_ptr(func), to_void_ptr(a1));
        else static_assert(always_false<decltype(func)>, "Unsupported 1-arg register");
    }

    // 2 args (registers or register + stack)
    template<reg R1, reg R2>
    inline void Call(auto func, auto a1, auto a2)
    {
        // No stack
        if constexpr (R1 == reg::eax && R2 == reg::ecx) Call_AC(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::eax && R2 == reg::edx) Call_AD(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::eax && R2 == reg::esi) Call_AS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::eax && R2 == reg::edi) Call_AI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::edx) Call_CD(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::esi) Call_CS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::edi) Call_CI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::esi) Call_DS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::edi) Call_DI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edi && R2 == reg::esi) Call_IS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::esi && R2 == reg::edi) Call_SI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::ecx) Call_DC(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));

        // With stack
        else if constexpr (R1 == reg::eax && R2 == reg::stack) Call_A_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::stack) Call_C_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::stack) Call_D_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::esi && R2 == reg::stack) Call_S_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edi && R2 == reg::stack) Call_I_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));

        else static_assert(always_false<decltype(func)>, "Unsupported 2-arg register combination");
    }

    // 3 args (registers or registers + stack)
    template<reg R1, reg R2, reg R3>
    inline void Call(auto func, auto a1, auto a2, auto a3)
    {
        // No stack
        if constexpr (R1 == reg::eax && R2 == reg::ecx && R3 == reg::edx) Call_ACD(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::eax && R2 == reg::ecx && R3 == reg::esi) Call_ACS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::eax && R2 == reg::edx && R3 == reg::esi) Call_ADS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::ecx && R2 == reg::edx && R3 == reg::esi) Call_CDS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::edx && R2 == reg::edi && R3 == reg::esi) Call_DIS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));

        // With stack
        else if constexpr (R1 == reg::eax && R2 == reg::ecx && R3 == reg::stack) Call_AC_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::eax && R2 == reg::edx && R3 == reg::stack) Call_AD_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::ecx && R2 == reg::edx && R3 == reg::stack) Call_CD_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::edx && R2 == reg::edi && R3 == reg::stack) Call_DI_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::edi && R2 == reg::esi && R3 == reg::stack) Call_IS_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));

        else static_assert(always_false<decltype(func)>, "Unsupported 3-arg register combination");
    }

    // 4 args (3 registers + stack)
    template<reg R1, reg R2, reg R3, reg R4>
    inline void Call(auto func, auto a1, auto a2, auto a3, auto a4)
    {
        if constexpr (R1 == reg::eax && R2 == reg::edx && R3 == reg::esi && R4 == reg::stack)
        {
            Call_ADS_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3), to_void_ptr(a4));
        }
        else if constexpr (R1 == reg::eax && R2 == reg::edi && R3 == reg::esi && R4 == reg::stack)
        {
            Call_AIS_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3), to_void_ptr(a4));
        }
        else
        {
            static_assert(always_false<decltype(func)>, "Unsupported 4-arg register combination");
        }
    }

    // 1 arg (register only)
    template<typename Ret, reg R1>
    inline Ret CallAndRet(auto func, auto a1)
    {
        uintptr_t result = 0;
        if constexpr (R1 == reg::eax) result = RetCall_A(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::ecx) result = RetCall_C(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::edx) result = RetCall_D(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::esi) result = RetCall_S(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::edi) result = RetCall_I(to_void_ptr(func), to_void_ptr(a1));
        else if constexpr (R1 == reg::ebx) result = RetCall_B(to_void_ptr(func), to_void_ptr(a1));
        else static_assert(always_false<decltype(func)>, "Unsupported 1-arg register");
        return static_cast<Ret>(result);
    }

    // 2 args (registers or register + stack)
    template<typename Ret, reg R1, reg R2>
    inline Ret CallAndRet(auto func, auto a1, auto a2)
    {
        uintptr_t result = 0;
        // No stack
        if constexpr (R1 == reg::eax && R2 == reg::ecx) result = RetCall_AC(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::eax && R2 == reg::edx) result = RetCall_AD(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::eax && R2 == reg::esi) result = RetCall_AS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::eax && R2 == reg::edi) result = RetCall_AI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::edx) result = RetCall_CD(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::esi) result = RetCall_CS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::edi) result = RetCall_CI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::esi) result = RetCall_DS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::edi) result = RetCall_DI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edi && R2 == reg::esi) result = RetCall_IS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::esi && R2 == reg::edi) result = RetCall_SI(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::ecx) result = RetCall_DC(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));

        // With stack
        else if constexpr (R1 == reg::eax && R2 == reg::stack) result = RetCall_A_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::ecx && R2 == reg::stack) result = RetCall_C_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edx && R2 == reg::stack) result = RetCall_D_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::esi && R2 == reg::stack) result = RetCall_S_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));
        else if constexpr (R1 == reg::edi && R2 == reg::stack) result = RetCall_I_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2));

        else static_assert(always_false<decltype(func)>, "Unsupported 2-arg register combination");
        return static_cast<Ret>(result);
    }

    // 3 args (registers or registers + stack)
    template<typename Ret, reg R1, reg R2, reg R3>
    inline Ret CallAndRet(auto func, auto a1, auto a2, auto a3)
    {
        uintptr_t result = 0;
        // No stack
        if constexpr (R1 == reg::eax && R2 == reg::ecx && R3 == reg::edx) result = RetCall_ACD(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::eax && R2 == reg::ecx && R3 == reg::esi) result = RetCall_ACS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::eax && R2 == reg::edx && R3 == reg::esi) result = RetCall_ADS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::ecx && R2 == reg::edx && R3 == reg::esi) result = RetCall_CDS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::edx && R2 == reg::edi && R3 == reg::esi) result = RetCall_DIS(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));

        // With stack
        else if constexpr (R1 == reg::eax && R2 == reg::ecx && R3 == reg::stack) result = RetCall_AC_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::eax && R2 == reg::edx && R3 == reg::stack) result = RetCall_AD_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::ecx && R2 == reg::edx && R3 == reg::stack) result = RetCall_CD_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::edx && R2 == reg::edi && R3 == reg::stack) result = RetCall_DI_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));
        else if constexpr (R1 == reg::edi && R2 == reg::esi && R3 == reg::stack) result = RetCall_IS_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3));

        else static_assert(always_false<decltype(func)>, "Unsupported 3-arg register combination");
        return static_cast<Ret>(result);
    }

    // 4 args (3 registers + stack)
    template<typename Ret, reg R1, reg R2, reg R3, reg R4>
    inline Ret CallAndRet(auto func, auto a1, auto a2, auto a3, auto a4)
    {
        uintptr_t result = 0;
        if constexpr (R1 == reg::eax && R2 == reg::edx && R3 == reg::esi && R4 == reg::stack)
        {
            result = RetCall_ADS_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3), to_void_ptr(a4));
        }
        else if constexpr (R1 == reg::eax && R2 == reg::edi && R3 == reg::esi && R4 == reg::stack)
        {
            result = RetCall_AIS_ST(to_void_ptr(func), to_void_ptr(a1), to_void_ptr(a2), to_void_ptr(a3), to_void_ptr(a4));
        }
        else
        {
            static_assert(always_false<decltype(func)>, "Unsupported 4-arg register combination");
        }
        return static_cast<Ret>(result);
    }

    template<typename Callback>
    inline auto MakeUsercall(auto pattern_addr, Callback&& callback, size_t nop_size = 5)
    {
        auto original = injector::GetBranchDestination(pattern_addr).get();
        injector::MakeNOP(pattern_addr, nop_size, true);
        [[maybe_unused]] static auto _ = safetyhook::create_mid(pattern_addr, std::forward<Callback>(callback));
        return original;
    }
}