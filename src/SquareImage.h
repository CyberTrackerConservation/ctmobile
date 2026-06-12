#pragma once
#include "pch.h"

namespace CyberTracker {

class SquareImage : public QQuickPaintedItem
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY(int, size)
    QML_WRITABLE_AUTO_PROPERTY(int, angle)
    QML_WRITABLE_AUTO_PROPERTY(QString, source)
    QML_WRITABLE_AUTO_PROPERTY(bool, clickable)

public:
    explicit SquareImage(QQuickItem *parent = nullptr);

signals:
    void clicked();

protected:
    void paint(QPainter *painter) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    void render();
    QPixmap m_pixmap;
    QString m_renderSig;
};

} // namespace CyberTracker
