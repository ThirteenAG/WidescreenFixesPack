module;

#include "stdafx.h"
#include <d3d12.h>
#include <wrl/client.h>

export module Renderer;

namespace
{
    SafetyHookInline shadingRateHook;
    SafetyHookMid resizePolicyHook;
    struct ShadingCapabilities
    {
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        bool additionalRates = false;
    };
    void SetShadingRate(uint8_t* context, uint8_t rate, uint8_t combiner)
    {
        // FD3D12CommandContext forwards EShadingRate directly to D3D12.
        // 2x4, 4x2 and 4x4 are optional, even on Tier 2 VRS devices. Query
        // the command list's actual device, rather than identifying a vendor.
        // Keep the device alive in the cache so pointer reuse cannot retain
        // the capabilities of a destroyed adapter.
        auto listOwner = *reinterpret_cast<uint8_t**>(context + 464);
        auto list = listOwner ? *reinterpret_cast<ID3D12GraphicsCommandList5**>(listOwner + 72) : nullptr;
        if (list)
        {
            thread_local ShadingCapabilities capabilities;
            Microsoft::WRL::ComPtr<ID3D12Device> device;
            bool additionalRates = false;
            if (SUCCEEDED(list->GetDevice(IID_PPV_ARGS(&device))))
            {
                if (device.Get() != capabilities.device.Get())
                {
                    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options{};
                    capabilities.additionalRates = SUCCEEDED(device->CheckFeatureSupport(
                        D3D12_FEATURE_D3D12_OPTIONS6, &options, sizeof(options)))
                        && options.AdditionalShadingRatesSupported;
                    capabilities.device = device;
                }
                additionalRates = capabilities.additionalRates;
            }
            // A failed capability query cannot establish optional support.
            if (!additionalRates && (rate == D3D12_SHADING_RATE_2X4
                || rate == D3D12_SHADING_RATE_4X2 || rate == D3D12_SHADING_RATE_4X4))
                rate = D3D12_SHADING_RATE_2X2;
        }
        shadingRateHook.ccall<void>(context, rate, combiner);
    }
}

class RendererModule
{
public:
    RendererModule()
    {
        WFP::onInitEvent() += []()
        {
            if (auto address = hook::pattern("80 3D ? ? ? ? 00 74 6D 4C 8B 89 D0 01 00 00 49 83 79 48 00 74 5F 45 0F B6 C0").get_first())
                shadingRateHook = safetyhook::create_inline(address, SetShadingRate);
            // DE's auxiliary views repeatedly alternate the requested height.
            // UE's grow-to-fit policy retains the largest allocation instead
            // of destroying and recreating the scene targets on every switch.
            // Install at the common join after native policy selection. DE
            // writes its console settings after plugin initialization, and
            // auxiliary views can take the branch bypassing the CVar read.
            if (auto address = hook::pattern("48 8B 0D ? ? ? ? 8B 51 04 85 D2 78 ? BE 03 00 00 00 3B D6 0F 4C F2").get_first(24))
            {
                resizePolicyHook = safetyhook::create_mid(address, [](SafetyHookContext& c)
                {
                    c.rsi = 2;
                });
            }
        };
    }
} RendererModuleInstance;
