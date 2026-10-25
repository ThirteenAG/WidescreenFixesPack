#pragma once
#include <d3d9.h>
#include "RTTI.h"

namespace ScarfaceDisplayModes
{
    inline SafetyHookInline RefreshHook;
    inline SafetyHookMid PopulateHook;
    inline void** Display = nullptr;
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
        if (!Display || !*Display || !Evaluate)
            return false;
        int count = 0;
        for (const auto& [width, height, refresh] : GetResolutionsList(false))
        {
            if (!Refresh(*Display, nullptr, width, height))
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
        auto displays = hook::range_pattern(begin, end, "8B 0D ? ? ? ? 8B 01 56 57 FF 50 38");
        const auto tables = ScarfaceRTTI::ScanVtables(module);
        const auto table = tables.find(".?AVd3dDisplay@pure3d@@");
        if (populate.size() != 1 || evaluate.size() != 1 || displays.empty() || table == tables.end())
        {
            OutputDebugStringA("Scarface: display-mode patterns unavailable.\n");
            return false;
        }
        Display = *displays.get_first<void**>(2);
        for (size_t index = 1; index < displays.size(); ++index)
        {
            if (*displays.get(index).get<void**>(2) != Display)
            {
                OutputDebugStringA("Scarface: display-mode renderer identity unavailable.\n");
                return false;
            }
        }
        Evaluate = reinterpret_cast<decltype(Evaluate)>(injector::GetBranchDestination(evaluate.get_first(13)).as_int());
        PopulateContinue = reinterpret_cast<uintptr_t>(populate.get_first(8));
        RefreshHook = safetyhook::create_inline(*reinterpret_cast<void**>(table->second + 31 * sizeof(void*)), Refresh);
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
            OutputDebugStringA("Scarface: display-mode hooks unavailable.\n");
            return false;
        }
        return true;
    }
}
