module;
#include "stdafx.h"
#include "d3dvtbl.h"
#include <d3d9.h>

export module SkinRendering;
import SkinCapture;
import CharacterBlood;

namespace
{
    SafetyHookInline drawHook, indexedHook, upHook, indexedUpHook;
    bool installed = false;

    HRESULT WINAPI Draw(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, UINT start, UINT count)
    {
        SkinCapture::CaptureDraw(d);
        auto draw = [&]() { return drawHook.stdcall<HRESULT>(d, type, start, count); };
        const auto result = draw();
        if (SUCCEEDED(result) && !SkinCapture::IsCapturing()) CharacterBlood::Render(d, draw);
        return result;
    }
    HRESULT WINAPI DrawIndexed(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, INT base, UINT min, UINT vertices, UINT start, UINT count)
    {
        SkinCapture::CaptureDraw(d);
        auto draw = [&]() { return indexedHook.stdcall<HRESULT>(d, type, base, min, vertices, start, count); };
        const auto result = draw();
        if (SUCCEEDED(result) && !SkinCapture::IsCapturing()) CharacterBlood::Render(d, draw);
        return result;
    }
    HRESULT WINAPI DrawUP(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, UINT count, const void* data, UINT stride)
    {
        SkinCapture::CaptureDraw(d);
        auto draw = [&]() { return upHook.stdcall<HRESULT>(d, type, count, data, stride); };
        const auto result = draw();
        if (SUCCEEDED(result) && !SkinCapture::IsCapturing()) CharacterBlood::Render(d, draw);
        return result;
    }
    HRESULT WINAPI DrawIndexedUP(IDirect3DDevice9* d, D3DPRIMITIVETYPE type, UINT min, UINT vertices, UINT count, const void* indices, D3DFORMAT format, const void* data, UINT stride)
    {
        SkinCapture::CaptureDraw(d);
        auto draw = [&]() { return indexedUpHook.stdcall<HRESULT>(d, type, min, vertices, count, indices, format, data, stride); };
        const auto result = draw();
        if (SUCCEEDED(result) && !SkinCapture::IsCapturing()) CharacterBlood::Render(d, draw);
        return result;
    }
}

export namespace SkinRendering
{
    void ObserveDevice(IDirect3DDevice9* device)
    {
        if ((!SkinCapture::IsEnabled() && !CharacterBlood::IsEnabled()) || installed || !device) return;
        // Share these entry points: diagnostics observe the original draw, restoration adds its pass.
        auto vtable = *reinterpret_cast<void***>(device);
        drawHook = safetyhook::create_inline(vtable[IDirect3DDevice9VTBL::DrawPrimitive], Draw);
        indexedHook = safetyhook::create_inline(vtable[IDirect3DDevice9VTBL::DrawIndexedPrimitive], DrawIndexed);
        upHook = safetyhook::create_inline(vtable[IDirect3DDevice9VTBL::DrawPrimitiveUP], DrawUP);
        indexedUpHook = safetyhook::create_inline(vtable[IDirect3DDevice9VTBL::DrawIndexedPrimitiveUP], DrawIndexedUP);
        installed = true;
        SkinCapture::ReportDrawHooks(bool(drawHook), bool(indexedHook), bool(upHook), bool(indexedUpHook));
    }

    void Shutdown()
    {
        drawHook.reset(); indexedHook.reset(); upHook.reset(); indexedUpHook.reset();
        installed = false;
    }
}
