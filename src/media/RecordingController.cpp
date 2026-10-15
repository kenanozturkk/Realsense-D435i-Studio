#include "RecordingController.h"

#include "depth/DepthCameraController.h"
#include "device/DeviceErrors.h"
#include "device/DeviceManager.h"
#include "imu/ImuController.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>

#include <stdexcept>

namespace {
constexpr double bytesPerMegabyte = 1024.0 * 1024.0;
}

RecordingController::RecordingController(DeviceManager *devices, ImuController *imu,
                                         DepthCameraController *depthCamera, QObject *parent)
    : FrameSource(parent)
    , deviceManager(devices)
    , imuController(imu)
    , depthCameraController(depthCamera)
    , outputDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                      + QStringLiteral("/RealSenseRecordings"))
{
    QDir().mkpath(outputDirectory);
    depthColorizer.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0.0f);
    depthColorizer.set_option(RS2_OPTION_MIN_DISTANCE, 0.3f);
    depthColorizer.set_option(RS2_OPTION_MAX_DISTANCE, 4.0f);

    progressTimer.setInterval(500);
    connect(&progressTimer, &QTimer::timeout, this, &RecordingController::updateProgress);

    streamCheckTimer.setSingleShot(true);
    streamCheckTimer.setInterval(streamCheckDelayMs);
    connect(&streamCheckTimer, &QTimer::timeout, this, &RecordingController::verifyStreamsStarted);
    connect(deviceManager, &DeviceManager::deviceDisconnected, this, &RecordingController::stopRecording);

    refreshRecordings();
}

RecordingController::~RecordingController()
{
    shouldResumeImu = false;
    stopRecording();
}

void RecordingController::setRecordsDepth(bool isSelected)
{
    isDepthSelected = isSelected;
    emit streamSelectionChanged();
}

void RecordingController::setRecordsColor(bool isSelected)
{
    isColorSelected = isSelected;
    emit streamSelectionChanged();
}

void RecordingController::setRecordsImu(bool isSelected)
{
    isImuSelected = isSelected;
    emit streamSelectionChanged();
}

void RecordingController::startRecording()
{
    if (isRecordingActive || !deviceManager->isConnected())
        return;
    if (!isDepthSelected && !isColorSelected && !isImuSelected) {
        setError(tr("Kaydedilecek en az bir akış seçin."));
        return;
    }

    const ImuController::TrackingState imuState = imuController->trackingState();
    shouldResumeImu = imuState != ImuController::TrackingState::Stopped && imuState != ImuController::TrackingState::Error;
    imuController->stop();
    depthCameraController->stop();

    activeFilePath = QStringLiteral("%1/kayit_%2.bag")
                         .arg(outputDirectory, QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")));

    try {
        for (rs2::device device : recordingContext.query_devices()) {
            if (QString::fromUtf8(device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)) == deviceManager->serialNumber())
                recordingDevice = device;
        }
        if (!recordingDevice)
            throw std::runtime_error("device not found");
        if (!openSensors())
            throw std::runtime_error("no matching stream profiles");
    } catch (const std::exception &error) {
        closeSensors();
        recordingDevice.reset();
        QFile::remove(activeFilePath);
        setError(tr("Kayıt başlatılamadı: %1").arg(describeDeviceError(error)));
        if (shouldResumeImu)
            imuController->start();
        return;
    }

    isRecordingActive = true;
    startAttempt = 1;
    recordingElapsedSeconds = 0.0;
    currentFileSizeMegabytes = 0.0;
    recordingClock.start();
    progressTimer.start();
    streamCheckTimer.start();
    setError({});
    emit progressChanged();
    emit recordingStateChanged();
}

bool RecordingController::openSensors()
{
    depthFrameCount = 0;
    colorFrameCount = 0;
    activeRecorder.emplace(activeFilePath.toStdString(), *recordingDevice, true);

    for (rs2::sensor sensor : activeRecorder->query_sensors()) {
        std::vector<rs2::stream_profile> profiles;
        if (isDepthSelected && sensor.is<rs2::depth_sensor>())
            profiles = selectProfiles(sensor, RS2_STREAM_DEPTH, RS2_FORMAT_Z16, videoFrameRate, videoWidth, videoHeight);
        else if (isColorSelected && sensor.is<rs2::color_sensor>())
            profiles = selectProfiles(sensor, RS2_STREAM_COLOR, RS2_FORMAT_YUYV, videoFrameRate, videoWidth, videoHeight);
        else if (isImuSelected && sensor.is<rs2::motion_sensor>()) {
            profiles = selectProfiles(sensor, RS2_STREAM_ACCEL, RS2_FORMAT_MOTION_XYZ32F, 200);
            const std::vector<rs2::stream_profile> gyroProfiles =
                selectProfiles(sensor, RS2_STREAM_GYRO, RS2_FORMAT_MOTION_XYZ32F, 400);
            profiles.insert(profiles.end(), gyroProfiles.begin(), gyroProfiles.end());
        }
        if (profiles.empty())
            continue;

        sensor.open(profiles);
        sensor.start([this](const rs2::frame &frame) { processFrame(frame); });
        activeSensors.push_back(sensor);
    }
    return !activeSensors.empty();
}

