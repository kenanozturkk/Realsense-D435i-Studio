#include "ContinuousEulerTracker.h"

#include <QtMath>

#include <cmath>

namespace imu {

float ContinuousEulerTracker::wrapDegrees(float angle)
{
    angle = std::fmod(angle + 180.0f, 360.0f);
    if (angle < 0.0f)
        angle += 360.0f;
    return angle - 180.0f;
}

float ContinuousEulerTracker::distance(const EulerAngles &first, const EulerAngles &second)
{
    return std::abs(wrapDegrees(first.rollDegrees - second.rollDegrees))
           + std::abs(wrapDegrees(first.pitchDegrees - second.pitchDegrees))
           + std::abs(wrapDegrees(first.yawDegrees - second.yawDegrees));
}

EulerAngles ContinuousEulerTracker::update(const QQuaternion &orientation)
{
    const EulerAngles primary = toEulerAngles(orientation);
    const EulerAngles alternate {wrapDegrees(primary.rollDegrees + 180.0f),
                                 wrapDegrees(-180.0f - primary.pitchDegrees),
                                 wrapDegrees(primary.yawDegrees + 180.0f)};

    if (!previousAngles) {
        previousAngles = primary;
        return primary;
    }

    EulerAngles chosen = distance(primary, *previousAngles) <= distance(alternate, *previousAngles) ? primary : alternate;

    const bool isNearGimbalLock = std::abs(std::cos(qDegreesToRadians(chosen.pitchDegrees))) < gimbalLockCosineThreshold;
    if (isNearGimbalLock) {
        chosen.rollDegrees = previousAngles->rollDegrees;
        chosen.yawDegrees = previousAngles->yawDegrees;
    }

    previousAngles = chosen;
    return chosen;
}

} // namespace imu
