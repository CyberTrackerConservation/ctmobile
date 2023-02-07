#include "SquareImage.h"

SquareImage::SquareImage(QQuickItem *parent): QQuickPaintedItem(parent)
{
    m_size = 0;
    m_source = "";

    connect(this, &SquareImage::sizeChanged, this, &SquareImage::render);
    connect(this, &SquareImage::sourceChanged, this, &SquareImage::render);
}

void SquareImage::render()
{
    if (m_size <= 0)
    {
        return;
    }

    auto renderSig = m_source + QString::number(m_size);
    if (m_renderSig == renderSig)
    {
        return;
    }

    setImplicitSize(m_size, m_size);

    auto filename = m_source;
    if (filename.isEmpty())
    {
        return;
    }

    if (filename.startsWith("qrc:"))
    {
        filename.remove(0, 3);
    }
    else if (filename.startsWith("file:"))
    {
        filename = QUrl(filename).toLocalFile();
    }

    Utils::renderSquarePixmap(&m_pixmap, filename, m_size, m_size);

    m_renderSig = renderSig;
    update();
}

void SquareImage::paint(QPainter *painter)
{
    render();
    painter->drawPixmap((width() - m_size) / 2, (height() - m_size) / 2, m_pixmap);
}
