#pragma once
#include <cstdint>

namespace IdleCursor
{
    enum class Action
    {
        None,
        Hide,
        Restore
    };

    class State
    {
    public:
        Action Update(int x, int y, uint64_t now, bool active, int timeout)
        {
            if (timeout != lastTimeout)
            {
                tracking = false;
                lastTimeout = timeout;
            }
            if (!active)
            {
                tracking = false;
                return Action::None;
            }
            if (!tracking || x != lastX || y != lastY || now < lastMoved || timeout <= 0)
            {
                tracking = timeout > 0;
                lastX = x;
                lastY = y;
                lastMoved = now;
                bool restore = hidden;
                hidden = false;
                return restore ? Action::Restore : Action::None;
            }
            if (now - lastMoved >= uint64_t(timeout))
            {
                hidden = true;
                return Action::Hide;
            }
            return Action::None;
        }

    private:
        bool tracking{};
        bool hidden{};
        int lastX{};
        int lastY{};
        int lastTimeout{};
        uint64_t lastMoved{};
    };
}
