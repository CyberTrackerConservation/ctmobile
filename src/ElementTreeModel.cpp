#include "ElementTreeModel.h"
#include "App.h"

namespace CyberTracker {

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
        m_schema = form->schema();
    }

    rebuild();
}

void ElementTreeModel::rebuild()
{
    clear();
    if (!m_schema)
    {
        return;
    }

    auto rootElement = m_schema->getElement(m_elementUid);
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

} // namespace CyberTracker