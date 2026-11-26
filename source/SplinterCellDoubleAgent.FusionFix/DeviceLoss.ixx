module;

#include "stdafx.h"
#include <d3d9.h>

export module DeviceLoss;

namespace
{
    using CreateDeviceFn = void(__fastcall*)(void*, void*, D3DPRESENT_PARAMETERS*, HWND);
    CreateDeviceFn CreateDevice = nullptr;
    ptrdiff_t DeviceOffset = 0;
    thread_local HRESULT CreateResult = D3D_OK;
    int** IsEditorImport = nullptr;
    CRITICAL_SECTION** LoadingLockImport = nullptr;
    uintptr_t FailureReturn = 0;

    IDirect3DDevice9* Device(uintptr_t renderDevice)
    {
        return *reinterpret_cast<IDirect3DDevice9**>(renderDevice + DeviceOffset);
    }

    void __fastcall CreateForWindow(void* renderer, void*, D3DPRESENT_PARAMETERS* parameters, HWND window)
    {
        const auto requested = *parameters;
        const auto start = GetTickCount64();
        for (;;)
        {
            CreateDevice(renderer, nullptr, parameters, window);
            if (Device(reinterpret_cast<uintptr_t>(renderer)) ||
                (CreateResult != D3DERR_DEVICELOST && CreateResult != D3DERR_NOTAVAILABLE) ||
                GetTickCount64() - start >= 5000 || !IsWindow(window))
                return;

            Sleep(100);
            *parameters = requested;
        }
    }
}

export void InitDeviceLoss()
{
    const auto d3dDrv = GetModuleHandleW(L"D3DDrv.dll");
    CreateDevice = reinterpret_cast<CreateDeviceFn>(GetProcAddress(d3dDrv, "?createDevice@UD3DRenderDevice@@QAEXAAU_D3DPRESENT_PARAMETERS_@@PAUHWND__@@@Z"));
    if (!CreateDevice)
        throw std::runtime_error("Render device creation function unavailable");

    // createDevice doesn't check the result of IDirect3D9::CreateDevice
    auto pattern = hook::module_pattern(d3dDrv, "8B 44 24 18 0F 57 C0 33 FF 80 BD ? ? ? ? 00 89 85");
    DeviceOffset = *pattern.get_first<int32_t>(18);
    static SafetyHookMid createResultHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CreateResult = HRESULT(regs.eax);
    });

    // SetRes holds the engine loading lock outside the editor.
    pattern = hook::module_pattern(d3dDrv, "8B 15 ? ? ? ? 83 3A 00 75 ? A1 ? ? ? ? 50 FF 15 ? ? ? ? 8B 4D ? 5F 5E");
    IsEditorImport = *pattern.get_first<int**>(2);
    LoadingLockImport = *pattern.get_first<CRITICAL_SECTION**>(12);
    pattern = hook::module_pattern(d3dDrv, "53 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B 4D ? 64 89 0D 00 00 00 00 5F 5E 5B 8B E5 5D C2 10 00");
    FailureReturn = reinterpret_cast<uintptr_t>(pattern.get_first(13));

    // Check after the native floating-point control word has been restored.
    pattern = hook::module_pattern(d3dDrv, "50 68 ? ? ? ? 8B CE E8 ? ? ? ? 68 00 00 03 00 68 00 00 01 00 FF 15 ? ? ? ? 83 C4 08");
    injector::MakeCALL(pattern.get_first(8), CreateForWindow, true);
    static SafetyHookMid setResHook = safetyhook::create_mid(pattern.get_first(32), [](SafetyHookContext& regs)
    {
        const auto renderDevice = uintptr_t(regs.esi);
        if (!Device(renderDevice))
        {
            if (!**IsEditorImport)
                LeaveCriticalSection(*LoadingLockImport);
            regs.eax = 0;
            regs.eip = FailureReturn;
        }
    });
}
