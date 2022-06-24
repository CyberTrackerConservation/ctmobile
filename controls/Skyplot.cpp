#include "Skyplot.h"

//====================================================================================================================================

Skyplot::Skyplot(QQuickItem *parent): QQuickPaintedItem(parent)
{
    m_satelliteManager = App::instance()->satelliteManager();
    m_compass = App::instance()->compass();

    setFillColor(QColor("transparent"));
    setDarkMode(App::instance()->settings()->darkTheme());
    setMiniMode(m_miniMode);

    m_legendVisible = false;
    m_legendDepth = 16;
    m_legendSpace = 4;

    m_progressVisible = false;
    m_progressPercent = 0;
    m_currentProgressPercent = 0.0;

    m_shieldAnimation.setEasingCurve(QEasingCurve::Linear);

    connect(&m_shieldAnimation, &QVariantAnimation::valueChanged, [this](const QVariant& value)
    {
        setShieldAngle(value.toFloat());
    });

    connect(&m_progressAnimation, &QVariantAnimation::valueChanged, [this](const QVariant& value)
    {
        m_currentProgressPercent = value.toFloat();
        update();
    });

    connect(this, &Skyplot::progressPercentChanged, [this]()
    {
        m_progressAnimation.stop();

        m_progressAnimation.setStartValue(m_currentProgressPercent);
        m_progressAnimation.setEndValue(m_progressPercent);
        m_progressAnimation.start();
    });
}

Skyplot::~Skyplot()
{
    setActive(false);
}

void Skyplot::azimuthChanged(float azimuth)
{
    m_shieldAnimation.stop();

    float start = m_shieldAngle;
    float end = -(azimuth + App::instance()->settings()->magneticDeclination());

    auto difference = qFabs(end - start);
    if (difference > 180)
    {
        if (end > start)
        {
            start += 360;
        }
        else
        {
            end += 360;
        }
    }

    m_shieldAnimation.setStartValue(start);
    m_shieldAnimation.setEndValue(end);
    m_shieldAnimation.setDuration(m_compass->frequencyInMilliseconds());
    m_shieldAnimation.start();
}

void Skyplot::satellitesChanged()
{
    m_satellites = m_satelliteManager->snapSatellites();

    std::sort(m_satellites.begin(), m_satellites.end(),
        [&](Satellite& s1, Satellite& s2) -> bool
        {
            auto strength1 = static_cast<int>(s1.used) * 1000 + s1.strength;
            auto strength2 = static_cast<int>(s2.used) * 1000 + s2.strength;

            if (strength1 == strength2)
            {
                return s1.id < s2.id;
            }
            else
            {
                return strength1 < strength2;
            }
        });

    update();
}

bool Skyplot::active() const
{
    return m_active;
}

void Skyplot::setActive(bool value)
{
    if (value == m_active)
    {
        return;
    }

    m_active = value;

    if (m_active)
    {
        connect(m_satelliteManager, &SatelliteManager::satellitesChanged, this, &Skyplot::satellitesChanged);
        m_satelliteManager->start();

        connect(m_compass, &Compass::azimuthChanged, this, &Skyplot::azimuthChanged, Qt::QueuedConnection);
        m_compass->start();
    }
    else
    {
        m_shieldAnimation.stop();

        m_satelliteManager->stop();
        disconnect(m_satelliteManager, &SatelliteManager::satellitesChanged, 0, 0);

        m_compass->stop();
        disconnect(m_compass, &Compass::azimuthChanged, 0, 0);

        m_shieldCache = m_numberCache1 = m_numberCache2 = QImage();
    }

    update();
}

void Skyplot::setShieldAngle(float value)
{
    if (value == m_shieldAngle)
    {
        return;
    }

    m_shieldAngle = value;
    m_compassAngle = value + App::instance()->settings()->magneticDeclination();

    update();
}

