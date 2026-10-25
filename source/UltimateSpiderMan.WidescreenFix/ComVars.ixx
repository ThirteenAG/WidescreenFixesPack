module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>

export module ComVars;

export GameRef<HWND> hWnd;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
