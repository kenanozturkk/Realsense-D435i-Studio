#pragma once

#include <QVector3D>

#include <deque>

namespace imu {

// Decides whether the device is standing still by looking at a short sliding window
// of gyro and accelerometer samples. The device must stay calm for a minimum duration
// before it is reported as stationary, so short pauses during motion are ignored.
class StationaryDetector
{
public:
    struct Thresholds
    {
        double windowSeconds = 0.4;
        double minimumStillSeconds = 0.5;
        float maxMeanAngularSpeed = 0.02f;      // rad/s, ~1.1 deg/s
        float maxAngularVelocityStdDev = 0.035f; // rad/s
        float maxAccelerationStdDev = 0.1f;      // m/s^2
    };

    StationaryDetector() = default;
    explicit StationaryDetector(const Thresholds &detectionThresholds);

    void reset();
    void addGyroSample(double timestampSeconds, const QVector3D &biasCorrectedAngularVelocity);
    void addAccelSample(double timestampSeconds, const QVector3D &acceleration);

    bool isStationary() const { return isCurrentlyStationary; }

private:
    struct TimedVector
    {
        double timestampSeconds;
        QVector3D value;
    };

    static void dropOlderThan(std::deque<TimedVector> &window, double oldestAllowedSeconds);
    static QVector3D mean(const std::deque<TimedVector> &window);
    static float standardDeviationNorm(const std::deque<TimedVector> &window, const QVector3D &mean);

    void evaluate(double nowSeconds);

    Thresholds thresholds;
    std::deque<TimedVector> gyroWindow;
    std::deque<TimedVector> accelWindow;
    double calmSinceSeconds = -1.0;
    bool isCurrentlyStationary = false;
};

} // namespace imu
