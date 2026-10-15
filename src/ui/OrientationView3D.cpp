#include "OrientationView3D.h"

#include <QPainter>
#include <QPolygonF>
#include <QtMath>

#include <algorithm>
#include <array>

namespace {

// Body dimensions in FLU, proportional to the real 90 x 25 x 25 mm housing.
constexpr float halfDepth = 0.13f;
constexpr float halfWidth = 0.47f;
constexpr float halfHeight = 0.13f;

constexpr float groundLevel = -0.36f;
constexpr float headingRingRadius = 0.68f;
constexpr float arrowLength = 0.42f;

const QVector3D defaultEyePosition(2.3f, -1.55f, 1.15f);
const QVector3D lightDirection = QVector3D(0.5f, -0.35f, 1.0f).normalized();

const std::array<QVector3D, 8> boxCorners = {{
    {-halfDepth, -halfWidth, -halfHeight}, {halfDepth, -halfWidth, -halfHeight},
    {halfDepth, halfWidth, -halfHeight},   {-halfDepth, halfWidth, -halfHeight},
    {-halfDepth, -halfWidth, halfHeight},  {halfDepth, -halfWidth, halfHeight},
    {halfDepth, halfWidth, halfHeight},    {-halfDepth, halfWidth, halfHeight},
}};

struct LensSpec
{
    float lateralPosition;
    float radius;
    bool isEmitter;
};

const std::array<LensSpec, 4> lenses = {{
    {0.34f, 0.058f, false}, {0.14f, 0.040f, true}, {-0.18f, 0.058f, false}, {-0.34f, 0.046f, false},
}};

} // namespace

OrientationView3D::OrientationView3D(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , eyePosition(defaultEyePosition)
{
    setAntialiasing(true);
    connect(this, &OrientationView3D::appearanceChanged, this, [this] { update(); });
    connect(this, &QQuickItem::widthChanged, this, &OrientationView3D::updateViewProjection);
    connect(this, &QQuickItem::heightChanged, this, &OrientationView3D::updateViewProjection);
    updateViewProjection();
}

void OrientationView3D::setOrientation(const QQuaternion &orientation)
{
    if (qFuzzyCompare(currentOrientation, orientation))
        return;
    currentOrientation = orientation;
    emit orientationChanged();
    update();
}

void OrientationView3D::updateViewProjection()
{
    const float aspectRatio = height() > 0.0 ? static_cast<float>(width() / height()) : 1.0f;
    QMatrix4x4 projection;
    projection.perspective(27.0f, aspectRatio, 0.1f, 50.0f);
    QMatrix4x4 view;
    view.lookAt(eyePosition, QVector3D(0.0f, 0.0f, -0.14f), QVector3D(0.0f, 0.0f, 1.0f));
    viewProjection = projection * view;
    update();
}

QPointF OrientationView3D::project(const QVector3D &worldPoint) const
{
    const QVector3D normalizedDevice = viewProjection.map(worldPoint);
    return {(normalizedDevice.x() + 1.0) * 0.5 * width(), (1.0 - normalizedDevice.y()) * 0.5 * height()};
}

QColor OrientationView3D::shade(const QColor &baseColor, const QVector3D &worldNormal) const
{
    const float diffuse = std::max(0.0f, QVector3D::dotProduct(worldNormal, lightDirection));
    const float intensity = 0.38f + 0.62f * diffuse;
    return QColor::fromRgbF(std::min(1.0, baseColor.redF() * intensity),
                            std::min(1.0, baseColor.greenF() * intensity),
                            std::min(1.0, baseColor.blueF() * intensity));
}

void OrientationView3D::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    drawHeadingRing(painter);

    const QVector3D arrowTip = currentOrientation.rotatedVector(QVector3D(halfDepth + arrowLength, 0.0f, 0.0f));
    const bool isArrowBehindBody = (arrowTip - eyePosition).length() > eyePosition.length();

    if (isArrowBehindBody)
        drawForwardArrow(painter);
    drawBody(painter);
    if (!isArrowBehindBody)
        drawForwardArrow(painter);
}

