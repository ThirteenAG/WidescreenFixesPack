/*
 *  Injectors - Useful Assembly Stuff
 *
 *  Copyright (C) 2012-2014 LINK/2012 <dma_2012@hotmail.com>
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 * 
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 *
 */
#pragma once

// This header is very restrict about compiler and architecture
#ifndef _MSC_VER    // MSVC is much more flexible when we're talking about inline assembly
#error  Cannot use this header in another compiler other than MSVC
#endif
#ifndef _M_IX86
#error  Supported only in x86
#endif

//
#include "../external/injector/include/injector/injector.hpp"

namespace injector
{
    struct reg_pack2
    {
        // The ordering is very important, don't change
        // The first field is the last to be pushed and first to be poped

        // PUSHAD/POPAD -- must be the lastest fields (because of esp)
        union
        {
            uint32_t arr[8];
            struct { uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; };
        };

        // PUSHFD / POPFD
        uint32_t ef;
        
        enum reg_name {
            reg_edi, reg_esi, reg_ebp, reg_esp, reg_ebx, reg_edx, reg_ecx, reg_eax 
        };
        
        enum ef_flag {
            carry_flag = 0, parity_flag = 2, adjust_flag = 4, zero_flag = 6, sign_flag = 7,
            direction_flag = 10, overflow_flag = 11
        };

        uint32_t& operator[](size_t i)
        { return this->arr[i]; }
        const uint32_t& operator[](size_t i) const
        { return this->arr[i]; }

        template<uint32_t bit>   // bit starts from 0, use ef_flag enum
        bool flag()
        {
            return (this->ef & (1 << bit)) != 0;
        }

        bool jnb()
        {
            return flag<carry_flag>() == false;
        }
    };

    // Lowest level stuff (actual assembly) goes on the following namespace
    // PRIVATE! Skip this, not interesting for you.
    namespace injector_asm2
    {
        // Wrapper functor, so the assembly can use some templating
        template<class T>
        struct wrapper
        {
            static void call(reg_pack2* regs)
            {
                T fun; fun(*regs);
            }
        };

        // Constructs a reg_pack2 and calls the wrapper functor
        template<class W>   // where W is of type wrapper
        inline void __declspec(naked) make_reg_pack_and_call()
        {
            _asm
            {
                // Construct the reg_pack2 structure on the stack
                pushfd              // Pushes EFLAGS to reg_pack2
                pushad              // Pushes general purposes registers to reg_pack2
                add dword ptr[esp+12], 8     // Add 4 to reg_pack2::esp 'cuz of our return pointer, let it be as before this func is called

                // Call wrapper sending reg_pack2 as parameter
                push esp
                call W::call
                add esp, 4

                // Destructs the reg_pack2 from the stack
                sub dword ptr[esp+12], 8   // Fix reg_pack2::esp before popping it (doesn't make a difference though) (+4 because eflags)
                popad
                popfd               // Warning: Do not use any instruction that changes EFLAGS after this (-> sub affects EF!! <-)

                // Back to normal flow
                ret
            }
        }
    };


    /*
     *  MakeInline2
     *      Makes inline assembly (but not assembly, an actual functor of type FuncT) at address
     */
    template<class FuncT>
    void MakeInline2(memory_pointer_tr at)
    {
        typedef injector_asm2::wrapper<FuncT> functor;
        if(false) functor::call(nullptr);   // To instantiate the template, if not done _asm will fail
        MakeCALL(at, injector_asm2::make_reg_pack_and_call<functor>);
    }

    /*
     *  MakeInline2
     *      Same as above, but it NOPs everything between at and end (exclusive), then performs MakeInline2
     */
    template<class FuncT>
    void MakeInline2(memory_pointer_tr at, memory_pointer_tr end)
    {
        MakeRangedNOP(at, end);
        MakeInline2<FuncT>(at);
    }

    /*
     *  MakeInline2
     *      Same as above, but (at,end) are template parameters.
     *      On this case the functor can be passed as argument since there will be one func instance for each at,end not just for each FuncT
     */
    template<uintptr_t at, uintptr_t end, class FuncT>
    void MakeInline2(FuncT func)
    {
        static std::unique_ptr<FuncT> static_func;
        static_func.reset(new FuncT(std::move(func)));

        // Encapsulates the call to static_func
        struct Caps
        {
            void operator()(reg_pack2& regs)
            { (*static_func)(regs); }
        };

        // Does the actual MakeInline2
        return MakeInline2<Caps>(lazy_pointer<at>::get(), lazy_pointer<end>::get());
    }

    /*
     *  MakeInline2
     *      Same as above, but (end) is calculated by the length of a call instruction
     */
    template<uintptr_t at, class FuncT>
    void MakeInline2(FuncT func)
    {
        return MakeInline2<at, at+5, FuncT>(func);
    }
};
