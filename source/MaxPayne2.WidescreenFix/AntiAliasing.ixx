module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/MaxPayne/TemporalAA.hpp"

export module AntiAliasing;

import ComVars;

// Anti-aliasing: SMAA (PostFX), and TAA, DLAA and FSR through source/Shared/MaxPayne/TemporalAA.hpp. In Max Payne 2,
// P_Camera keeps its view at +0x98 and its projection at +0x120, P_D3D its device at +0x100 and its depth-stencil at
// +0x104.
//
// Pain and bullet time render the game view into a render target with a depth-stencil of its own, and warp it onto the
// screen: those frames aren't resolved and the camera isn't jittered while it lasts.
export namespace AntiAliasing
{
    bool bWarped = false;           // this frame went through the game's post-processing
    bool bWarpedBefore = false;     // the frame before did

    bool IsSMAA()
    {
        return MaxPayneAA::IsSMAA();
    }

    void SetMainCamera(void* pCamera)
    {
        MaxPayneAA::SetMainCamera(pCamera);
    }

    // The post-processing warps the game view
    void OnPostProcessing()
    {
        bWarped = true;
    }

    void BeginGameView()
    {
        bWarpedBefore = bWarped;
        bWarped = false;
        MaxPayneAA::BeginGameView();
        MaxPayneAA::SkipJitter(bWarpedBefore);
    }

    void OnCameraPrepared(uint8_t* pCamera, bool bDirty)
    {
        MaxPayneAA::OnCameraPrepared(pCamera, bDirty, Screen.nWidth, Screen.nHeight);
    }

    void Render(IDirect3DDevice9* device)
    {
        MaxPayneAA::Render(device, !bWarped);
    }

    void BeforeBeginScene(uint8_t* pD3D, bool bReset)
    {
        MaxPayneAA::BeforeBeginScene(pD3D, bReset);
    }

    void AfterBeginScene(uint8_t* pD3D, IDirect3DDevice9* device)
    {
        MaxPayneAA::AfterBeginScene(pD3D, device);
    }

    void OnFreeMode(uint8_t* pD3D)
    {
        MaxPayneAA::OnFreeMode(pD3D);
    }

    void Init()
    {
        MaxPayneAA::Init(FixSettings(), { 0x98, 0x120, 0x100, 0x104 }, "MaxPayne2.WidescreenFix", "GRAPHICS.Sharpness", "MAIN.GraphicsAPI");
    }
}
