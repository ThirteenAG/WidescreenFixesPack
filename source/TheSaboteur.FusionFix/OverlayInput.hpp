#pragma once
#include <map>
#include <cmath>
#include <cstdint>
#include <gameref.hpp>

namespace SaboteurOverlayInput
{
    struct Action
    {
        float value;
        float held;
        uint32_t flags;
        uint32_t state;
        uint32_t previous;
    };
    static_assert(sizeof(Action) == 20);
    inline std::map<uintptr_t, bool> Held;

    inline void Filter(uintptr_t player, bool blocked)
    {
        if (!player)
            return;
        auto& held = Held[player];
        held |= blocked;
        if (!held)
            return;
        bool active = false;
        for (const auto [offset, count] : { std::pair{ 564u, 238u }, { 5324u, 14u }, { 5604u, 32u }, { 6824u, 95u }, { 8756u, 28u } })
        {
            auto actions = reinterpret_cast<Action*>(player + offset);
            for (unsigned i = 0; i < count; ++i)
            {
                if (offset == 564u || offset == 5604u || (offset == 5324u && i >= 6))
                    active |= std::abs(actions[i].value) > 0.001f;
                // The four native virtual stick axes range from zero to one, centered at one half.
                else if (offset == 8756u && i < 4)
                    active |= std::abs(actions[i].value - 0.5f) > 0.15f;
                const float neutral = offset == 8756u && i < 4 ? 0.5f : 0.0f;
                actions[i] = Action{ neutral, -2.0f, actions[i].flags & ~3u, 0u, 0u };
            }
        }
        if (!blocked && !active)
            held = false;
    }
}
