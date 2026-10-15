#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QQuickPaintedItem>
#include <QVector3D>

// Lightweight 3D view of the D435i body, rotated by the fused orientation.
// Rendering is done with QPainter (flat-shaded faces sorted back-to-front), which keeps
// the project free of Qt 3D / Quick 3D dependencies that are not available on Qt 5.15.
class OrientationView3D : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QQuaternion orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QColor bodyColor MEMBER bodyColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor frontColor MEMBER frontColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor accentColor MEMBER accentColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor gridColor MEMBER gridColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor labelColor MEMBER labelColor NOTIFY appearanceChanged)

public:
    explicit OrientationView3D(QQuickItem *parent = nullptr);

    QQuaternion orientation() const { return currentOrientation; }
    void setOrientation(const QQuaternion &orientation);

    void paint(QPainter *painter) override;

signals:
    void orientationChanged();
    void appearanceChanged();

private:
    struct Face
    {
        int cornerIndices[4];
        QVector3D normal;
        bool isFront;
    };

    void updateViewProjection();
    QPointF project(const QVector3D &worldPoint) const;
    QColor shade(const QColor &baseColor, const QVector3D &worldNormal) const;

    void drawHeadingRing(QPainter *painter) const;
    void drawBody(QPainter *painter) const;
    void drawFrontDetails(QPainter *painter) const;
    void drawForwardArrow(QPainter *painter) const;

    QQuaternion currentOrientation;
    QMatrix4x4 viewProjection;
    QVector3D eyePosition;

    QColor bodyColor {"#64748B"};
    QColor frontColor {"#111827"};
    QColor accentColor {"#22C55E"};
    QColor gridColor {"#334155"};
    QColor labelColor {"#94A3B8"};
};
