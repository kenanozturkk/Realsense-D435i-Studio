#include "OrientationFilter.h"

#include "ImuTypes.h"

#include <QtMath>

namespace imu {

namespace {
const QVector3D worldUp(0.0f, 0.0f, 1.0f);
}

OrientationFilter::OrientationFilter(const Gains &filterGains)
    : gains(filterGains)
{
}

void OrientationFilter::reset()
{
    currentOrientation = QQuaternion();
    elapsedSeconds = 0.0;
    hasInitialOrientation = false;
}

void OrientationFilter::initializeFromGravity(const QVector3D &acceleration)
{
    const QQuaternion tiltWithArbitraryYaw = QQuaternion::rotationTo(acceleration.normalized(), worldUp);
    currentOrientation = (headingOnly(tiltWithArbitraryYaw).conjugated() * tiltWithArbitraryYaw).normalized();
    elapsedSeconds = 0.0;
    hasInitialOrientation = true;
}

void OrientationFilter::update(const QVector3D &angularVelocity, const QVector3D &acceleration,
                               float gravityMagnitude, bool isStationary, double deltaSeconds)
{
    if (!hasInitialOrientation) {
        initializeFromGravity(acceleration);
        return;
    }

    elapsedSeconds += deltaSeconds;

    // Gravity direction the filter currently believes in, expressed in the body frame.
    const QVector3D estimatedUp = currentOrientation.conjugated().rotatedVector(worldUp);
    const QVector3D tiltError = QVector3D::crossProduct(acceleration.normalized(), estimatedUp);
    const float correctionGain = currentGain(isStationary) * accelerationTrust(acceleration, gravityMagnitude);

    // While stationary the gyro only contributes noise, so we skip integrating it.
    const QVector3D measuredRotation = isStationary ? QVector3D() : angularVelocity;
    rotateBy(measuredRotation + tiltError * correctionGain, deltaSeconds);
}

float OrientationFilter::accelerationTrust(const QVector3D &acceleration, float gravityMagnitude) const
{
    const float relativeDeviation = std::abs(acceleration.length() - gravityMagnitude) / gravityMagnitude;
    return qBound(0.0f, 1.0f - relativeDeviation / gains.accelerationTrustBand, 1.0f);
}

float OrientationFilter::currentGain(bool isStationary) const
{
    if (elapsedSeconds < gains.initialConvergenceSeconds)
        return gains.initial;
    return isStationary ? gains.stationary : gains.moving;
}

void OrientationFilter::rotateBy(const QVector3D &angularVelocity, double deltaSeconds)
{
    const float angleRadians = angularVelocity.length() * static_cast<float>(deltaSeconds);
    if (angleRadians <= 0.0f)
        return;

    const QQuaternion step = QQuaternion::fromAxisAndAngle(angularVelocity.normalized(),
                                                           qRadiansToDegrees(angleRadians));
    currentOrientation = (currentOrientation * step).normalized();
}

} // namespace imu
