module;

#include <stdafx.h>

export module CpuOptimizations;

import ComVars;

SafetyHookInline shD3DXCpuOptimizations{};
HRESULT WINAPI D3DXCpuOptimizationsHook(BOOL Enable)
{
    return shD3DXCpuOptimizations.stdcall<HRESULT>(FALSE);
}

export void InitCpuOptimizations()
{
    auto hD3DX = GetModuleHandleW(L"d3dx10_41.dll");
    if (!hD3DX)
        hD3DX = LoadLibraryW(L"d3dx10_41.dll");

    if (hD3DX)
    {
        auto pD3DXCpuOptimizations = reinterpret_cast<void*>(GetProcAddress(hD3DX, "D3DXCpuOptimizations"));
        if (pD3DXCpuOptimizations)
        {
            shD3DXCpuOptimizations = safetyhook::create_inline(pD3DXCpuOptimizations, reinterpret_cast<void*>(&D3DXCpuOptimizationsHook));
        }
    }
}
