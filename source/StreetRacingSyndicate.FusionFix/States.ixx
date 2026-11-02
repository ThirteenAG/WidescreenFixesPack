module;

#include <stdafx.h>
#include "FixSettings.hpp"

// The game's state tree: every state has its name at +4 and its active child at +80, from GlobalTopState down.

export module States;

import ComVars;

export namespace SRSStates
{

    inline void Init()
    {
        auto pattern = hook::pattern("53 B9 ? ? ? ? E8 ? ? ? ? 33 DB B0 01 68");
        if (auto* top = *pattern.get_first<uint8_t*>(2))
            StateTop.SetAddress(top);
    }

    inline const char* Name(const uint8_t* state)
    {
        return reinterpret_cast<const char*>(state + 4);
    }

    inline const uint8_t* Child(const uint8_t* state)
    {
        return *reinterpret_cast<uint8_t* const*>(state + 80);
    }

    // True when a state of this name is active anywhere in the chain
    inline bool Active(std::string_view name)
    {
        int depth = 0;
        for (auto state = static_cast<const uint8_t*>(StateTop.is_initialized() ? StateTop.get_ptr() : nullptr); state && depth < 32; state = Child(state), ++depth)
        {
            if (name == Name(state))
                return true;
        }
        return false;
    }

    inline const uint8_t* Deepest()
    {
        const uint8_t* last = StateTop.is_initialized() ? StateTop.get_ptr() : nullptr;
        int depth = 0;
        for (auto state = static_cast<const uint8_t*>(StateTop.is_initialized() ? StateTop.get_ptr() : nullptr); state && depth < 32; state = Child(state), ++depth)
            last = state;
        return last;
    }
}
