#pragma once

#include <QVector3D>

namespace imu {

// Receives raw (uncalibrated) IMU samples in the body frame.
// Called on the RealSense callback thread, so implementations must be thread-safe and fast.
class MotionSampleListener
{
public:
    virtual ~MotionSampleListener() = default;
    virtual void handleRawGyroSample(double timestampSeconds, const QVector3D &angularVelocity) = 0;
    virtual void handleRawAccelSample(double timestampSeconds, const QVector3D &acceleration) = 0;
};

} // namespace imu
