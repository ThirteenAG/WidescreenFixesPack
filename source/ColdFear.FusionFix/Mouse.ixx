module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Mouse;


export namespace ColdFearMouse
{
    inline void Init()
    {
        auto events = hook::pattern("8B 45 E0 83 F8 0E 77 ? 33 D2 8A 90 ? ? ? ? FF 24 95");
        static auto accumulate = safetyhook::create_mid(events.get_first(), [](SafetyHookContext& regs)
        {
            const auto axis = *reinterpret_cast<uint32_t*>(regs.ebp - 0x20);
            if (axis == 0 || axis == 4 || axis == 8)
            {
                auto& delta = *reinterpret_cast<int32_t*>(regs.ebp - 0x1C);
                delta += static_cast<int32_t>(*reinterpret_cast<float*>(regs.esi + 24 + axis));
            }
        });

        auto buffer = hook::pattern("B8 10 00 00 00 8D 55 EC 89 45 F0 89 45 FC 8B 07 C7 45 EC 14 00 00 00");
        static auto bufferSize = safetyhook::create_mid(buffer.get_first(14), [](SafetyHookContext& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.ebp - 4) = 1024;
        });

        // ScreenToClient has already removed the frame; the native branch subtracts it again.
        auto cursor = hook::pattern("A9 00 00 C0 00 74 ? 8B 3D ? ? ? ? 6A 05 FF D7 8B 4D F4");
        injector::WriteMemory<uint8_t>(cursor.get_first<uint8_t>(5), 0xEB, true);
    }
}
