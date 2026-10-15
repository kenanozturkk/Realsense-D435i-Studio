#pragma once

#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>

class FrameSource;

// Draws the latest depth or color image of a FrameSource, aspect-fit and centered.
// Also reports the hovered position in normalized image coordinates (0..1) so QML can
// ask the source for the distance under the cursor.
class VideoFrameView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int stream READ stream WRITE setStream NOTIFY sourceChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY imageChanged)
    Q_PROPERTY(QRectF imageRect READ imageRect NOTIFY imageChanged)
    Q_PROPERTY(bool isHovering READ isHovering NOTIFY hoverChanged)
    Q_PROPERTY(QPointF hoverPosition READ hoverPosition NOTIFY hoverChanged)

public:
    explicit VideoFrameView(QQuickItem *parent = nullptr);

    QObject *source() const;
    void setSource(QObject *source);
    int stream() const { return streamKind; }
    void setStream(int kind);

    bool hasImage() const { return !currentImage.isNull(); }
    QRectF imageRect() const;
    bool isHovering() const { return hovering; }
    QPointF hoverPosition() const { return normalizedHoverPosition; }

    void paint(QPainter *painter) override;

signals:
    void sourceChanged();
    void imageChanged();
    void hoverChanged();

protected:
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

private:
    void refreshImage();
    void updateHover(const QPointF &itemPosition);

    QPointer<FrameSource> frameSource;
    QMetaObject::Connection frameConnection;
    int streamKind = 0;
    QImage currentImage;
    QSize lastImageSize;
    bool hovering = false;
    QPointF normalizedHoverPosition;
};
