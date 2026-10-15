#include "AccelCalibrationController.h"

#include "ImuController.h"
#include "device/DeviceManager.h"

#include <QQuaternion>
#include <QVariantMap>

#include <algorithm>

AccelCalibrationController::AccelCalibrationController(DeviceManager *devices, ImuController *imu, QObject *parent)
    : QObject(parent)
    , deviceManager(devices)
    , imuController(imu)
    , stillnessDetector(stillnessThresholds())
{
    refreshTimer.setInterval(refreshIntervalMs);
    connect(&refreshTimer, &QTimer::timeout, this, &AccelCalibrationController::refreshCaptureStatus);

    connect(deviceManager, &DeviceManager::deviceConnected, this, &AccelCalibrationController::loadStoredCalibration);
    connect(deviceManager, &DeviceManager::deviceDisconnected, this, [this] {
        cancelWizard();
        loadStoredCalibration();
    });

    imuController->addSampleListener(this);
    loadStoredCalibration();
}

AccelCalibrationController::~AccelCalibrationController()
{
    imuController->removeSampleListener(this);
}

imu::StationaryDetector::Thresholds AccelCalibrationController::stillnessThresholds()
{
    // Raw gyro is used here (bias not removed), so the mean speed limit is more tolerant.
    imu::StationaryDetector::Thresholds thresholds;
    thresholds.maxMeanAngularSpeed = 0.05f;
    thresholds.maxAngularVelocityStdDev = 0.04f;
    thresholds.maxAccelerationStdDev = 0.1f;
    return thresholds;
}

QVector3D AccelCalibrationController::upAxisForPose(int poseIndex)
{
    static const std::array<QVector3D, poseCount> upAxes = {{
        {0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0},
    }};
    return upAxes.at(poseIndex);
}

int AccelCalibrationController::poseIndexForAcceleration(const QVector3D &acceleration)
{
    const QVector3D direction = acceleration.normalized();
    for (int poseIndex = 0; poseIndex < poseCount; ++poseIndex) {
        if (QVector3D::dotProduct(direction, upAxisForPose(poseIndex)) > poseAlignmentThreshold)
            return poseIndex;
    }
    return -1;
}

QVariantList AccelCalibrationController::poses() const
{
    const std::array<QString, poseCount> titles = {
        tr("Düz"), tr("Ters"), tr("Lensler yukarı"), tr("Lensler aşağı"), tr("Sol uç yukarı"), tr("Sağ uç yukarı"),
    };
    const std::array<QString, poseCount> instructions = {
        tr("Kamerayı normal duruşunda düz bir yüzeye koyun."),
        tr("Kamerayı baş aşağı çevirin; üst yüzü yüzeye değsin."),
        tr("Kamerayı sırtüstü yatırın; lensler tavana baksın."),
        tr("Kamerayı yüzüstü yatırın; lensleri korumak için altına yumuşak bir bez koyun."),
        tr("Kamerayı dikine çevirin; arkasından bakınca sol ucu yukarıda kalsın."),
        tr("Kamerayı dikine çevirin; arkasından bakınca sağ ucu yukarıda kalsın."),
    };

    QVariantList poseList;
    for (int poseIndex = 0; poseIndex < poseCount; ++poseIndex) {
        poseList.append(QVariantMap {
            {QStringLiteral("title"), titles[poseIndex]},
            {QStringLiteral("instruction"), instructions[poseIndex]},
            {QStringLiteral("isCaptured"), capturedPoseFlags[poseIndex]},
            {QStringLiteral("targetOrientation"),
             QQuaternion::rotationTo(upAxisForPose(poseIndex), QVector3D(0, 0, 1))},
        });
    }
    return poseList;
}

int AccelCalibrationController::capturedPoseCount() const
{
    return static_cast<int>(std::count(capturedPoseFlags.begin(), capturedPoseFlags.end(), true));
}

int AccelCalibrationController::suggestedPoseIndex() const
{
    if (detectedPose >= 0 && !capturedPoseFlags[detectedPose])
        return detectedPose;
    for (int poseIndex = 0; poseIndex < poseCount; ++poseIndex) {
        if (!capturedPoseFlags[poseIndex])
            return poseIndex;
    }
    return -1;
}

QVector3D AccelCalibrationController::storedOffset() const
{
    return storedCalibration ? storedCalibration->accel.offset : QVector3D(0, 0, 0);
}

QVector3D AccelCalibrationController::storedScale() const
{
    return storedCalibration ? storedCalibration->accel.scale : QVector3D(1, 1, 1);
}

QString AccelCalibrationController::storedCalibrationDate() const
{
    return storedCalibration ? storedCalibration->createdAt.toString(QStringLiteral("dd.MM.yyyy HH:mm")) : QString();
}

void AccelCalibrationController::startWizard()
{
    if (!deviceManager->isConnected())
        return;
    imuController->start();

    {
        std::lock_guard<std::mutex> lock(captureMutex);
        poseCaptures = {};
        stillnessDetector.reset();
        activeCapturePose = -1;
        latestDetectedPose = -1;
        isCaptureActive = true;
    }

    capturedPoseFlags = {};
    detectedPose = -1;
    deviceStill = false;
    activePoseProgress = 0.0f;
    pendingResult = {};
    emit posesChanged();
    emit captureStatusChanged();
    emit resultChanged();

    refreshTimer.start();
    setWizardState(WizardState::Capturing);
}

