#include "StationaryDetector.h"

#include <cmath>

namespace imu {

StationaryDetector::StationaryDetector(const Thresholds &detectionThresholds)
    : thresholds(detectionThresholds)
{
}

void StationaryDetector::reset()
{
    gyroWindow.clear();
    accelWindow.clear();
    calmSinceSeconds = -1.0;
    isCurrentlyStationary = false;
}

void StationaryDetector::addGyroSample(double timestampSeconds, const QVector3D &biasCorrectedAngularVelocity)
{
    gyroWindow.push_back({timestampSeconds, biasCorrectedAngularVelocity});
    dropOlderThan(gyroWindow, timestampSeconds - thresholds.windowSeconds);
    evaluate(timestampSeconds);
}

void StationaryDetector::addAccelSample(double timestampSeconds, const QVector3D &acceleration)
{
    accelWindow.push_back({timestampSeconds, acceleration});
    dropOlderThan(accelWindow, timestampSeconds - thresholds.windowSeconds);
}

void StationaryDetector::dropOlderThan(std::deque<TimedVector> &window, double oldestAllowedSeconds)
{
    while (!window.empty() && window.front().timestampSeconds < oldestAllowedSeconds)
        window.pop_front();
}

QVector3D StationaryDetector::mean(const std::deque<TimedVector> &window)
{
    QVector3D sum;
    for (const TimedVector &sample : window)
        sum += sample.value;
    return sum / static_cast<float>(window.size());
}

float StationaryDetector::standardDeviationNorm(const std::deque<TimedVector> &window, const QVector3D &mean)
{
    float sumOfSquares = 0.0f;
    for (const TimedVector &sample : window)
        sumOfSquares += (sample.value - mean).lengthSquared();
    return std::sqrt(sumOfSquares / static_cast<float>(window.size()));
}

void StationaryDetector::evaluate(double nowSeconds)
{
    const bool hasEnoughSamples = gyroWindow.size() >= 20 && accelWindow.size() >= 10;
    bool isCalm = false;

    if (hasEnoughSamples) {
        const QVector3D gyroMean = mean(gyroWindow);
        const QVector3D accelMean = mean(accelWindow);
        isCalm = gyroMean.length() < thresholds.maxMeanAngularSpeed
                 && standardDeviationNorm(gyroWindow, gyroMean) < thresholds.maxAngularVelocityStdDev
                 && standardDeviationNorm(accelWindow, accelMean) < thresholds.maxAccelerationStdDev;
    }

    if (!isCalm) {
        calmSinceSeconds = -1.0;
        isCurrentlyStationary = false;
        return;
    }

    if (calmSinceSeconds < 0.0)
        calmSinceSeconds = nowSeconds;

    isCurrentlyStationary = (nowSeconds - calmSinceSeconds) >= thresholds.minimumStillSeconds;
}

} // namespace imu
