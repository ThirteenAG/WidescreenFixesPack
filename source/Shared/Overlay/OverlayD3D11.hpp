#pragma once
#include "Overlay.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include "backends/imgui_impl_dx11.h"

namespace asi_overlay
{
    class D3D11Overlay : public Overlay
    {
    public:
        void Present(IDXGISwapChain* swapchain, HMODULE module)
        {
            if (!swapchain) return;
            ID3D11Device* current = nullptr;
            if (FAILED(swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&current)))) return;
            if (device != current)
            {
                ShutdownRenderer();
                DXGI_SWAP_CHAIN_DESC desc{}; swapchain->GetDesc(&desc);
                if (!Initialize(desc.OutputWindow, module)) { current->Release(); return; }
                Scope scope(*this);
                current->GetImmediateContext(&immediate);
                if (!ImGui_ImplDX11_Init(current, immediate))
                {
                    immediate->Release(); immediate = nullptr; current->Release(); return;
                }
                device = current;
            }
            current->Release();
            Scope scope(*this);
            if (!IsOpen()) return;
            // No retained backbuffer: ResizeBuffers does not depend on closing the overlay.
            ID3D11Texture2D* texture = nullptr;
            if (FAILED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) return;
            ID3D11RenderTargetView* target = nullptr;
            D3D11_TEXTURE2D_DESC desc{}; texture->GetDesc(&desc);
            auto result = device->CreateRenderTargetView(texture, nullptr, &target);
            texture->Release();
            if (FAILED(result)) return;
            std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> oldTargets{};
            ID3D11DepthStencilView* depth = nullptr;
            immediate->OMGetRenderTargets(UINT(oldTargets.size()), oldTargets.data(), &depth);
            // Upstream ImGui deliberately clears HS/DS/CS without restoring them.
            // Games with tessellation or compute passes need those stages preserved too.
            ID3D11HullShader* hull = nullptr;
            ID3D11DomainShader* domain = nullptr;
            ID3D11ComputeShader* compute = nullptr;
            std::array<ID3D11ClassInstance*, 256> hullInstances{}, domainInstances{}, computeInstances{};
            UINT hullCount = 256, domainCount = 256, computeCount = 256;
            immediate->HSGetShader(&hull, hullInstances.data(), &hullCount);
            immediate->DSGetShader(&domain, domainInstances.data(), &domainCount);
            immediate->CSGetShader(&compute, computeInstances.data(), &computeCount);
            immediate->OMSetRenderTargets(1, &target, nullptr);
            ImGui_ImplDX11_NewFrame();
            NewFrame(float(desc.Width), float(desc.Height), true);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            immediate->HSSetShader(hull, hullInstances.data(), hullCount);
            immediate->DSSetShader(domain, domainInstances.data(), domainCount);
            immediate->CSSetShader(compute, computeInstances.data(), computeCount);
            if (hull) hull->Release();
            if (domain) domain->Release();
            if (compute) compute->Release();
            for (UINT i = 0; i < hullCount; ++i) if (hullInstances[i]) hullInstances[i]->Release();
            for (UINT i = 0; i < domainCount; ++i) if (domainInstances[i]) domainInstances[i]->Release();
            for (UINT i = 0; i < computeCount; ++i) if (computeInstances[i]) computeInstances[i]->Release();
            immediate->OMSetRenderTargets(UINT(oldTargets.size()), oldTargets.data(), depth);
            for (auto old : oldTargets) if (old) old->Release();
            if (depth) depth->Release(); target->Release();
        }

        void ShutdownRenderer()
        {
            if (!device) return;
            Scope scope(*this);
            SetOpen(false);
            ImGui_ImplDX11_Shutdown();
            if (scope.previous == context) scope.previous = nullptr;
            immediate->Release(); immediate = nullptr;
            device = nullptr; // ImGui backend owned the device reference
            Shutdown();
        }
    private:
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* immediate = nullptr;
    };
}
