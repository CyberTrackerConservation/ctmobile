#include "SketchPad.h"
#include "App.h"

namespace CyberTracker {

SketchPad::SketchPad(QQuickItem *parent): QQuickPaintedItem(parent)
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptTouchEvents(false);
    setFlag(QQuickItem::ItemHasContents, true);

    m_penColor = Qt::black;
    m_penWidth = App::instance()->scaleByFontSize(2);
    m_baseLine = true;

    connect(this, &SketchPad::penColorChanged, this, &SketchPad::redraw);
    connect(this, &SketchPad::penWidthChanged, this, &SketchPad::redraw);
    connect(this, &SketchPad::baseLineChanged, this, &SketchPad::redraw);
}

void SketchPad::clear()
{
    m_image.fill(Qt::transparent);
    update();

    m_lastLine.clear();
    m_lines.clear();
}

void SketchPad::redraw()
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

void SketchPad::mousePressEvent(QMouseEvent *event)
{
    m_penDown = true;
    addPoint(event->pos());
}

void SketchPad::mouseMoveEvent(QMouseEvent *event)
{
    if (m_penDown)
    {
        addPoint(event->pos());
    }
}

void SketchPad::mouseReleaseEvent(QMouseEvent *event)
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

void SketchPad::paint(QPainter *painter)
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

void SketchPad::addPoint(const QPoint& point)
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

QVariant SketchPad::sketch()
{
    return Utils::sketchToVariant(m_lines);
}

void SketchPad::setSketch(const QVariant& sketchVariant)
{
    clear();
    m_lines = Utils::variantToSketch(sketchVariant);
    redraw();
    
    emit sketchChanged();
}

} // namespace CyberTracker
