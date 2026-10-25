module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/MaxPayne/TemporalAA.hpp"

export module AntiAliasing;

import ComVars;

// Anti-aliasing: SMAA (PostFX), and TAA, DLAA and FSR through source/Shared/MaxPayne/TemporalAA.hpp. In Max Payne,
// P_Camera keeps its view at +0x8C and its projection at +0x114, P_D3D its device at +0xD0 and its depth-stencil at
// +0xD4.
export namespace AntiAliasing
{
    bool IsSMAA()
    {
        return MaxPayneAA::IsSMAA();
    }

    void SetMainCamera(void* pCamera)
    {
        MaxPayneAA::SetMainCamera(pCamera);
    }

    void BeginGameView()
    {
        MaxPayneAA::BeginGameView();
    }

    void OnCameraPrepared(uint8_t* pCamera, bool bDirty)
    {
        MaxPayneAA::OnCameraPrepared(pCamera, bDirty, Screen.nWidth, Screen.nHeight);
    }

    void Render(IDirect3DDevice9* device)
    {
        MaxPayneAA::Render(device);
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
        MaxPayneAA::Init(FixSettings(), { 0x8C, 0x114, 0xD0, 0xD4 }, "MaxPayne.WidescreenFix", "GRAPHICS.Sharpness", "MAIN.GraphicsAPI");
    }
}
