module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include "../Shared/RWGTA/AntiAliasingSettings.hpp"

export module AntiAliasing;

import Skeleton;
import Main;
import Depth8;

// SMAA, TAA, DLAA and FSR: see Shared/RWGTA/AntiAliasing.hpp
export namespace AntiAliasing
{
    bool IsSMAA()
    {
        return RWGTA::AntiAliasing::IsSMAA();
    }

    void Render(IDirect3DDevice9* device)
    {
        RWGTA::AntiAliasing::Render(device);
    }
}

class AntiAliasingInit
{
public:
    AntiAliasingInit()
    {
        WFP::onInitEvent() += []()
        {
            RWGTA::AntiAliasing::Game game;
            game.settings = &FixSettings();
            game.clientName = "GTAVC.WidescreenFix";
            game.sceneCamera = []() -> void* { return Scene.is_initialized() ? Scene.get_ptr()->m_pRwCamera : nullptr; };
            game.device8 = []() -> IUnknown* { return pD3D8Device ? *pD3D8Device : nullptr; };
            game.replaceDepth = Depth8::Replace;
            game.restoreDepth = Depth8::Restore;
            game.depthSize = Depth8::Size;
            RWGTA::AntiAliasing::Init(game, "GRAPHICS.Sharpness", "MAIN.GraphicsAPI");
        };
    }
} AntiAliasingInit;
