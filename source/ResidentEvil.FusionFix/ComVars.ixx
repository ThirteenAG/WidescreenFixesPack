module;

#include <stdafx.h>

export module ComVars;

// sRender: the renderer, its D3D device at +98h
export GameRef<uint8_t*> sRender;
