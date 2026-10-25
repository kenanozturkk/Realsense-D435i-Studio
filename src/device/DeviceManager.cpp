#include "DeviceManager.h"

#include <QDebug>
#include <QMetaObject>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    realSenseContext.set_devices_changed_callback([this](rs2::event_information &) {
        QMetaObject::invokeMethod(this, &DeviceManager::refreshDevices, Qt::QueuedConnection);
    });
    refreshDevices();
}

void DeviceManager::refreshDevices()
{
    rs2::device_list devices;
    try {
        devices = realSenseContext.query_devices();
    } catch (const rs2::error &error) {
        qWarning() << "RealSense device query failed:" << error.what();
    }

    if (devices.size() == 0) {
        if (isConnected())
            clearDevice();
        return;
    }

    const rs2::device device = devices[0];
    const QString serialNumber = readInfo(device, RS2_CAMERA_INFO_SERIAL_NUMBER);
    if (serialNumber == connectedSerialNumber)
        return;

    connectedDeviceName = readInfo(device, RS2_CAMERA_INFO_NAME);
    connectedSerialNumber = serialNumber;
    connectedFirmwareVersion = readInfo(device, RS2_CAMERA_INFO_FIRMWARE_VERSION);
    connectedUsbType = readInfo(device, RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
    connectedDeviceHasImu = deviceHasMotionModule(device);

    emit deviceChanged();
    emit deviceConnected(connectedSerialNumber);
}

void DeviceManager::clearDevice()
{
    connectedDeviceName.clear();
    connectedSerialNumber.clear();
    connectedFirmwareVersion.clear();
    connectedUsbType.clear();
    connectedDeviceHasImu = false;

    emit deviceChanged();
    emit deviceDisconnected();
}

QString DeviceManager::readInfo(const rs2::device &device, rs2_camera_info info)
{
    if (!device.supports(info))
        return {};
    return QString::fromUtf8(device.get_info(info));
}

bool DeviceManager::deviceHasMotionModule(const rs2::device &device)
{
    for (const rs2::sensor &sensor : device.query_sensors()) {
        if (sensor.is<rs2::motion_sensor>())
            return true;
    }
    return false;
}
