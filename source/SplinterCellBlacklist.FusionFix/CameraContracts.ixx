module;

#include <stdafx.h>
#include <cstddef>

export module CameraContracts;

export namespace CameraContracts
{
    struct Entry
    {
        const char* pattern;
        ptrdiff_t returnOffset;
        unsigned short popBytes;
    };

    inline constexpr Entry DX11[] =
    {
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 4B 4C F9 FF", 568, 20 },
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 0B 4A F9 FF", 560, 24 },
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 CB 47 F9 FF", 554, 32 },
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 9B 45 F9 FF", 562, 28 },
    };

    inline constexpr Entry DX9[] =
    {
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 6B 96 F9 FF", 568, 20 },
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 2B 94 F9 FF", 560, 24 },
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 EB 91 F9 FF", 554, 32 },
        { "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 10 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 00 00 00 00 8B F1 89 75 F0 8B 45 08 50 E8 BB 8F F9 FF", 562, 28 },
    };
}
