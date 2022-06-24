#include "ElementTreeModel.h"
#include "App.h"

ElementTreeModel::ElementTreeModel(QObject* parent): VariantListModel(parent)
{
    connect(this, &ElementTreeModel::elementUidChanged, this, &ElementTreeModel::rebuild);
}

ElementTreeModel::~ElementTreeModel()
{

}

void ElementTreeModel::classBegin()
{
}

void ElementTreeModel::componentComplete()
{
    auto form = Form::parentForm(this);
    if (form)
    {
        m_elementManager = form->elementManager();
    }

    rebuild();
}

void ElementTreeModel::rebuild()
{
    clear();
    if (!m_elementManager)
    {
        return;
    }

    auto rootElement = m_elementManager->getElement(m_elementUid);
    auto elementList = QVariantList();

    std::function<void(Element*, int)> buildNode = [&](Element* rootElement, int depth)
    {
        for (auto obj: rootElement->children())
        {
            auto element = qobject_cast<Element*>(obj);

            if (element->hidden())
            {
                continue;
            }

            auto item = QVariantMap();
            item["elementUid"] = element->uid();
            item["depth"] = depth;
            elementList.append(item);

            if (element->children().count() != 0)
            {
                buildNode(element, depth + 1);
            }
        }
    };

    buildNode(rootElement, 0);

    appendList(elementList);
}
