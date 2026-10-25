#pragma once

#include "ImuTypes.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include <optional>

namespace imu {

struct StoredCalibration
{
    AccelCalibration accel;
    QDateTime createdAt;
};

// Persists accelerometer calibrations in a JSON file, keyed by device serial number,
// so each camera keeps its own correction. Nothing is written to the device itself.
class CalibrationStore
{
public:
    CalibrationStore();
    explicit CalibrationStore(const QString &customFilePath);

    std::optional<StoredCalibration> load(const QString &serialNumber) const;
    bool save(const QString &serialNumber, const StoredCalibration &calibration);
    bool remove(const QString &serialNumber);

    QString filePath() const { return storageFilePath; }

private:
    static QString defaultFilePath();
    QJsonObject readAll() const;
    bool writeAll(const QJsonObject &root) const;

    QString storageFilePath;
};

} // namespace imu
