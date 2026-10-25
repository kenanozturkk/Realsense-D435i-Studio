#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QQuickPaintedItem>
#include <QVariantList>

#include <deque>
#include <vector>

// Scrolling line chart for live sensor values. Each appended sample carries one
// value per series; samples older than the visible time window are discarded.
class RealtimeChart : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QVariantList seriesColors READ seriesColors WRITE setSeriesColors NOTIFY appearanceChanged)
    Q_PROPERTY(QColor gridColor MEMBER gridColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor labelColor MEMBER labelColor NOTIFY appearanceChanged)
    Q_PROPERTY(double timeWindowSeconds MEMBER timeWindowSeconds NOTIFY appearanceChanged)
    Q_PROPERTY(double minimumRange MEMBER minimumRange NOTIFY appearanceChanged)
    Q_PROPERTY(QString unit MEMBER unit NOTIFY appearanceChanged)

public:
    explicit RealtimeChart(QQuickItem *parent = nullptr);

    Q_INVOKABLE void appendSample(const QVariantList &values);
    Q_INVOKABLE void clear();

    QVariantList seriesColors() const;
    void setSeriesColors(const QVariantList &colors);

    void paint(QPainter *painter) override;

signals:
    void appearanceChanged();

private:
    struct Sample
    {
        double timeSeconds;
        std::vector<double> values;
    };

    double visibleAmplitude() const;
    static double niceStep(double range, int targetTickCount);

    std::deque<Sample> samples;
    std::vector<QColor> seriesColorList;
    QElapsedTimer elapsedClock;
    QColor gridColor {"#273248"};
    QColor labelColor {"#94A3B8"};
    QString unit;
    double timeWindowSeconds = 10.0;
    double minimumRange = 1.0;
};
