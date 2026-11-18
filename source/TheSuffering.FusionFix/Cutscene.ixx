module;

#include <stdafx.h>
#include <d3d9.h>

export module Cutscene;

export
{
SafetyHookInline CutsceneBordersHook;
const int32_t* pCutsceneBorders = nullptr;
IDirect3DDevice9* pCutsceneDevice = nullptr; // the game's device, set every frame

bool bCutsceneBars = false; // the game drew (or would draw) its cutscene bars this frame
bool bInCutscene = false;    // the same, kept for the whole frame (cutscenes keep the director's field of view)
bool bInCutsceneView() { return bInCutscene; }

// The side bars are cleared where the game draws its own bars: after the 3D view, before the HUD and subtitles, so
// text wider than the 4:3 picture stays visible
void DrawPillarbox(IDirect3DDevice9* device)
{
    if (!device)
        return;
    IDirect3DSurface9* surface = nullptr;
    if (FAILED(device->GetRenderTarget(0, &surface)))
        return;
    D3DSURFACE_DESC desc{};
    surface->GetDesc(&desc);
    surface->Release();
    const LONG side = std::lround((float(desc.Width) - float(desc.Height) * 4.0f / 3.0f) * 0.5f);
    if (side <= 0)
        return;
    D3DVIEWPORT9 viewport{};
    if (FAILED(device->GetViewport(&viewport)))
        return;
    const D3DRECT rects[] = { { 0, 0, side, LONG(desc.Height) }, { LONG(desc.Width) - side, 0, LONG(desc.Width), LONG(desc.Height) } };
    D3DVIEWPORT9 full{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
    device->SetViewport(&full);
    device->Clear(2, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    device->SetViewport(&viewport);
}

int __fastcall DrawCutsceneBorders(int* self, void*)
{
    // 0 Off, 1 Letterbox (the original bars), 2 Pillarbox (the picture kept at the original 4:3 between side bars),
    // 3 Both
    const int mode = pCutsceneBorders ? *pCutsceneBorders : 1;
    bCutsceneBars = self[13] != 0;
    if (bCutsceneBars && (mode & 2))
        DrawPillarbox(pCutsceneDevice);
    if (!(mode & 1))
        return 0;
    return CutsceneBordersHook.thiscall<int>(self);
}

// Called before Present: the cutscene state of the frame just drawn
void EndCutsceneFrame(IDirect3DDevice9* device)
{
    pCutsceneDevice = device;
    bInCutscene = std::exchange(bCutsceneBars, false);
}

void InitCutsceneBorders()
{
    hook::pattern pattern;
    pattern = hook::pattern("51 53 55 56 8B F1 8B 46 34 85 C0"); //0x4B0B30
    CutsceneBordersHook = safetyhook::create_inline(pattern.get_first(), DrawCutsceneBorders);
}

}
