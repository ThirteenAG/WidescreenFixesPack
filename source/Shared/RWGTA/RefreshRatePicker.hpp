#pragma once

#include <stdafx.h>

namespace RWGTA
{
    template<bool d3d9>
    inline void InstallRefreshRatePicker(const char* signature)
    {
        static const auto adapter = d3d9 ?
            *hook::pattern("8B 0D ? ? ? ? 8B 06 53 51 56 FF 50 18").get_first<const UINT*>(2) :
            *hook::pattern("A1 ? ? ? ? 8B 3B 50 53 FF 57 18").get_first<const UINT*>(1);
        auto compare = hook::pattern(signature).get_first<uint8_t>();
        static const uintptr_t nextMode = reinterpret_cast<uintptr_t>(compare + 11);
        static auto picker = safetyhook::create_mid(compare, [](SafetyHookContext& regs)
        {
            constexpr uint32_t noMode = INT32_MAX;
            thread_local uint32_t desktop = 0;
            const uint32_t best = regs.ebp;
            const uint32_t candidate = regs.eax;
            if (best == noMode)
            {
                // GetAdapterDisplayMode has the same vtable slot and mode layout in D3D8 and D3D9.
                struct DisplayMode { UINT width, height, refresh, format; } mode{};
                auto device = reinterpret_cast<void*>(d3d9 ? regs.esi : regs.ebx);
                using GetDisplayMode = HRESULT (STDMETHODCALLTYPE*)(void*, UINT, DisplayMode*);
                auto query = reinterpret_cast<GetDisplayMode>((*reinterpret_cast<void***>(device))[8]);
                desktop = SUCCEEDED(query(device, *adapter, &mode)) && mode.refresh > 1 ? mode.refresh : GetDesktopRefreshRate();
            }
            const bool withinDesktop = candidate <= desktop;
            if (best == noMode || (withinDesktop != (best <= desktop) ? withinDesktop : candidate > best))
                regs.ebp = candidate;
            regs.eip = nextMode;
        });
    }
}
