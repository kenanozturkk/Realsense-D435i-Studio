#pragma once

#include "ImuTypes.h"

#include <QQuaternion>

#include <optional>

namespace imu {

// Converts orientations to roll/pitch/yaw without the 180-degree jumps of a plain
// conversion. Every orientation has two equivalent Euler solutions:
//   (roll, pitch, yaw)  and  (roll + 180, -180 - pitch, yaw + 180)
// The plain conversion always picks |pitch| <= 90, so tilting past vertical flips roll and
// yaw. Here the solution closest to the previous output is chosen, which lets pitch run on
// past 90 degrees while roll and yaw stay put.
class ContinuousEulerTracker
{
public:
    void reset() { previousAngles.reset(); }
    EulerAngles update(const QQuaternion &orientation);

private:
    // Within ~1 degree of straight up/down, roll and yaw are no longer separable
    // (only their sum is defined), so they are held instead of jittering.
    static constexpr float gimbalLockCosineThreshold = 0.02f;

    static float wrapDegrees(float angle);
    static float distance(const EulerAngles &first, const EulerAngles &second);

    std::optional<EulerAngles> previousAngles;
};

} // namespace imu
