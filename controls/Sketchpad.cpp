#include "Sketchpad.h"

Sketchpad::Sketchpad(QQuickItem *parent): QQuickPaintedItem(parent)
{
    m_penColor = Qt::black;
    m_penWidth = 2;
    m_baseLine = true;

    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);

    connect(this, &Sketchpad::penColorChanged, this, &Sketchpad::redraw);
    connect(this, &Sketchpad::penWidthChanged, this, &Sketchpad::redraw);
    connect(this, &Sketchpad::baseLineChanged, this, &Sketchpad::redraw);
}

void Sketchpad::clear()
{
    m_image.fill(Qt::transparent);
    update();

    m_lastLine.clear();
    m_lines.clear();
}

void Sketchpad::redraw()
{
    if (m_image.isNull())
    {
        return;
    }

    m_image.fill(Qt::transparent);

    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (const auto& line: m_lines)
    {
        painter.drawPolyline(line);
    }

    update();
}

void Sketchpad::mousePressEvent(QMouseEvent *event)
{
    m_penDown = true;
    addPoint(event->pos());
}

void Sketchpad::mouseMoveEvent(QMouseEvent *event)
{
    if (m_penDown)
    {
        addPoint(event->pos());
    }
}

void Sketchpad::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_penDown)
    {
        addPoint(event->pos());
        m_lines << m_lastLine;
        m_lastLine.clear();
        m_penDown = false;

        emit sketchChanged();
    }
}

void Sketchpad::paint(QPainter *painter)
{
    int w = width();
    int h = height();

    if (m_image.size() != QSize(w, h))
    {
        m_image = QImage(w, h, QImage::Format_ARGB32);
        redraw();
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(fillColor());
    painter->drawRect(0, 0, w, h);
    painter->restore();

    if (m_baseLine)
    {
        painter->setPen(QPen(Qt::lightGray, 2, Qt::SolidLine, Qt::SquareCap));
        const auto y = h * 3/4;
        painter->drawLine(QPoint(0, y), QPoint(w, y));
    }

    painter->drawImage(boundingRect().topLeft(), m_image, boundingRect());
}

void Sketchpad::addPoint(const QPoint& point)
{
    if (m_lastLine.count() == 0)
    {
        m_lastLine << point;
        return;
    }

    if (m_lastLine.last() == point)
    {
        return;
    }

    auto prevPoint = m_lastLine.last();
    m_lastLine << point;

    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(prevPoint, point);

    int r = (m_penWidth / 2) + 2;
    update(QRect(prevPoint, point).normalized().adjusted(-r, -r, +r, +r));
}

QVariant Sketchpad::sketch()
{
    return Utils::sketchToVariant(m_lines);
}

void Sketchpad::setSketch(const QVariant& sketchVariant)
{
    clear();
    m_lines = Utils::variantToSketch(sketchVariant);
    redraw();
    
    emit sketchChanged();
}
