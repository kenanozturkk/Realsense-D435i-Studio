#pragma once

#include <QQuaternion>
#include <QVector3D>

// Body frame used throughout the IMU pipeline (FLU):
//   +X forward (camera looking direction), +Y left, +Z up.
// World frame: +Z up, yaw measured counter-clockwise from the heading at start.

namespace imu {

constexpr float standardGravity = 9.80665f;

struct EulerAngles
{
    float rollDegrees = 0.0f;
    float pitchDegrees = 0.0f;
    float yawDegrees = 0.0f;
};

struct AccelCalibration
{
    QVector3D offset {0.0f, 0.0f, 0.0f};
    QVector3D scale {1.0f, 1.0f, 1.0f};

    QVector3D apply(const QVector3D &rawAcceleration) const
    {
        return (rawAcceleration - offset) * scale;
    }
};

struct FusionSnapshot
{
    QQuaternion orientation;
    EulerAngles angles;
    QVector3D angularVelocity;
    QVector3D acceleration;
    QVector3D gyroBias;
    float gravityMagnitude = standardGravity;
    float biasCalibrationProgress = 0.0f;
    bool isInitialized = false;
    bool isStationary = false;
    bool isBiasCalibrated = false;
};

// RealSense sensor frame is X right, Y down, Z forward; the IMU pipeline works in FLU.
inline QVector3D cameraToBodyFrame(float x, float y, float z)
{
    return {z, -x, -y};
}

EulerAngles toEulerAngles(const QQuaternion &orientation);
QQuaternion headingOnly(const QQuaternion &orientation);

} // namespace imu
