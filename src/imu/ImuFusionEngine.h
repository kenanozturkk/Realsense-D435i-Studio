#pragma once

#include "GyroBiasEstimator.h"
#include "ImuTypes.h"
#include "OrientationFilter.h"
#include "StationaryDetector.h"

#include <mutex>

namespace imu {

// Thread-safe facade over the IMU processing chain. Samples are pushed from the
// RealSense callback thread, snapshots are pulled from the UI thread.
//
// Gyro samples drive the filter (400 Hz); accelerometer samples (200 Hz) only update
// the latest acceleration, which is used for the tilt correction on the next gyro step.
class ImuFusionEngine
{
public:
    void reset();
    void resetHeading();
    void restartBiasCalibration();
    void setAccelCalibration(const AccelCalibration &calibration);

    void addGyroSample(double timestampSeconds, const QVector3D &rawAngularVelocity);
    void addAccelSample(double timestampSeconds, const QVector3D &rawAcceleration);

    FusionSnapshot snapshot() const;

private:
    static constexpr double maxValidStepSeconds = 0.1;
    static constexpr float gravityTrackingRate = 0.002f;

    void updateGravityReference(const QVector3D &acceleration);

    mutable std::mutex stateMutex;

    OrientationFilter orientationFilter;
    StationaryDetector stationaryDetector;
    GyroBiasEstimator biasEstimator;
    AccelCalibration accelCalibration;

    QQuaternion headingReference;
    QVector3D latestAngularVelocity;
    QVector3D latestAcceleration;
    float gravityMagnitude = standardGravity;
    double lastGyroTimestamp = -1.0;
    bool hasAcceleration = false;
};

} // namespace imu
