#pragma once

#include "ImuTypes.h"

#include <QString>
#include <QVector3D>

#include <vector>

namespace imu {

struct CalibrationPose
{
    QVector3D upAxis;           // body axis that pointed up while this pose was recorded
    QVector3D meanAcceleration; // raw accelerometer average for the pose
};

struct AccelCalibrationResult
{
    AccelCalibration calibration;
    double rawRmsError = 0.0; // how far |raw acceleration| was from gravity, m/s^2
    bool isValid = false;
    QString failureReason;
};

// Computes per-axis offset and scale so that the corrected acceleration has a magnitude
// of exactly one g in every recorded pose:  | scale * (raw - offset) | = g
// A first guess comes from the opposite-pose pairs, then Gauss-Newton refines it, which
// removes the error caused by poses that were not perfectly axis aligned.
class AccelCalibrationSolver
{
public:
    static AccelCalibrationResult solve(const std::vector<CalibrationPose> &poses,
                                        double gravity = standardGravity);

private:
    static constexpr int maxIterations = 50;
    static constexpr double convergenceThreshold = 1e-10;
    static constexpr double minAcceptedScale = 0.9;
    static constexpr double maxAcceptedScale = 1.1;
    static constexpr double maxAcceptedOffset = 1.5; // m/s^2
};

} // namespace imu
