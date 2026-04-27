module;

#include <stdafx.h>
#include "common.h"
#include <dxsdk/dx8/d3d8.h>

export module MSAA;

import Skeleton;

export GameRef<IDirect3DDevice8*> RwD3DDevice([]() -> IDirect3DDevice8**
{
    auto pattern = hook::pattern("A1 ? ? ? ? 53 8B 18");
    if (!pattern.empty())
        return *pattern.get_first<IDirect3DDevice8**>(1);
    return nullptr;
});

export GameRef<unsigned int> MaxMultisamplingLevels([]() -> unsigned int*
{
    auto pattern = find_pattern("89 1D ? ? ? ? 89 1D ? ? ? ? BE ? ? ? ? EB", "89 15 ? ? ? ? BE ? ? ? ? 83 C4 ? A1 ? ? ? ? 89 74 24");
    if (!pattern.empty())
        return *pattern.get_first<unsigned int*>(2);
    return nullptr;
});

export GameRef<unsigned int> SelectedMultisamplingLevels([]() -> unsigned int*
{
    auto pattern = find_pattern("89 1D ? ? ? ? A1 ? ? ? ? 8B 0D", "89 15 ? ? ? ? 89 15 ? ? ? ? BE");
    if (!pattern.empty())
        return *pattern.get_first<unsigned int*>(2);
    return nullptr;
});

class MSAA
{
public:
    MSAA()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto szSelectedMultisamplingLevels = iniReader.ReadString("GRAPHICS", "ForceMultisamplingLevel", "");
            static auto nHideAABug = std::clamp(iniReader.ReadInteger("GRAPHICS", "HideAABug", 1), 0, 2);
            static unsigned int MultisamplingLevel = 1;

            auto is_number = [](const std::string& s) -> bool
            {
                return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
            };

            if (is_number(szSelectedMultisamplingLevels))
                MultisamplingLevel = iniReader.ReadInteger("GRAPHICS", "ForceMultisamplingLevel", 1);
            else if (iequals(szSelectedMultisamplingLevels, "max"))
                MultisamplingLevel = 99999;

            auto pattern = hook::pattern("8B C3 5E 5B 81 C4 30 04 00 00 C3 A1 ? ? ? ? 33 FF");
            if (!pattern.empty())
            {
                static auto D3D8System1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    SelectedMultisamplingLevels = std::min(MultisamplingLevel, MaxMultisamplingLevels.get());
                });
            }
            else
            {
                pattern = hook::pattern("43 83 FB ? 76 ? ? ? ? 8B 74 24");
                static auto D3D8System1 = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    SelectedMultisamplingLevels = std::min(MultisamplingLevel, MaxMultisamplingLevels.get());
                });
            }

            pattern = find_pattern("8B C3 5E 5B 81 C4 30 04 00 00 C3 A1 ? ? ? ? 85 C0", "33 C0 ? ? ? 8B 74 24 ? 8B E5");
            static auto D3D8System2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                SelectedMultisamplingLevels = std::min(MultisamplingLevel, MaxMultisamplingLevels.get());
            });

            pattern = hook::pattern("8B C3 5E 5B 81 C4 30 04 00 00 C3 8B 8C 24");
            if (!pattern.empty())
            {
                static auto D3D8System3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    SelectedMultisamplingLevels = std::min(MultisamplingLevel, MaxMultisamplingLevels.get());
                });
            }

            WFP::onEndScene() += []()
            {
                if (SelectedMultisamplingLevels <= 1 || !nHideAABug)
                    return;

                if (!RwD3DDevice)
                    return;

                D3DCOLOR black = D3DCOLOR_ARGB(255, 0, 0, 0);

                // Top 1px
                D3DRECT topRect = { 0, -5, RsGlobal->width, 1 };
                RwD3DDevice->Clear(1, &topRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                // Left 1px
                D3DRECT leftRect = { -5, 0, 1, RsGlobal->height };
                RwD3DDevice->Clear(1, &leftRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                if (nHideAABug > 1)
                {
                    // Bottom 1px
                    D3DRECT bottomRect = { 0, RsGlobal->height - 1, RsGlobal->width, RsGlobal->height + 5 };
                    RwD3DDevice->Clear(1, &bottomRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                    // Right 1px
                    D3DRECT rightRect = { RsGlobal->width - 1, 0, RsGlobal->width + 5, RsGlobal->height + 5 };
                    RwD3DDevice->Clear(1, &rightRect, D3DCLEAR_TARGET, black, 1.0f, 0);
                }
            };
        };
    }
} MSAA;