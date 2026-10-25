#pragma once

#include "ContinuousEulerTracker.h"
#include "ImuFusionEngine.h"
#include "MotionSampleListener.h"

#include <QObject>
#include <QQuaternion>
#include <QTimer>
#include <QVector3D>

#include <librealsense2/rs.hpp>

#include <atomic>
#include <mutex>
#include <vector>

class DeviceManager;

// Bridges the D435i motion module and QML: owns the RealSense pipeline for the IMU
// streams, feeds the fusion engine and publishes its results at display rate.
class ImuController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TrackingState trackingState READ trackingState NOTIFY trackingStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY trackingStateChanged)
    Q_PROPERTY(QQuaternion orientation READ orientation NOTIFY measurementsUpdated)
    Q_PROPERTY(float roll READ roll NOTIFY measurementsUpdated)
    Q_PROPERTY(float pitch READ pitch NOTIFY measurementsUpdated)
    Q_PROPERTY(float yaw READ yaw NOTIFY measurementsUpdated)
    Q_PROPERTY(QVector3D angularVelocity READ angularVelocity NOTIFY measurementsUpdated)
    Q_PROPERTY(QVector3D acceleration READ acceleration NOTIFY measurementsUpdated)
    Q_PROPERTY(QVector3D gyroBias READ gyroBias NOTIFY measurementsUpdated)
    Q_PROPERTY(float gravityMagnitude READ gravityMagnitude NOTIFY measurementsUpdated)
    Q_PROPERTY(float biasCalibrationProgress READ biasCalibrationProgress NOTIFY measurementsUpdated)
    Q_PROPERTY(bool isStationary READ isStationary NOTIFY measurementsUpdated)
    Q_PROPERTY(int gyroRate READ gyroRate NOTIFY ratesChanged)
    Q_PROPERTY(int accelRate READ accelRate NOTIFY ratesChanged)

public:
    enum class TrackingState { Stopped, WaitingForDevice, Calibrating, Tracking, Error };
    Q_ENUM(TrackingState)

    explicit ImuController(DeviceManager *manager, QObject *parent = nullptr);
    ~ImuController() override;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void resetHeading();
    Q_INVOKABLE void recalibrateGyroBias();

    void setAccelCalibration(const imu::AccelCalibration &calibration);
    void addSampleListener(imu::MotionSampleListener *listener);
    void removeSampleListener(imu::MotionSampleListener *listener);

    TrackingState trackingState() const { return currentTrackingState; }
    QString errorMessage() const { return currentErrorMessage; }

    QQuaternion orientation() const { return latestSnapshot.orientation; }
    float roll() const { return latestSnapshot.angles.rollDegrees; }
    float pitch() const { return latestSnapshot.angles.pitchDegrees; }
    float yaw() const { return latestSnapshot.angles.yawDegrees; }
    QVector3D angularVelocity() const;
    QVector3D acceleration() const { return latestSnapshot.acceleration; }
    QVector3D gyroBias() const;
    float gravityMagnitude() const { return latestSnapshot.gravityMagnitude; }
    float biasCalibrationProgress() const { return latestSnapshot.biasCalibrationProgress; }
    bool isStationary() const { return latestSnapshot.isStationary; }
    int gyroRate() const { return measuredGyroRate; }
    int accelRate() const { return measuredAccelRate; }

signals:
    void trackingStateChanged();
    void measurementsUpdated();
    void ratesChanged();

private:
    static constexpr int gyroFrequencyHz = 400;
    static constexpr int accelFrequencyHz = 200;
    static constexpr int publishIntervalMs = 16;

    void startPipeline();
    void stopPipeline();
    void handleFrame(const rs2::frame &frame);
    void handleMotionFrame(const rs2::motion_frame &frame);
    void publishSnapshot();
    void updateSampleRates();
    void setTrackingState(TrackingState state, const QString &errorMessage = {});

    DeviceManager *deviceManager;
    rs2::pipeline motionPipeline;
    imu::ImuFusionEngine fusionEngine;
    imu::FusionSnapshot latestSnapshot;
    imu::ContinuousEulerTracker eulerTracker;

    std::mutex listenerMutex;
    std::vector<imu::MotionSampleListener *> sampleListeners;

    QTimer publishTimer;
    QTimer sampleRateTimer;
    std::atomic<int> gyroSampleCounter {0};
    std::atomic<int> accelSampleCounter {0};
    int measuredGyroRate = 0;
    int measuredAccelRate = 0;

    TrackingState currentTrackingState = TrackingState::Stopped;
    QString currentErrorMessage;
    bool isStartRequested = false;
    bool isPipelineRunning = false;
};
