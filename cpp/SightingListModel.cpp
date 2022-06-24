#include "SightingListModel.h"
#include "App.h"

SightingListModel::SightingListModel(QObject* parent): VariantListModel(parent)
{
}

SightingListModel::~SightingListModel()
{
}

void SightingListModel::classBegin()
{
}

void SightingListModel::componentComplete()
{
    m_form = Form::parentForm(this);

    connect(m_form, &Form::sightingSaved, this, [&](const QString& sightingUid)
    {
        auto value = sightingToVariant(sightingUid);

        auto index = m_sightingUids.indexOf(sightingUid);
        if (index != -1)
        {
            replace(index, value);
        }
        else
        {
            m_sightingUids.append(sightingUid);
            append(value);
        }
    });

    connect(m_form, &Form::sightingsChanged, this, &SightingListModel::rebuild);

    connect(m_form, &Form::sightingRemoved, this, [&](const QString& sightingUid)
    {
        auto index = m_sightingUids.indexOf(sightingUid);
        if (index != -1)
        {
            m_sightingUids.removeAt(index);
            remove(index);
        }
    });

    connect(m_form, &Form::sightingModified, this, [&](const QString& sightingUid)
    {
        auto index = m_sightingUids.indexOf(sightingUid);
        if (index != -1)
        {
            replace(index, sightingToVariant(sightingUid));
        }
    });

    connect(m_form, &Form::connectedChanged, this, [&]()
    {
        // Rebuild only if something changed.
        if (!m_form->connected() || m_sightingUids.isEmpty())
        {
            rebuild();
        }
    });

    rebuild();
}

QVariant SightingListModel::sightingToVariant(const QString& sightingUid)
{
    auto flags = App::instance()->database()->getSightingFlags(m_form->project()->uid(), sightingUid);
    auto sighting = m_form->createSightingPtr(sightingUid);

    return QVariantMap
    {
        { "rootRecordUid", sighting->rootRecordUid() },
        { "createTime", sighting->createTime() },
        { "updateTime", sighting->updateTime() },
        { "records", sighting->recordManager()->save(true, nullptr) },
        { "stateSpace", m_form->stateSpace() },
        { "exported", !!(flags & Sighting::DB_EXPORTED_FLAG) },
        { "uploaded", !!(flags & Sighting::DB_UPLOADED_FLAG) },
        { "submitted", !!(flags & Sighting::DB_SUBMITTED_FLAG) },
        { "readonly", !!(flags & Sighting::DB_READONLY_FLAG) },
        { "completed", !!(flags & Sighting::DB_COMPLETED_FLAG) },
        { "summary", sighting->summary() },
    };
}

void SightingListModel::rebuild()
{
    m_sightingUids.clear();
    clear();

    if (!m_form || !m_form->connected())
    {
        return;
    }

    auto app = App::instance();
    auto database = app->database();
    auto projectUid = m_form->project()->uid();
    auto stateSpace = m_form->stateSpace();

    database->getSightings(projectUid, stateSpace, Sighting::DB_SIGHTING_FLAG, &m_sightingUids);
    auto sightingList = QVariantList();
    sightingList.reserve(m_sightingUids.size());

    for (auto it = m_sightingUids.constBegin(); it != m_sightingUids.constEnd(); it++)
    {
        sightingList.append(sightingToVariant(*it));
    }

    appendList(sightingList);
}
