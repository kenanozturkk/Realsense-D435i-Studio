#include "DepthCameraController.h"

#include "device/DeviceErrors.h"
#include "device/DeviceManager.h"

#include <QDebug>

#include <algorithm>

namespace {

struct ColorSchemeOption
{
    const char *name;
    float librealsenseValue;
};

const std::vector<ColorSchemeOption> &colorSchemeOptions()
{
    static const std::vector<ColorSchemeOption> options = {
        {QT_TRANSLATE_NOOP("DepthCameraController", "Jet"), 0.0f},
        {QT_TRANSLATE_NOOP("DepthCameraController", "Klasik"), 1.0f},
        {QT_TRANSLATE_NOOP("DepthCameraController", "Beyazdan siyaha"), 2.0f},
        {QT_TRANSLATE_NOOP("DepthCameraController", "Siyahtan beyaza"), 3.0f},
        {QT_TRANSLATE_NOOP("DepthCameraController", "Soğuk"), 5.0f},
        {QT_TRANSLATE_NOOP("DepthCameraController", "Sıcak"), 6.0f},
    };
    return options;
}

constexpr int medianWindowRadius = 2;
constexpr int validRatioSampleStep = 4;

} // namespace

DepthCameraController::DepthCameraController(DeviceManager *devices, QObject *parent)
    : FrameSource(parent)
    , deviceManager(devices)
{
    frameRateTimer.setInterval(1000);
    connect(&frameRateTimer, &QTimer::timeout, this, &DepthCameraController::updateFrameRates);

    connect(deviceManager, &DeviceManager::deviceConnected, this, [this] {
        if (isStartRequested)
            startPipeline();
    });
    connect(deviceManager, &DeviceManager::deviceDisconnected, this, &DepthCameraController::stopPipeline);
}

DepthCameraController::~DepthCameraController()
{
    stopPipeline();
}

const std::vector<DepthCameraController::ResolutionPreset> &DepthCameraController::resolutionPresets()
{
    static const std::vector<ResolutionPreset> presets = {
        {848, 480, 30},
        {1280, 720, 30},
        {640, 480, 30},
        {848, 480, 60},
    };
    return presets;
}

const std::vector<int> &DepthCameraController::visualPresetValues()
{
    static const std::vector<int> values = {
        RS2_RS400_VISUAL_PRESET_DEFAULT,
        RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY,
        RS2_RS400_VISUAL_PRESET_HIGH_DENSITY,
        RS2_RS400_VISUAL_PRESET_MEDIUM_DENSITY,
        RS2_RS400_VISUAL_PRESET_HAND,
    };
    return values;
}

QStringList DepthCameraController::resolutionPresetNames() const
{
    QStringList names;
    for (const ResolutionPreset &preset : resolutionPresets())
        names << tr("%1 × %2 · %3 fps").arg(preset.width).arg(preset.height).arg(preset.frameRate);
    return names;
}

QStringList DepthCameraController::colorSchemeNames() const
{
    QStringList names;
    for (const ColorSchemeOption &option : colorSchemeOptions())
        names << tr(option.name);
    return names;
}

QStringList DepthCameraController::visualPresetNames() const
{
    return {tr("Varsayılan"), tr("Yüksek doğruluk"), tr("Yüksek yoğunluk"), tr("Orta yoğunluk"), tr("El takibi")};
}

void DepthCameraController::start()
{
    isStartRequested = true;
    startPipeline();
}

void DepthCameraController::stop()
{
    isStartRequested = false;
    stopPipeline();
}

void DepthCameraController::startPipeline()
{
    if (isPipelineRunning || !deviceManager->isConnected())
        return;

    const ResolutionPreset &preset = resolutionPresets().at(selectedResolutionPreset);
    rs2::config config;
    config.enable_device(deviceManager->serialNumber().toStdString());
    config.enable_stream(RS2_STREAM_DEPTH, preset.width, preset.height, RS2_FORMAT_Z16, preset.frameRate);
    config.enable_stream(RS2_STREAM_COLOR, preset.width, preset.height, RS2_FORMAT_RGB8, preset.frameRate);

    try {
        videoPipeline = rs2::pipeline();
        const rs2::pipeline_profile profile = videoPipeline.start(config, [this](const rs2::frame &frame) {
            if (const rs2::frameset frames = frame.as<rs2::frameset>())
                processFrames(frames);
        });
        activeDepthSensor = profile.get_device().first<rs2::depth_sensor>();
    } catch (const rs2::error &error) {
        setError(tr("Derinlik akışı başlatılamadı: %1").arg(describeDeviceError(error)));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        appliedColorizerSettings.reset();
    }
    applySensorOptions();
    readSensorOptions();

    isPipelineRunning = true;
    frameRateTimer.start();
    setError({});
    emit streamingStateChanged();
}

