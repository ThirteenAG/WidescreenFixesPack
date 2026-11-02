module;

#include <stdafx.h>
#include <d3d11.h>
#include "FixSettings.hpp"

export module Graphics;

import ComVars;

// Engine settings the game does not offer. Shadow resolution is applied before renderer settings are cached.

namespace
{
    std::atomic<uint32_t> nAnisotropy{ 16 };
    std::atomic<uint32_t> nShadowResolution{ 0 };
    SafetyHookInline GetContainerHook;
    SafetyHookInline GetSamplerHook;

    uintptr_t __fastcall GetSampler(void* cache, void* edx, uintptr_t result, const D3D11_SAMPLER_DESC* description)
    {
        auto minimum = nAnisotropy.load();
        if (minimum && description && (static_cast<uint32_t>(description->Filter) & 0x7F) == D3D11_FILTER_ANISOTROPIC)
        {
            auto filtered = *description;
            filtered.MaxAnisotropy = std::clamp(std::max(filtered.MaxAnisotropy, minimum), 1u, 16u);
            return GetSamplerHook.unsafe_fastcall<uintptr_t>(cache, edx, result, &filtered);
        }
        return GetSamplerHook.unsafe_fastcall<uintptr_t>(cache, edx, result, description);
    }

    void ReadGraphicsSettings()
    {
        nAnisotropy = std::clamp(FixSettings().Int("GRAPHICS.AnisotropicFiltering"), 0, 16);
        nShadowResolution = std::clamp(FixSettings().Int("GRAPHICS.ShadowResolution"), 0, 8192);
    }

    template<typename T>
    void SetField(void* object, const fb::Settings::ClassInfoData* data, const char* name, T value, bool minimum = false)
    {
        for (uint8_t i = 0; i < data->fieldCount; ++i)
        {
            const auto& field = data->fields[i];
            if (field.name && std::string_view(field.name) == name)
            {
                T* target = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(object) + field.offset);
                *target = minimum ? std::max(*target, value) : value;
                return;
            }
        }
    }

    void* __fastcall GetContainer(void* manager, void* edx, void* typeInfo)
    {
        void* object = GetContainerHook.unsafe_fastcall<void*>(manager, edx, typeInfo);
        if (!object || !typeInfo)
            return object;

        auto data = *reinterpret_cast<const fb::Settings::ClassInfoData**>(typeInfo);
        if (!data || !data->name)
            return object;

        // Apply before the renderer copies these settings into shadow resources and sampler caches.
        std::string_view name(data->name);
        if (name == "WorldRenderSettings")
        {
            if (auto resolution = nShadowResolution.load())
                SetField(object, data, "ShadowmapResolution", resolution);
        }
        else if (name == "ShaderSystemSettings")
        {
            if (auto anisotropy = nAnisotropy.load())
                SetField(object, data, "MaxAnisotropy", anisotropy, true);
        }

        return object;
    }
}

class Graphics
{
public:
    Graphics()
    {
        WFP::onInitEvent() += []()
        {
            ReadGraphicsSettings();
            GetContainerHook = safetyhook::create_inline(reinterpret_cast<void*>(fb::Settings::getContainer), GetContainer);

            // Include the requested filtering in the cache key, before a sampler is reused or created.
            auto pattern = hook::pattern("83 EC 08 53 8B 5C 24 14 56 8D 71 60 53 8D 44 24 0C 50 8B CE");
            GetSamplerHook = safetyhook::create_inline(pattern.get_first(), GetSampler);

            WFP::onGameProcessEvent() += []()
            {
                // Filtering takes effect from the next level
                nAnisotropy = std::clamp(FixSettings().Int("GRAPHICS.AnisotropicFiltering"), 0, 16);

                // The minimap misses parts of the road when its render target isn't cleared first
                if (auto clear = fb::Settings::Field<bool>("GameRenderSettings", "InitialClearEnable"))
                    *clear = true;

            };
        };
    }
} Graphics;
