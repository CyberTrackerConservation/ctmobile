#pragma once
#include <App.h>

class Skyplot : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode)
    Q_PROPERTY(bool miniMode READ miniMode WRITE setMiniMode)

    QML_WRITABLE_AUTO_PROPERTY(int, padding)
    QML_WRITABLE_AUTO_PROPERTY(int, numberOfInnerCircles)
    QML_WRITABLE_AUTO_PROPERTY(int, numberOfStraightLines)
    QML_WRITABLE_AUTO_PROPERTY(int, numberOfDegreeTexts)

    QML_WRITABLE_AUTO_PROPERTY(QColor, shieldColor)
    QML_WRITABLE_AUTO_PROPERTY(QColor, shieldColorLines)
    QML_WRITABLE_AUTO_PROPERTY(float, shieldLineWidth)

    QML_WRITABLE_AUTO_PROPERTY(bool, shieldTextVisible)

    QML_WRITABLE_AUTO_PROPERTY(QColor, satIdColorForeground)
    QML_WRITABLE_AUTO_PROPERTY(QColor, satIdColorBackground)

    QML_WRITABLE_AUTO_PROPERTY(bool, satNumbersVisible)

    QML_WRITABLE_AUTO_PROPERTY(bool, compassVisible)
    QML_WRITABLE_AUTO_PROPERTY(float, compassOpacity)
    QML_WRITABLE_AUTO_PROPERTY(QColor, compassColorNorth)
    QML_WRITABLE_AUTO_PROPERTY(QColor, compassColorSouth)

    QML_WRITABLE_AUTO_PROPERTY(bool, legendVisible)
    QML_WRITABLE_AUTO_PROPERTY(int, legendDepth)
    QML_WRITABLE_AUTO_PROPERTY(int, legendSpace)

    QML_WRITABLE_AUTO_PROPERTY(bool, progressVisible)
    QML_WRITABLE_AUTO_PROPERTY(float, progressPercent)
    QML_WRITABLE_AUTO_PROPERTY(QColor, progressColor)

public:
    Skyplot(QQuickItem *parent = nullptr);
    ~Skyplot() override;

    void paint(QPainter* painter) override;

    bool active() const;
    void setActive(bool value);

    bool darkMode() const;
    void setDarkMode(bool value);

    bool miniMode() const;
    void setMiniMode(bool value);

private slots:
    void azimuthChanged(float azimuth);
    void satellitesChanged();

private:
    SatelliteManager* m_satelliteManager = nullptr;
    Compass* m_compass = nullptr;
    bool m_active = false;
    bool m_darkMode = false;
    bool m_miniMode = false;

    float m_shieldAngle = 0;
    QVector<qreal> m_shieldLineDashPattern = { 1 };
    QVariantAnimation m_shieldAnimation;

    float m_currentProgressPercent = 0;
    QVariantAnimation m_progressAnimation;

    QList<QColor> m_satStrenghColors;
    QList<int> m_satStrengthBars;

    float m_compassAngle = 0;

    QImage m_shieldCache;
    QImage m_numberCache1, m_numberCache2;
    int m_numberCacheCharW = 0;
    int m_numberCacheCharH = 0;
    int m_numberCacheCharT = 0;

    QList<Satellite> m_satellites;

    QColor getSatelliteColor(int strength);

    void renderShield(QPainter* target, int side, int offsetX, int offsetY, float angle);
    void renderNumber(QPainter* target, QPointF center, int fontHeight, int number, bool invert);
    void renderSatellites(QPainter* target, int side, int offsetX, int offsetY);
    void renderCompass(QPainter* target, int side, int offsetX, int offsetY);
    void renderLegend(QPainter* target, int width, int height, int offsetX, int offsetY);

    void setShieldAngle(float value);
};
