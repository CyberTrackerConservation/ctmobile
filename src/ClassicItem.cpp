#include "ClassicItem.h"
#include <algorithm>

namespace CyberTracker {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CControlDocker for ClassicItem

class ClassicItemDocker
{
protected:
    ClassicItem* m_parent = nullptr;
    QRect m_rect;
    QList<ClassicItem*> m_dockList;
    int m_scaleX = 100;
    int m_scaleY = 100;
    int m_scaleBorder = 100;
    int m_scaleHitSize = 100;

protected:
    void enumChildren(ClassicItem* parent, const std::function<void(ClassicItem*)>& callback);
    bool insertBefore(ClassicItem* control1, ClassicItem* control2, DockType dock);
    void doPosition(ClassicItem* control, DockType dock);
    void doAlign(DockType dock);
    bool alignWork();
    void alignSubControls(ClassicItem* parent);
    void internalExecute(ClassicItem* parent);
    void internalResetScale(ClassicItem* control);
    void internalScale(ClassicItem* parent);

public:
    ClassicItemDocker();
    void execute(ClassicItem* parent, int scaleX, int scaleY, int scaleBorder, int scaleHitSize);
};

ClassicItemDocker::ClassicItemDocker()
{
}

void ClassicItemDocker::enumChildren(ClassicItem* parent, const std::function<void(ClassicItem*)>& callback)
{
    const auto& children = parent->childItems();
    for (auto childIt = children.constBegin(); childIt != children.constEnd(); childIt++)
    {
        auto classicChild = qobject_cast<ClassicItem*>(*childIt);
        if (classicChild && classicChild->isVisible())
        {
            callback(classicChild);
        }
    }
}

bool ClassicItemDocker::insertBefore(ClassicItem* control1, ClassicItem* control2, DockType dock)
{
    switch (dock)
    {
    case DockType::DockTop:
        return control1->y() < control2->y();

    case DockType::DockBottom:
        return control1->y() >= control2->y();

    case DockType::DockLeft:
        return control1->x() < control2->x();

    case DockType::DockRight:
        return control1->x() >= control2->x();

    default:
        ;
    }

    return false;
}

void ClassicItemDocker::doPosition(ClassicItem* control, DockType dock)
{
    auto newLeft = m_rect.left();
    auto newTop  = m_rect.top();

    auto newWidth = m_rect.right() - m_rect.left() + 1;
    if ((newWidth < 0) || ((dock == DockType::DockLeft) || (dock == DockType::DockRight) || (dock == DockType::DockNone)))
    {
        newWidth = control->width();
    }

    auto newHeight = m_rect.bottom() - m_rect.top() + 1;
    if ((newHeight < 0) || ((dock == DockType::DockTop) || (dock == DockType::DockBottom) || (dock == DockType::DockNone)))
    {
        newHeight = control->height();
    }

    switch (dock)
    {
    case DockType::DockTop:
        m_rect.setTop(m_rect.top() + newHeight);
        break;

    case DockType::DockBottom:
        m_rect.setBottom(m_rect.bottom() - newHeight);
        newTop = m_rect.bottom() + 1;
        break;

    case DockType::DockLeft:
        m_rect.setLeft(m_rect.left() + newWidth);
        break;

    case DockType::DockRight:
        m_rect.setRight(m_rect.right() - newWidth);
        newLeft = m_rect.right() + 1;
        break;

    case DockType::DockNone:
        newLeft = control->x();
        newTop = control->y();
        break;

    default:
        break;
    }

    control->setPosition(QPointF(newLeft, newTop));
    control->setSize(QSizeF(newWidth, newHeight));

    if ((control->width() != newWidth) || (control->height() != newHeight))
    {
        switch (dock)
        {
        case DockType::DockTop:
            m_rect.setTop(m_rect.top() - (newHeight - control->height()));
            break;

        case DockType::DockBottom:
            m_rect.setBottom(m_rect.bottom() + (newHeight - control->height()));
            break;

        case DockType::DockLeft:
            m_rect.setLeft(m_rect.left() - (newWidth - control->width()));
            break;

        case DockType::DockRight:
            m_rect.setRight(m_rect.right() + (newWidth - control->width()));
            m_rect.setBottom(m_rect.bottom() + (newHeight - control->height()));
            break;

        default:
            break;
        }
    }
}

void ClassicItemDocker::doAlign(DockType dock)
{
    m_dockList.clear();
    
    enumChildren(m_parent, [&](ClassicItem* classicChild)
    {
        if (classicChild->dock() == dock)
        {
            auto j = 0;
            while ((j < m_dockList.count()) && (!insertBefore(classicChild, m_dockList[j], dock)))
            {
                j++;
            }

            m_dockList.insert(j, classicChild);
        }
    });

    for (auto controlIt = m_dockList.begin(); controlIt != m_dockList.end(); controlIt++)
    {
        doPosition(*controlIt, dock);
    }
}

bool ClassicItemDocker::alignWork()
{
    auto result = false;
    auto left = 0;
    auto top = m_parent->y() + m_parent->height();

    enumChildren(m_parent, [&](ClassicItem* classicChild)
    {
        auto dock = classicChild->dock();
        if (dock == DockType::DockAction)
        {
            classicChild->setPosition(QPointF(left, top));
            classicChild->setSize(QSizeF(0, 0));
            left += classicChild->width() + 4;
        }
        else
        {
            result = result || (dock != DockType::DockNone);
        }
    });

    return result;
}

void ClassicItemDocker::alignSubControls(ClassicItem* parent)
{
    m_parent = parent;
    m_rect = parent->getClientBounds();

    if (alignWork())
    {
        m_rect.translate(-m_parent->x(), -m_parent->y());

        doAlign(DockType::DockTop);
        doAlign(DockType::DockBottom);
        doAlign(DockType::DockLeft);
        doAlign(DockType::DockRight);
        doAlign(DockType::DockFill);
        doAlign(DockType::DockNone);
    }
}

void ClassicItemDocker::internalResetScale(ClassicItem* control)
{
    control->set_canScale(true);

    enumChildren(control, [&](ClassicItem* classicChild)
    {
        internalResetScale(classicChild);
    });
}

void ClassicItemDocker::internalScale(ClassicItem* control)
{
    if (control->canScale())
    {
        control->scale(m_scaleX, m_scaleY, m_scaleBorder, m_scaleHitSize);
        control->set_canScale(false);
    }

    enumChildren(control, [&](ClassicItem* classicChild)
    {
        internalScale(classicChild);
    });
}

void ClassicItemDocker::internalExecute(ClassicItem* control)
{
    alignSubControls(control);

    enumChildren(control, [&](ClassicItem* classicChild)
    {
        internalExecute(classicChild);
    });
}

void ClassicItemDocker::execute(ClassicItem* parent, int scaleX, int scaleY, int scaleBorder, int scaleHitSize)
{
    m_scaleX = scaleX;
    m_scaleY = scaleY;
    m_scaleBorder = scaleBorder;
    m_scaleHitSize = scaleHitSize;

    internalScale(parent);
    internalExecute(parent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ClassicItem

ClassicItem::ClassicItem(QQuickItem *parent): QQuickItem(parent)
{
    m_dock = DockType::DockNone;

    m_canScale = false;
    m_transparent = false;

    m_borderWidth = 1;
    m_borderRadius = 0;
    m_borderColor = QColor("black");

    m_borderLineWidth = 1;
    m_borderLineColor = QColor("black");

    m_scaleX = m_scaleY = m_scaleBorder = m_scaleHitSize = 100;
}

void ClassicItem::componentComplete()
{
    QQuickItem::componentComplete();

    alignChildren();

    m_complete = true;
}

void ClassicItem::scale(int x, int y, int border, int hitSize)
{
    m_scaleX = x;
    m_scaleY = y;
    m_scaleBorder = border;
    m_scaleHitSize = hitSize;
}

QRect ClassicItem::getClientBounds() const
{
    auto result = QRect(x(), y(), width(), height());
    result.adjust(m_borderWidth, m_borderWidth, -m_borderWidth, -m_borderWidth);
    return result;
}

void ClassicItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (!m_complete)
    {
        return;
    }
    
    if (newGeometry.size() != oldGeometry.size())
    {
        alignChildren();
    }
}

void ClassicItem::alignChildren()
{
    if (!qobject_cast<ClassicItem*>(parent()))
    {
        auto docker = ClassicItemDocker();
        docker.execute(this, m_scaleX, m_scaleY, m_scaleBorder, m_scaleHitSize);
    }
}

} // namespace CyberTracker
