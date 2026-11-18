module;

#include "stdafx.h"
#include <d3d9.h>

export module DeviceLoss;

// A windowed device is lost while another program changes the display mode or holds it in fullscreen, and Windows
// resizes the game window then. SetRes for that resize fails its Reset, releases the device, and can't create the
// new one yet; the game went on with no device and crashed. SetRes now waits for the device, as the game's frame
// already does for a lost one: as long as the device is lost or unavailable, and a few seconds for other errors.
namespace
{
    using CreateDeviceFn = void(__fastcall*)(void*, void*, D3DPRESENT_PARAMETERS*, HWND);
    CreateDeviceFn CreateDevice = nullptr;
    ptrdiff_t DeviceOffset = 0;
    D3DPRESENT_PARAMETERS* Parameters = nullptr;
    HRESULT CreateResult = D3D_OK;

    IDirect3DDevice9* Device(uintptr_t renderDevice)
    {
        return *reinterpret_cast<IDirect3DDevice9**>(renderDevice + DeviceOffset);
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

    // Right after SetRes created the device
    pattern = hook::module_pattern(d3dDrv, "50 68 ? ? ? ? 8B CE E8 ? ? ? ? 68 00 00 03 00 68 00 00 01 00 FF 15");
    Parameters = *pattern.get_first<D3DPRESENT_PARAMETERS*>(2);
    static SafetyHookMid setResHook = safetyhook::create_mid(pattern.get_first(13), [](SafetyHookContext& regs)
    {
        const auto renderDevice = uintptr_t(regs.esi);
        const auto start = GetTickCount64();
        while (!Device(renderDevice) && (CreateResult == D3DERR_DEVICELOST || CreateResult == D3DERR_NOTAVAILABLE || GetTickCount64() - start < 5000))
        {
            Sleep(100);
            CreateDevice(reinterpret_cast<void*>(renderDevice), nullptr, Parameters, Parameters->hDeviceWindow);
        }
    });
}