void AccelCalibrationController::cancelWizard()
{
    refreshTimer.stop();
    {
        std::lock_guard<std::mutex> lock(captureMutex);
        isCaptureActive = false;
    }
    setWizardState(WizardState::Idle);
}

void AccelCalibrationController::recapturePose(int poseIndex)
{
    if (poseIndex < 0 || poseIndex >= poseCount)
        return;
    {
        std::lock_guard<std::mutex> lock(captureMutex);
        poseCaptures[poseIndex] = {};
    }
    capturedPoseFlags[poseIndex] = false;
    emit posesChanged();
}

void AccelCalibrationController::saveResult()
{
    if (!pendingResult.isValid || !deviceManager->isConnected())
        return;

    const imu::StoredCalibration calibration {pendingResult.calibration, QDateTime::currentDateTime()};
    if (!calibrationStore.save(deviceManager->serialNumber(), calibration)) {
        pendingResult.isValid = false;
        pendingResult.failureReason = tr("Kalibrasyon dosyası yazılamadı: %1").arg(calibrationStore.filePath());
        emit resultChanged();
        return;
    }

    storedCalibration = calibration;
    imuController->setAccelCalibration(calibration.accel);
    emit storedCalibrationChanged();
    setWizardState(WizardState::Idle);
}

void AccelCalibrationController::discardResult()
{
    pendingResult = {};
    emit resultChanged();
    setWizardState(WizardState::Idle);
}

void AccelCalibrationController::clearStoredCalibration()
{
    if (deviceManager->isConnected())
        calibrationStore.remove(deviceManager->serialNumber());
    storedCalibration.reset();
    imuController->setAccelCalibration(imu::AccelCalibration());
    emit storedCalibrationChanged();
}

void AccelCalibrationController::handleRawGyroSample(double timestampSeconds, const QVector3D &angularVelocity)
{
    std::lock_guard<std::mutex> lock(captureMutex);
    if (isCaptureActive)
        stillnessDetector.addGyroSample(timestampSeconds, angularVelocity);
}

void AccelCalibrationController::handleRawAccelSample(double timestampSeconds, const QVector3D &acceleration)
{
    std::lock_guard<std::mutex> lock(captureMutex);
    if (!isCaptureActive)
        return;

    stillnessDetector.addAccelSample(timestampSeconds, acceleration);
    latestDetectedPose = poseIndexForAcceleration(acceleration);

    const bool canCapture = latestDetectedPose >= 0 && stillnessDetector.isStationary()
                            && !poseCaptures[latestDetectedPose].isCaptured;

    if (!canCapture || latestDetectedPose != activeCapturePose) {
        if (activeCapturePose >= 0 && !poseCaptures[activeCapturePose].isCaptured)
            poseCaptures[activeCapturePose] = {};
        activeCapturePose = canCapture ? latestDetectedPose : -1;
    }
    if (!canCapture)
        return;

    PoseCapture &capture = poseCaptures[activeCapturePose];
    capture.accumulatedAcceleration += acceleration;
    ++capture.sampleCount;

    if (capture.sampleCount >= requiredSamplesPerPose) {
        capture.meanAcceleration = capture.accumulatedAcceleration / static_cast<float>(capture.sampleCount);
        capture.isCaptured = true;
        activeCapturePose = -1;
    }
}

void AccelCalibrationController::refreshCaptureStatus()
{
    std::array<bool, poseCount> latestCapturedFlags {};
    {
        std::lock_guard<std::mutex> lock(captureMutex);
        for (int poseIndex = 0; poseIndex < poseCount; ++poseIndex)
            latestCapturedFlags[poseIndex] = poseCaptures[poseIndex].isCaptured;
        detectedPose = latestDetectedPose;
        deviceStill = stillnessDetector.isStationary();
        activePoseProgress = activeCapturePose >= 0
                                 ? static_cast<float>(poseCaptures[activeCapturePose].sampleCount) / requiredSamplesPerPose
                                 : 0.0f;
    }

    if (latestCapturedFlags != capturedPoseFlags) {
        capturedPoseFlags = latestCapturedFlags;
        emit posesChanged();
    }
    emit captureStatusChanged();

    if (capturedPoseCount() == poseCount)
        finishCapture();
}

void AccelCalibrationController::finishCapture()
{
    refreshTimer.stop();

    std::vector<imu::CalibrationPose> recordedPoses;
    {
        std::lock_guard<std::mutex> lock(captureMutex);
        isCaptureActive = false;
        for (int poseIndex = 0; poseIndex < poseCount; ++poseIndex)
            recordedPoses.push_back({upAxisForPose(poseIndex), poseCaptures[poseIndex].meanAcceleration});
    }

    pendingResult = imu::AccelCalibrationSolver::solve(recordedPoses);
    emit resultChanged();
    setWizardState(WizardState::ResultReady);
}

void AccelCalibrationController::loadStoredCalibration()
{
    storedCalibration.reset();
    if (deviceManager->isConnected())
        storedCalibration = calibrationStore.load(deviceManager->serialNumber());

    imuController->setAccelCalibration(storedCalibration ? storedCalibration->accel : imu::AccelCalibration());
    emit storedCalibrationChanged();
}

void AccelCalibrationController::setWizardState(WizardState state)
{
    if (currentWizardState == state)
        return;
    currentWizardState = state;
    emit wizardStateChanged();
}
