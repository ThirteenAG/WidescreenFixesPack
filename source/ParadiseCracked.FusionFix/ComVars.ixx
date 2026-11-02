module;
#include <stdafx.h>
#include <ddraw.h>
#include <d3d.h>

export module ComVars;

export namespace ParadiseDisplay
{
    GameRef<IDirectDraw7*> DirectDraw;
    GameRef<IDirectDrawSurface7*> Primary;
    GameRef<IDirectDrawSurface7*> Back;
    GameRef<IDirect3DDevice7*> Device;
    GameRef<int32_t> Logical;
    GameRef<int32_t> Corner;
    GameRef<uint8_t*> Movie;
}
export namespace ParadiseOverlay
{
    GameRef<uint8_t*> MouseInput;
    GameRef<uint8_t*> KeyboardInput;
}
export namespace ParadiseHud
{
    GameRef<uint8_t> SlotItem;
}