void DepthCameraController::stopPipeline()
{
    if (!isPipelineRunning)
        return;

    frameRateTimer.stop();
    try {
        videoPipeline.stop();
    } catch (const rs2::error &error) {
        qWarning() << "Depth pipeline stop failed:" << error.what();
    }
    activeDepthSensor.reset();
    isPipelineRunning = false;

    {
        std::lock_guard<std::mutex> lock(frameMutex);
        latestDepthFrame.reset();
    }
    clearImages();
    measuredDepthFrameRate = 0;
    measuredColorFrameRate = 0;
    emit statisticsChanged();
    emit streamingStateChanged();
}

void DepthCameraController::restartIfStreaming()
{
    if (!isPipelineRunning)
        return;
    stopPipeline();
    startPipeline();
}

void DepthCameraController::processFrames(rs2::frameset frames)
{
    ProcessingSettings settings;
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        settings = processingSettings;
    }
    applyColorizerSettings(settings);

    // Intel recommends filtering in disparity space, and filtering before alignment.
    if (settings.spatialFilter || settings.temporalFilter) {
        frames = frames.apply_filter(depthToDisparity);
        if (settings.spatialFilter)
            frames = frames.apply_filter(spatialFilter);
        if (settings.temporalFilter)
            frames = frames.apply_filter(temporalFilter);
        frames = frames.apply_filter(disparityToDepth);
    }
    if (settings.holeFilling)
        frames = frames.apply_filter(holeFillingFilter);
    if (settings.alignToColor)
        frames = frames.apply_filter(alignToColor);

    const rs2::depth_frame depth = frames.get_depth_frame();
    const rs2::video_frame color = frames.get_color_frame();

    QImage colorizedDepth;
    double centerDistanceMeters = 0.0;
    double validPixels = 0.0;
    if (depth) {
        colorizedDepth = toImage(depthColorizer.colorize(depth));
        centerDistanceMeters = medianDistance(depth, depth.get_width() / 2, depth.get_height() / 2);
        validPixels = validRatio(depth);
        ++depthFrameCounter;
    }
    const QImage colorFrameImage = color ? toImage(color) : QImage();
    if (color)
        ++colorFrameCounter;

    if (depth) {
        storeDepthImage(colorizedDepth);
        std::lock_guard<std::mutex> lock(frameMutex);
        latestDepthFrame = depth;
        processedCenterDistance = centerDistanceMeters;
        processedValidPixelRatio = validPixels;
        processedDepthResolution = QSize(depth.get_width(), depth.get_height());
    }
    if (color)
        storeColorImage(colorFrameImage);

    notifyFramesUpdated();
}

void DepthCameraController::applyColorizerSettings(const ProcessingSettings &settings)
{
    const bool isAlreadyApplied = appliedColorizerSettings
                                  && appliedColorizerSettings->colorScheme == settings.colorScheme
                                  && appliedColorizerSettings->minimumDistance == settings.minimumDistance
                                  && appliedColorizerSettings->maximumDistance == settings.maximumDistance;
    if (isAlreadyApplied)
        return;

    depthColorizer.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0.0f);
    depthColorizer.set_option(RS2_OPTION_COLOR_SCHEME, colorSchemeOptions().at(settings.colorScheme).librealsenseValue);
    depthColorizer.set_option(RS2_OPTION_MIN_DISTANCE, static_cast<float>(settings.minimumDistance));
    depthColorizer.set_option(RS2_OPTION_MAX_DISTANCE, static_cast<float>(settings.maximumDistance));
    appliedColorizerSettings = settings;
}

void DepthCameraController::onFramesPublished()
{
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        latestCenterDistance = processedCenterDistance;
        latestValidPixelRatio = processedValidPixelRatio;
        latestDepthResolution = processedDepthResolution;
    }
    emit statisticsChanged();
}

