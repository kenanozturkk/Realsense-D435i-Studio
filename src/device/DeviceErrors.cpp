#include "DeviceErrors.h"

#include <QCoreApplication>

QString describeDeviceError(const std::exception &error)
{
    const QString details = QString::fromUtf8(error.what());
    if (details.contains(QLatin1String("busy"), Qt::CaseInsensitive))
        return QCoreApplication::translate("DeviceErrors", "Kamera başka bir uygulama tarafından kullanılıyor.");
    return details;
}
