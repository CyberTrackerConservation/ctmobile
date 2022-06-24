#include "FieldListModel.h"
#include "App.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FieldListModel

FieldListModel::FieldListModel(QObject* parent): QAbstractListModel(parent), QQmlParserStatus()
{
    m_count = 0;
    connect(this, &FieldListModel::recordUidChanged, this, &FieldListModel::rebuild);
    connect(this, &FieldListModel::fieldUidsChanged, this, &FieldListModel::rebuild);
}

void FieldListModel::classBegin()
{
}

void FieldListModel::componentComplete()
{
    m_form = Form::parentForm(this);
    connect(m_form, &Form::fieldValueChanged, this, &FieldListModel::onFieldValueChanged);

    rebuild();
}

bool FieldListModel::getVisible(int row) const
{
    return m_visibleMap.value(m_fieldUids.at(row));
}

QVariant FieldListModel::get(int row) const
{
    auto result = QVariantMap();
    auto modelIndex = index(row, 0);

    auto roles = roleNames();
    for (auto it = roles.constKeyValueBegin(); it != roles.constKeyValueEnd(); it++)
    {
        result[it->second] = data(modelIndex, it->first);
    }

    return result;
}

int FieldListModel::indexOfField(const QString& fieldUid) const
{
    // Look for the field in the current model.
    auto index = m_fieldUids.indexOf(fieldUid);
    if (index != -1)
    {
        return index;
    }

    // Must be a child field, so go up the chain until we find it (or not).
    auto field = m_form->getField(fieldUid);
    for (auto parentField = field; parentField; parentField = parentField->parentField())
    {
        index = m_fieldUids.indexOf(parentField->uid());
        if (index != -1)
        {
            return index;
        }
    }

    return -1;
}

int FieldListModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_fieldUids.count();
}

QVariant FieldListModel::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() >= m_fieldUids.count())
    {
        return QVariant();
    }

    auto fieldUid = m_fieldUids[index.row()];

    if (role == RecordUidRole)
    {
        return m_recordUid;
    }
    else if (role == FieldUidRole)
    {
        return fieldUid;
    }
    else if (role == SectionRole)
    {
        return m_form->getElementName(m_form->fieldManager()->getField(fieldUid)->sectionElementUid());
    }
    else if (role == VisibleRole)
    {
        return m_form->getFieldVisible(m_recordUid, fieldUid);
    }
    else if (role == TitleVisibleRole)
    {
        return m_form->getFieldTitleVisible(fieldUid);
    }
    else if (role == ValidRole)
    {
        return m_form->getFieldValueValid(m_recordUid, fieldUid);
    }
    else if (role == ConstraintMessageRole)
    {
        auto field = m_form->getField(fieldUid);
        if (!field->constraint().isEmpty() && !field->constraintElementUid().isEmpty())
        {
            return m_form->getFieldConstraintMessage(m_recordUid, fieldUid);
        }
        else
        {
            return QString();
        }
    }

    return QVariant();
}

QHash<int, QByteArray> FieldListModel::roleNames() const
{
    auto roles = QHash<int, QByteArray>();
    roles[RecordUidRole] = "recordUid";
    roles[FieldUidRole] = "fieldUid";
    roles[SectionRole] = "section";
    roles[VisibleRole] = "visible";
    roles[TitleVisibleRole] = "titleVisible";
    roles[ValidRole] = "valid";
    roles[ConstraintMessageRole] = "constraintMessage";

    return roles;
}

void FieldListModel::onFieldValueChanged(const QString& recordUid, const QString& fieldUid, const QVariant& /*oldValue*/, const QVariant& /*newValue*/)
{
    if (recordUid != m_recordUid)
    {
        return;
    }

    auto listIndex = m_fieldUids.indexOf(fieldUid);
    if (listIndex == -1)
    {
        return;
    }

    // Check visible changed.
    for (auto i = 0; i < m_fieldUids.count(); i++)
    {
        auto fieldUid = m_fieldUids.at(i);
        auto fieldVisible = m_form->getFieldVisible(m_recordUid, fieldUid);

        if (fieldVisible != m_visibleMap.value(fieldUid))
        {
            m_visibleMap[fieldUid] = fieldVisible;
            emit dataChanged(index(i), index(i));
        }
    }
}

