module;

#include <stdafx.h>

export module ComVars;

// sRender: the renderer, its D3D device at +98h
export GameRef<uint8_t*> sRender;
// The renderer whose scene the video aspect follows
export GameRef<void*> renderer;

// Resolution: adds the saved windowed size to the native catalog
export void (*AddWindowedResolution)(const char*) = nullptr;
