/*
 *  Injectors - Utility / Helpers
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

namespace injector
{
    template<class T, T value>
    T return_value()
    {
        return value;
    }
    
    template<class T>
    void* force_ptr(const T& fun)
    {
        auto ptr = fun;
        return *(void**)&ptr;
    }


    // Helper structure to help calling back what was there before a hook
    // e.g. hb.fun = MakeCALL(0x0, raw_ptr(my_hook));
    template<class FuncType>
    struct hook_back
    {
        typedef FuncType func_type;
        
        func_type fun;
        
        hook_back() : fun(nullptr)
        {}
    };
};