void FieldListModel::rebuild()
{
    if (!m_form)
    {
        return;
    }

    if (!m_form->connected() || m_recordUid.isEmpty() || m_fieldUids.isEmpty())
    {
        update_count(0);
        beginResetModel();
        endResetModel();
        return;
    }

    m_visibleMap.clear();
    for (auto fieldIt = m_fieldUids.constBegin(); fieldIt != m_fieldUids.constEnd(); fieldIt++)
    {
        auto fieldUid = *fieldIt;
        m_visibleMap[fieldUid] = m_form->getFieldVisible(m_recordUid, fieldUid);
    }

    update_count(m_fieldUids.count());

    if (!m_fieldUids.isEmpty())
    {
        emit dataChanged(index(0), index(m_fieldUids.count() - 1));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FieldListProxyModel

FieldListProxyModel::FieldListProxyModel(QObject* parent): QSortFilterProxyModel(parent), QQmlParserStatus()
{
    m_count = 0;
    m_fieldListModel = new FieldListModel(this);
    setSourceModel(m_fieldListModel);

    connect(this, &FieldListProxyModel::recordUidChanged, this, &FieldListProxyModel::rebuild);
    connect(this, &FieldListProxyModel::filterFieldUidsChanged, this, &FieldListProxyModel::rebuild);
    connect(this, &FieldListProxyModel::filterElementUidChanged, this, &FieldListProxyModel::rebuild);
    connect(this, &FieldListProxyModel::filterTagNameChanged, this, &FieldListProxyModel::rebuild);
    connect(this, &FieldListProxyModel::filterTagValueChanged, this, &FieldListProxyModel::rebuild);
    connect(m_fieldListModel, &FieldListModel::dataChanged, this, [&]() { update_count(rowCount()); });
}

void FieldListProxyModel::classBegin()
{
    m_fieldListModel->classBegin();
}

void FieldListProxyModel::componentComplete()
{
    m_form = Form::parentForm(this);
    m_fieldListModel->componentComplete();

    rebuild();
}

QVariant FieldListProxyModel::get(int row) const
{
    return m_fieldListModel->get(mapToSource(index(row, 0)).row());
}

int FieldListProxyModel::indexOfField(const QString& fieldUid) const
{
    return mapFromSource(m_fieldListModel->index(m_fieldListModel->indexOfField(fieldUid), 0)).row();
}

bool FieldListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& /*sourceParent*/) const
{
    return m_fieldListModel->getVisible(sourceRow);
}

void FieldListProxyModel::rebuild()
{
    if (!m_form)
    {
        return;
    }

    // Update the source if the recordUid changed.
    if (m_recordUid != m_fieldListModel->recordUid())
    {
        m_fieldListModel->set_fieldUids(QStringList());
        m_fieldListModel->set_recordUid(m_recordUid);
    }

    // Compute the field list.
    auto fieldUids = QStringList();

    if (!m_filterFieldUids.isEmpty())
    {
        // Field list is explictly specified.
        fieldUids = m_filterFieldUids;
    }
    else if (!m_filterElementUid.isEmpty())
    {
        // Field list comes from Element::fieldUids.
        fieldUids = m_form->getElementFieldList(m_filterElementUid);
    }
    else
    {
        // Field list comes from the fields in the record.
        auto recordFieldUid = m_form->sighting()->getRecord(m_recordUid)->recordFieldUid();
        fieldUids = m_form->fieldManager()->buildFieldList(recordFieldUid, m_filterTagName, m_filterTagValue);
    }

    m_fieldListModel->set_fieldUids(fieldUids);

    // Update count.
    update_count(rowCount());
}
