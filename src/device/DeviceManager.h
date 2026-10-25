#pragma once

#include <QObject>
#include <QString>

#include <librealsense2/rs.hpp>

// Watches the USB bus for RealSense devices and exposes the active device to QML.
// Hot-plug events arrive on a librealsense thread and are forwarded to the Qt thread.
class DeviceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY deviceChanged)
    Q_PROPERTY(bool hasMotionModule READ hasMotionModule NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceChanged)
    Q_PROPERTY(QString serialNumber READ serialNumber NOTIFY deviceChanged)
    Q_PROPERTY(QString firmwareVersion READ firmwareVersion NOTIFY deviceChanged)
    Q_PROPERTY(QString usbType READ usbType NOTIFY deviceChanged)

public:
    explicit DeviceManager(QObject *parent = nullptr);

    bool isConnected() const { return !connectedSerialNumber.isEmpty(); }
    bool hasMotionModule() const { return connectedDeviceHasImu; }
    QString deviceName() const { return connectedDeviceName; }
    QString serialNumber() const { return connectedSerialNumber; }
    QString firmwareVersion() const { return connectedFirmwareVersion; }
    QString usbType() const { return connectedUsbType; }

signals:
    void deviceChanged();
    void deviceConnected(const QString &serialNumber);
    void deviceDisconnected();

private:
    void refreshDevices();
    void clearDevice();
    static QString readInfo(const rs2::device &device, rs2_camera_info info);
    static bool deviceHasMotionModule(const rs2::device &device);

    rs2::context realSenseContext;
    QString connectedDeviceName;
    QString connectedSerialNumber;
    QString connectedFirmwareVersion;
    QString connectedUsbType;
    bool connectedDeviceHasImu = false;
};