void Skyplot::renderShield(QPainter* target, int side, int offsetX, int offsetY, float angle)
{
    auto cx = side / 2;
    auto cy = side / 2;
    auto iside = side - m_padding * 2;
    auto r = iside / 2;
    auto center = QPointF(cx, cy);

    if (m_shieldCache.width() != side || m_shieldCache.height() != side)
    {
        m_shieldCache = QImage(side, side, QImage::Format_ARGB32);

        QPainter painter(&m_shieldCache);

        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(0, 0, side, side, QBrush(QColor("transparent")));
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.setRenderHint(QPainter::RenderHint::Antialiasing);

        // Colors.
        painter.setBrush(m_shieldColor);
        painter.setPen(m_shieldColorLines);

        auto pen = painter.pen();
        pen.setWidthF(m_shieldLineWidth);

        // Outer circle.
        painter.setPen(pen);
        painter.drawEllipse(center, r, r);
        painter.setBrush(QColor("transparent"));

        // Lines.
        if (m_shieldLineDashPattern.count() == 1)
        {
            pen.setStyle(Qt::PenStyle::SolidLine);
        }
        else
        {
            pen.setDashPattern(m_shieldLineDashPattern);
        }

        painter.setPen(pen);
        for (auto i = 0; i < m_numberOfStraightLines; i++)
        {
            auto angleDeg = (i * 360) / m_numberOfDegreeTexts;
            auto angleRad = DEG2RAD(angleDeg);
            auto length = m_miniMode ? r : r * 0.92;
            auto tx = cx + length * qSin(angleRad);
            auto ty = cy - length * qCos(angleRad);
            painter.drawLine(cx, cy, tx, ty);
        }

        // First inner circle.
        auto innerDelta = m_miniMode ? 0 : iside * 0.04;
        if (!m_miniMode)
        {
            painter.drawEllipse(center, r - innerDelta, r - innerDelta);
        }

        // Inner circles.
        int d = (iside - innerDelta * 2) / (m_numberOfInnerCircles + 1);
        for (auto i = 0; i < m_numberOfInnerCircles; i++)
        {
            innerDelta += d / 2;

            if (i == m_numberOfInnerCircles - 1)
            {
                painter.setBrush(m_shieldColor);
            }

            painter.drawEllipse(center, r - innerDelta, r - innerDelta);
        }

        if (m_shieldTextVisible)
        {
            auto font = painter.font();
            font.setFamily("sans serif");
            font.setPointSize(iside * 0.03);
            painter.setFont(font);
            painter.setPen(m_shieldColorLines);

            // Outer circle text.
            for (auto i = 0; i < m_numberOfDegreeTexts; i++)
            {
                auto angleDeg = (i * 360) / m_numberOfDegreeTexts;
                auto angleRad = DEG2RAD(angleDeg);
                auto angleText = QString::number(angleDeg);

                auto tx = cx + r * 0.96 * qSin(angleRad);
                auto ty = cy - r * 0.96 * qCos(angleRad);

                painter.save();
                QPainterPath path;
                path.addText(0, 0, painter.font(), angleText);

                QRectF br(painter.fontMetrics().boundingRect(angleText));
                QPointF center(br.center());
                painter.translate(QPointF(tx, ty) - center);
                painter.translate(center);
                painter.rotate(angleDeg);
                painter.translate(-center);
                painter.fillPath(path, QBrush(m_shieldColorLines));
                painter.restore();
            }
        }
    }

    QTransform transform;
    cx += offsetX;
    cy += offsetY;
    transform.translate(cx, cy);
    transform.rotate(angle);
    transform.translate(-cx, -cy);

    target->setTransform(transform);
    target->setRenderHint(QPainter::SmoothPixmapTransform);
    target->drawImage(QPoint(offsetX, offsetY), m_shieldCache);
    target->resetTransform();

    if (m_progressVisible)
    {
        target->save();
        target->setPen(Qt::NoPen);
        target->setBrush(m_progressColor);
        target->drawPie(QRectF(offsetX + m_padding, offsetY + m_padding, side - m_padding * 2, side - m_padding * 2), -m_shieldAngle * 16 + 90*16, -(m_currentProgressPercent * 360 * 16) / 100);
        target->restore();
    }
}

