#pragma once
#include <cstdint>

namespace console
{
class VehicleCameraHold
{
public:
    void Clear() { held = false; }

    bool Update(uint32_t now, bool moved, uint32_t duration)
    {
        if (moved)
        {
            held = true;
            lastMovement = now;
        }
        else if (held && (int32_t(now - lastUpdate) < 0 || now - lastMovement >= duration))
        {
            Clear();
        }
        lastUpdate = now;
        return held;
    }

private:
    uint32_t lastMovement = 0, lastUpdate = 0;
    bool held = false;
};
}
