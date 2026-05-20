module;

#include "stdafx.h"
#include <d3d9.h>

export module ComVars;

export float fTimeStep = 0.0f;
export bool bPaused = false;

export GameRef<HWND> hWnd;
export GameRef<int> BackbufferWidth;
export GameRef<int> BackbufferHeight;
export GameRef<IDirect3DDevice9*> Direct3DDevice;

template<typename... Args>
class ResChange : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

export __declspec(noinline) ResChange<int, int>& onResChange()
{
    static ResChange<int, int> ResChangeEvent;
    return ResChangeEvent;
}

export bool IsKeyboardKeyPressed(int vkeycode)
{
    return (GetAsyncKeyState(vkeycode) & 0x8000) != 0;
}