void Skyplot::renderNumber(QPainter* target, QPointF center, int fontHeight, int number, bool invert)
{
    target->save();

    auto font = QFont("Monospace", fontHeight);
    auto fontMetrics = QFontMetrics(font);
    auto charW = m_numberCacheCharW;
    auto charH = m_numberCacheCharH;
    auto charT = m_numberCacheCharT;

    static const QString numbers = "0123456789";

    if (m_numberCache1.height() != fontMetrics.height())
    {
        charW = m_numberCacheCharW = fontMetrics.horizontalAdvance('0');

        int w = charW * 10;
        int h = fontMetrics.height();
        m_numberCache1 = QImage(w, h, QImage::Format_ARGB32);
        m_numberCache2 = QImage(w, h, QImage::Format_ARGB32);

        QPainter painter1(&m_numberCache1);
        QPainter painter2(&m_numberCache2);

        m_satIdColorBackground.setAlpha(0xff);
        painter1.setBrush(m_satIdColorBackground);
        painter1.fillRect(0, 0, w, h, painter1.brush());
        painter1.setPen(m_satIdColorForeground);
        painter1.setBrush(m_satIdColorForeground);
        painter1.setFont(font);
        painter1.drawText(QRect(0, 0, w, h), Qt::TextFlag::TextDontClip, numbers);

        m_satIdColorForeground.setAlpha(0xff);
        painter2.setBrush(m_satIdColorForeground);
        painter2.fillRect(0, 0, w, h, painter1.brush());
        painter2.setPen(m_satIdColorBackground);
        painter2.setBrush(m_satIdColorBackground);
        painter2.setFont(font);
        painter2.drawText(QRect(0, 0, w, h), Qt::TextFlag::TextDontClip, numbers);

        auto backColorRgba = static_cast<qint32>(m_satIdColorBackground.rgba());
        int tFound = -1;
        int bFound = -1;
        for (auto j = 0; j < h; j++)
        {
            if (tFound == -1)
            {
                auto sl = reinterpret_cast<qint32*>(m_numberCache1.scanLine(j));
                for (auto i = 0; i < w; i++)
                {
                    if (*sl != backColorRgba)
                    {
                        tFound = j;
                        break;
                    }
                    sl++;
                }
            }

            if (bFound == -1)
            {
                auto y = h - j - 1;
                auto sl = reinterpret_cast<qint32*>(m_numberCache1.scanLine(y));
                for (auto i = 0; i < w; i++)
                {
                    if (*sl != backColorRgba)
                    {
                        bFound = y;
                        break;
                    }
                    sl++;
                }
            }
        }

        charH = m_numberCacheCharH = bFound - tFound + 1;
        charT = m_numberCacheCharT = tFound;
    }

    auto colorBackground = invert ? m_satIdColorForeground : m_satIdColorBackground;
    auto colorForeground = invert ? m_satIdColorBackground : m_satIdColorForeground;

    auto text = QString::number(number);
    if (text.length() == 1)
    {
        text.insert(0, '0');
    }

    auto dstRect = QRectF(0, 0, text.count() * charW, charH);
    auto padding = charH / 8 + 1;
    dstRect.adjust(-padding, -padding, padding, padding);
    dstRect.moveCenter(center);
    dstRect.adjust(0, 0, 0, 1);

    auto pen = target->pen();
    pen.setWidthF(2);
    pen.setColor(invert ? colorBackground : colorForeground);
    target->setPen(pen);

    target->setBrush(colorBackground);

    target->drawRoundRect(dstRect, 25, 50);

    dstRect = QRectF(0, 0, text.count() * charW, charH);
    dstRect.moveCenter(center);
    dstRect.setWidth(charW);
    for (auto i = 0; i < text.count(); i++)
    {
        auto srcRect = QRectF(text[i].digitValue() * charW, charT, charW, charH);
        target->drawImage(dstRect, invert ? m_numberCache2 : m_numberCache1, srcRect);
        dstRect.moveLeft(dstRect.left() + charW);
    }

    target->restore();
}

