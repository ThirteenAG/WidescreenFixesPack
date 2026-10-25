module;

#include <stdafx.h>
#include <minidx8/d3d8.h>
#include "../Shared/RWGTA/Depth8.hpp"

export module Depth8;

// The scene camera's depth-stencil as an INTZ texture (Shared/RWGTA/Depth8.hpp), on its own: the Direct3D 8 and 9
// headers can't be in one file
export namespace Depth8
{
    bool Replace(void* device8, void* camera, int32_t extOffset, bool& created)
    {
        return RWGTA::Depth8::Replace(device8, camera, extOffset, created);
    }

    void Restore(void* device8, void* camera, int32_t extOffset)
    {
        RWGTA::Depth8::Restore(device8, camera, extOffset);
    }

    std::pair<uint32_t, uint32_t> Size()
    {
        return RWGTA::Depth8::Size();
    }
}
