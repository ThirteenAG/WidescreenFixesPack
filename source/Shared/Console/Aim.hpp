#pragma once
#include <cmath>

namespace console {
struct alignas(16) AimVector { float x, y, z, w; };

// Used once when entering manual aim. Native code owns the target object,
// weapon range, collision queries and subsequent stick/mouse movement.
inline bool cameraAim(AimVector& target, const AimVector& origin, const AimVector& forward, float range) {
    const float length2 = forward.x*forward.x + forward.y*forward.y + forward.z*forward.z;
    if (!std::isfinite(length2) || length2 < 0.000001f || !std::isfinite(range) || range <= 0.0f) return false;
    const float distance = range / std::sqrt(length2);
    target = {origin.x + forward.x*distance, origin.y + forward.y*distance,
              origin.z + forward.z*distance, origin.w};
    return true;
}
}
