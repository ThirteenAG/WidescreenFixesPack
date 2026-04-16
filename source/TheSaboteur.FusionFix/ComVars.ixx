module;

#include "stdafx.h"
#include "d3d9.h"

export module ComVars;

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