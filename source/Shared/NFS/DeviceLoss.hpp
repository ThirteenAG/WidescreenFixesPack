#pragma once
#include <windows.h>
#include <d3d9.h>
#include <cstdlib>
#include "iathook.hpp"

// Another program taking the screen in exclusive full screen or changing the display mode makes the game's device
// creation fail, and its device's Reset fails when the device is lost again between the game's check and the
// Reset. The games went on without a device when both of their creation attempts failed, and some left the device
// half reset when the Reset failed: both crashed on a null pointer. A failed creation waits for the display and
// tries again; in those games a Reset on a lost device waits until the device can be reset.
namespace NFSDeviceLoss
{
    namespace Detail
    {
        using CreateFn = IDirect3D9* (WINAPI*)(UINT);
        inline CreateFn OriginalCreate = nullptr;
        inline SafetyHookInline CreateDevice{};
        inline SafetyHookInline Reset{};
        inline bool WaitOnReset = false;
        inline bool RetryCreate = true;
        inline bool Waiting = false;

        // Handles the window's messages for a while; a window closed meanwhile ends the game
        inline void PumpMessages(DWORD ms)
        {
            MsgWaitForMultipleObjects(0, nullptr, FALSE, ms, QS_ALLINPUT);
            MSG msg{};
            while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    ExitProcess(0);
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        inline HRESULT WINAPI ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pp)
        {
            auto hr = Reset.stdcall<HRESULT>(device, pp);
            if (Waiting)
                return hr;
            Waiting = true;
            while (hr == D3DERR_DEVICELOST)
            {
                PumpMessages(100);
                if (device->TestCooperativeLevel() != D3DERR_DEVICELOST)
                    hr = Reset.stdcall<HRESULT>(device, pp);
            }
            Waiting = false;
            return hr;
        }

        inline HRESULT Created(HRESULT hr, IDirect3DDevice9** device)
        {
            if (WaitOnReset && SUCCEEDED(hr) && device && *device && !Reset)
                Reset = safetyhook::create_inline((*reinterpret_cast<void***>(*device))[16], ResetHook);
            return hr;
        }

        inline HRESULT WINAPI CreateDeviceHook(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
            D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** device)
        {
            auto create = [&]() { return CreateDevice.stdcall<HRESULT>(d3d, adapter, type, window, flags, pp, device); };
            if (Waiting || !RetryCreate)
                return Created(create(), device);

            // A lost display is waited for; other failures get a few seconds, then the game's own fallback (its
            // second attempt) takes over
            Waiting = true;
            auto hr = create();
            auto giveUp = GetTickCount64() + 3000;
            while (FAILED(hr) && (hr == D3DERR_DEVICELOST || GetTickCount64() < giveUp))
            {
                PumpMessages(250);
                hr = create();
            }
            Waiting = false;
            return Created(hr, device);
        }

        inline IDirect3D9* WINAPI Create(UINT version)
        {
            auto d3d = OriginalCreate(version);
            if (d3d && !CreateDevice)
                CreateDevice = safetyhook::create_inline((*reinterpret_cast<void***>(d3d))[16], CreateDeviceHook);
            return d3d;
        }
    }

    // waitOnReset: for a game that goes on with a half reset device when its Reset fails; retryCreate: false for a
    // game whose own hooks already retry its device creation
    inline void Install(bool waitOnReset, bool retryCreate = true)
    {
        Detail::WaitOnReset = waitOnReset;
        Detail::RetryCreate = retryCreate;
        auto hooks = IATHook::Replace(GetModuleHandleW(nullptr), "d3d9.dll", std::forward_as_tuple("Direct3DCreate9", Detail::Create));
        auto it = hooks.find("Direct3DCreate9");
        if (it == hooks.end())
            std::abort();
        Detail::OriginalCreate = reinterpret_cast<Detail::CreateFn>(it->second.get());
        if (!Detail::OriginalCreate)
            std::abort();
    }
}
