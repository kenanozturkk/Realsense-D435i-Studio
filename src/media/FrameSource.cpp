#include "FrameSource.h"

#include <QMetaObject>

FrameSource::FrameSource(QObject *parent)
    : QObject(parent)
{
}

QImage FrameSource::latestImage(StreamKind kind) const
{
    std::lock_guard<std::mutex> lock(imageMutex);
    return kind == Depth ? depthImage : colorImage;
}

QImage FrameSource::toImage(const rs2::video_frame &frame)
{
    if (!frame || frame.get_profile().format() != RS2_FORMAT_RGB8)
        return {};
    const QImage view(static_cast<const uchar *>(frame.get_data()), frame.get_width(), frame.get_height(),
                      frame.get_stride_in_bytes(), QImage::Format_RGB888);
    return view.copy();
}

void FrameSource::storeDepthImage(const QImage &image)
{
    std::lock_guard<std::mutex> lock(imageMutex);
    depthImage = image;
}

void FrameSource::storeColorImage(const QImage &image)
{
    std::lock_guard<std::mutex> lock(imageMutex);
    colorImage = image;
}

void FrameSource::clearImages()
{
    {
        std::lock_guard<std::mutex> lock(imageMutex);
        depthImage = QImage();
        colorImage = QImage();
    }
    emit framesUpdated();
}

void FrameSource::notifyFramesUpdated()
{
    if (!hasPendingUpdate.exchange(true))
        QMetaObject::invokeMethod(this, &FrameSource::publishFrames, Qt::QueuedConnection);
}

void FrameSource::publishFrames()
{
    hasPendingUpdate = false;
    onFramesPublished();
    emit framesUpdated();
}
