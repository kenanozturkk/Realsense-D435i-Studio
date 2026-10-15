#pragma once

#include "AccelCalibrationSolver.h"
#include "CalibrationStore.h"
#include "MotionSampleListener.h"
#include "StationaryDetector.h"

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVector3D>

#include <array>
#include <mutex>
#include <optional>

class DeviceManager;
class ImuController;

// Drives the six-pose accelerometer calibration wizard.
// The pose currently facing up is detected from the raw acceleration; once the camera has
// been still for a moment, samples are averaged and the pose is recorded automatically.
// Poses can be recorded in any order. After all six, the solver computes the correction.
class AccelCalibrationController : public QObject, public imu::MotionSampleListener
{
    Q_OBJECT
    Q_PROPERTY(WizardState wizardState READ wizardState NOTIFY wizardStateChanged)
    Q_PROPERTY(QVariantList poses READ poses NOTIFY posesChanged)
    Q_PROPERTY(int capturedPoseCount READ capturedPoseCount NOTIFY posesChanged)
    Q_PROPERTY(int detectedPoseIndex READ detectedPoseIndex NOTIFY captureStatusChanged)
    Q_PROPERTY(int suggestedPoseIndex READ suggestedPoseIndex NOTIFY captureStatusChanged)
    Q_PROPERTY(bool isDeviceStill READ isDeviceStill NOTIFY captureStatusChanged)
    Q_PROPERTY(float captureProgress READ captureProgress NOTIFY captureStatusChanged)
    Q_PROPERTY(bool isResultValid READ isResultValid NOTIFY resultChanged)
    Q_PROPERTY(QString resultFailureReason READ resultFailureReason NOTIFY resultChanged)
    Q_PROPERTY(QVector3D resultOffset READ resultOffset NOTIFY resultChanged)
    Q_PROPERTY(QVector3D resultScale READ resultScale NOTIFY resultChanged)
    Q_PROPERTY(double resultRawError READ resultRawError NOTIFY resultChanged)
    Q_PROPERTY(bool hasStoredCalibration READ hasStoredCalibration NOTIFY storedCalibrationChanged)
    Q_PROPERTY(QVector3D storedOffset READ storedOffset NOTIFY storedCalibrationChanged)
    Q_PROPERTY(QVector3D storedScale READ storedScale NOTIFY storedCalibrationChanged)
    Q_PROPERTY(QString storedCalibrationDate READ storedCalibrationDate NOTIFY storedCalibrationChanged)

public:
    enum class WizardState { Idle, Capturing, ResultReady };
    Q_ENUM(WizardState)

    AccelCalibrationController(DeviceManager *devices, ImuController *imu, QObject *parent = nullptr);
    ~AccelCalibrationController() override;

    Q_INVOKABLE void startWizard();
    Q_INVOKABLE void cancelWizard();
    Q_INVOKABLE void recapturePose(int poseIndex);
    Q_INVOKABLE void saveResult();
    Q_INVOKABLE void discardResult();
    Q_INVOKABLE void clearStoredCalibration();

    WizardState wizardState() const { return currentWizardState; }
    QVariantList poses() const;
    int capturedPoseCount() const;
    int detectedPoseIndex() const { return detectedPose; }
    int suggestedPoseIndex() const;
    bool isDeviceStill() const { return deviceStill; }
    float captureProgress() const { return activePoseProgress; }

    bool isResultValid() const { return pendingResult.isValid; }
    QString resultFailureReason() const { return pendingResult.failureReason; }
    QVector3D resultOffset() const { return pendingResult.calibration.offset; }
    QVector3D resultScale() const { return pendingResult.calibration.scale; }
    double resultRawError() const { return pendingResult.rawRmsError; }

    bool hasStoredCalibration() const { return storedCalibration.has_value(); }
    QVector3D storedOffset() const;
    QVector3D storedScale() const;
    QString storedCalibrationDate() const;

    void handleRawGyroSample(double timestampSeconds, const QVector3D &angularVelocity) override;
    void handleRawAccelSample(double timestampSeconds, const QVector3D &acceleration) override;

signals:
    void wizardStateChanged();
    void posesChanged();
    void captureStatusChanged();
    void resultChanged();
    void storedCalibrationChanged();

private:
    static constexpr int poseCount = 6;
    static constexpr int requiredSamplesPerPose = 400; // 2 s at 200 Hz
    static constexpr float poseAlignmentThreshold = 0.94f; // cos(20 deg)
    static constexpr int refreshIntervalMs = 50;

    struct PoseCapture
    {
        QVector3D accumulatedAcceleration;
        int sampleCount = 0;
        bool isCaptured = false;
        QVector3D meanAcceleration;
    };

    static int poseIndexForAcceleration(const QVector3D &acceleration);
    static QVector3D upAxisForPose(int poseIndex);
    static imu::StationaryDetector::Thresholds stillnessThresholds();

    void refreshCaptureStatus();
    void finishCapture();
    void loadStoredCalibration();
    void setWizardState(WizardState state);

    DeviceManager *deviceManager;
    ImuController *imuController;
    imu::CalibrationStore calibrationStore;
    QTimer refreshTimer;

    std::mutex captureMutex;
    imu::StationaryDetector stillnessDetector;
    std::array<PoseCapture, poseCount> poseCaptures;
    int activeCapturePose = -1;
    int latestDetectedPose = -1;
    bool isCaptureActive = false;

    WizardState currentWizardState = WizardState::Idle;
    std::array<bool, poseCount> capturedPoseFlags {};
    int detectedPose = -1;
    bool deviceStill = false;
    float activePoseProgress = 0.0f;

    imu::AccelCalibrationResult pendingResult;
    std::optional<imu::StoredCalibration> storedCalibration;
};
