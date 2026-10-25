QT += quick quickcontrols2
CONFIG += c++17

TARGET = RealsenseStudio

INCLUDEPATH += src
LIBS += -lrealsense2

HEADERS += \
        src/depth/DepthCameraController.h \
        src/device/DeviceErrors.h \
        src/device/DeviceManager.h \
        src/imu/AccelCalibrationController.h \
        src/imu/AccelCalibrationSolver.h \
        src/imu/CalibrationStore.h \
        src/imu/ContinuousEulerTracker.h \
        src/imu/GyroBiasEstimator.h \
        src/imu/ImuController.h \
        src/imu/ImuFusionEngine.h \
        src/imu/ImuTypes.h \
        src/imu/MotionSampleListener.h \
        src/imu/OrientationFilter.h \
        src/imu/StationaryDetector.h \
        src/media/FrameSource.h \
        src/media/PlaybackController.h \
        src/media/RecordingController.h \
        src/ui/OrientationView3D.h \
        src/ui/RealtimeChart.h \
        src/ui/VideoFrameView.h

SOURCES += \
        main.cpp \
        src/depth/DepthCameraController.cpp \
        src/device/DeviceErrors.cpp \
        src/device/DeviceManager.cpp \
        src/imu/AccelCalibrationController.cpp \
        src/imu/AccelCalibrationSolver.cpp \
        src/imu/CalibrationStore.cpp \
        src/imu/ContinuousEulerTracker.cpp \
        src/imu/GyroBiasEstimator.cpp \
        src/imu/ImuController.cpp \
        src/imu/ImuFusionEngine.cpp \
        src/imu/ImuTypes.cpp \
        src/imu/OrientationFilter.cpp \
        src/imu/StationaryDetector.cpp \
        src/media/FrameSource.cpp \
        src/media/PlaybackController.cpp \
        src/media/RecordingController.cpp \
        src/ui/OrientationView3D.cpp \
        src/ui/RealtimeChart.cpp \
        src/ui/VideoFrameView.cpp

RESOURCES += qml.qrc

QML_IMPORT_PATH =
QML_DESIGNER_IMPORT_PATH =

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
