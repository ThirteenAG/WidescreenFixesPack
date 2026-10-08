module;

#include "stdafx.h"

export module Pools;

export bool PrepareClassicLODPools()
{
    // Full-map streaming registers more CEntity sector links than DE's local
    // streaming window. CPool::New returns nullptr on exhaustion and native
    // sector insertion dereferences it. Resize at CPools::Initialise, before
    // entities exist; changing a live pool would invalidate its node pointers.
    // Each constructor inlines object allocation, flag allocation, capacity
    // and flag initialization. All four immediates must agree.
    const std::array doubleNodes{
        hook::pattern("BA 20 BF 02 00 FF 50 10 48 89 07").get_first<uint8_t>(1),
        hook::pattern("BA 4C 1D 00 00 FF 50 10 C6 47 18 01").get_first<uint8_t>(1),
        hook::pattern("C7 47 10 4C 1D 00 00 C7 47 14 FF FF FF FF").get_first<uint8_t>(3),
        hook::pattern("48 81 F9 4C 1D 00 00 7C E4").get_first<uint8_t>(3),
    };
    const std::array dummies{
        hook::pattern("BA 60 90 0F 00 FF 50 10 48 89 07").get_first<uint8_t>(1),
        hook::pattern("BA 34 21 00 00 FF 50 10 C6 47 18 01").get_first<uint8_t>(1),
        hook::pattern("C7 47 10 34 21 00 00 C7 47 14 FF FF FF FF").get_first<uint8_t>(3),
        hook::pattern("48 81 F9 34 21 00 00 7C E4").get_first<uint8_t>(3),
    };
    const auto missing = [](const auto& addresses)
    {
        return std::any_of(addresses.begin(), addresses.end(), [](const auto address) { return !address; });
    };
    if (missing(doubleNodes) || missing(dummies))
        return false;
    const auto resize = [](const auto& addresses, uint32_t count, uint32_t stride)
    {
        injector::WriteMemory<uint32_t>(addresses[0], count * stride, true);
        for (size_t i = 1; i < addresses.size(); ++i)
            injector::WriteMemory<uint32_t>(addresses[i], count, true);
    };
    resize(doubleNodes, 100000, 24);
    resize(dummies, 60000, 120);
    return true;
}
