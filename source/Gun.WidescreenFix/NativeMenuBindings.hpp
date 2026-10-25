#pragma once
#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"

namespace GunMenu
{
    template <class T> T Read(const void* base, size_t offset = 0)
    {
        T value{};
        auto address = reinterpret_cast<uintptr_t>(base) + offset;
        MEMORY_BASIC_INFORMATION memory{};
        if (base && VirtualQuery(reinterpret_cast<void*>(address), &memory, sizeof(memory)) && memory.State == MEM_COMMIT &&
            !(memory.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
            address + sizeof(T) <= reinterpret_cast<uintptr_t>(memory.BaseAddress) + memory.RegionSize)
            std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));
        return value;
    }

    inline auto symbol = reinterpret_cast<void*(__cdecl*)(uint32_t)>(0x4204A0);
    inline auto run = reinterpret_cast<char(__cdecl*)(uint32_t, const Structure*, void*, char)>(0x41C910);
    inline auto lookup = reinterpret_cast<WeakSlot*(__stdcall*)(WeakSlot*, uint32_t, int)>(0x4F8040);
    inline auto unlink = reinterpret_cast<void(__thiscall*)(void*, WeakSlot*)>(0x43EB20);
    inline auto props = reinterpret_cast<bool(__cdecl*)(const Structure*, void*)>(0x4FA1F0);
    inline auto destroy = reinterpret_cast<bool(__cdecl*)(const Structure*, void*)>(0x4FA9B0);
    inline auto factory = reinterpret_cast<WeakSlot*(__thiscall*)(void*, WeakSlot*, uint32_t, uint32_t, const Structure*)>(0x4F8D70);

    inline bool Create(uint32_t id, uint32_t type, const Parameters& parameters)
    {
        auto* manager = Read<void*>(reinterpret_cast<void*>(0x81248C));
        if (!manager) return false;
        WeakSlot slot;
        factory(manager, &slot, type, id, &parameters.value);
        const bool result = slot.element != nullptr;
        if (slot.element) unlink(slot.element, &slot);
        return result;
    }

    inline void* Lookup(uint32_t id)
    {
        if (!Read<void*>(reinterpret_cast<void*>(0x81248C))) return nullptr;
        WeakSlot slot;
        lookup(&slot, id, 0);
        auto* result = slot.element;
        if (result) unlink(result, &slot);
        return result;
    }
    inline void* Lookup(const char* id) { return Lookup(Checksum(id)); }
    inline bool Script(const char* name)
    {
        return Read<void*>(reinterpret_cast<void*>(0x6C9494)) && Read<uint8_t>(symbol(Checksum(name)), 2) == 7;
    }
    inline bool Run(const char* name, const Parameters& parameters)
    {
        if (!Script(name)) return false;
        run(Checksum(name), &parameters.value, nullptr, 0);
        return true;
    }
    inline bool Properties(uint32_t id, Parameters& parameters)
    {
        if (!Lookup(id)) return false;
        parameters.Name("id", id);
        props(&parameters.value, nullptr);
        return true;
    }
    inline void Destroy(uint32_t id)
    {
        if (!Lookup(id)) return;
        Parameters parameters;
        parameters.Name("id", id);
        destroy(&parameters.value, nullptr);
    }
    inline std::string AutoLanguage()
    {
        // The native English script function compares this same frontend language value with 2.
        switch (Read<int>(reinterpret_cast<void*>(0x81C538)))
        {
        case 3: return "fr";
        case 4: return "es";
        case 5: return "de";
        case 6: return "it";
        default: return "en";
        }
    }
} // namespace GunMenu
