#pragma once

#include <QImage>
#include <QObject>

#include <librealsense2/rs.hpp>

#include <atomic>
#include <mutex>

// Base for everything that produces images for VideoFrameView (live depth, recording
// preview, playback). Images are stored thread-safely from the RealSense thread and the
// UI is notified at most once per event-loop turn, so a slow UI never queues up frames.
class FrameSource : public QObject
{
    Q_OBJECT

public:
    enum StreamKind { Depth, Color };
    Q_ENUM(StreamKind)

    explicit FrameSource(QObject *parent = nullptr);

    QImage latestImage(StreamKind kind) const;

    static QImage toImage(const rs2::video_frame &frame);

signals:
    void framesUpdated();

protected:
    void storeDepthImage(const QImage &image);
    void storeColorImage(const QImage &image);
    void clearImages();
    void notifyFramesUpdated();

    // Called on the UI thread right before framesUpdated is emitted.
    virtual void onFramesPublished() {}

private:
    void publishFrames();

    mutable std::mutex imageMutex;
    QImage depthImage;
    QImage colorImage;
    std::atomic<bool> hasPendingUpdate {false};
};
