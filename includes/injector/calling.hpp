/*
 *  Injectors - Function Calls Using Variadic Templates
 *
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
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
#include "injector.hpp"
#include <utility>

#if __cplusplus >= 201103L || _MSC_VER >= 1800   // MSVC 2013
#else
#error "This feature is not supported on this compiler"
#endif

namespace injector
{
    template<class Prototype>
    struct cstd;

    template<class Ret, class ...Args>
    struct cstd<Ret(Args...)>
    {
        // Call function at @p returning @Ret with args @Args
        static Ret call(memory_pointer_tr p, Args... a)
        {
            auto fn = (Ret(*)(Args...)) p.get<void>();
            return fn(std::forward<Args>(a)...);
        }

        template<uintptr_t addr>    // Uses lazy pointer
        static Ret call(Args... a)
        {
            return call(lazy_pointer<addr>::get(), std::forward<Args>(a)...);
        }
    };

    template<class Prototype>
    struct stdcall;

    template<class Ret, class ...Args>
    struct stdcall<Ret(Args...)>
    {
        // Call function at @p returning @Ret with args @Args
        static Ret call(memory_pointer_tr p, Args... a)
        {
            auto fn = (Ret(__stdcall *)(Args...)) p.get<void>();
            return fn(std::forward<Args>(a)...);
        }

        template<uintptr_t addr>    // Uses lazy pointer
        static Ret call(Args... a)
        {
            return call(lazy_pointer<addr>::get(), std::forward<Args>(a)...);
        }
    };

    template<class Prototype>
    struct fastcall;

    template<class Ret, class ...Args>
    struct fastcall<Ret(Args...)>
    {
        // Call function at @p returning @Ret with args @Args
        static Ret call(memory_pointer_tr p, Args... a)
        {
            auto fn = (Ret(__fastcall *)(Args...)) p.get<void>();;
            return fn(std::forward<Args>(a)...);
        }

        template<uintptr_t addr>    // Uses lazy pointer
        static Ret call(Args... a)
        {
            return call(lazy_pointer<addr>::get(), std::forward<Args>(a)...);
        }
    };

    template<class Prototype>
    struct thiscall;

    template<class Ret, class ...Args>
    struct thiscall<Ret(Args...)>
    {
        // Call function at @p returning @Ret with args @Args
        static Ret call(memory_pointer_tr p, Args... a)
        {
            auto fn = (Ret(__thiscall *)(Args...)) p.get<void>();
            return fn(std::forward<Args>(a)...);
        }

        // Call function at the index @i from the vtable of the object @a[0]
        template<size_t i>
        static Ret vtbl(Args... a)
        {
            auto obj = raw_ptr(std::get<0>(std::forward_as_tuple(a...)));
            auto p   = raw_ptr( (*obj.template get<void**>()) [i] );
            return call(p, std::forward<Args>(a)...);
        }

        template<uintptr_t addr>    // Uses lazy pointer
        static Ret call(Args... a)
        {
            return call(lazy_pointer<addr>::get(), std::forward<Args>(a)...);
        }
    };
} 

