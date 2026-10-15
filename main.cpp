#include "depth/DepthCameraController.h"
#include "device/DeviceManager.h"
#include "imu/AccelCalibrationController.h"
#include "imu/ImuController.h"
#include "media/PlaybackController.h"
#include "media/RecordingController.h"
#include "ui/OrientationView3D.h"
#include "ui/RealtimeChart.h"
#include "ui/VideoFrameView.h"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>

namespace {

void loadApplicationFonts()
{
    const QStringList fontFiles = {
        QStringLiteral(":/resources/fonts/FiraSans-Regular.ttf"),
        QStringLiteral(":/resources/fonts/FiraSans-Medium.ttf"),
        QStringLiteral(":/resources/fonts/FiraSans-SemiBold.ttf"),
        QStringLiteral(":/resources/fonts/FiraMono-Regular.ttf"),
        QStringLiteral(":/resources/fonts/FiraMono-Medium.ttf"),
    };
    for (const QString &fontFile : fontFiles) {
        if (QFontDatabase::addApplicationFont(fontFile) < 0)
            qWarning() << "Font could not be loaded:" << fontFile;
    }
    QGuiApplication::setFont(QFont(QStringLiteral("Fira Sans"), 10));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("RealSense Studio"));
    QGuiApplication::setOrganizationName(QStringLiteral("RealSenseStudio"));
    loadApplicationFonts();

    DeviceManager deviceManager;
    ImuController imuController(&deviceManager);
    AccelCalibrationController calibrationController(&deviceManager, &imuController);
    DepthCameraController depthCameraController(&deviceManager);
    RecordingController recordingController(&deviceManager, &imuController, &depthCameraController);
    PlaybackController playbackController;

    qmlRegisterSingletonInstance("RealSense.Core", 1, 0, "DeviceManager", &deviceManager);
    qmlRegisterSingletonInstance("RealSense.Core", 1, 0, "ImuController", &imuController);
    qmlRegisterSingletonInstance("RealSense.Core", 1, 0, "CalibrationController", &calibrationController);
    qmlRegisterSingletonInstance("RealSense.Core", 1, 0, "DepthCameraController", &depthCameraController);
    qmlRegisterSingletonInstance("RealSense.Core", 1, 0, "RecordingController", &recordingController);
    qmlRegisterSingletonInstance("RealSense.Core", 1, 0, "PlaybackController", &playbackController);
    qmlRegisterType<OrientationView3D>("RealSense.Ui", 1, 0, "OrientationView3D");
    qmlRegisterType<RealtimeChart>("RealSense.Ui", 1, 0, "RealtimeChart");
    qmlRegisterType<VideoFrameView>("RealSense.Ui", 1, 0, "VideoFrameView");
    qmlRegisterUncreatableType<FrameSource>("RealSense.Ui", 1, 0, "FrameSource", QStringLiteral("FrameSource is abstract"));

    QQmlApplicationEngine engine;
    const QUrl mainQmlUrl(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [mainQmlUrl](QObject *object, const QUrl &objectUrl) {
            if (!object && mainQmlUrl == objectUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(mainQmlUrl);

    imuController.start();
    return app.exec();
}
