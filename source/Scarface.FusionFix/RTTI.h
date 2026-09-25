#pragma once

#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ScarfaceRTTI
{
    // MSVC x86 layout, as used by kananlib: vtable[-1] -> Complete Object
    // Locator -> TypeDescriptor. All discovery reads come from a snapshot.
    // Do not call foreign type_info methods or probe pointers by raising faults.
    inline std::unordered_map<std::string, uintptr_t> ScanVtables(HMODULE module)
    {
        static_assert(sizeof(void*) == 4);
        std::unordered_map<std::string, uintptr_t> result;
        const auto base = reinterpret_cast<uintptr_t>(module);
        const auto copy = [](uintptr_t address, void* output, size_t size)
        {
            SIZE_T copied = 0;
            return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
                                     output, size, &copied) && copied == size;
        };
        IMAGE_DOS_HEADER dos{};
        IMAGE_NT_HEADERS32 nt{};
        if (!base || !copy(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
            dos.e_lfanew < 0 || static_cast<uintptr_t>(dos.e_lfanew) > UINTPTR_MAX - base ||
            !copy(base + dos.e_lfanew, &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE ||
            nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            return result;
        const auto size = nt.OptionalHeader.SizeOfImage;
        if (!size || size > UINTPTR_MAX - base) return result;
        const auto end = base + size;
        struct Range { uintptr_t begin, end; };
        std::vector<Range> readable;
        std::vector<uint8_t> image(size);
        for (auto address = base; address < end;)
        {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi))) break;
            const auto region = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            if (region > address || mbi.RegionSize <= address - region) break;
            const auto length = std::min<size_t>(mbi.RegionSize - (address - region), end - address);
            constexpr DWORD readAccess = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
            if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
                (mbi.Protect & readAccess) && copy(address, image.data() + (address - base), length))
            {
                if (!readable.empty() && readable.back().end == address)
                    readable.back().end += length;
                else
                    readable.push_back({ address, address + length });
            }
            address += length;
        }
        const auto available = [&](uintptr_t address) -> size_t
        {
            const auto it = std::upper_bound(readable.begin(), readable.end(), address,
                [](uintptr_t value, const Range& range) { return value < range.begin; });
            if (it == readable.begin()) return 0;
            const auto& range = *std::prev(it);
            return address < range.end ? range.end - address : 0;
        };
        const auto read = [&](uintptr_t address, auto& value)
        {
            if (available(address) < sizeof(value)) return false;
            memcpy(&value, image.data() + (address - base), sizeof(value));
            return true;
        };
        for (const auto& range : readable)
        {
            auto slot = range.begin + ((4 - range.begin % 4) % 4);
            for (; slot < range.end && range.end - slot >= 8; slot += 4)
            {
                uint32_t locator = 0;
                uint32_t col[5]{}; // signature, offset, cdOffset, type, hierarchy.
                if (!read(slot, locator) || !read(locator, col) || col[0] != 0 || col[1] != 0) continue;
                // Only primary vtables are needed by Scarface's hooks.
                uint32_t hierarchy[4]{};
                if (!read(col[4], hierarchy) || hierarchy[0] != 0 || !hierarchy[2]) continue;
                const auto descriptorSize = available(col[3]);
                if (descriptorSize < 12) continue;
                const auto name = reinterpret_cast<const char*>(image.data() + (col[3] - base) + 8);
                if (name[0] != '.' || name[1] != '?') continue;
                const auto terminator = static_cast<const char*>(memchr(name, 0, descriptorSize - 8));
                if (!terminator || !memchr(name, '@', terminator - name)) continue;
                result.try_emplace(std::string(name, terminator), slot + 4);
            }
        }
        return result;
    }

    inline uintptr_t FindVtable(std::string_view rawName)
    {
        static const auto vtables = ScanVtables(GetModuleHandleW(nullptr));
        const auto it = vtables.find(std::string(rawName));
        return it == vtables.end() ? 0 : it->second;
    }
}
