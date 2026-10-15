#pragma once

#include "media/FrameSource.h"

#include <QPointF>
#include <QSize>
#include <QStringList>
#include <QTimer>

#include <librealsense2/rs.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <vector>

class DeviceManager;

// Streams depth + color from the D435i in its own pipeline (independent from the IMU),
// runs the post-processing chain on the RealSense thread and hands ready-to-draw images
// to the UI. Distances can be queried at any normalized image position.
class DepthCameraController : public FrameSource
{
    Q_OBJECT
    Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY streamingStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY streamingStateChanged)

    Q_PROPERTY(QStringList resolutionPresetNames READ resolutionPresetNames CONSTANT)
    Q_PROPERTY(int resolutionPresetIndex READ resolutionPresetIndex WRITE setResolutionPresetIndex NOTIFY settingsChanged)
    Q_PROPERTY(bool isAlignedToColor READ isAlignedToColor WRITE setAlignedToColor NOTIFY settingsChanged)
    Q_PROPERTY(QStringList colorSchemeNames READ colorSchemeNames CONSTANT)
    Q_PROPERTY(int colorSchemeIndex READ colorSchemeIndex WRITE setColorSchemeIndex NOTIFY settingsChanged)
    Q_PROPERTY(double minimumDistance READ minimumDistance WRITE setMinimumDistance NOTIFY settingsChanged)
    Q_PROPERTY(double maximumDistance READ maximumDistance WRITE setMaximumDistance NOTIFY settingsChanged)
    Q_PROPERTY(bool isSpatialFilterEnabled READ isSpatialFilterEnabled WRITE setSpatialFilterEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool isTemporalFilterEnabled READ isTemporalFilterEnabled WRITE setTemporalFilterEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool isHoleFillingEnabled READ isHoleFillingEnabled WRITE setHoleFillingEnabled NOTIFY settingsChanged)

    Q_PROPERTY(QStringList visualPresetNames READ visualPresetNames CONSTANT)
    Q_PROPERTY(int visualPresetIndex READ visualPresetIndex WRITE setVisualPresetIndex NOTIFY sensorOptionsChanged)
    Q_PROPERTY(bool isEmitterEnabled READ isEmitterEnabled WRITE setEmitterEnabled NOTIFY sensorOptionsChanged)
    Q_PROPERTY(double laserPower READ laserPower WRITE setLaserPower NOTIFY sensorOptionsChanged)
    Q_PROPERTY(double laserPowerMaximum READ laserPowerMaximum NOTIFY sensorOptionsChanged)

    Q_PROPERTY(int depthFrameRate READ depthFrameRate NOTIFY statisticsChanged)
    Q_PROPERTY(int colorFrameRate READ colorFrameRate NOTIFY statisticsChanged)
    Q_PROPERTY(double centerDistance READ centerDistance NOTIFY statisticsChanged)
    Q_PROPERTY(double validPixelRatio READ validPixelRatio NOTIFY statisticsChanged)
    Q_PROPERTY(QSize depthResolution READ depthResolution NOTIFY statisticsChanged)

public:
    explicit DepthCameraController(DeviceManager *devices, QObject *parent = nullptr);
    ~DepthCameraController() override;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE double distanceAt(const QPointF &normalizedPosition) const;

    bool isStreaming() const { return isPipelineRunning; }
    QString errorMessage() const { return currentErrorMessage; }

    QStringList resolutionPresetNames() const;
    int resolutionPresetIndex() const { return selectedResolutionPreset; }
    void setResolutionPresetIndex(int presetIndex);
    bool isAlignedToColor() const;
    void setAlignedToColor(bool isAligned);
    QStringList colorSchemeNames() const;
    int colorSchemeIndex() const;
    void setColorSchemeIndex(int schemeIndex);
    double minimumDistance() const;
    void setMinimumDistance(double meters);
    double maximumDistance() const;
    void setMaximumDistance(double meters);
    bool isSpatialFilterEnabled() const;
    void setSpatialFilterEnabled(bool isEnabled);
    bool isTemporalFilterEnabled() const;
    void setTemporalFilterEnabled(bool isEnabled);
    bool isHoleFillingEnabled() const;
    void setHoleFillingEnabled(bool isEnabled);

    QStringList visualPresetNames() const;
    int visualPresetIndex() const { return selectedVisualPreset; }
    void setVisualPresetIndex(int presetIndex);
    bool isEmitterEnabled() const { return emitterEnabled; }
    void setEmitterEnabled(bool isEnabled);
    double laserPower() const { return requestedLaserPower; }
    void setLaserPower(double power);
    double laserPowerMaximum() const { return maximumLaserPower; }

    int depthFrameRate() const { return measuredDepthFrameRate; }
    int colorFrameRate() const { return measuredColorFrameRate; }
    double centerDistance() const { return latestCenterDistance; }
    double validPixelRatio() const { return latestValidPixelRatio; }
    QSize depthResolution() const { return latestDepthResolution; }

signals:
    void streamingStateChanged();
    void settingsChanged();
    void sensorOptionsChanged();
    void statisticsChanged();

protected:
    void onFramesPublished() override;

private:
    struct ResolutionPreset
    {
        int width;
        int height;
        int frameRate;
    };

    struct ProcessingSettings
    {
        bool alignToColor = true;
        bool spatialFilter = true;
        bool temporalFilter = true;
        bool holeFilling = false;
        int colorScheme = 0;
        double minimumDistance = 0.3;
        double maximumDistance = 4.0;
    };

    static const std::vector<ResolutionPreset> &resolutionPresets();
    static const std::vector<int> &visualPresetValues();

    void startPipeline();
    void stopPipeline();
    void restartIfStreaming();
    void processFrames(rs2::frameset frames);
    void applyColorizerSettings(const ProcessingSettings &settings);
    void updateFrameRates();
    void applySensorOptions();
    void readSensorOptions();
    void updateProcessingSettings(const std::function<void(ProcessingSettings &)> &change);
    static double medianDistance(const rs2::depth_frame &depth, int centerX, int centerY);
    static double validRatio(const rs2::depth_frame &depth);
    void setError(const QString &message);

    DeviceManager *deviceManager;
    rs2::pipeline videoPipeline;
    std::optional<rs2::depth_sensor> activeDepthSensor;

    rs2::align alignToColor {RS2_STREAM_COLOR};
    rs2::disparity_transform depthToDisparity {true};
    rs2::disparity_transform disparityToDepth {false};
    rs2::spatial_filter spatialFilter;
    rs2::temporal_filter temporalFilter;
    rs2::hole_filling_filter holeFillingFilter;
    rs2::colorizer depthColorizer;

    mutable std::mutex settingsMutex;
    ProcessingSettings processingSettings;
    std::optional<ProcessingSettings> appliedColorizerSettings;

    mutable std::mutex frameMutex;
    std::optional<rs2::depth_frame> latestDepthFrame;
    double processedCenterDistance = 0.0;
    double processedValidPixelRatio = 0.0;
    QSize processedDepthResolution;

    std::atomic<int> depthFrameCounter {0};
    std::atomic<int> colorFrameCounter {0};
    QTimer frameRateTimer;

    int selectedResolutionPreset = 0;
    int selectedVisualPreset = 0;
    bool emitterEnabled = true;
    double requestedLaserPower = 150.0;
    double maximumLaserPower = 360.0;

    int measuredDepthFrameRate = 0;
    int measuredColorFrameRate = 0;
    double latestCenterDistance = 0.0;
    double latestValidPixelRatio = 0.0;
    QSize latestDepthResolution;

    QString currentErrorMessage;
    bool isStartRequested = false;
    bool isPipelineRunning = false;
};
