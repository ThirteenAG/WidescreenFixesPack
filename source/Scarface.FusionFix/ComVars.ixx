module;
#include "stdafx.h"

export module ComVars;

export namespace Game
{
    template<class T> T& Field(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }

}
