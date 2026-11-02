module;

#include <stdafx.h>

export module ComVars;

// sRender: the renderer, its D3D device at +98h
export GameRef<uint8_t*> sRender;
// The object that holds the player (dword_D7C938)
export GameRef<void*> dword_D7C938;