void RecordingController::closeSensors()
{
    for (rs2::sensor &sensor : activeSensors) {
        try {
            sensor.stop();
            sensor.close();
        } catch (const rs2::error &error) {
            qWarning() << "Recording sensor stop failed:" << error.what();
        }
    }
    activeSensors.clear();
    // The bag file is finalized when the recorder is destroyed.
    activeRecorder.reset();
}

void RecordingController::verifyStreamsStarted()
{
    // The D435i stereo module occasionally starts without delivering any frame when opened
    // through the sensor API; reopening it reliably recovers, so retry a few times.
    const bool isDepthMissing = isDepthSelected && depthFrameCount == 0;
    const bool isColorMissing = isColorSelected && colorFrameCount == 0;
    if (!isRecordingActive || (!isDepthMissing && !isColorMissing))
        return;

    if (startAttempt >= maxStartAttempts) {
        setError(isDepthMissing ? tr("Derinlik akışı başlamadı; kayıtta derinlik olmayacak.")
                                : tr("Renk akışı başlamadı; kayıtta renk olmayacak."));
        return;
    }

    qWarning() << "Recording stream did not start, reopening sensors. Missing:"
               << (isDepthMissing ? "depth" : "") << (isColorMissing ? "color" : "") << "Attempt" << startAttempt + 1;
    closeSensors();
    QFile::remove(activeFilePath);
    try {
        openSensors();
    } catch (const rs2::error &error) {
        setError(tr("Kayıt yeniden başlatılamadı: %1").arg(describeDeviceError(error)));
        stopRecording();
        QFile::remove(activeFilePath);
        refreshRecordings();
        return;
    }
    ++startAttempt;
    recordingClock.restart();
    streamCheckTimer.start();
}

std::vector<rs2::stream_profile> RecordingController::selectProfiles(const rs2::sensor &sensor, rs2_stream stream,
                                                                     rs2_format format, int frameRate,
                                                                     int width, int height)
{
    for (const rs2::stream_profile &profile : sensor.get_stream_profiles()) {
        if (profile.stream_type() != stream || profile.format() != format || profile.fps() != frameRate)
            continue;
        if (width > 0) {
            const rs2::video_stream_profile videoProfile = profile.as<rs2::video_stream_profile>();
            if (!videoProfile || videoProfile.width() != width || videoProfile.height() != height)
                continue;
        }
        return {profile};
    }
    return {};
}

void RecordingController::stopRecording()
{
    if (!isRecordingActive)
        return;

    progressTimer.stop();
    streamCheckTimer.stop();
    closeSensors();
    recordingDevice.reset();
    isRecordingActive = false;

    updateProgress();
    clearImages();
    refreshRecordings();
    emit recordingStateChanged();

    if (shouldResumeImu)
        imuController->start();
}

void RecordingController::processFrame(const rs2::frame &frame)
{
    if (const rs2::depth_frame depth = frame.as<rs2::depth_frame>()) {
        ++depthFrameCount;
        storeDepthImage(toImage(depthColorizer.colorize(depth)));
        notifyFramesUpdated();
    } else if (frame.get_profile().stream_type() == RS2_STREAM_COLOR) {
        ++colorFrameCount;
        storeColorImage(toImage(colorDecoder.process(frame)));
        notifyFramesUpdated();
    }
}

void RecordingController::updateProgress()
{
    if (isRecordingActive)
        recordingElapsedSeconds = recordingClock.elapsed() / 1000.0;
    currentFileSizeMegabytes = QFileInfo(activeFilePath).size() / bytesPerMegabyte;
    emit progressChanged();
}

void RecordingController::refreshRecordings()
{
    recordingList.clear();
    const QFileInfoList files = QDir(outputDirectory).entryInfoList({QStringLiteral("*.bag")}, QDir::Files, QDir::Time);
    for (const QFileInfo &file : files) {
        recordingList.append(QVariantMap {
            {QStringLiteral("fileName"), file.fileName()},
            {QStringLiteral("filePath"), file.absoluteFilePath()},
            {QStringLiteral("sizeMegabytes"), file.size() / bytesPerMegabyte},
            {QStringLiteral("modifiedText"), file.lastModified().toString(QStringLiteral("dd.MM.yyyy HH:mm"))},
        });
    }
    emit recordingsChanged();
}

void RecordingController::deleteRecording(const QString &filePath)
{
    const QFileInfo file(filePath);
    const bool isInsideRecordingsDirectory = file.absolutePath() == QFileInfo(outputDirectory).absoluteFilePath();
    if (!isInsideRecordingsDirectory || file.suffix() != QStringLiteral("bag") || (filePath == activeFilePath && isRecordingActive))
        return;

    if (!QFile::remove(filePath))
        setError(tr("Dosya silinemedi: %1").arg(file.fileName()));
    refreshRecordings();
}

void RecordingController::openRecordingsDirectory() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputDirectory));
}

void RecordingController::setError(const QString &message)
{
    if (currentErrorMessage == message)
        return;
    currentErrorMessage = message;
    emit recordingStateChanged();
}
