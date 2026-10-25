#pragma once

#include <QString>

#include <exception>

// Turns a librealsense exception into a message suitable for the UI. Common, actionable
// failures get a plain-language explanation; anything else keeps the library's own text.
QString describeDeviceError(const std::exception &error);
