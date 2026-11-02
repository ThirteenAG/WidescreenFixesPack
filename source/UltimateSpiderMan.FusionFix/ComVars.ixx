module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>

export module ComVars;

export GameRef<uint32_t> nativeWindowed;
export GameRef<D3DPRESENT_PARAMETERS> displayParameters;
export GameRef<IDirect3DDevice9*> displayDevice;
export std::array<GameRef<int32_t>, 6> displayDimensions;
export GameRef<void*> displayQuery;

export GameRef<float> PROJ_ASPECT;
export GameRef<float> PROJ_FIELD_OF_VIEW;
export GameRef<float> PROJ_ZOOM;
export GameRef<uint8_t> ignoreZoom;
export GameRef<int32_t> disableOcclusionCulling;
export GameRef<float> visibilityOrigin;
export GameRef<float> originalDistances;
export GameRef<float> originalSquared;
export GameRef<uintptr_t> game;
export GameRef<uintptr_t> comicCamera;
export GameRef<uintptr_t> nglCurScene;
export GameRef<int32_t> screenWidth;
export GameRef<void*> whiteTexture;
export GameRef<HWND> inputWindow;
export GameRef<uintptr_t> inputManager;
export GameRef<uintptr_t> menuControls;
export GameRef<uintptr_t> regionPool;

export struct DefaultControl
{
    char name[48];
    uint32_t primary, fixedPrimary, secondary, fixedSecondary, pad, fixedPad;
};
export GameRef<DefaultControl> defaultControls;

export template<class T> GameRef<T>* BindGameRef(GameRef<T>& ref, T* address)
{
    if (!address) return nullptr;
    ref.SetAddress(address);
    return &ref;
}


export GameRef<HWND> hWnd;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
