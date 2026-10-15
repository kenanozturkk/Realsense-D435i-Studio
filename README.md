# RealSense Studio

A Qt Quick (QML) desktop application for the **Intel® RealSense™ D435i** depth camera:
IMU orientation tracking, accelerometer calibration, live depth/color view and
compressed recording with playback.

## Features

**IMU orientation**
- Gyroscope + accelerometer fusion with a Mahony filter
- Stationary detection with automatic, continuous gyro bias estimation
- Gimbal-lock-safe continuous roll / pitch / yaw, live 3D orientation view and charts
- Heading reset and on-demand bias re-learning

**Accelerometer calibration**
- Six-pose guided wizard with automatic pose detection and capture
- Offset and scale solved with Gauss-Newton least squares
- Stored per camera serial number and applied automatically on connect

**Depth and color**
- Live depth (colorized) and color streams at 848 × 480 @ 30 fps
- Post-processing filters and depth-to-color alignment
- Click-to-measure distance (5 × 5 median)

**Recording and playback**
- Depth, color and IMU recorded into a single LZ4-compressed `.bag` file (~3.7× smaller)
- Stream watchdog: sensors are reopened automatically if a stream fails to start
- Playback without a camera, including IMU orientation from the recorded samples;
  recordings with a missing stream still play the remaining ones

**General**
- Camera hot-plug detection
- Material Dark theme, Fira Sans / Fira Mono fonts, Lucide icons

## Requirements

- Linux (developed and tested on Ubuntu 22.04)
- Qt 5.15 with Qt Quick, Qt Quick Controls 2 and Qt Graphical Effects
- [librealsense](https://github.com/IntelRealSense/librealsense) 2.56 or newer
- A C++17 compiler
- Intel RealSense D435i connected over USB 3

On Ubuntu 22.04, the Qt packages are:

```bash
sudo apt install qtbase5-dev qtdeclarative5-dev qtquickcontrols2-5-dev \
    qml-module-qtquick2 qml-module-qtquick-controls2 qml-module-qtquick-layouts \
    qml-module-qtquick-window2 qml-module-qtgraphicaleffects
```

Install librealsense (`librealsense2-dev`, `librealsense2-utils`) from Intel's package
repository by following the
[official Linux installation guide](https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_linux.md).

## Building

```bash
git clone https://github.com/kenanozturkk/Realsense-D435i-Studio.git
mkdir build && cd build
qmake ../Realsense-D435i-Studio/Realsense.pro
make -j$(nproc)
./RealsenseStudio
```

The project can also be opened directly in Qt Creator via `Realsense.pro`.

## Data locations

| Data | Location |
|---|---|
| Recordings | `~/Documents/RealSenseRecordings/` |
| IMU calibration | `~/.config/RealSenseStudio/RealSense Studio/imu_calibration.json` |

## Project structure

```
src/
  device/   camera discovery, hot-plug, error messages
  imu/      sensor fusion, bias estimation, calibration solver and storage
  depth/    depth/color streaming, filters, distance measurement
  media/    recording and playback
  ui/       custom Qt Quick items (3D orientation view, charts, video view)
qml/
  pages/    application pages
  components/, theme/
resources/  fonts and icons
```

## License

Released under the [MIT License](LICENSE).

Bundled third-party assets:
- Fira Sans and Fira Mono fonts: SIL Open Font License 1.1 (`resources/fonts/OFL.txt`)
- Lucide icons: ISC License (`resources/icons/LICENSE-lucide.txt`)

## Trademarks

Intel and RealSense are trademarks of Intel Corporation. This project is not affiliated
with or endorsed by Intel.
