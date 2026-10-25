#include "ImuTypes.h"

#include <QtMath>

namespace imu {

EulerAngles toEulerAngles(const QQuaternion &orientation)
{
    const float w = orientation.scalar();
    const float x = orientation.x();
    const float y = orientation.y();
    const float z = orientation.z();

    const float roll = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
    const float sinPitch = qBound(-1.0f, 2.0f * (w * y - z * x), 1.0f);
    const float pitch = std::asin(sinPitch);
    const float yaw = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));

    // In FLU a positive rotation about +Y tips the nose down; flip it so "nose up" reads positive.
    return {qRadiansToDegrees(roll), -qRadiansToDegrees(pitch), qRadiansToDegrees(yaw)};
}

QQuaternion headingOnly(const QQuaternion &orientation)
{
    const float yawDegrees = toEulerAngles(orientation).yawDegrees;
    return QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, yawDegrees);
}

} // namespace imu
