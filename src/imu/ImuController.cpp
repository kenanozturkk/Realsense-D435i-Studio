#include "ImuController.h"

#include "device/DeviceErrors.h"
#include "device/DeviceManager.h"

#include <QDebug>
#include <QtMath>

#include <algorithm>

ImuController::ImuController(DeviceManager *manager, QObject *parent)
    : QObject(parent)
    , deviceManager(manager)
{
    publishTimer.setInterval(publishIntervalMs);
    connect(&publishTimer, &QTimer::timeout, this, &ImuController::publishSnapshot);

    sampleRateTimer.setInterval(1000);
    connect(&sampleRateTimer, &QTimer::timeout, this, &ImuController::updateSampleRates);

    connect(deviceManager, &DeviceManager::deviceConnected, this, [this] {
        if (isStartRequested)
            startPipeline();
    });
    connect(deviceManager, &DeviceManager::deviceDisconnected, this, [this] {
        stopPipeline();
        if (isStartRequested)
            setTrackingState(TrackingState::WaitingForDevice);
    });
}

ImuController::~ImuController()
{
    stopPipeline();
}

void ImuController::start()
{
    isStartRequested = true;
    startPipeline();
}

void ImuController::stop()
{
    isStartRequested = false;
    stopPipeline();
    setTrackingState(TrackingState::Stopped);
}

void ImuController::resetHeading()
{
    fusionEngine.resetHeading();
}

void ImuController::recalibrateGyroBias()
{
    fusionEngine.restartBiasCalibration();
}

void ImuController::setAccelCalibration(const imu::AccelCalibration &calibration)
{
    fusionEngine.setAccelCalibration(calibration);
}

void ImuController::addSampleListener(imu::MotionSampleListener *listener)
{
    std::lock_guard<std::mutex> lock(listenerMutex);
    if (std::find(sampleListeners.begin(), sampleListeners.end(), listener) == sampleListeners.end())
        sampleListeners.push_back(listener);
}

void ImuController::removeSampleListener(imu::MotionSampleListener *listener)
{
    std::lock_guard<std::mutex> lock(listenerMutex);
    sampleListeners.erase(std::remove(sampleListeners.begin(), sampleListeners.end(), listener), sampleListeners.end());
}

QVector3D ImuController::angularVelocity() const
{
    const QVector3D radiansPerSecond = latestSnapshot.angularVelocity;
    return {qRadiansToDegrees(radiansPerSecond.x()), qRadiansToDegrees(radiansPerSecond.y()),
            qRadiansToDegrees(radiansPerSecond.z())};
}

QVector3D ImuController::gyroBias() const
{
    const QVector3D radiansPerSecond = latestSnapshot.gyroBias;
    return {qRadiansToDegrees(radiansPerSecond.x()), qRadiansToDegrees(radiansPerSecond.y()),
            qRadiansToDegrees(radiansPerSecond.z())};
}

void ImuController::startPipeline()
{
    if (isPipelineRunning)
        return;

    if (!deviceManager->isConnected()) {
        setTrackingState(TrackingState::WaitingForDevice);
        return;
    }
    if (!deviceManager->hasMotionModule()) {
        setTrackingState(TrackingState::Error, tr("Bağlı cihazda IMU bulunmuyor."));
        return;
    }

    fusionEngine.reset();
    eulerTracker.reset();

    rs2::config config;
    config.enable_device(deviceManager->serialNumber().toStdString());
    config.enable_stream(RS2_STREAM_GYRO, RS2_FORMAT_MOTION_XYZ32F, gyroFrequencyHz);
    config.enable_stream(RS2_STREAM_ACCEL, RS2_FORMAT_MOTION_XYZ32F, accelFrequencyHz);

    try {
        motionPipeline = rs2::pipeline();
        motionPipeline.start(config, [this](const rs2::frame &frame) { handleFrame(frame); });
    } catch (const rs2::error &error) {
        setTrackingState(TrackingState::Error, tr("IMU başlatılamadı: %1").arg(describeDeviceError(error)));
        return;
    }

    isPipelineRunning = true;
    publishTimer.start();
    sampleRateTimer.start();
    setTrackingState(TrackingState::Calibrating);
}

void ImuController::stopPipeline()
{
    if (!isPipelineRunning)
        return;

    publishTimer.stop();
    sampleRateTimer.stop();
    try {
        motionPipeline.stop();
    } catch (const rs2::error &error) {
        qWarning() << "IMU pipeline stop failed:" << error.what();
    }
    isPipelineRunning = false;
    measuredGyroRate = 0;
    measuredAccelRate = 0;
    emit ratesChanged();
}

void ImuController::handleFrame(const rs2::frame &frame)
{
    if (const rs2::frameset frames = frame.as<rs2::frameset>()) {
        for (const rs2::frame &subFrame : frames) {
            if (const rs2::motion_frame motionFrame = subFrame.as<rs2::motion_frame>())
                handleMotionFrame(motionFrame);
        }
        return;
    }

    if (const rs2::motion_frame motionFrame = frame.as<rs2::motion_frame>())
        handleMotionFrame(motionFrame);
}

void ImuController::handleMotionFrame(const rs2::motion_frame &frame)
{
    const double timestampSeconds = frame.get_timestamp() / 1000.0;
    const rs2_vector cameraVector = frame.get_motion_data();
    const QVector3D bodyVector = imu::cameraToBodyFrame(cameraVector.x, cameraVector.y, cameraVector.z);

    const bool isGyroSample = frame.get_profile().stream_type() == RS2_STREAM_GYRO;

    if (isGyroSample) {
        fusionEngine.addGyroSample(timestampSeconds, bodyVector);
        ++gyroSampleCounter;
    } else {
        fusionEngine.addAccelSample(timestampSeconds, bodyVector);
        ++accelSampleCounter;
    }

    std::lock_guard<std::mutex> lock(listenerMutex);
    for (imu::MotionSampleListener *listener : sampleListeners) {
        if (isGyroSample)
            listener->handleRawGyroSample(timestampSeconds, bodyVector);
        else
            listener->handleRawAccelSample(timestampSeconds, bodyVector);
    }
}

void ImuController::publishSnapshot()
{
    latestSnapshot = fusionEngine.snapshot();
    latestSnapshot.angles = eulerTracker.update(latestSnapshot.orientation);
    if (latestSnapshot.isInitialized)
        setTrackingState(latestSnapshot.isBiasCalibrated ? TrackingState::Tracking : TrackingState::Calibrating);
    emit measurementsUpdated();
}

void ImuController::updateSampleRates()
{
    measuredGyroRate = gyroSampleCounter.exchange(0);
    measuredAccelRate = accelSampleCounter.exchange(0);
    emit ratesChanged();
}

void ImuController::setTrackingState(TrackingState state, const QString &errorMessage)
{
    if (currentTrackingState == state && currentErrorMessage == errorMessage)
        return;
    currentTrackingState = state;
    currentErrorMessage = errorMessage;
    emit trackingStateChanged();
}
