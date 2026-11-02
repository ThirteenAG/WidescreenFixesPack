module;
#include <stdafx.h>
#include <algorithm>
#include <cstdint>
#include <vector>

export module Imports;



export namespace JCImports
{
    // Find indirect call and jump slots once, including imports rebuilt by packed executables.
    inline const std::vector<uintptr_t>& Slots()
    {
        static const auto slots = []()
        {
            const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            const auto sections = IMAGE_FIRST_SECTION(nt);
            std::vector<uintptr_t> result;
            auto add = [&](const uint32_t* operand)
            {
                const uintptr_t address = *operand;
                for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
                {
                    const auto& section = sections[i];
                    if (!(section.Characteristics & IMAGE_SCN_MEM_READ))
                        continue;
                    const auto start = base + section.VirtualAddress;
                    const auto size = std::min<size_t>(section.Misc.VirtualSize,
                        nt->OptionalHeader.SizeOfImage - section.VirtualAddress);
                    if (size >= sizeof(uintptr_t) && address >= start && address - start <= size - sizeof(uintptr_t))
                    {
                        if (std::find(result.begin(), result.end(), address) == result.end())
                            result.push_back(address);
                        return;
                    }
                }
            };
            auto first = hook::pattern("FF 15 ? ? ? ?").get_first<uint32_t>(2);
            add(first);
            hook::pattern("FF 15 ? ? ? ?").for_each_result([&](hook::pattern_match match) { add(match.get<uint32_t>(2)); });
            hook::pattern("FF 25 ? ? ? ?").for_each_result([&](hook::pattern_match match) { add(match.get<uint32_t>(2)); });
            return result;
        }();
        return slots;
    }

    template<typename T>
    T Replace(const char* library, const char* name, T replacement)
    {
        const auto module = GetModuleHandleA(library);
        const auto target = module ? GetProcAddress(module, name) : nullptr;
        if (!target)
            return nullptr;
        T original = nullptr;
        for (auto address : Slots())
        {
            auto slot = reinterpret_cast<T*>(address);
            if (reinterpret_cast<FARPROC>(*slot) != target)
                continue;
            if (!original)
                original = *slot;
            injector::WriteMemory<T>(slot, replacement, true);
        }
        return original;
    }
}
