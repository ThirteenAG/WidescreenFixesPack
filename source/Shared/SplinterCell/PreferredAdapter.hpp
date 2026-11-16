#pragma once
#include <dxgi.h>
#include <algorithm>
#include <utility>
#include <vector>
#pragma comment(lib, "dxgi.lib")

// Direct3D 8/9 number adapters by display output, primary display first, so the GPU chosen for the game in the Windows
// graphics settings is ignored. DXGI lists that GPU first: when it drives a display, its first output is swapped with
// adapter 0 in every IDirect3D call that takes an adapter number.
namespace SCPreferredAdapter
{
    inline UINT Preferred = 0;

    inline UINT Map(UINT adapter)
    {
        if (adapter == 0)
            return Preferred;
        if (adapter == Preferred)
            return 0;
        return adapter;
    }

    template<int Version, size_t Slot>
    inline void* Original = nullptr;

    template<int Version, size_t Slot, typename... Args>
    uintptr_t __stdcall Remap(void* d3d, UINT adapter, Args... args)
    {
        return reinterpret_cast<uintptr_t(__stdcall*)(void*, UINT, Args...)>(Original<Version, Slot>)(d3d, Map(adapter), args...);
    }

    template<int Version, size_t Slot, size_t... I>
    void* RemapThunk(std::index_sequence<I...>)
    {
        return reinterpret_cast<void*>(&Remap<Version, Slot, std::conditional_t<true, uintptr_t, std::integral_constant<size_t, I>>...>);
    }

    template<int Version, size_t Slot, size_t ArgCount>
    void Patch(void** vtable)
    {
        auto thunk = RemapThunk<Version, Slot>(std::make_index_sequence<ArgCount>{});
        if (vtable[Slot] == thunk)
            return;
        Original<Version, Slot> = vtable[Slot];
        injector::WriteMemory(&vtable[Slot], thunk, true);
    }

    inline UINT FindPreferred(void* d3d, void** vtable, size_t monitorSlot)
    {
        auto adapterCount = reinterpret_cast<UINT(__stdcall*)(void*)>(vtable[4])(d3d);
        if (adapterCount < 2)
            return 0;

        std::vector<HMONITOR> monitors;
        IDXGIFactory1* factory = nullptr;
        if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))))
            return 0;

        IDXGIAdapter1* adapter = nullptr;
        if (SUCCEEDED(factory->EnumAdapters1(0, &adapter)))
        {
            IDXGIOutput* output = nullptr;
            for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_OUTPUT_DESC desc{};
                if (SUCCEEDED(output->GetDesc(&desc)))
                    monitors.push_back(desc.Monitor);
                output->Release();
            }
            adapter->Release();
        }
        factory->Release();

        auto getAdapterMonitor = reinterpret_cast<HMONITOR(__stdcall*)(void*, UINT)>(vtable[monitorSlot]);
        for (UINT i = 0; i < adapterCount; ++i)
        {
            if (std::find(monitors.begin(), monitors.end(), getAdapterMonitor(d3d, i)) != monitors.end())
                return i;
        }
        return 0;
    }

    inline void Install8(void* d3d)
    {
        auto vtable = *reinterpret_cast<void***>(d3d);
        static bool once = false;
        if (std::exchange(once, true))
            return;

        Preferred = FindPreferred(d3d, vtable, 14);
        if (Preferred == 0)
            return;

        Patch<8, 5, 2>(vtable);  // GetAdapterIdentifier
        Patch<8, 6, 0>(vtable);  // GetAdapterModeCount
        Patch<8, 7, 2>(vtable);  // EnumAdapterModes
        Patch<8, 8, 1>(vtable);  // GetAdapterDisplayMode
        Patch<8, 9, 4>(vtable);  // CheckDeviceType
        Patch<8, 10, 5>(vtable); // CheckDeviceFormat
        Patch<8, 11, 4>(vtable); // CheckDeviceMultiSampleType
        Patch<8, 12, 4>(vtable); // CheckDepthStencilMatch
        Patch<8, 13, 2>(vtable); // GetDeviceCaps
        Patch<8, 14, 0>(vtable); // GetAdapterMonitor
        Patch<8, 15, 5>(vtable); // CreateDevice
    }

    inline void Install9(void* d3d)
    {
        auto vtable = *reinterpret_cast<void***>(d3d);
        static bool once = false;
        if (std::exchange(once, true))
            return;

        Preferred = FindPreferred(d3d, vtable, 15);
        if (Preferred == 0)
            return;

        Patch<9, 5, 2>(vtable);  // GetAdapterIdentifier
        Patch<9, 6, 1>(vtable);  // GetAdapterModeCount
        Patch<9, 7, 3>(vtable);  // EnumAdapterModes
        Patch<9, 8, 1>(vtable);  // GetAdapterDisplayMode
        Patch<9, 9, 4>(vtable);  // CheckDeviceType
        Patch<9, 10, 5>(vtable); // CheckDeviceFormat
        Patch<9, 11, 5>(vtable); // CheckDeviceMultiSampleType
        Patch<9, 12, 4>(vtable); // CheckDepthStencilMatch
        Patch<9, 13, 3>(vtable); // CheckDeviceFormatConversion
        Patch<9, 14, 2>(vtable); // GetDeviceCaps
        Patch<9, 15, 0>(vtable); // GetAdapterMonitor
        Patch<9, 16, 5>(vtable); // CreateDevice
    }

    inline SafetyHookInline shDirect3DCreate{};

    inline void* __stdcall Direct3DCreate8Hook(UINT sdkVersion)
    {
        auto d3d = shDirect3DCreate.stdcall<void*>(sdkVersion);
        if (d3d)
            Install8(d3d);
        return d3d;
    }

    inline void* __stdcall Direct3DCreate9Hook(UINT sdkVersion)
    {
        auto d3d = shDirect3DCreate.stdcall<void*>(sdkVersion);
        if (d3d)
            Install9(d3d);
        return d3d;
    }

    // Call after the module importing d3d8.dll / d3d9.dll is loaded and before it creates the device
    inline void Init(int version)
    {
        if (shDirect3DCreate)
            return;
        if (version == 8)
            shDirect3DCreate = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"d3d8.dll"), "Direct3DCreate8"), Direct3DCreate8Hook);
        else
            shDirect3DCreate = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"d3d9.dll"), "Direct3DCreate9"), Direct3DCreate9Hook);
    }
}
