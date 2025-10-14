#pragma once
#include <span>
#include <initializer_list>
#include "injector/injector.hpp"

namespace injector
{
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