void Skyplot::renderSatellites(QPainter* target, int side, int offsetX, int offsetY)
{
    auto iside = side - m_padding * 2;
    auto r = iside / 2;

    for (auto satellite: m_satellites)
    {
        auto radius = qMax((16 * side) / 280, 8);

        auto maxDistanceFromCenter = r * 0.92;
        auto distanceFromCenter = maxDistanceFromCenter - (satellite.elevation * maxDistanceFromCenter) / 90;
        auto angle = DEG2RAD(satellite.azimuth + m_shieldAngle);
        auto satelliteX = offsetX + side / 2 + distanceFromCenter * qSin(angle);
        auto satelliteY = offsetY + side / 2 - distanceFromCenter * qCos(angle);

        target->save();

        if (m_satNumbersVisible)
        {
            renderNumber(target, QPointF(satelliteX, satelliteY + radius * 1.0), radius / 1.5, satellite.id, satellite.used);
        }

        target->setPen(Qt::NoPen);
        target->setBrush(getSatelliteColor(satellite.strength));

        switch (satellite.type)
        {
        case SatelliteType::Navstar:
            target->drawEllipse(QPointF(satelliteX, satelliteY), radius / 2, radius / 2);
            break;

        case SatelliteType::Glonass:
            {
                auto triangle = QPolygonF();
                auto d3 = (radius * qSqrt(3)) * 0.27;
                triangle.append(QPointF(satelliteX, satelliteY - d3));
                triangle.append(QPointF(satelliteX - d3, satelliteY + d3));
                triangle.append(QPointF(satelliteX + d3, satelliteY + d3));
                target->drawConvexPolygon(triangle);
            }
            break;

        case SatelliteType::Galileo:
            {
                auto pentagon = QPolygonF();
                auto step = 2 * M_PI / 5;
                auto shift = (M_PI / 180.0) * -18;
                auto r = radius * 0.6;
                for (auto i = 0; i < 5; i++)
                {
                    auto currStep = i * step + shift;
                    pentagon.append(QPointF(satelliteX + r * qCos(currStep), satelliteY + r * qSin(currStep)));
                }

                target->drawConvexPolygon(pentagon);
            }
            break;

        case SatelliteType::Beidou:
            {
                auto star = QPolygonF();
                auto rot = M_PI / 2 * 3;
                auto x = satelliteX;
                auto y = satelliteY;
                auto r = radius * 0.5;
                auto spikes = 4;

                auto step = M_PI / spikes;
                auto innerRadius = r / 3;

                for (auto i = 0; i < spikes; i++)
                {
                    x = satelliteX + qCos(rot) * r;
                    y = satelliteY + qSin(rot) * r;
                    star.append(QPointF(x, y));
                    rot += step;
                    x = satelliteX + qCos(rot) * innerRadius;
                    y = satelliteY + qSin(rot) * innerRadius;
                    star.append(QPointF(x, y));
                    rot += step;
                }

                target->drawConvexPolygon(star);
            }

            break;

        case SatelliteType::Qzss:
            {
                auto rect = QRectF(0, 0, radius, radius);
                rect.moveCenter(QPointF(satelliteX, satelliteY));
                target->drawRect(rect);
            }
            break;

       case SatelliteType::Sbas:
            {
                auto square = QPolygonF();
                auto r = radius / 2;
                square.append(QPointF(satelliteX, satelliteY - r));
                square.append(QPointF(satelliteX + r, satelliteY));
                square.append(QPointF(satelliteX, satelliteY + r));
                square.append(QPointF(satelliteX - r, satelliteY));
                target->drawConvexPolygon(square);
            }
            break;

        default:
            target->drawEllipse(QPointF(satelliteX, satelliteY), radius / 3, radius / 2);
            break;
        };

        target->restore();
    }
}

void Skyplot::renderCompass(QPainter* target, int side, int offsetX, int offsetY)
{
    auto iside = side - m_padding * 2;
    auto r = iside / 2;

    target->save();
    target->setOpacity(m_compassOpacity);

    auto scale = iside / 200.0;

    QPoint hand[] =
    {
        QPoint(7 * scale, 0),
        QPoint(-7 * scale, 0),
        QPoint(0, -40 * scale)
    };

    target->setRenderHint(QPainter::Antialiasing);
    target->translate(offsetX + side / 2, offsetY + side / 2);

    auto pen = target->pen();
    pen.setWidth(2);

    // North
    pen.setColor(m_compassColorNorth);
    target->save();
    target->rotate(m_compassAngle);
    target->setPen(Qt::NoPen);
    target->setBrush(m_compassColorNorth);
    target->drawConvexPolygon(hand, 3);
    target->drawEllipse(QPointF(0, -r * 0.92), scale * 2, scale * 2);
    target->setPen(pen);
    target->drawLine(0, -39 * scale, 0, -r * 0.92 + scale * 2);
    target->restore();

    // South
    pen.setColor(m_compassColorSouth);
    target->save();
    target->rotate(m_compassAngle + 180);
    target->setPen(Qt::NoPen);
    target->setBrush(m_compassColorSouth);
    target->drawConvexPolygon(hand, 3);
    target->drawEllipse(QPointF(0, -r * 0.92), scale * 2, scale * 2);
    target->setPen(pen);
    target->drawLine(0, -39 * scale, 0, -r * 0.92 + scale * 2);
    target->restore();

    target->restore();
}

