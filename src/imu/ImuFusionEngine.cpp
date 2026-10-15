#include "ImuFusionEngine.h"

namespace imu {

void ImuFusionEngine::reset()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    orientationFilter.reset();
    stationaryDetector.reset();
    biasEstimator.reset();
    headingReference = QQuaternion();
    latestAngularVelocity = QVector3D();
    latestAcceleration = QVector3D();
    gravityMagnitude = standardGravity;
    lastGyroTimestamp = -1.0;
    hasAcceleration = false;
}

void ImuFusionEngine::resetHeading()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    headingReference = headingOnly(orientationFilter.orientation()).conjugated();
}

void ImuFusionEngine::restartBiasCalibration()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    biasEstimator.reset();
    stationaryDetector.reset();
}

void ImuFusionEngine::setAccelCalibration(const AccelCalibration &calibration)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    accelCalibration = calibration;
}

void ImuFusionEngine::addAccelSample(double timestampSeconds, const QVector3D &rawAcceleration)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    latestAcceleration = accelCalibration.apply(rawAcceleration);
    hasAcceleration = true;
    stationaryDetector.addAccelSample(timestampSeconds, latestAcceleration);
    updateGravityReference(latestAcceleration);
}

void ImuFusionEngine::addGyroSample(double timestampSeconds, const QVector3D &rawAngularVelocity)
{
    std::lock_guard<std::mutex> lock(stateMutex);

    const double deltaSeconds = timestampSeconds - lastGyroTimestamp;
    const bool hasValidStep = lastGyroTimestamp > 0.0 && deltaSeconds > 0.0 && deltaSeconds < maxValidStepSeconds;
    lastGyroTimestamp = timestampSeconds;

    latestAngularVelocity = rawAngularVelocity - biasEstimator.bias();
    stationaryDetector.addGyroSample(timestampSeconds, latestAngularVelocity);

    if (!hasValidStep || !hasAcceleration)
        return;

    const bool isStationary = stationaryDetector.isStationary();
    const bool wasBiasCalibrated = biasEstimator.isCalibrated();
    if (isStationary)
        biasEstimator.addStationarySample(rawAngularVelocity, deltaSeconds);

    orientationFilter.update(latestAngularVelocity, latestAcceleration, gravityMagnitude, isStationary, deltaSeconds);

    // Yaw collected before the bias was known contains drift, so the first completed
    // calibration defines where yaw = 0 is.
    if (!wasBiasCalibrated && biasEstimator.isCalibrated())
        headingReference = headingOnly(orientationFilter.orientation()).conjugated();
}

void ImuFusionEngine::updateGravityReference(const QVector3D &acceleration)
{
    // The uncalibrated accelerometer reads ~9.64 instead of 9.81; learning the real
    // magnitude while still keeps the tilt correction from being wrongly down-weighted.
    if (stationaryDetector.isStationary())
        gravityMagnitude += (acceleration.length() - gravityMagnitude) * gravityTrackingRate;
}

FusionSnapshot ImuFusionEngine::snapshot() const
{
    std::lock_guard<std::mutex> lock(stateMutex);

    FusionSnapshot result;
    result.orientation = (headingReference * orientationFilter.orientation()).normalized();
    result.angles = toEulerAngles(result.orientation);
    result.angularVelocity = latestAngularVelocity;
    result.acceleration = latestAcceleration;
    result.gyroBias = biasEstimator.bias();
    result.gravityMagnitude = gravityMagnitude;
    result.biasCalibrationProgress = biasEstimator.calibrationProgress();
    result.isInitialized = orientationFilter.isInitialized();
    result.isStationary = stationaryDetector.isStationary();
    result.isBiasCalibrated = biasEstimator.isCalibrated();
    return result;
}

} // namespace imu
