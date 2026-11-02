#pragma once
#include <intrin.h>
#include <float.h>

namespace Godfather
{
    class FloatingPointScope
    {
        alignas(16) unsigned char state[512];
    public:
        FloatingPointScope() noexcept
        {
            _fxsave(state);
            _fpreset();
        }
        ~FloatingPointScope()
        {
            _fxrstor(state);
        }
    };
}
