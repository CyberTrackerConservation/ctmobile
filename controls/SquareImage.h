#pragma once
#include <App.h>

class SquareImage : public QQuickPaintedItem
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY(int, size)
    QML_WRITABLE_AUTO_PROPERTY(QString, source)

public:
    explicit SquareImage(QQuickItem *parent = nullptr);

protected:
    void paint(QPainter *painter) override;

private:
    void render();
    QPixmap m_pixmap;
    QString m_renderSig;
};
