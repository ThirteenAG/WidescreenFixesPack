/*
 *  Injectors - Useful Assembly Stuff
 *
 *  Copyright (C) 2012-2014 LINK/2012 <dma_2012@hotmail.com>
 *  Copyright (C) 2021 ThirteenAG
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

#include "injector/injector.hpp"
#include "Trampoline.h"

#include "asmjit/x86.h"
#include <span>
using namespace asmjit;
typedef void (*asmjit_Func)();
using namespace x86;

#ifdef _WIN64

namespace injector
{
#define JMPSIZE 14
#define CALLSIZE 16

#define pushad() \
{ \
    a.pushfq();  \
    a.push(rax); \
    a.push(rcx); \
    a.push(rdx); \
    a.push(rbx); \
    a.push(rsi); \
    a.push(rdi); \
    a.push(rsp); \
    a.push(rbp); \
    a.push(r8);  \
    a.push(r9);  \
    a.push(r10); \
    a.push(r11); \
    a.push(r12); \
    a.push(r13); \
    a.push(r14); \
    a.push(r15); \
}

#define popad() \
{ \
    a.pop(r15); \
    a.pop(r14); \
    a.pop(r13); \
    a.pop(r12); \
    a.pop(r11); \
    a.pop(r10); \
    a.pop(r9);  \
    a.pop(r8);  \
    a.pop(rbp); \
    a.pop(rsp); \
    a.pop(rdi); \
    a.pop(rsi); \
    a.pop(rbx); \
    a.pop(rdx); \
    a.pop(rcx); \
    a.pop(rax); \
    a.popfq();  \
}

    inline asmjit_Func asm_invoke(std::function<void(x86::Assembler& a)> asm_code_add)
    {
        asmjit_Func fn = nullptr;
        static JitRuntime rt;
        CodeHolder code;
        code.init(rt.environment());
        x86::Assembler a(&code);
        asm_code_add(a);
        rt.add(&fn, &code);
        return fn;
    };

    inline injector::memory_pointer_raw MakeAbsCALL(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
    {
        injector::WriteMemory<uint16_t>(at, 0x15FF, vp);
        injector::WriteMemory<uint32_t>(at + sizeof(uint16_t), 2, vp);
        injector::WriteMemory<uint16_t>(at + sizeof(uint16_t) + sizeof(uint32_t), 0x08EB, vp);
        injector::WriteMemory<uint64_t>(at + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), dest.as_int(), vp);
        return at.as_int() + CALLSIZE;
    }

    inline injector::memory_pointer_raw MakeAbsJMP(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
    {
        injector::WriteMemory<uint16_t>(at, 0x25FF, vp);
        injector::WriteMemory<uint32_t>(at + sizeof(uint16_t), 0, vp);
        injector::WriteMemory<uint64_t>(at + sizeof(uint16_t) + sizeof(uint32_t), dest.as_int(), vp);
        return at.as_int() + JMPSIZE;
    }

    inline injector::memory_pointer_raw ReadRelativeAddress(memory_pointer_tr at, size_t sizeof_addr = 4, bool vp = true)
    {
        uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
        switch (sizeof_addr)
        {
        case 1: return (base + ReadMemory<int8_t>(at, vp));
        case 2: return (base + ReadMemory<int16_t>(at, vp));
        case 4: return (base + ReadMemory<int32_t>(at, vp));
        }
        return nullptr;
    }

    inline bool UnprotectMemory(memory_pointer_tr addr, size_t size)
    {
        DWORD out_oldprotect = 0;
        return VirtualProtect(addr.get(), size, PAGE_EXECUTE_READWRITE, &out_oldprotect) != 0;
    }

    inline injector::memory_pointer_raw MakeCALLTrampoline(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
    {
        auto trampoline = Trampoline::MakeTrampoline((void*)at.as_int());
        return MakeCALL(at, trampoline->Jump(dest));
    }

    class raw_mem
    {
    public:
        raw_mem(memory_pointer_tr addr, std::initializer_list<uint8_t> bytes, bool offset_back = false)
        {
            ptr = addr.as_int() - (offset_back ? bytes.size() : 0);
            new_code.assign(std::move(bytes));
            old_code.resize(new_code.size());
            ReadMemoryRaw(ptr, old_code.data(), old_code.size(), true);
        }

        raw_mem(memory_pointer_tr addr, std::function<void(x86::Assembler& a)> asm_code_add, bool offset_back = false)
        {
            asmjit_Func fn = nullptr;
            JitRuntime rt;
            CodeHolder code;
            code.init(rt.environment());
            x86::Assembler a(&code);
            asm_code_add(a);
            rt.add(&fn, &code);
            std::span<uint8_t> bytes{ (uint8_t*)fn, code.codeSize() };
            ptr = addr.as_int() - (offset_back ? bytes.size() : 0);
            new_code.assign(bytes.begin(), bytes.end());
            old_code.resize(new_code.size());
            ReadMemoryRaw(ptr, old_code.data(), old_code.size(), true);
            rt.release(fn);
        }

        void Write()
        {
            WriteMemoryRaw(ptr, new_code.data(), new_code.size(), true);
        }

        void Restore()
        {
            WriteMemoryRaw(ptr, old_code.data(), old_code.size(), true);
        }

        size_t Size()
        {
            return old_code.size();
        }

    private:
        injector::memory_pointer ptr;
        std::vector<uint8_t> old_code;
        std::vector<uint8_t> new_code;
    };

   struct reg_pack
    {
        // The ordering is very important, don't change
        // The first field is the last to be pushed and first to be poped

        // PUSHAD/POPAD -- must be the lastest fields (because of rsp)
        union
        {
            uint64_t arr[16];
            struct { uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rsp, rdi, rsi, rbx, rdx, rcx, rax; };
        };

        // PUSHFD / POPFD
        uint64_t rf;

        uint64_t ret_ptr;
        
        enum reg_name {
            reg_r15, reg_r14, reg_r13, reg_r12, reg_r11, reg_r10, reg_r9, reg_r8, reg_rbp,
            reg_rsp, reg_rdi, reg_rsi, reg_rbx, reg_rdx, reg_rcx, reg_rax, reg_rf, reg_ret_ptr
        };
        
        enum rf_flag {
            carry_flag = 0, parity_flag = 2, adjust_flag = 4, zero_flag = 6, sign_flag = 7,
            direction_flag = 10, overflow_flag = 11
        };

        uint64_t& operator[](size_t i)
        { return this->arr[i]; }
        const uint64_t& operator[](size_t i) const
        { return this->arr[i]; }

        template<uint64_t bit>   // bit starts from 0, use ef_flag enum
        bool flag()
        {
            return (this->rf & (1 << bit)) != 0;
        }

        bool jnb()
        {
            return flag<carry_flag>() == false;
        }
    }; 

    // Lowest level stuff (actual assembly) goes on the following namespace
    // PRIVATE! Skip this, not interesting for you.
    namespace injector_asm
    {
        // Wrapper functor, so the assembly can use some templating
        template<class T>
        struct wrapper
        {
            static void call(reg_pack* regs)
            {
                T fun; fun(*regs);
            }
        };

        // Constructs a reg_pack and calls the wrapper functor
        template<class W>   // where W is of type wrapper
        inline uintptr_t make_reg_pack_and_call()
        {
            static reg_pack pack = { 0 };

            auto fn = injector::asm_invoke([](x86::Assembler& a)
            {
                pushad();

                a.mov(r15, &pack);
                for (size_t i = 0; i < reg_pack::reg_rf; i++)
                {
                    a.mov(r14, qword_ptr(rsp, i * sizeof(uint64_t)));
                    a.mov(qword_ptr(r15, i * sizeof(uint64_t)), r14);
                }
                a.mov(r14, rsp);
                a.add(r14, 0x88);
                a.mov(r14, qword_ptr(r14));
                a.mov(qword_ptr(r15, reg_pack::reg_ret_ptr * sizeof(uint64_t)), r14);

                a.mov(rcx, &pack);
                a.call(W::call);

                popad();

                a.mov(rax, &pack);
                a.mov(r15, qword_ptr(rax, 0 * sizeof(uint64_t)));
                a.mov(r14, qword_ptr(rax, 1 * sizeof(uint64_t)));
                a.mov(r13, qword_ptr(rax, 2 * sizeof(uint64_t)));
                a.mov(r12, qword_ptr(rax, 3 * sizeof(uint64_t)));
                a.mov(r11, qword_ptr(rax, 4 * sizeof(uint64_t)));
                a.mov(r10, qword_ptr(rax, 5 * sizeof(uint64_t)));
                a.mov(r9, qword_ptr(rax, 6 * sizeof(uint64_t)));
                a.mov(r8, qword_ptr(rax, 7 * sizeof(uint64_t)));
                a.mov(rbp, qword_ptr(rax, 8 * sizeof(uint64_t)));
                a.mov(rsp, qword_ptr(rax, 9 * sizeof(uint64_t)));
                a.pushfq();
                a.add(rsp, 0x38);
                a.popfq();
                a.mov(rdi, qword_ptr(rax, 10 * sizeof(uint64_t)));
                a.mov(rsi, qword_ptr(rax, 11 * sizeof(uint64_t)));
                a.mov(rbx, qword_ptr(rax, 12 * sizeof(uint64_t)));
                a.mov(rdx, qword_ptr(rax, 13 * sizeof(uint64_t)));
                a.mov(rcx, qword_ptr(rax, 14 * sizeof(uint64_t)));
                a.mov(rax, qword_ptr(rax, 15 * sizeof(uint64_t)));
                a.ret();
            });

            return (uintptr_t)fn;
        }
    };


    /*
     *  MakeInline
     *      Makes inline assembly (but not assembly, an actual functor of type FuncT) at address
     */
    template<class FuncT>
    void MakeInline(memory_pointer_tr at)
    {
        typedef injector_asm::wrapper<FuncT> functor;
        if(false) functor::call(nullptr);   // To instantiate the template, if not done _asm will fail
        MakeCALLTrampoline(at, injector_asm::make_reg_pack_and_call<functor>());
    }

    /*
     *  MakeInline
     *      Same as above, but it NOPs everything between at and end (exclusive), then performs MakeInline
     */
    template<class FuncT>
    void MakeInline(memory_pointer_tr at, memory_pointer_tr end)
    {
        MakeRangedNOP(at, end);
        MakeInline<FuncT>(at);
    }

    /*
     *  MakeInline
     *      Same as above, but (at,end) are template parameters.
     *      On this case the functor can be passed as argument since there will be one func instance for each at,end not just for each FuncT
     */
    template<uintptr_t at, uintptr_t end, class FuncT>
    void MakeInline(FuncT func)
    {
        static std::unique_ptr<FuncT> static_func;
        static_func.reset(new FuncT(std::move(func)));

        // Encapsulates the call to static_func
        struct Caps
        {
            void operator()(reg_pack& regs)
            { (*static_func)(regs); }
        };

        // Does the actual MakeInline
        return MakeInline<Caps>(lazy_pointer<at>::get(), lazy_pointer<end>::get());
    }

    /*
     *  MakeInline
     *      Same as above, but (end) is calculated by the length of a call instruction
     */
    template<uintptr_t at, class FuncT>
    void MakeInline(FuncT func)
    {
        return MakeInline<at, at+5, FuncT>(func);
    }
};

#endif