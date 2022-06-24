#include "ElementListModel.h"
#include "Form.h"

ElementListModel::ElementListModel(QObject* parent): VariantListModel(parent)
{
    m_flatten = m_sorted = m_other = false;

    connect(this, &ElementListModel::elementUidChanged, this, &ElementListModel::rebuild);
    connect(this, &ElementListModel::flattenChanged, this, &ElementListModel::rebuild);
    connect(this, &ElementListModel::sortedChanged, this, &ElementListModel::rebuild);
    connect(this, &ElementListModel::searchFilterChanged, this, &ElementListModel::rebuild);
    // Do not rebuild on searchFilterIgnoreChanged, because this creates a render loop when the value changes.
    // connect(this, &ElementListModel::searchFilterIgnoreChanged, this, &ElementListModel::rebuild);
    connect(this, &ElementListModel::filterByFieldRecordUidChanged, this, &ElementListModel::rebuild);
    connect(this, &ElementListModel::filterByFieldChanged, this, &ElementListModel::rebuild);
    connect(this, &ElementListModel::filterByTagChanged, this, &ElementListModel::rebuild);
}

ElementListModel::~ElementListModel()
{
}

void ElementListModel::classBegin()
{
}

void ElementListModel::componentComplete()
{
    auto form = Form::parentForm(this);
    if (form)
    {
        m_elementManager = form->elementManager();
    }

    rebuild();
}

void ElementListModel::init(const QString& elementUid, bool flatten, bool sorted, bool useFilter, const QVariantMap& filter)
{
    m_elementUid = elementUid;
    m_flatten = flatten;
    m_sorted = sorted;
    m_filter = useFilter ? filter : QVariantMap();

    componentComplete();
}

void ElementListModel::rebuild()
{
    clear();

    if (!m_elementManager || m_elementUid.isEmpty())
    {
        return;
    }

    auto form = Form::parentForm(m_elementManager);
    auto elementList = QVariantList();
    Element* otherElement = nullptr;

    m_elementManager->enumChildren(m_elementUid, m_flatten, [&](Element* element)
    {
        if (element->hidden())
        {
            return;
        }

        if (element->other())
        {
            otherElement = element;
            return;
        }

        if (!m_filter.isEmpty() && !m_filter.value(element->uid()).toBool())
        {
            return;
        }

        if (!m_filterByField.isEmpty() && !m_filterByFieldRecordUid.isEmpty())
        {
            auto filterFieldValue = form->getFieldValue(m_filterByFieldRecordUid, m_filterByField).toMap();
            if (!filterFieldValue.value(element->uid()).toBool())
            {
                return;
            }
        }

        if (!m_filterByTag.isEmpty())
        {
            if (!form->evaluate(m_filterByTag, m_filterByFieldRecordUid, "", element->tag()).toBool())
            {
                return;
            }
        }

        if (!m_searchFilter.isEmpty())
        {
            auto elementName = form->getElementName(element->uid());
            if (!elementName.contains(m_searchFilter, Qt::CaseInsensitive))
            {
                if (!m_searchFilterIgnore.toMap().value(element->uid()).toBool())
                {
                    return;
                }
            }
        }

        elementList.append(QVariant::fromValue<Element*>(element));
    });

    if (m_sorted)
    {
        std::sort(elementList.begin(), elementList.end(),
            [&](QVariant& r1, QVariant& r2) -> bool
            {
                auto e1 = qvariant_cast<Element*>(r1);
                auto e2 = qvariant_cast<Element*>(r2);

                auto c1 = e1->hasChildren() ? 1 : 0;
                auto c2 = e2->hasChildren() ? 1 : 0;

                if (c1 != c2)
                {
                    return c1 > c2;
                }
                else
                {
                    auto en1 = form->getElementName(e1->uid());
                    auto en2 = form->getElementName(e2->uid());
                    return en1.compare(en2) < 0;
                }
            });
    }

    if (m_other && otherElement)
    {
        elementList.append(QVariant::fromValue<Element*>(otherElement));
    }

    appendList(elementList);
}
