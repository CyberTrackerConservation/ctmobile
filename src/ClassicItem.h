#pragma once
#include "pch.h"

namespace CyberTracker {

namespace ClassicEnums {
Q_NAMESPACE
QML_NAMED_ELEMENT(ClassicEnums)

enum class TextAlignmentType { TextAlignmentLeft = 0, TextAlignmentCenter, TextAlignmentRight };
Q_ENUM_NS(TextAlignmentType)

enum class DockType { DockNone = 0, DockTop, DockBottom, DockLeft, DockRight, DockFill, DockAction };
Q_ENUM_NS(DockType)

} // namespace ClassicEnums

using namespace ClassicEnums;

class ClassicItem : public QQuickItem
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY(DockType, dock)
    QML_WRITABLE_AUTO_PROPERTY(bool, canScale)
    QML_WRITABLE_AUTO_PROPERTY(bool, transparent)

    QML_WRITABLE_AUTO_PROPERTY(int, borderWidth)
    QML_WRITABLE_AUTO_PROPERTY(qreal, borderRadius)
    QML_WRITABLE_AUTO_PROPERTY(QColor, borderColor)
    QML_WRITABLE_AUTO_PROPERTY(int, borderLineWidth)
    QML_WRITABLE_AUTO_PROPERTY(QColor, borderLineColor)

    QML_READONLY_AUTO_PROPERTY(int, scaleX)
    QML_READONLY_AUTO_PROPERTY(int, scaleY)
    QML_READONLY_AUTO_PROPERTY(int, scaleBorder)
    QML_READONLY_AUTO_PROPERTY(int, scaleHitSize)

public:
    explicit ClassicItem(QQuickItem *parent = nullptr);

    void componentComplete() override;

    void scale(int x, int y, int border, int hitSize);
    QRect getClientBounds() const;

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void alignChildren();

private:
    bool m_complete = false;
};

} // namespace CyberTracker 
