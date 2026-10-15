#include "VideoFrameView.h"

#include "media/FrameSource.h"

#include <QPainter>

VideoFrameView::VideoFrameView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAcceptHoverEvents(true);
    connect(this, &QQuickItem::widthChanged, this, &VideoFrameView::imageChanged);
    connect(this, &QQuickItem::heightChanged, this, &VideoFrameView::imageChanged);
}

QObject *VideoFrameView::source() const
{
    return frameSource.data();
}

void VideoFrameView::setSource(QObject *source)
{
    auto *newSource = qobject_cast<FrameSource *>(source);
    if (newSource == frameSource)
        return;

    disconnect(frameConnection);
    frameSource = newSource;
    if (frameSource)
        frameConnection = connect(frameSource, &FrameSource::framesUpdated, this, &VideoFrameView::refreshImage);

    emit sourceChanged();
    refreshImage();
}

void VideoFrameView::setStream(int kind)
{
    if (streamKind == kind)
        return;
    streamKind = kind;
    emit sourceChanged();
    refreshImage();
}

QRectF VideoFrameView::imageRect() const
{
    if (currentImage.isNull() || width() <= 0 || height() <= 0)
        return {};
    const QSizeF fittedSize = QSizeF(currentImage.size()).scaled(size(), Qt::KeepAspectRatio);
    return QRectF(QPointF((width() - fittedSize.width()) / 2.0, (height() - fittedSize.height()) / 2.0), fittedSize);
}

void VideoFrameView::paint(QPainter *painter)
{
    if (currentImage.isNull())
        return;
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawImage(imageRect(), currentImage);
}

void VideoFrameView::refreshImage()
{
    const bool hadImage = hasImage();
    currentImage = frameSource ? frameSource->latestImage(static_cast<FrameSource::StreamKind>(streamKind)) : QImage();

    if (hadImage != hasImage() || currentImage.size() != lastImageSize) {
        lastImageSize = currentImage.size();
        emit imageChanged();
    }
    update();
}

void VideoFrameView::hoverMoveEvent(QHoverEvent *event)
{
    updateHover(event->posF());
}

void VideoFrameView::hoverLeaveEvent(QHoverEvent *)
{
    hovering = false;
    emit hoverChanged();
}

void VideoFrameView::updateHover(const QPointF &itemPosition)
{
    const QRectF drawnRect = imageRect();
    const bool isInsideImage = drawnRect.contains(itemPosition);
    hovering = isInsideImage;
    if (isInsideImage) {
        normalizedHoverPosition = QPointF((itemPosition.x() - drawnRect.left()) / drawnRect.width(),
                                          (itemPosition.y() - drawnRect.top()) / drawnRect.height());
    }
    emit hoverChanged();
}
