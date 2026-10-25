#include "GyroBiasEstimator.h"

#include <algorithm>

namespace imu {

void GyroBiasEstimator::reset()
{
    estimatedBias = QVector3D();
    accumulatedStillSeconds = 0.0;
}

void GyroBiasEstimator::addStationarySample(const QVector3D &rawAngularVelocity, double deltaSeconds)
{
    accumulatedStillSeconds += deltaSeconds;
    const double averagingWindow = std::min(accumulatedStillSeconds, trackingTimeConstantSeconds);
    const float blendFactor = static_cast<float>(deltaSeconds / averagingWindow);
    estimatedBias += (rawAngularVelocity - estimatedBias) * blendFactor;
}

float GyroBiasEstimator::calibrationProgress() const
{
    return static_cast<float>(std::min(1.0, accumulatedStillSeconds / requiredCalibrationSeconds));
}

} // namespace imu
