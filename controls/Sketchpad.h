#pragma once
#include <App.h>

class Sketchpad : public QQuickPaintedItem
{
    Q_OBJECT
    QML_WRITABLE_AUTO_PROPERTY(QColor, penColor)
    QML_WRITABLE_AUTO_PROPERTY(int, penWidth)
    QML_WRITABLE_AUTO_PROPERTY(bool, baseLine)
    Q_PROPERTY(QVariant sketch READ sketch WRITE setSketch NOTIFY sketchChanged)

public:
    explicit Sketchpad(QQuickItem *parent = nullptr);

    Q_INVOKABLE void clear();

signals:
    void sketchChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paint(QPainter *painter) override;

private:
    void redraw();
    void addPoint(const QPoint& point);
    QVariant sketch();
    void setSketch(const QVariant& sketchVariant);

    bool m_penDown = false;
    QImage m_image;
    QPolygon m_lastLine;
    QList<QPolygon> m_lines;
};

