#pragma once
#include <cstdint>
#include <cstdlib>
#include <format>
#include <string>

// The PC renderer queues the models of each pass in one fixed list of 4096 entries and drops every model past it,
// so scenery vanished in busy views, more so at wide aspect ratios. The list and its sort keys get room for many
// more models; the console builds have no such limit.
namespace NFSRenderList
{
    constexpr int32_t MaxModels = 32768;

    inline std::string AddressBytes(uintptr_t address)
    {
        return std::format("{:02X} {:02X} {:02X} {:02X}", address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF, (address >> 24) & 0xFF);
    }

    // count: the pass's model counter; models: the game's list; limitChecks: "mov esi, count / cmp esi, 4096" sites;
    // listUses: "add esi, models" sites. Runs before the game's static constructors and renderer init.
    inline void Install(uint32_t* count, uint8_t* models, size_t entrySize, uint32_t limitChecks, uint32_t listUses, bool constructorLoop)
    {
        auto list = static_cast<uint8_t*>(std::calloc(MaxModels, entrySize));

        auto checks = hook::pattern("8B 35 " + AddressBytes(uintptr_t(count)) + " 81 FE 00 10 00 00").count(limitChecks);
        for (size_t i = 0; i < checks.size(); i++)
            injector::WriteMemory(checks.get(i).get<int32_t>(8), MaxModels, true);

        auto uses = hook::pattern("81 C6 " + AddressBytes(uintptr_t(models))).count(listUses);
        for (size_t i = 0; i < uses.size(); i++)
            injector::WriteMemory(uses.get(i).get<uint8_t*>(2), list, true);

        // The static constructor clearing each entry
        if (constructorLoop)
        {
            auto pattern = hook::pattern("B8 " + AddressBytes(uintptr_t(models)) + " B9 00 10 00 00");
            injector::WriteMemory(pattern.get_first<uint8_t*>(1), list, true);
            injector::WriteMemory(pattern.get_first<int32_t>(6), MaxModels, true);
        }

        // The sort keys' vector: reserved once at renderer init and filled by index, never grown
        auto pattern = hook::pattern("89 48 14 68 00 10 00 00 B9");
        injector::WriteMemory(pattern.get_first<int32_t>(4), MaxModels, true);
    }
}
