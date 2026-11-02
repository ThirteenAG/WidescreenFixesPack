module;

#include "stdafx.h"
#include "d3d9.h"

export module ComVars;

export ProtectedGameRef<uint8_t> PauseInBackground;

export GameRef<uintptr_t> gHUD;
export GameRef<uintptr_t> gMovies;
export GameRef<uintptr_t> gPlayers;
export GameRef<uintptr_t> gRenderer;
export GameRef<uintptr_t> fovGameCamera;
export GameRef<int> fovCameraCount;
export GameRef<uintptr_t> cutsceneGameCamera;
export GameRef<int> cutsceneCameraCount;
export GameRef<float> minimapBounds;
export GameRef<float> mapCanvasHeight;
export GameRef<float> mapCanvasWidth;
export GameRef<uintptr_t> jobSystem;
export GameRef<uint8_t> gameIsRunning;
export GameRef<uintptr_t> nativeLanguage;

export template<class T> GameRef<T>* BindGameRef(GameRef<T>& ref, T* address)
{
    if (!address) return nullptr;
    ref.SetAddress(address);
    return &ref;
}

export struct PblVector4
{
    float x;
    float y;
    float z;
    float w;
};

export struct PblMatrix4x4
{
    PblVector4 _m[4];
};

export IDirect3DDevice9* Direct3DDevice = nullptr;