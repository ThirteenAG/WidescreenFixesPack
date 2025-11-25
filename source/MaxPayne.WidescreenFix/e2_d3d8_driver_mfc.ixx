module;

#include <stdafx.h>
#include "dxsdk\dx8\d3d8.h"
#include "dxsdk\d3dvtbl.h"

export module e2_d3d8_driver_mfc;

import ComVars;

BOOL WINAPI DllMainHook(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH || fdwReason == DLL_PROCESS_DETACH)
        return TRUE;

    return shDllMainHook.unsafe_stdcall<BOOL>(hinstDLL, fdwReason, lpvReserved);
}

void DrawRect(LPDIRECT3DDEVICE8 pDevice, int32_t x, int32_t y, int32_t w, int32_t h, D3DCOLOR color = D3DCOLOR_XRGB(0, 0, 0))
{
    D3DRECT BarRect = { x, y, x + w, y + h };
    pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 0, 0);
}

HRESULT WINAPI EndScene(LPDIRECT3DDEVICE8 pDevice)
{
    if (Screen.bDrawBordersToFillGap)
    {
        //hiding top/left 1px gap
        DrawRect(pDevice, 0, 0, Screen.nWidth, 1);
        DrawRect(pDevice, 0, 0, 1, Screen.nHeight);
        Screen.bDrawBordersToFillGap = false;
    }

    if ((Screen.bDrawBorders || Screen.bDrawBordersForCameraOverlay) && !Screen.bIsInGraphicNovel)
    {
        float fBadCamPosOffset = Screen.bDrawBordersForCameraOverlay ? 10.0f : 0.0f; // for motel camera gap https://i.imgur.com/JGNdm6y.jpg

        DrawRect(pDevice, 0, 0, static_cast<int32_t>(Screen.fHudOffsetReal), Screen.nHeight);
        DrawRect(pDevice, static_cast<int32_t>(Screen.fWidth - Screen.fHudOffsetReal - fBadCamPosOffset), 0, static_cast<int32_t>(Screen.fHudOffsetReal + Screen.fHudOffsetReal), Screen.nHeight);
        Screen.bDrawBorders = false;
    }

    return RealEndScene(pDevice);
}

export void InitE2_D3D8_DRIVER_MFC()
{
    //D3D Hook for borders
    CIniReader iniReader("");
    bool bD3DHookBorders = iniReader.ReadInteger("MAIN", "D3DHookBorders", 1) != 0;
    if (bD3DHookBorders)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "8B 86 ? ? ? ? 8B 08 50 FF 91");
        EndSceneHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs) //0x1002856D
        {
            if (!RealEndScene)
            {
                auto addr = *(uint32_t*)(regs.eax) + (IDirect3DDevice8VTBL::EndScene * 4);
                RealEndScene = *(EndScene_t*)addr;
                injector::WriteMemory(addr, &EndScene, true);
            }
        });
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"e2_d3d8_driver_mfc"), "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 28 8B 45 ? 53 56 33 F6");
    shDllMainHook = safetyhook::create_inline(pattern.get_first(0), DllMainHook);
}