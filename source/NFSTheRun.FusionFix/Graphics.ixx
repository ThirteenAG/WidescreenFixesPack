module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Graphics;

import ComVars;

// Engine settings the game doesn't offer. They're applied on every frame, as the game sets some of them again when
// a level loads.

namespace
{
    bool bMinimapFix = true;
    uint32_t nAnisotropy = 16;
    uint32_t nShadowResolution = 0;
    float fShadowDistance = 0.0f;
}

class Graphics
{
public:
    Graphics()
    {
        WFP::onInitEvent() += []()
        {
            WFP::onGameProcessEvent() += []()
            {
                // Read on every frame: the minimap fix applies at once, filtering from the next level
                bMinimapFix = FixSettings().Int("GRAPHICS.MinimapFix") != 0;
                nAnisotropy = std::clamp(FixSettings().Int("GRAPHICS.AnisotropicFiltering"), 0, 16);
                nShadowResolution = std::clamp(FixSettings().Int("GRAPHICS.ShadowResolution"), 0, 8192);
                fShadowDistance = std::max(FixSettings().Float("GRAPHICS.ShadowDistance"), 0.0f);

                // The minimap misses parts of the road when its render target isn't cleared first
                static int gameClear = -1;  // the game's own value while the fix is on
                if (bMinimapFix || gameClear >= 0)
                {
                    if (auto clear = fb::Settings::Field<bool>("GameRenderSettings", "InitialClearEnable"))
                    {
                        if (bMinimapFix)
                        {
                            if (gameClear < 0)
                                gameClear = *clear;
                            *clear = true;
                        }
                        else
                        {
                            *clear = gameClear != 0;
                            gameClear = -1;
                        }
                    }
                }

                // At least this much anisotropic filtering; the game sets its own value again when a level loads
                if (nAnisotropy)
                {
                    if (auto anisotropy = fb::Settings::Field<uint32_t>("ShaderSystemSettings", "MaxAnisotropy"))
                        *anisotropy = std::max(*anisotropy, nAnisotropy);
                }

                // Sharper shadows and shadows further away, from the next level loaded
                if (nShadowResolution)
                {
                    if (auto resolution = fb::Settings::Field<uint32_t>("WorldRenderSettings", "ShadowmapResolution"))
                        *resolution = nShadowResolution;
                }

                if (fShadowDistance > 0.0f)
                {
                    if (auto distance = fb::Settings::Field<float>("WorldRenderSettings", "ShadowmapViewDistance"))
                        *distance = fShadowDistance;
                }
            };
        };
    }
} Graphics;