void Skyplot::renderLegend(QPainter* target, int width, int height, int offsetX, int offsetY)
{
    target->save();

    auto x = 0;
    auto maxFontHeight = height;

    for (auto i = 0; i < m_satStrengthBars.count(); i++)
    {
        auto segmentWidth = (m_satStrengthBars[i] * width) / 100;
        auto segmentColor = m_satStrenghColors[i];

        target->fillRect(offsetX + x, offsetY, segmentWidth, height, segmentColor);

        x += segmentWidth;
        maxFontHeight = qMin(maxFontHeight, segmentWidth);
    }

    auto padding = 4;

    auto font = QFont("Monospace", maxFontHeight / 2);
    target->setFont(font);
    //auto fontHeight = target->fontMetrics().ascent();
    target->setBrush(QColor("black"));
    target->setPen(QColor("black"));

    x = 0;
    auto number = 0;

    for (auto i = 0; i < m_satStrengthBars.count(); i++)
    {
        auto segmentWidth = (m_satStrengthBars[i] * width) / 100;
        target->drawText(QRectF(offsetX + x + padding, offsetY + padding, height - padding * 2, height - padding * 2), Qt::TextFlag::TextDontClip | Qt::AlignVCenter, QString::number(number));
        x += segmentWidth;
        number += m_satStrengthBars[i];
    }

    target->drawText(QRect(offsetX + width - padding, offsetY + padding, 0, height - padding * 2), Qt::TextFlag::TextDontClip | Qt::AlignVCenter | Qt::AlignRight, "100");

    auto signalStrength = m_satelliteManager->signalStrength();
    target->fillRect(offsetX + (signalStrength * width) / 100, offsetY, 4, height, QColor("black"));

    target->restore();
}

void Skyplot::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::RenderHint::Antialiasing);

    auto w = painter->window().width();
    auto h = painter->window().height();
    auto side = qMin(w, h);

    auto offsetX = 0;
    auto offsetY = 0;

    if (m_legendVisible)
    {
        side -= m_legendDepth;
        side -= m_legendSpace;
        offsetX = (w - side) / 2;
        offsetY = (h - (side + m_legendDepth + m_legendSpace)) / 2;
    }
    else
    {
        offsetX = (w - side) / 2;
        offsetY = (h - side) / 2;
    }

    renderShield(painter, side, offsetX, offsetY, m_shieldAngle);
    renderSatellites(painter, side, offsetX, offsetY);

    if (m_compassVisible)
    {
        renderCompass(painter, side, offsetX, offsetY);
    }

    if (m_legendVisible)
    {
        renderLegend(painter, side - m_padding * 2, m_legendDepth, offsetX + m_padding, offsetY + side + m_legendSpace);
    }
}

QColor Skyplot::getSatelliteColor(int strength)
{
    auto total = 0;
    for (auto i = 0; i < m_satStrengthBars.count(); i++)
    {
        total += m_satStrengthBars[i];
        if (strength < total)
        {
            return m_satStrenghColors[i];
        }
    }

    return m_satStrenghColors.last();
}

bool Skyplot::darkMode() const
{
    return m_darkMode;
}

void Skyplot::setDarkMode(bool value)
{
    m_darkMode = value;

    m_shieldColor = value ? "black" : "white";
    m_shieldColorLines = value ? "#C0C0D0" : "black";

    m_satIdColorForeground = value ? "white" : "black";
    m_satIdColorBackground = value ? "black" : "white";

    m_satStrenghColors = value ? QList<QColor> { "red", "orange", "yellow", "limegreen", "lightgreen" } :
                                 QList<QColor> { "crimson", "darkorange", "goldenrod", "forestGreen", "green" };
    m_satStrengthBars = value ? QList<int> { 10, 10, 10, 20, 50 } :
                                QList<int> { 10, 10, 10, 20, 50 };
    m_compassOpacity = value ? 0.8 : 0.5;
    m_compassColorNorth = "red";
    m_compassColorSouth = value ? "white" : "blue";

    m_progressColor = value ? QColor(128, 128, 128, 128) : QColor(64, 64, 64, 128);
}

bool Skyplot::miniMode() const
{
    return m_miniMode;
}

void Skyplot::setMiniMode(bool value)
{
    m_miniMode = value;

    m_padding = value ? 2 : 8;
    m_numberOfInnerCircles = value ? 2 : 3;
    m_numberOfStraightLines = value ? 4 : 12;
    m_numberOfDegreeTexts = value ? 4 : 12;
    m_shieldLineWidth = value ? 1 : 1.5;
    m_shieldTextVisible = !value;
    m_shieldLineDashPattern = value ? QVector<qreal> { 1 } : QVector<qreal> { 1, 3 };
    m_satNumbersVisible = value ? false : true;
    m_compassVisible = !value;
}
