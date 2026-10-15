#include "RealtimeChart.h"

#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace {
constexpr double labelAreaWidth = 52.0;
constexpr double verticalPadding = 8.0;
}

RealtimeChart::RealtimeChart(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    elapsedClock.start();
    connect(this, &RealtimeChart::appearanceChanged, this, [this] { update(); });
}

void RealtimeChart::appendSample(const QVariantList &values)
{
    Sample sample;
    sample.timeSeconds = elapsedClock.elapsed() / 1000.0;
    sample.values.reserve(values.size());
    for (const QVariant &value : values)
        sample.values.push_back(value.toDouble());
    samples.push_back(std::move(sample));

    const double oldestVisibleTime = samples.back().timeSeconds - timeWindowSeconds;
    while (samples.size() > 2 && samples[1].timeSeconds < oldestVisibleTime)
        samples.pop_front();

    update();
}

void RealtimeChart::clear()
{
    samples.clear();
    update();
}

QVariantList RealtimeChart::seriesColors() const
{
    QVariantList colors;
    for (const QColor &color : seriesColorList)
        colors.append(color);
    return colors;
}

void RealtimeChart::setSeriesColors(const QVariantList &colors)
{
    seriesColorList.clear();
    for (const QVariant &color : colors)
        seriesColorList.push_back(color.value<QColor>());
    emit appearanceChanged();
}

double RealtimeChart::visibleAmplitude() const
{
    double largestMagnitude = 0.0;
    for (const Sample &sample : samples) {
        for (double value : sample.values)
            largestMagnitude = std::max(largestMagnitude, std::abs(value));
    }
    return std::max(largestMagnitude * 1.15, minimumRange);
}

double RealtimeChart::niceStep(double range, int targetTickCount)
{
    const double roughStep = range / targetTickCount;
    const double magnitude = std::pow(10.0, std::floor(std::log10(roughStep)));
    for (double multiplier : {1.0, 2.0, 2.5, 5.0, 10.0}) {
        if (roughStep <= multiplier * magnitude)
            return multiplier * magnitude;
    }
    return 10.0 * magnitude;
}

void RealtimeChart::paint(QPainter *painter)
{
    const QRectF plotArea(labelAreaWidth, verticalPadding,
                          width() - labelAreaWidth, height() - 2.0 * verticalPadding);
    if (plotArea.width() <= 0.0 || plotArea.height() <= 0.0)
        return;

    const double amplitude = visibleAmplitude();
    const double step = niceStep(amplitude, 2);
    const double axisLimit = std::ceil(amplitude / step) * step;
    const auto valueToY = [&](double value) {
        return plotArea.center().y() - (value / axisLimit) * plotArea.height() / 2.0;
    };

    QFont labelFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    labelFont.setFamily(QStringLiteral("Fira Mono"));
    labelFont.setPixelSize(11);
    painter->setFont(labelFont);

    for (double tick = -axisLimit; tick <= axisLimit + step * 0.5; tick += step) {
        const double y = valueToY(tick);
        const bool isZeroLine = std::abs(tick) < step * 0.01;
        painter->setPen(QPen(gridColor, isZeroLine ? 1.5 : 1.0, isZeroLine ? Qt::SolidLine : Qt::DashLine));
        painter->drawLine(QPointF(plotArea.left(), y), QPointF(plotArea.right(), y));

        painter->setPen(labelColor);
        const QString label = QString::number(tick, 'g', 4) + (isZeroLine ? unit : QString());
        painter->drawText(QRectF(0.0, y - 8.0, labelAreaWidth - 8.0, 16.0), Qt::AlignRight | Qt::AlignVCenter, label);
    }

    if (samples.size() < 2)
        return;

    const double newestTime = samples.back().timeSeconds;
    const auto timeToX = [&](double timeSeconds) {
        return plotArea.right() - (newestTime - timeSeconds) / timeWindowSeconds * plotArea.width();
    };

    painter->save();
    painter->setClipRect(plotArea);
    const size_t seriesCount = samples.back().values.size();
    for (size_t seriesIndex = 0; seriesIndex < seriesCount; ++seriesIndex) {
        QPainterPath path;
        bool isFirstPoint = true;
        for (const Sample &sample : samples) {
            if (seriesIndex >= sample.values.size())
                continue;
            const QPointF point(timeToX(sample.timeSeconds), valueToY(sample.values[seriesIndex]));
            if (isFirstPoint) {
                path.moveTo(point);
                isFirstPoint = false;
            } else {
                path.lineTo(point);
            }
        }
        const QColor color = seriesIndex < seriesColorList.size() ? seriesColorList[seriesIndex] : labelColor;
        painter->setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(path);
    }
    painter->restore();
}
