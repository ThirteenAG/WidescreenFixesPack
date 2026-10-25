#pragma once
#include "NativeMenuABI.hpp"
#include <iomanip>
#include <sstream>

namespace THAWMenu
{
    template <class T> T Read(const void* base, size_t offset = 0)
    {
        T value{};
        if (!base)
            return value;
        auto address = reinterpret_cast<uintptr_t>(base) + offset;
        MEMORY_BASIC_INFORMATION memory{};
        if (VirtualQuery(reinterpret_cast<void*>(address), &memory, sizeof(memory)) && memory.State == MEM_COMMIT &&
            !(memory.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
            address + sizeof(T) <= reinterpret_cast<uintptr_t>(memory.BaseAddress) + memory.RegionSize)
            std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));
        return value;
    }
    inline void* find_pattern(const std::string& bytes)
    {
        auto pattern = hook::pattern(bytes);
        return pattern.size() == 1 ? pattern.get_first() : nullptr;
    }
    inline std::string Bytes(const void* data, size_t count)
    {
        std::ostringstream result;
        for (size_t i = 0; i < count; ++i)
            result << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<unsigned>(static_cast<const uint8_t*>(data)[i]) << ' ';
        return result.str();
    }
    inline void* Native(const char* name)
    {
        auto text = find_pattern(Bytes(name, std::strlen(name) + 1));
        if (!text)
            return nullptr;
        auto row = find_pattern(Bytes(&text, sizeof(text)));
        return row ? Read<void*>(row, sizeof(void*)) : nullptr;
    }
    struct Bindings
    {
        void*(__cdecl* symbol)(uint32_t){};
        char(__cdecl* run)(uint32_t, const Structure*, void*, char){};
        void*(__cdecl* spawn)(uint32_t, const Structure*, char, uint32_t, char, char, int, const Structure*, int,
                              int){};
        WeakSlot*(__thiscall* lookup)(void*, WeakSlot*, const Structure*, uint32_t, int){};
        void(__thiscall* unlink)(void*, WeakSlot*){};
        bool(__cdecl* props)(const Structure*, void*){};
        bool(__cdecl* destroy)(const Structure*, void*){};
        void** manager{};
        void** symbols{};
        int* language{};
        void* call{};
        void* update{};
        void* launch{};
    };
    inline Bindings api;
    inline void* Branch(void* address, size_t offset)
    {
        return address ? reinterpret_cast<void*>(
                             injector::GetBranchDestination(static_cast<uint8_t*>(address) + offset).as_int())
                       : nullptr;
    }
    inline bool Bind()
    {
        auto run = find_pattern("55 8B 6C 24 08 56 55 E8 ? ? ? ? 8B F0 83 C4 04 85 F6");
        api.run = reinterpret_cast<decltype(api.run)>(run);
        api.symbol = reinterpret_cast<decltype(api.symbol)>(Branch(run, 7));
        auto symbol = reinterpret_cast<uint8_t*>(api.symbol);
        if (symbol && symbol[7] == 0x8B && symbol[8] == 0x35)
            api.symbols = Read<void**>(symbol, 9);
        api.spawn = reinterpret_cast<decltype(api.spawn)>(
            find_pattern("53 8B 5C 24 08 56 57 53 E8 ? ? ? ? 8B F8 83 C4 04 85 FF"));
        api.unlink = reinterpret_cast<decltype(api.unlink)>(
            find_pattern("8B 44 24 04 8B 50 04 56 33 F6 3B D6 74 08 8B 48 08 89 4A 08"));
        api.props = reinterpret_cast<decltype(api.props)>(Native("SetScreenElementProps"));
        api.destroy = reinterpret_cast<decltype(api.destroy)>(Native("DestroyScreenElement"));
        auto english = static_cast<uint8_t*>(Native("English"));
        if (english && english[0] == 0x8B && english[1] == 0x0D)
            api.language = Read<int*>(english, 2);
        auto lookup = find_pattern(
            "8B 0D ? ? ? ? 56 8B 74 24 20 6A 01 68 AF 98 C6 40 56 8D 44 24 10 50 E8 ? ? ? ? 8B 4C 24 04 8B 11 56 C7 44 "
            "24 1C 00 00 00 00 FF 52 04");
        if (lookup)
        {
            api.manager = Read<void**>(lookup, 2);
            api.lookup = reinterpret_cast<decltype(api.lookup)>(Branch(lookup, 24));
        }
        auto call = find_pattern("85 C9 6A 00 57 75 03 8B 4E 14 51 50 8B CE 53 E8 ? ? ? ? 5F 5E 5D 32 C0 5B C3");
        api.call = Branch(call, 15);
        api.update = find_pattern("56 57 8B 3D ? ? ? ? 85 FF C6 05 ? ? ? ? 01 C6 05 ? ? ? ? 00 8B F7 0F 84 ? ? ? ?");
        auto controller = find_pattern("68 74 EE B2 36 68 74 EE B2 36 51 8B 4C 24 2C E8");
        api.launch = Branch(controller, 15);
        return api.run && api.symbol && api.spawn && api.unlink && api.props && api.destroy && api.manager &&
               api.symbols && api.lookup && api.call && api.update && api.launch;
    }
    inline void* Lookup(uint32_t id)
    {
        auto manager = Read<void*>(api.manager);
        if (!manager)
            return nullptr;
        Parameters parameters;
        parameters.Name("id", id);
        WeakSlot slot;
        api.lookup(manager, &slot, &parameters.value, Checksum("id"), 0);
        auto value = slot.element;
        if (value)
            api.unlink(value, &slot);
        return value;
    }
    inline void* Lookup(const char* name)
    {
        return Lookup(Checksum(name));
    }
    inline bool Script(const char* name)
    {
        if (!Read<void*>(api.symbols))
            return false;
        auto symbol = api.symbol(Checksum(name));
        return Read<uint8_t>(symbol, 2) == 7;
    }
    inline bool Run(const char* name, const Parameters& parameters)
    {
        if (!Read<void*>(api.symbols))
            return false;
        auto symbol = api.symbol(Checksum(name));
        auto type = Read<uint8_t>(symbol, 2);
        if (type != 7 && type != 8)
            return false;
        api.run(Checksum(name), &parameters.value, nullptr, 0);
        return true;
    }
    inline bool Spawn(const char* name, const Parameters& parameters)
    {
        return Script(name) && api.spawn(Checksum(name), &parameters.value, 0, 0, 0, 0, 0, nullptr, 0, 0);
    }
    inline bool Integer(const Structure* parameters, const char* name, int& value)
    {
        if (!parameters)
            return false;
        auto component = Read<Component*>(parameters, 4);
        for (size_t i = 0; component && i < 256; ++i)
        {
            if (Read<uint32_t>(component, 4) == Checksum(name))
            {
                value = Read<uint32_t>(component, 8);
                return true;
            }
            component = Read<Component*>(component, 12);
        }
        return false;
    }
    inline bool Properties(const Parameters& parameters)
    {
        int id{};
        // The native name getter uses the same component representation as integer.
        for (auto component = parameters.value.head; component; component = component->next)
            if (component->name == Checksum("id"))
                id = component->value;
        return Lookup(id) && api.props(&parameters.value, nullptr);
    }
} // namespace THAWMenu
