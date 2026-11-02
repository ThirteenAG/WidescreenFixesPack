module;

#include "stdafx.h"
#include <d3d9.h>
#include <stdexcept>
#include "RTTI.h"

export module DisplayModes;

import ComVars;

export namespace ScarfaceDisplayModes
{
    inline SafetyHookInline RefreshHook;
    inline SafetyHookMid PopulateHook;
    inline const char* (__cdecl* Evaluate)(const char*, int, const char*, int, int) = nullptr;
    inline uintptr_t PopulateContinue = 0;

    inline unsigned int __fastcall Refresh(void* display, void*, int width, int height)
    {
        if (!display || width <= 0 || height <= 0)
            return 0;
        const auto api = *reinterpret_cast<IDirect3D9**>(static_cast<uint8_t*>(display) + 12);
        if (!api)
            return 0;
        unsigned int refresh = 0;
        const auto count = api->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
        for (UINT index = 0; index < count; ++index)
        {
            D3DDISPLAYMODE mode{};
            if (SUCCEEDED(api->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, index, &mode)) &&
                mode.Width == static_cast<UINT>(width) && mode.Height == static_cast<UINT>(height) && mode.Format == D3DFMT_X8R8G8B8)
                refresh = std::max(refresh, mode.RefreshRate);
        }
        return refresh;
    }

    inline bool Populate()
    {
        if (!modeDisplay.is_initialized() || !modeDisplay.get() || !Evaluate)
            return false;
        int count = 0;
        for (const auto& [width, height, refresh] : GetResolutionsList(false))
        {
            if (!Refresh(modeDisplay.get(), nullptr, width, height))
                continue;
            // The native script expects height, width and color depth in that order.
            char command[96]{};
            std::snprintf(command, sizeof(command), "AddScreenResolutionEntry( %i, %i, %i );", height, width, 32);
            Evaluate(command, 0, nullptr, 0, -1);
            ++count;
        }
        return count != 0;
    }

    inline bool Install(HMODULE module = GetModuleHandle(nullptr))
    {
        if (RefreshHook && PopulateHook)
            return true;
        const auto base = reinterpret_cast<uintptr_t>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto begin = base + nt->OptionalHeader.SizeOfHeaders;
        const auto end = base + nt->OptionalHeader.SizeOfImage;
        auto populate = hook::range_pattern(begin, end, "83 C4 04 E8 ? ? ? ? A1 ? ? ? ? 85 C0 74 ? 80 38 00 74");
        auto evaluate = hook::range_pattern(begin, end, "6A FF 6A 00 6A 00 6A 00 68 ? ? ? ? E8 ? ? ? ? 83 C4 14 C2 04 00 83 F8 01 75");
        auto displays = hook::range_pattern(begin, end, "83 EC 18 8B 0D ? ? ? ? 8B 01 56 57 FF 50 38 8B 0D ? ? ? ? 8B 11 89 44 24 1C DB 44 24 1C");
        const auto tables = ScarfaceRTTI::ScanVtables(module);
        const auto table = tables.at(".?AVd3dDisplay@pure3d@@");
        modeDisplay.SetAddress(*displays.get_first<void**>(5));
        Evaluate = reinterpret_cast<decltype(Evaluate)>(injector::GetBranchDestination(evaluate.get_first(13)).as_int());
        PopulateContinue = reinterpret_cast<uintptr_t>(populate.get_first(8));
        RefreshHook = safetyhook::create_inline(*reinterpret_cast<void**>(table + 31 * sizeof(void*)), Refresh);
        if (RefreshHook)
        {
            PopulateHook = safetyhook::create_mid(populate.get_first(3), [](SafetyHookContext& regs)
            {
                if (Populate())
                    regs.eip = PopulateContinue;
                else
                    OutputDebugStringA("Scarface: no detected display modes available; retaining the native list.\n");
            });
        }
        if (!RefreshHook || !PopulateHook)
        {
            RefreshHook.reset();
            PopulateHook.reset();
            throw std::runtime_error("Display-mode hooks unavailable");
        }
        return true;
    }
}
