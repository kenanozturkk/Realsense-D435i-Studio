#pragma once

#include "FrameSource.h"
#include "imu/CalibrationStore.h"
#include "imu/ContinuousEulerTracker.h"
#include "imu/ImuFusionEngine.h"

#include <QQuaternion>
#include <QTimer>

#include <librealsense2/rs.hpp>

#include <optional>
#include <vector>

// Plays back a .bag recording without a camera attached. Depth and color are shown like the
// live view; recorded IMU samples run through the same fusion engine as the live IMU, with
// the stored accelerometer calibration of the recording camera applied.
// Sensors of the playback device are started directly instead of through a pipeline, so each
// stream plays independently: a recording with a missing stream still shows the others.
class PlaybackController : public FrameSource
{
    Q_OBJECT
    Q_PROPERTY(bool isLoaded READ isLoaded NOTIFY playbackStateChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playbackStateChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY playbackStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY playbackStateChanged)
    Q_PROPERTY(bool hasDepth READ hasDepth NOTIFY playbackStateChanged)
    Q_PROPERTY(bool hasColor READ hasColor NOTIFY playbackStateChanged)
    Q_PROPERTY(bool hasImu READ hasImu NOTIFY playbackStateChanged)
    Q_PROPERTY(double durationSeconds READ durationSeconds NOTIFY playbackStateChanged)
    Q_PROPERTY(double positionSeconds READ positionSeconds NOTIFY positionChanged)
    Q_PROPERTY(QQuaternion orientation READ orientation NOTIFY positionChanged)
    Q_PROPERTY(float roll READ roll NOTIFY positionChanged)
    Q_PROPERTY(float pitch READ pitch NOTIFY positionChanged)
    Q_PROPERTY(float yaw READ yaw NOTIFY positionChanged)

public:
    explicit PlaybackController(QObject *parent = nullptr);
    ~PlaybackController() override;

    Q_INVOKABLE void open(const QString &filePath);
    Q_INVOKABLE void close();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void seek(double seconds);

    bool isLoaded() const { return playbackDevice.has_value(); }
    bool isPlaying() const { return playing; }
    QString fileName() const { return loadedFileName; }
    QString errorMessage() const { return currentErrorMessage; }
    bool hasDepth() const { return containsDepth; }
    bool hasColor() const { return containsColor; }
    bool hasImu() const { return containsImu; }
    double durationSeconds() const { return recordingDurationSeconds; }
    double positionSeconds() const { return currentPositionSeconds; }
    QQuaternion orientation() const { return latestSnapshot.orientation; }
    float roll() const { return latestSnapshot.angles.rollDegrees; }
    float pitch() const { return latestSnapshot.angles.pitchDegrees; }
    float yaw() const { return latestSnapshot.angles.yawDegrees; }

signals:
    void playbackStateChanged();
    void positionChanged();

private:
    static constexpr double nanosecondsPerSecond = 1e9;

    void startSensors();
    void stopSensors();
    void handlePlaybackFinished();
    void processFrame(const rs2::frame &frame);
    void handleMotionFrame(const rs2::motion_frame &frame);
    void refreshPosition();
    void setError(const QString &message);

    rs2::context playbackContext;
    std::optional<rs2::playback> playbackDevice;
    std::vector<rs2::sensor> playbackSensors;
    QString loadedFilePath;
    rs2::colorizer depthColorizer;
    rs2::yuy_decoder colorDecoder;
    QTimer positionTimer;

    imu::ImuFusionEngine fusionEngine;
    imu::ContinuousEulerTracker eulerTracker;
    imu::FusionSnapshot latestSnapshot;
    imu::CalibrationStore calibrationStore;

    QString loadedFileName;
    QString currentErrorMessage;
    bool playing = false;
    bool hasReachedEnd = false;
    bool containsDepth = false;
    bool containsColor = false;
    bool containsImu = false;
    double recordingDurationSeconds = 0.0;
    double currentPositionSeconds = 0.0;
};
