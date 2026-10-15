#include "CalibrationStore.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace imu {

namespace {

QJsonArray toJson(const QVector3D &vector)
{
    return {vector.x(), vector.y(), vector.z()};
}

QVector3D vectorFromJson(const QJsonValue &value, const QVector3D &fallback)
{
    const QJsonArray array = value.toArray();
    if (array.size() != 3)
        return fallback;
    return QVector3D(static_cast<float>(array[0].toDouble()), static_cast<float>(array[1].toDouble()),
                     static_cast<float>(array[2].toDouble()));
}

} // namespace

CalibrationStore::CalibrationStore()
    : storageFilePath(defaultFilePath())
{
}

CalibrationStore::CalibrationStore(const QString &customFilePath)
    : storageFilePath(customFilePath)
{
}

QString CalibrationStore::defaultFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/imu_calibration.json");
}

std::optional<StoredCalibration> CalibrationStore::load(const QString &serialNumber) const
{
    const QJsonObject entry = readAll().value(serialNumber).toObject();
    if (entry.isEmpty())
        return std::nullopt;

    StoredCalibration calibration;
    calibration.accel.offset = vectorFromJson(entry.value(QStringLiteral("accelOffset")), QVector3D(0, 0, 0));
    calibration.accel.scale = vectorFromJson(entry.value(QStringLiteral("accelScale")), QVector3D(1, 1, 1));
    calibration.createdAt = QDateTime::fromString(entry.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    return calibration;
}

bool CalibrationStore::save(const QString &serialNumber, const StoredCalibration &calibration)
{
    QJsonObject root = readAll();
    root.insert(serialNumber, QJsonObject {
                                  {QStringLiteral("accelOffset"), toJson(calibration.accel.offset)},
                                  {QStringLiteral("accelScale"), toJson(calibration.accel.scale)},
                                  {QStringLiteral("createdAt"), calibration.createdAt.toString(Qt::ISODate)},
                              });
    return writeAll(root);
}

bool CalibrationStore::remove(const QString &serialNumber)
{
    QJsonObject root = readAll();
    root.remove(serialNumber);
    return writeAll(root);
}

QJsonObject CalibrationStore::readAll() const
{
    QFile file(storageFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

bool CalibrationStore::writeAll(const QJsonObject &root) const
{
    QDir().mkpath(QFileInfo(storageFilePath).absolutePath());
    QSaveFile file(storageFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Calibration file could not be opened:" << storageFilePath;
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

} // namespace imu
