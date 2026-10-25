#pragma once
#include "Overlay.hpp"
#include <d3d9.h>
#include <vector>
#include "backends/imgui_impl_dx9.h"

namespace asi_overlay
{
    class D3D9Overlay : public Overlay
    {
    public:
        // Call immediately before Present (after the game's EndScene/UI), or at the game's final EndScene
        // while its scene is still open (GTA IV): BeginScene then fails and the overlay draws into that scene.
        void Present(IDirect3DDevice9* current, HMODULE module)
        {
            if (!current) return;
            if (device != current)
            {
                ShutdownRenderer();
                D3DDEVICE_CREATION_PARAMETERS parameters{};
                if (FAILED(current->GetCreationParameters(&parameters)) || !Initialize(parameters.hFocusWindow, module)) return;
                Scope scope(*this);
                if (!ImGui_ImplDX9_Init(current)) return;
                device = current; // backend holds its own reference
            }
            Scope scope(*this);
            if (FAILED(current->TestCooperativeLevel())) return;
            if (!IsOpen()) return;
            IDirect3DSurface9* backbuffer = nullptr;
            if (FAILED(current->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer))) return;
            IDirect3DStateBlock9* state = nullptr;
            if (FAILED(current->CreateStateBlock(D3DSBT_ALL, &state))) { backbuffer->Release(); return; }
            state->Capture();
            D3DCAPS9 caps{}; current->GetDeviceCaps(&caps);
            std::array<IDirect3DSurface9*, 4> targets{};
            UINT count = std::min<UINT>(4, caps.NumSimultaneousRTs);
            for (UINT i = 0; i < count; ++i) current->GetRenderTarget(i, &targets[i]);
            IDirect3DSurface9* depth = nullptr; current->GetDepthStencilSurface(&depth);
            current->SetDepthStencilSurface(nullptr);
            for (UINT i = 1; i < count; ++i) current->SetRenderTarget(i, nullptr);
            current->SetRenderTarget(0, backbuffer);
            // The official backend sets only the states it uses; reset others that change what reaches the
            // backbuffer (sRGB write, colour mask, extra stages). The state block restores all of them.
            current->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            current->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
            current->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFFFFFF);
            current->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
            current->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            current->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            current->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT);
            current->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
            current->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            current->SetSamplerState(0, D3DSAMP_MAXMIPLEVEL, 0);
            current->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, 0);
            for (DWORD stage = 1; stage < 8; ++stage) current->SetTexture(stage, nullptr);
            bool ownScene = SUCCEEDED(current->BeginScene());
            ImGui_ImplDX9_NewFrame();
            NewFrame();
            D3DSURFACE_DESC desc{}; backbuffer->GetDesc(&desc);
            ScaleD3D9DrawData(ImGui::GetDrawData(), float(desc.Width), float(desc.Height));
            if (pretransformed) RenderPretransformed(current, ImGui::GetDrawData());
            else ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            if (ownScene) current->EndScene();
            for (UINT i = 0; i < count; ++i) { current->SetRenderTarget(i, targets[i]); if (targets[i]) targets[i]->Release(); }
            current->SetDepthStencilSurface(depth);
            if (depth) depth->Release();
            state->Apply(); state->Release(); backbuffer->Release();
        }

        // Draw with pre-transformed (XYZRHW) vertices instead of the official backend's fixed-function
        // transforms. GTA IV leaves device state under which fixed-function T&L outputs black RGB (native D3D9
        // and DXVK alike); pre-transformed vertices skip T&L. Textures still go through the official backend.
        bool pretransformed = false;

        // Required before the original Reset, including resets while the overlay is closed.
        void BeforeReset()
        {
            if (!device) return;
            Scope scope(*this);
            ImGui_ImplDX9_InvalidateDeviceObjects();
        }

        void ShutdownRenderer()
        {
            if (!device) return;
            Scope scope(*this);
            SetOpen(false);
            ImGui_ImplDX9_Shutdown();
            if (scope.previous == context) scope.previous = nullptr;
            device = nullptr;
            Shutdown();
        }
    private:
        IDirect3DDevice9* device = nullptr;
        struct Vertex { float x, y, z, rhw; D3DCOLOR color; float u, v; };
        std::vector<Vertex> vertices;

        void RenderPretransformed(IDirect3DDevice9* d, ImDrawData* data)
        {
            if (!data || data->DisplaySize.x <= 0 || data->DisplaySize.y <= 0) return;
            if (data->Textures) for (ImTextureData* texture : *data->Textures) if (texture->Status != ImTextureStatus_OK) ImGui_ImplDX9_UpdateTexture(texture);
            D3DVIEWPORT9 viewport{ 0, 0, DWORD(data->DisplaySize.x), DWORD(data->DisplaySize.y), 0.0f, 1.0f };
            d->SetViewport(&viewport);
            d->SetPixelShader(nullptr);
            d->SetVertexShader(nullptr);
            d->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
            d->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            d->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
            d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            d->SetRenderState(D3DRS_ZENABLE, FALSE);
            d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
            d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
            d->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
            d->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
            d->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
            d->SetRenderState(D3DRS_FOGENABLE, FALSE);
            d->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
            d->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            d->SetRenderState(D3DRS_LIGHTING, FALSE);
            d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            d->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            d->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            d->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            d->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            auto origin = data->DisplayPos;
            for (const ImDrawList* list : data->CmdLists)
            {
                // D3D9 samples at pixel centres; ImGui's coordinates are pixel corners (the backend's +0.5 ortho).
                vertices.resize(list->VtxBuffer.Size);
                for (int i = 0; i < list->VtxBuffer.Size; ++i)
                {
                    auto& v = list->VtxBuffer[i];
                    auto c = v.col; // ImGui RGBA -> D3DCOLOR ARGB
                    vertices[i] = { v.pos.x - origin.x - 0.5f, v.pos.y - origin.y - 0.5f, 0.0f, 1.0f,
                        (c & 0xFF00FF00) | ((c & 0xFF0000) >> 16) | ((c & 0xFF) << 16), v.uv.x, v.uv.y };
                }
                for (auto& command : list->CmdBuffer)
                {
                    if (command.UserCallback) { if (command.UserCallback != ImDrawCallback_ResetRenderState) command.UserCallback(list, &command); continue; }
                    RECT clip{ LONG(command.ClipRect.x - origin.x), LONG(command.ClipRect.y - origin.y), LONG(command.ClipRect.z - origin.x), LONG(command.ClipRect.w - origin.y) };
                    if (clip.right <= clip.left || clip.bottom <= clip.top || command.ElemCount == 0) continue;
                    d->SetScissorRect(&clip);
                    d->SetTexture(0, reinterpret_cast<IDirect3DTexture9*>(command.GetTexID()));
                    d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, UINT(list->VtxBuffer.Size - int(command.VtxOffset)), command.ElemCount / 3,
                        list->IdxBuffer.Data + command.IdxOffset, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
                        vertices.data() + command.VtxOffset, sizeof(Vertex));
                }
            }
        }
    };
}