void OrientationView3D::drawHeadingRing(QPainter *painter) const
{
    constexpr int ringSegmentCount = 72;
    QPolygonF ring;
    for (int segment = 0; segment <= ringSegmentCount; ++segment) {
        const float angle = 2.0f * float(M_PI) * segment / ringSegmentCount;
        ring << project({headingRingRadius * std::cos(angle), headingRingRadius * std::sin(angle), groundLevel});
    }
    painter->setPen(QPen(gridColor, 1.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(ring);

    QFont labelFont(QStringLiteral("Fira Mono"));
    labelFont.setPixelSize(11);
    painter->setFont(labelFont);

    for (int degrees = 0; degrees < 360; degrees += 30) {
        const float angle = qDegreesToRadians(float(degrees));
        const QVector3D direction(std::cos(angle), std::sin(angle), 0.0f);
        const bool isMajorTick = degrees % 90 == 0;
        const float innerRadius = headingRingRadius - (isMajorTick ? 0.08f : 0.04f);
        const QPointF inner = project(direction * innerRadius + QVector3D(0, 0, groundLevel));
        const QPointF outer = project(direction * headingRingRadius + QVector3D(0, 0, groundLevel));
        painter->setPen(QPen(gridColor, isMajorTick ? 2.0 : 1.2));
        painter->drawLine(inner, outer);

        if (isMajorTick) {
            const QPointF labelCenter = project(direction * (headingRingRadius + 0.13f) + QVector3D(0, 0, groundLevel));
            painter->setPen(labelColor);
            painter->drawText(QRectF(labelCenter.x() - 20, labelCenter.y() - 8, 40, 16), Qt::AlignCenter,
                              QStringLiteral("%1°").arg(degrees));
        }
    }

    const QVector3D forward = currentOrientation.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D headingOnGround = QVector3D(forward.x(), forward.y(), 0.0f);
    if (headingOnGround.length() > 0.05f) {
        const QPointF center = project({0.0f, 0.0f, groundLevel});
        const QPointF tip = project(headingOnGround.normalized() * headingRingRadius + QVector3D(0, 0, groundLevel));
        QColor headingColor = accentColor;
        headingColor.setAlphaF(0.55);
        painter->setPen(QPen(headingColor, 2.0, Qt::DashLine, Qt::RoundCap));
        painter->drawLine(center, tip);
    }
}

void OrientationView3D::drawBody(QPainter *painter) const
{
    static const std::array<Face, 6> faces = {{
        {{1, 2, 6, 5}, {1, 0, 0}, true},
        {{0, 4, 7, 3}, {-1, 0, 0}, false},
        {{2, 3, 7, 6}, {0, 1, 0}, false},
        {{0, 1, 5, 4}, {0, -1, 0}, false},
        {{4, 5, 6, 7}, {0, 0, 1}, false},
        {{0, 3, 2, 1}, {0, 0, -1}, false},
    }};

    std::array<QVector3D, 8> worldCorners;
    for (size_t index = 0; index < boxCorners.size(); ++index)
        worldCorners[index] = currentOrientation.rotatedVector(boxCorners[index]);

    struct VisibleFace
    {
        const Face *face;
        QVector3D worldNormal;
        float depth;
    };
    std::vector<VisibleFace> visibleFaces;

    for (const Face &face : faces) {
        const QVector3D worldNormal = currentOrientation.rotatedVector(face.normal);
        QVector3D faceCenter;
        for (int corner : face.cornerIndices)
            faceCenter += worldCorners[corner];
        faceCenter /= 4.0f;

        if (QVector3D::dotProduct(worldNormal, eyePosition - faceCenter) <= 0.0f)
            continue;
        visibleFaces.push_back({&face, worldNormal, (faceCenter - eyePosition).length()});
    }

    std::sort(visibleFaces.begin(), visibleFaces.end(),
              [](const VisibleFace &a, const VisibleFace &b) { return a.depth > b.depth; });

    for (const VisibleFace &visibleFace : visibleFaces) {
        QPolygonF polygon;
        for (int corner : visibleFace.face->cornerIndices)
            polygon << project(worldCorners[corner]);

        const QColor baseColor = visibleFace.face->isFront ? frontColor : bodyColor;
        const QColor fillColor = shade(baseColor, visibleFace.worldNormal);
        painter->setPen(QPen(fillColor.darker(140), 1.0));
        painter->setBrush(fillColor);
        painter->drawPolygon(polygon);

        if (visibleFace.face->isFront)
            drawFrontDetails(painter);
    }
}

void OrientationView3D::drawFrontDetails(QPainter *painter) const
{
    constexpr int circleSegments = 24;
    const float frontSurface = halfDepth + 0.001f;

    for (const LensSpec &lens : lenses) {
        QPolygonF outline;
        for (int segment = 0; segment < circleSegments; ++segment) {
            const float angle = 2.0f * float(M_PI) * segment / circleSegments;
            const QVector3D localPoint(frontSurface, lens.lateralPosition + lens.radius * std::cos(angle),
                                       lens.radius * std::sin(angle));
            outline << project(currentOrientation.rotatedVector(localPoint));
        }
        painter->setPen(QPen(QColor("#475569"), 1.2));
        painter->setBrush(lens.isEmitter ? QColor("#3F1D2B") : QColor("#020617"));
        painter->drawPolygon(outline);
    }
}

void OrientationView3D::drawForwardArrow(QPainter *painter) const
{
    const QVector3D base = currentOrientation.rotatedVector(QVector3D(halfDepth, 0.0f, 0.0f));
    const QVector3D tip = currentOrientation.rotatedVector(QVector3D(halfDepth + arrowLength, 0.0f, 0.0f));
    const QVector3D headLeft = currentOrientation.rotatedVector(QVector3D(halfDepth + arrowLength - 0.1f, 0.05f, 0.0f));
    const QVector3D headRight = currentOrientation.rotatedVector(QVector3D(halfDepth + arrowLength - 0.1f, -0.05f, 0.0f));

    painter->setPen(QPen(accentColor, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(project(base), project(tip));
    painter->setBrush(accentColor);
    painter->drawPolygon(QPolygonF({project(tip), project(headLeft), project(headRight)}));
}
