#pragma once

#include "FrameSource.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QVariantList>

#include <librealsense2/rs.hpp>

#include <atomic>
#include <optional>
#include <vector>

class DeviceManager;
class DepthCameraController;
class ImuController;

// Records the selected streams into a single LZ4-compressed .bag file.
// The sensors are opened through an rs2::recorder (not a pipeline) because only the recorder
// can enable compression, which makes files ~3.7x smaller. Color is stored as the camera's
// native YUYV. While recording, the live IMU and depth views are paused; the IMU resumes afterwards.
class RecordingController : public FrameSource
{
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY recordingStateChanged)
    Q_PROPERTY(bool recordsDepth READ recordsDepth WRITE setRecordsDepth NOTIFY streamSelectionChanged)
    Q_PROPERTY(bool recordsColor READ recordsColor WRITE setRecordsColor NOTIFY streamSelectionChanged)
    Q_PROPERTY(bool recordsImu READ recordsImu WRITE setRecordsImu NOTIFY streamSelectionChanged)
    Q_PROPERTY(double elapsedSeconds READ elapsedSeconds NOTIFY progressChanged)
    Q_PROPERTY(double fileSizeMegabytes READ fileSizeMegabytes NOTIFY progressChanged)
    Q_PROPERTY(int recordedDepthFrames READ recordedDepthFrames NOTIFY progressChanged)
    Q_PROPERTY(int recordedColorFrames READ recordedColorFrames NOTIFY progressChanged)
    Q_PROPERTY(QString recordingsDirectory READ recordingsDirectory CONSTANT)
    Q_PROPERTY(QVariantList recordings READ recordings NOTIFY recordingsChanged)

public:
    RecordingController(DeviceManager *devices, ImuController *imu, DepthCameraController *depthCamera,
                        QObject *parent = nullptr);
    ~RecordingController() override;

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void refreshRecordings();
    Q_INVOKABLE void deleteRecording(const QString &filePath);
    Q_INVOKABLE void openRecordingsDirectory() const;

    bool isRecording() const { return isRecordingActive; }
    QString errorMessage() const { return currentErrorMessage; }
    bool recordsDepth() const { return isDepthSelected; }
    void setRecordsDepth(bool isSelected);
    bool recordsColor() const { return isColorSelected; }
    void setRecordsColor(bool isSelected);
    bool recordsImu() const { return isImuSelected; }
    void setRecordsImu(bool isSelected);
    double elapsedSeconds() const { return recordingElapsedSeconds; }
    double fileSizeMegabytes() const { return currentFileSizeMegabytes; }
    int recordedDepthFrames() const { return depthFrameCount; }
    int recordedColorFrames() const { return colorFrameCount; }
    QString recordingsDirectory() const { return outputDirectory; }
    QVariantList recordings() const { return recordingList; }

signals:
    void recordingStateChanged();
    void streamSelectionChanged();
    void progressChanged();
    void recordingsChanged();

private:
    static constexpr int videoWidth = 848;
    static constexpr int videoHeight = 480;
    static constexpr int videoFrameRate = 30;
    static constexpr int streamCheckDelayMs = 2000;
    static constexpr int maxStartAttempts = 3;

    void processFrame(const rs2::frame &frame);
    bool openSensors();
    void closeSensors();
    void verifyStreamsStarted();
    static std::vector<rs2::stream_profile> selectProfiles(const rs2::sensor &sensor, rs2_stream stream,
                                                           rs2_format format, int frameRate,
                                                           int width = 0, int height = 0);
    void updateProgress();
    void setError(const QString &message);

    DeviceManager *deviceManager;
    ImuController *imuController;
    DepthCameraController *depthCameraController;

    rs2::context recordingContext;
    std::optional<rs2::device> recordingDevice;
    std::optional<rs2::recorder> activeRecorder;
    std::vector<rs2::sensor> activeSensors;
    rs2::colorizer depthColorizer;
    rs2::yuy_decoder colorDecoder;
    QTimer progressTimer;
    QTimer streamCheckTimer;
    std::atomic<int> depthFrameCount {0};
    std::atomic<int> colorFrameCount {0};
    int startAttempt = 0;
    QElapsedTimer recordingClock;

    QString outputDirectory;
    QString activeFilePath;
    QVariantList recordingList;
    QString currentErrorMessage;

    bool isDepthSelected = true;
    bool isColorSelected = true;
    bool isImuSelected = true;
    bool shouldResumeImu = false;
    bool isRecordingActive = false;
    double recordingElapsedSeconds = 0.0;
    double currentFileSizeMegabytes = 0.0;
};
