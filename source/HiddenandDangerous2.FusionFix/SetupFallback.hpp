#pragma once

namespace HD2Setup
{
    inline uintptr_t resume{};

    inline void __stdcall Defaults(uint8_t* data)
    {
        std::memset(data, 0, 35);
        auto write16 = [data](size_t offset, uint16_t value) { std::memcpy(data + offset, &value, sizeof(value)); };
        auto write32 = [data](size_t offset, uint32_t value) { std::memcpy(data + offset, &value, sizeof(value)); };
        auto [width, height] = GetDesktopRes();
        if (width <= 0 || height <= 0)
            std::tie(width, height) = std::make_tuple(640, 480);
        write16(0, 12);
        write32(2, static_cast<uint32_t>(width));
        write32(6, static_cast<uint32_t>(height));
        write32(10, 32);
        write16(22, 1);
        write16(24, 256);
        write16(26, 257);
        write16(30, 256);
        write16(32, 1025);
    }

    inline __declspec(naked) void Missing()
    {
        __asm
        {
            lea eax, [ebp - 30h]
            push eax
            call Defaults
            jmp dword ptr [resume]
        }
    }

    inline void Init()
    {
        auto fallback = hook::pattern("6A 00 68 ? ? ? ? 68 ? ? ? ? 6A 00 E8 ? ? ? ? 83 C4 10 8D 55 FC 52 68 ? ? ? ? 68 02 00 00 80 FF D7");
        auto copy = hook::pattern("B9 08 00 00 00 8D 75 D0 BF ? ? ? ? F3 A5 66 A5 B9 ? ? ? ? A4");
        resume = reinterpret_cast<uintptr_t>(copy.get_first());
        injector::MakeJMP(fallback.get_first(), Missing, true);
    }
}
