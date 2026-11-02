module;
#include <stdafx.h>
#include <intrin.h>
#include <float.h>

export module FloatingPoint;


export namespace PsiOps
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
