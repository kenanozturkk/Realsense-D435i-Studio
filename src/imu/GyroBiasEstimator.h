#pragma once

#include <QVector3D>

namespace imu {

// Learns the gyroscope zero-rate offset from samples taken while the device is still.
// The first seconds behave like a plain average (fast initial lock); afterwards it turns
// into a slow moving average so the estimate keeps following temperature-induced drift.
class GyroBiasEstimator
{
public:
    void reset();
    void addStationarySample(const QVector3D &rawAngularVelocity, double deltaSeconds);

    QVector3D bias() const { return estimatedBias; }
    bool isCalibrated() const { return accumulatedStillSeconds >= requiredCalibrationSeconds; }
    float calibrationProgress() const;

private:
    static constexpr double requiredCalibrationSeconds = 1.5;
    static constexpr double trackingTimeConstantSeconds = 5.0;

    QVector3D estimatedBias;
    double accumulatedStillSeconds = 0.0;
};

} // namespace imu
