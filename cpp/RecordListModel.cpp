#include "RecordListModel.h"
#include "App.h"

RecordListModel::RecordListModel(QObject* parent): VariantListModel(parent)
{
    connect(this, &RecordListModel::recordUidChanged, this, &RecordListModel::rebuild);
    connect(this, &RecordListModel::fieldUidChanged, this, &RecordListModel::rebuild);
}

RecordListModel::~RecordListModel()
{
}

void RecordListModel::classBegin()
{
}

void RecordListModel::componentComplete()
{
    m_completed = true;

    // Pick up the default form.
    m_form = Form::parentForm(this);

    // Connect to form events.
    connect(m_form, &Form::sightingModified, this, &RecordListModel::sightingModified);
    connect(m_form, &Form::sightingSaved, this, &RecordListModel::sightingModified);
    connect(m_form, &Form::recordCreated, this, &RecordListModel::recordCreated);
    connect(m_form, &Form::recordDeleted, this, &RecordListModel::recordDeleted);
    connect(m_form, &Form::fieldValueChanged, this, &RecordListModel::fieldValueChanged);
    connect(m_form, &Form::recordChildValueChanged, this, &RecordListModel::recordChildValueChanged);

    rebuild();
}

QVariant RecordListModel::makeItem(const QString& recordUid)
{
    auto recordModel = QVariantList();
    auto recordManager = m_form->sighting()->recordManager();

    m_form->sighting()->buildRecordModel(&recordModel, [&](const FieldValue& fieldValue, int /*depth*/) -> bool
    {
        return fieldValue.recordUid() == recordUid;
    });

    auto item = QVariantMap
    {
        { "recordName", recordManager->getRecord(recordUid)->name() },
        { "recordIcon", recordManager->getRecord(recordUid)->icon() },
        { "recordUid", recordUid },
        { "recordData", recordModel },
        { "recordValid", recordManager->getRecord(recordUid)->isValid() }
    };

    return item;
}

void RecordListModel::sightingModified(const QString& /*sightingUid*/)
{
    rebuild();
}

void RecordListModel::recordCreated(const QString& recordUid)
{
    if (m_recordUid.isEmpty() || m_form->sighting()->getRecord(recordUid)->parentRecordUid() == m_recordUid)
    {
        append(makeItem(recordUid));
        m_recordUids.append(recordUid);
    }
}

void RecordListModel::recordDeleted(const QString& recordUid)
{
    auto index = m_recordUids.indexOf(recordUid);
    if (index == -1)
    {
        // Not a record we care about: probably a child record.
        return;
    }

    remove(index);
    m_recordUids.removeAt(index);

    // Relabel subsequent records.
    for (auto i = index; i < count(); i++)
    {
        auto itemValue = get(i).toMap();
        auto recordUid = itemValue["recordUid"].toString();
        auto recordName = m_form->sighting()->getRecord(recordUid)->name();

        if (itemValue["recordName"] != recordName)
        {
            itemValue["recordName"] = recordName;
            replace(i, itemValue);
        }
    }
}

void RecordListModel::fieldValueChanged(const QString& recordUid, const QString& /*fieldUid*/, const QVariant& /*oldValue*/, const QVariant& /*newValue*/)
{
    auto index = m_recordUids.indexOf(recordUid);

    // Index may be -1 in the create/delete record case.
    if (index != -1)
    {
        replace(index, makeItem(recordUid));
    }
}

void RecordListModel::recordChildValueChanged(const QString& recordUid, const QString& /*fieldUid*/)
{
    auto index = m_recordUids.indexOf(recordUid);

    // Index may be -1 in the create/delete record case.
    if (index != -1)
    {
        replace(index, makeItem(recordUid));
    }
}

void RecordListModel::rebuild()
{
    if (!m_completed)
    {
        return;
    }

    m_recordUids.clear();
    clear();

    // Check if parameters invalid or not connected.
    if (!m_form->connected())
    {
        return;
    }

    // Read the record list from the field or use the default list.
    if (!m_recordUid.isEmpty() && !m_fieldUid.isEmpty())
    {
        m_recordUids = m_form->getFieldValue(m_recordUid, m_fieldUid).toStringList();
    }
    else
    {
        m_recordUids = m_form->sighting()->recordManager()->recordUids();
    }

    // Create the model.
    auto list = QVariantList();
    list.reserve(m_recordUids.count());
    for (auto it = m_recordUids.constBegin(); it != m_recordUids.constEnd(); it++)
    {
        list.append(makeItem(*it));
    }

    appendList(list);
}