void DepthCameraController::updateFrameRates()
{
    measuredDepthFrameRate = depthFrameCounter.exchange(0);
    measuredColorFrameRate = colorFrameCounter.exchange(0);
    emit statisticsChanged();
}

double DepthCameraController::distanceAt(const QPointF &normalizedPosition) const
{
    std::lock_guard<std::mutex> lock(frameMutex);
    if (!latestDepthFrame)
        return 0.0;

    const int width = latestDepthFrame->get_width();
    const int height = latestDepthFrame->get_height();
    const int pixelX = std::clamp(static_cast<int>(normalizedPosition.x() * width), 0, width - 1);
    const int pixelY = std::clamp(static_cast<int>(normalizedPosition.y() * height), 0, height - 1);
    return medianDistance(*latestDepthFrame, pixelX, pixelY);
}

double DepthCameraController::medianDistance(const rs2::depth_frame &depth, int centerX, int centerY)
{
    const auto *depthData = static_cast<const uint16_t *>(depth.get_data());
    const int width = depth.get_width();
    const int height = depth.get_height();
    const double depthUnits = depth.get_units();

    std::vector<uint16_t> validSamples;
    validSamples.reserve((2 * medianWindowRadius + 1) * (2 * medianWindowRadius + 1));
    for (int y = std::max(0, centerY - medianWindowRadius); y <= std::min(height - 1, centerY + medianWindowRadius); ++y) {
        for (int x = std::max(0, centerX - medianWindowRadius); x <= std::min(width - 1, centerX + medianWindowRadius); ++x) {
            const uint16_t rawDepth = depthData[y * width + x];
            if (rawDepth > 0)
                validSamples.push_back(rawDepth);
        }
    }
    if (validSamples.empty())
        return 0.0;

    const auto middle = validSamples.begin() + validSamples.size() / 2;
    std::nth_element(validSamples.begin(), middle, validSamples.end());
    return *middle * depthUnits;
}

double DepthCameraController::validRatio(const rs2::depth_frame &depth)
{
    const auto *depthData = static_cast<const uint16_t *>(depth.get_data());
    const int width = depth.get_width();
    const int height = depth.get_height();

    int sampledCount = 0;
    int validCount = 0;
    for (int y = 0; y < height; y += validRatioSampleStep) {
        for (int x = 0; x < width; x += validRatioSampleStep) {
            ++sampledCount;
            if (depthData[y * width + x] > 0)
                ++validCount;
        }
    }
    return sampledCount > 0 ? static_cast<double>(validCount) / sampledCount : 0.0;
}

void DepthCameraController::updateProcessingSettings(const std::function<void(ProcessingSettings &)> &change)
{
    {
        std::lock_guard<std::mutex> lock(settingsMutex);
        change(processingSettings);
    }
    emit settingsChanged();
}

void DepthCameraController::setResolutionPresetIndex(int presetIndex)
{
    if (presetIndex == selectedResolutionPreset || presetIndex < 0
        || presetIndex >= static_cast<int>(resolutionPresets().size()))
        return;
    selectedResolutionPreset = presetIndex;
    emit settingsChanged();
    restartIfStreaming();
}

bool DepthCameraController::isAlignedToColor() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return processingSettings.alignToColor;
}

void DepthCameraController::setAlignedToColor(bool isAligned)
{
    updateProcessingSettings([isAligned](ProcessingSettings &settings) { settings.alignToColor = isAligned; });
}

int DepthCameraController::colorSchemeIndex() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return processingSettings.colorScheme;
}

void DepthCameraController::setColorSchemeIndex(int schemeIndex)
{
    if (schemeIndex < 0 || schemeIndex >= static_cast<int>(colorSchemeOptions().size()))
        return;
    updateProcessingSettings([schemeIndex](ProcessingSettings &settings) { settings.colorScheme = schemeIndex; });
}

double DepthCameraController::minimumDistance() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return processingSettings.minimumDistance;
}

void DepthCameraController::setMinimumDistance(double meters)
{
    updateProcessingSettings([meters](ProcessingSettings &settings) {
        settings.minimumDistance = std::min(meters, settings.maximumDistance - 0.1);
    });
}

double DepthCameraController::maximumDistance() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return processingSettings.maximumDistance;
}

