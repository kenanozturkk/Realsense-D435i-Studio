#pragma once

#include <QQuaternion>
#include <QVector3D>

namespace imu {

// Quaternion complementary filter (Mahony style).
//  - The gyroscope is integrated to follow fast rotations.
//  - The accelerometer pulls roll/pitch back toward gravity, which removes their drift.
//  - Yaw has no absolute reference without a magnetometer; it is only protected by
//    good bias removal and by freezing integration while the device is stationary.
class OrientationFilter
{
public:
    struct Gains
    {
        float initial = 10.0f;
        float moving = 0.5f;
        float stationary = 2.0f;
        double initialConvergenceSeconds = 1.0;
        float accelerationTrustBand = 0.1f; // accepted |a| deviation from gravity, as a fraction
    };

    OrientationFilter() = default;
    explicit OrientationFilter(const Gains &filterGains);

    void reset();
    bool isInitialized() const { return hasInitialOrientation; }

    void initializeFromGravity(const QVector3D &acceleration);
    void update(const QVector3D &angularVelocity, const QVector3D &acceleration,
                float gravityMagnitude, bool isStationary, double deltaSeconds);

    QQuaternion orientation() const { return currentOrientation; }

private:
    float accelerationTrust(const QVector3D &acceleration, float gravityMagnitude) const;
    float currentGain(bool isStationary) const;
    void rotateBy(const QVector3D &angularVelocity, double deltaSeconds);

    Gains gains;
    QQuaternion currentOrientation;
    double elapsedSeconds = 0.0;
    bool hasInitialOrientation = false;
};

} // namespace imu
