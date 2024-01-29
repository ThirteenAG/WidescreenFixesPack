#pragma once
#include <span>
#include <initializer_list>
#include "injector/injector.hpp"
#include "../external/asmjit/src/asmjit/x86.h"
#include "Trampoline.h"
using namespace asmjit;
using namespace x86;
typedef void (*asmjit_Func)();

namespace injector
{
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

    class raw_mem
    {
    public:
        raw_mem(injector::memory_pointer_tr addr, std::initializer_list<uint8_t> bytes, bool offset_back = false)
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
};

namespace injector
{
    inline bool UnprotectMemory(memory_pointer_tr addr, size_t size)
    {
        DWORD out_oldprotect = 0;
        return VirtualProtect(addr.get(), size, PAGE_EXECUTE_READWRITE, &out_oldprotect) != 0;
    }

#ifdef _WIN64
    inline injector::memory_pointer_raw MakeCALLTrampoline(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
    {
        auto trampoline = Trampoline::MakeTrampoline((void*)at.as_int());
        return MakeCALL(at, trampoline->Jump(dest));
    }
#endif
};
