#include "PlaybackController.h"

#include <QDebug>
#include <QMetaObject>
#include <QFileInfo>

#include <chrono>

PlaybackController::PlaybackController(QObject *parent)
    : FrameSource(parent)
{
    depthColorizer.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0.0f);
    depthColorizer.set_option(RS2_OPTION_MIN_DISTANCE, 0.3f);
    depthColorizer.set_option(RS2_OPTION_MAX_DISTANCE, 4.0f);

    positionTimer.setInterval(33);
    connect(&positionTimer, &QTimer::timeout, this, &PlaybackController::refreshPosition);
}

PlaybackController::~PlaybackController()
{
    close();
}

void PlaybackController::open(const QString &filePath)
{
    close();

    try {
        playbackDevice = playbackContext.load_device(filePath.toStdString());
        playbackDevice->set_real_time(true);
        playbackDevice->set_status_changed_callback([this](rs2_playback_status status) {
            if (status == RS2_PLAYBACK_STATUS_STOPPED)
                QMetaObject::invokeMethod(this, &PlaybackController::handlePlaybackFinished, Qt::QueuedConnection);
        });
    } catch (const rs2::error &error) {
        playbackDevice.reset();
        setError(tr("Kayıt açılamadı: %1").arg(QString::fromUtf8(error.what())));
        return;
    }

    containsDepth = containsColor = false;
    bool containsAccel = false;
    bool containsGyro = false;
    for (const rs2::sensor &sensor : playbackDevice->query_sensors()) {
        for (const rs2::stream_profile &stream : sensor.get_stream_profiles()) {
            containsDepth |= stream.stream_type() == RS2_STREAM_DEPTH;
            containsColor |= stream.stream_type() == RS2_STREAM_COLOR;
            containsAccel |= stream.stream_type() == RS2_STREAM_ACCEL;
            containsGyro |= stream.stream_type() == RS2_STREAM_GYRO;
        }
    }
    containsImu = containsAccel && containsGyro;
    recordingDurationSeconds = playbackDevice->get_duration().count() / nanosecondsPerSecond;

    fusionEngine.reset();
    eulerTracker.reset();
    if (playbackDevice->supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) {
        const std::optional<imu::StoredCalibration> calibration =
            calibrationStore.load(QString::fromUtf8(playbackDevice->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)));
        fusionEngine.setAccelCalibration(calibration ? calibration->accel : imu::AccelCalibration());
    }

    loadedFilePath = filePath;
    loadedFileName = QFileInfo(filePath).fileName();
    startSensors();
    positionTimer.start();
    setError({});
    emit playbackStateChanged();
}

void PlaybackController::startSensors()
{
    for (rs2::sensor sensor : playbackDevice->query_sensors()) {
        sensor.open(sensor.get_stream_profiles());
        sensor.start([this](const rs2::frame &frame) { processFrame(frame); });
        playbackSensors.push_back(sensor);
    }
    playing = true;
    hasReachedEnd = false;
}

void PlaybackController::stopSensors()
{
    for (rs2::sensor &sensor : playbackSensors) {
        try {
            sensor.stop();
            sensor.close();
        } catch (const rs2::error &error) {
            qWarning() << "Playback sensor stop failed:" << error.what();
        }
    }
    playbackSensors.clear();
}

void PlaybackController::close()
{
    if (!playbackDevice)
        return;

    positionTimer.stop();
    // Stopping a paused playback can block inside librealsense, so resume first.
    if (!playing && !hasReachedEnd)
        playbackDevice->resume();
    stopSensors();
    playbackDevice.reset();
    playbackContext.unload_device(loadedFilePath.toStdString());

    playing = false;
    hasReachedEnd = false;
    loadedFilePath.clear();
    loadedFileName.clear();
    currentPositionSeconds = 0.0;
    recordingDurationSeconds = 0.0;
    latestSnapshot = {};
    clearImages();
    emit positionChanged();
    emit playbackStateChanged();
}

void PlaybackController::play()
{
    if (!playbackDevice || playing)
        return;

    if (hasReachedEnd) {
        stopSensors();
        fusionEngine.reset();
        eulerTracker.reset();
        startSensors();
    } else {
        playbackDevice->resume();
        playing = true;
    }
    emit playbackStateChanged();
}

void PlaybackController::pause()
{
    if (!playbackDevice || !playing)
        return;
    playbackDevice->pause();
    playing = false;
    emit playbackStateChanged();
}

void PlaybackController::seek(double seconds)
{
    if (!playbackDevice)
        return;
    if (hasReachedEnd)
        play();
    const auto target = std::chrono::nanoseconds(static_cast<long long>(seconds * nanosecondsPerSecond));
    playbackDevice->seek(target);
    refreshPosition();
}

void PlaybackController::handlePlaybackFinished()
{
    if (!playbackDevice || !playing)
        return;
    playing = false;
    hasReachedEnd = true;
    currentPositionSeconds = recordingDurationSeconds;
    emit positionChanged();
    emit playbackStateChanged();
}

void PlaybackController::processFrame(const rs2::frame &frame)
{
    if (const rs2::motion_frame motionFrame = frame.as<rs2::motion_frame>()) {
        handleMotionFrame(motionFrame);
    } else if (const rs2::depth_frame depth = frame.as<rs2::depth_frame>()) {
        storeDepthImage(toImage(depthColorizer.colorize(depth)));
        notifyFramesUpdated();
    } else if (const rs2::video_frame color = frame.as<rs2::video_frame>()) {
        if (color.get_profile().stream_type() != RS2_STREAM_COLOR)
            return;
        // New recordings store the camera's native YUYV; older ones may already be RGB.
        const bool isYuyv = color.get_profile().format() == RS2_FORMAT_YUYV;
        storeColorImage(toImage(isYuyv ? colorDecoder.process(color).as<rs2::video_frame>() : color));
        notifyFramesUpdated();
    }
}

void PlaybackController::handleMotionFrame(const rs2::motion_frame &frame)
{
    const double timestampSeconds = frame.get_timestamp() / 1000.0;
    const rs2_vector cameraVector = frame.get_motion_data();
    const QVector3D bodyVector = imu::cameraToBodyFrame(cameraVector.x, cameraVector.y, cameraVector.z);

    if (frame.get_profile().stream_type() == RS2_STREAM_GYRO)
        fusionEngine.addGyroSample(timestampSeconds, bodyVector);
    else
        fusionEngine.addAccelSample(timestampSeconds, bodyVector);
}

void PlaybackController::refreshPosition()
{
    if (!playbackDevice || hasReachedEnd)
        return;
    currentPositionSeconds = playbackDevice->get_position() / nanosecondsPerSecond;
    if (containsImu) {
        latestSnapshot = fusionEngine.snapshot();
        latestSnapshot.angles = eulerTracker.update(latestSnapshot.orientation);
    }
    emit positionChanged();
}

void PlaybackController::setError(const QString &message)
{
    if (currentErrorMessage == message)
        return;
    currentErrorMessage = message;
    emit playbackStateChanged();
}
