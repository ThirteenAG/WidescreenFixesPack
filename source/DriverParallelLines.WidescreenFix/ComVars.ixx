module;

#include "stdafx.h"
#include <d3d9.h>

export module ComVars;

export GameRef<HWND> hWnd;
export GameRef<uint32_t> dwWindowedMode;
export GameRef<int> BackbufferWidth;
export GameRef<int> BackbufferHeight;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
export GameRef<int32_t> pCurrentMovieStatus;

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