void DepthCameraController::setMaximumDistance(double meters)
{
    updateProcessingSettings([meters](ProcessingSettings &settings) {
        settings.maximumDistance = std::max(meters, settings.minimumDistance + 0.1);
    });
}

bool DepthCameraController::isSpatialFilterEnabled() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return processingSettings.spatialFilter;
}

void DepthCameraController::setSpatialFilterEnabled(bool isEnabled)
{
    updateProcessingSettings([isEnabled](ProcessingSettings &settings) { settings.spatialFilter = isEnabled; });
}

bool DepthCameraController::isTemporalFilterEnabled() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return processingSettings.temporalFilter;
}

void DepthCameraController::setTemporalFilterEnabled(bool isEnabled)
{
    updateProcessingSettings([isEnabled](ProcessingSettings &settings) { settings.temporalFilter = isEnabled; });
}

bool DepthCameraController::isHoleFillingEnabled() const
{
    std::lock_guard<std::mutex> lock(settingsMutex);
    return processingSettings.holeFilling;
}

void DepthCameraController::setHoleFillingEnabled(bool isEnabled)
{
    updateProcessingSettings([isEnabled](ProcessingSettings &settings) { settings.holeFilling = isEnabled; });
}

void DepthCameraController::setVisualPresetIndex(int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= static_cast<int>(visualPresetValues().size()))
        return;
    selectedVisualPreset = presetIndex;

    // A visual preset rewrites many sensor options, laser power included, so read them back.
    if (activeDepthSensor) {
        try {
            activeDepthSensor->set_option(RS2_OPTION_VISUAL_PRESET, static_cast<float>(visualPresetValues().at(presetIndex)));
        } catch (const rs2::error &error) {
            qWarning() << "Visual preset could not be applied:" << error.what();
        }
        readSensorOptions();
    }
    emit sensorOptionsChanged();
}

void DepthCameraController::setEmitterEnabled(bool isEnabled)
{
    emitterEnabled = isEnabled;
    if (activeDepthSensor) {
        try {
            activeDepthSensor->set_option(RS2_OPTION_EMITTER_ENABLED, isEnabled ? 1.0f : 0.0f);
        } catch (const rs2::error &error) {
            qWarning() << "Emitter option could not be applied:" << error.what();
        }
    }
    emit sensorOptionsChanged();
}

void DepthCameraController::setLaserPower(double power)
{
    requestedLaserPower = std::clamp(power, 0.0, maximumLaserPower);
    if (activeDepthSensor) {
        try {
            activeDepthSensor->set_option(RS2_OPTION_LASER_POWER, static_cast<float>(requestedLaserPower));
        } catch (const rs2::error &error) {
            qWarning() << "Laser power could not be applied:" << error.what();
        }
    }
    emit sensorOptionsChanged();
}

void DepthCameraController::applySensorOptions()
{
    if (!activeDepthSensor)
        return;
    try {
        activeDepthSensor->set_option(RS2_OPTION_VISUAL_PRESET, static_cast<float>(visualPresetValues().at(selectedVisualPreset)));
        activeDepthSensor->set_option(RS2_OPTION_EMITTER_ENABLED, emitterEnabled ? 1.0f : 0.0f);
        if (emitterEnabled)
            activeDepthSensor->set_option(RS2_OPTION_LASER_POWER, static_cast<float>(requestedLaserPower));
    } catch (const rs2::error &error) {
        qWarning() << "Depth sensor options could not be applied:" << error.what();
    }
}

void DepthCameraController::readSensorOptions()
{
    if (!activeDepthSensor)
        return;
    try {
        maximumLaserPower = activeDepthSensor->get_option_range(RS2_OPTION_LASER_POWER).max;
        requestedLaserPower = activeDepthSensor->get_option(RS2_OPTION_LASER_POWER);
        emitterEnabled = activeDepthSensor->get_option(RS2_OPTION_EMITTER_ENABLED) > 0.5f;
    } catch (const rs2::error &error) {
        qWarning() << "Depth sensor options could not be read:" << error.what();
    }
    emit sensorOptionsChanged();
}

void DepthCameraController::setError(const QString &message)
{
    if (currentErrorMessage == message)
        return;
    currentErrorMessage = message;
    emit streamingStateChanged();
}
