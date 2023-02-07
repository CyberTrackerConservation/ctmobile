#include "SightingListModel.h"
#include "App.h"
#include "TrackFile.h"

SightingListModel::SightingListModel(QObject* parent): VariantListModel(parent)
{
    m_canSubmit = false;
}

SightingListModel::~SightingListModel()
{
}

void SightingListModel::classBegin()
{
}

void SightingListModel::sightingChanged(const QString& sightingUid)
{
    auto index = m_sightingUids.indexOf(sightingUid);
    if (index != -1)
    {
        replace(index, sightingToVariant(sightingUid));
        computeCanSubmit();
    }
    else
    {
        rebuild();
    }
}

void SightingListModel::componentComplete()
{
    m_form = Form::parentForm(this);

    connect(m_form, &Form::sightingSaved, this, &SightingListModel::sightingChanged);
    connect(m_form, &Form::sightingModified, this, &SightingListModel::sightingChanged);
    connect(m_form, &Form::sightingFlagsChanged, this, &SightingListModel::rebuild);
    connect(m_form, &Form::sightingsChanged, this, &SightingListModel::rebuild);

    connect(m_form, &Form::sightingRemoved, this, [&](const QString& sightingUid)
    {
        auto index = m_sightingUids.indexOf(sightingUid);
        if (index != -1)
        {
            m_sightingUids.removeAt(index);
            remove(index);
            computeCanSubmit();
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

QVariant SightingListModel::sightingToVariant(Sighting* sighting, int flags) const
{
    auto trackFileSummary = QString();

    if (!sighting->trackFile().isEmpty())
    {
        auto app = App::instance();
        TrackFile trackFile(app->getMediaFilePath(sighting->trackFile()));
        auto trackFileInfo = trackFile.getInfo();
        trackFileSummary = QString("%1: %2, %3, %4 %5")
            .arg(tr("Track"), app->getDistanceText(trackFileInfo.distance), app->getTimeText(trackFileInfo.timeTotal))
            .arg(trackFileInfo.count).arg(tr("points"));
    }

    return QVariantMap
    {
        { "type", "sighting" },
        { "rootRecordUid", sighting->rootRecordUid() },
        { "createTime", sighting->createTime() },
        { "updateTime", sighting->updateTime() },
        { "records", sighting->recordManager()->save(true, nullptr) },
        { "stateSpace", m_form->stateSpace() },
        { "username", sighting->username() },
        { "exported", !!(flags & Sighting::DB_EXPORTED_FLAG) },
        { "uploaded", !!(flags & Sighting::DB_UPLOADED_FLAG) },
        { "submitted", !!(flags & Sighting::DB_SUBMITTED_FLAG) },
        { "readonly", !!(flags & Sighting::DB_READONLY_FLAG) },
        { "completed", !!(flags & Sighting::DB_COMPLETED_FLAG) },
        { "summaryText", m_form->getSightingSummaryText(sighting) },
        { "summaryIcon", m_form->getSightingSummaryIcon(sighting).toString() },
        { "statusIcon", m_form->getSightingStatusIcon(sighting, flags).toString() },
        { "trackFile", trackFileSummary },
    };
}

QVariant SightingListModel::sightingToVariant(const QString& sightingUid) const
{
    auto flags = App::instance()->database()->getSightingFlags(m_form->project()->uid(), sightingUid);
    auto sighting = m_form->createSightingPtr(sightingUid);
    return sightingToVariant(sighting.get(), flags);
}

void SightingListModel::rebuild()
{
    m_sightingUids.clear();
    clear();

    if (!m_form || !m_form->connected())
    {
        return;
    }

    auto database = App::instance()->database();
    auto projectUid = m_form->project()->uid();
    auto stateSpace = m_form->stateSpace();

    // Enumerate and build groups and sightings.
    auto groupsMap = QVariantMap();
    auto sightings = QVariantList();

    database->enumSightings(projectUid, stateSpace, Sighting::DB_SIGHTING_FLAG, [&](const QString& /*sightingUid*/, int flags, const QVariantMap& data, const QStringList& /*attachments*/)
    {
        auto sighting = m_form->createSightingPtr(data);

        // Build group.
        if (flags & Sighting::DB_EXPORTED_FLAG)
        {
            auto deviceId = sighting->deviceId();
            auto createTime = Utils::decodeTimestamp(sighting->createTime());

            if (!groupsMap.contains(deviceId))
            {
                groupsMap.insert(deviceId, QVariantMap
                    {
                        { "type", "group" },
                        { "deviceId", deviceId },
                        { "username", sighting->username() },
                        { "firstTime", sighting->createTime()},
                        { "lastTime", sighting->createTime() },
                        { "count", 0 },
                    });
            }

            auto group = groupsMap[deviceId].toMap();
            auto firstTime = Utils::decodeTimestamp(group["firstTime"].toString());
            auto lastTime = Utils::decodeTimestamp(group["lastTime"].toString());
            if (createTime < firstTime)
            {
                group["firstTime"] = sighting->createTime();
            }

            if (createTime > lastTime)
            {
                group["lastTime"] = sighting->createTime();
            }

            auto counter = group["counter"].toInt() + 1;
            group["counter"] = counter;
            group["summary"] = QString("%1: %2 %3").arg(sighting->username(), QString::number(counter), tr("observations"));

            groupsMap[deviceId] = group;

            return;
        }

        // Build sighting.
        sightings.append(sightingToVariant(sighting.get(), flags));
    });

    // Sort groups.
    auto groups = QVariantList();
    for (auto groupIt = groupsMap.constKeyValueBegin(); groupIt != groupsMap.constKeyValueEnd(); groupIt++)
    {
        groups.append(groupIt->second);
    }

    std::sort(groups.begin(), groups.end(), [](const QVariant& group1, const QVariant& group2) -> bool
    {
        auto u1 = group1.toMap()["username"].toString();
        auto u2 = group2.toMap()["username"].toString();

        return u1.compare(u2, Qt::CaseSensitive);
    });

    // Sort sightings.
    std::sort(sightings.begin(), sightings.end(), [](const QVariant& sighting1, const QVariant& sighting2) -> bool
    {
        const auto s1 = sighting1.toMap();
        const auto s2 = sighting2.toMap();

        auto compare = [&](const QString& flagName) -> int
        {
            return s1[flagName].toInt() - s2[flagName].toInt();
        };

        auto v = compare("completed");
        if (v != 0)
        {
            return v < 0;
        }

        v = compare("submitted");
        if (v != 0)
        {
            return v < 0;
        }

        v = compare("exported");
        if (v != 0)
        {
            return v < 0;
        }

        // Sort by time.
        auto t1 = Utils::decodeTimestampMSecs(s1["createTime"].toString());
        auto t2 = Utils::decodeTimestampMSecs(s2["createTime"].toString());
        return t1 < t2;
    });

    // Build sighting uids.
    m_sightingUids.reserve(sightings.count());
    for (auto sightingIt = sightings.constBegin(); sightingIt != sightings.constEnd(); sightingIt++)
    {
        m_sightingUids.append(sightingIt->toMap()["rootRecordUid"].toString());
    }

    // Build model.
    auto sightingList = QVariantList();
    sightingList.reserve(sightings.count() + groups.count());
    sightingList.append(sightings);
    sightingList.append(groups);

    appendList(sightingList);

    // Update canSubmit.
    computeCanSubmit();
}

void SightingListModel::computeCanSubmit()
{
    auto canSubmit = false;
    for (auto i = 0; i < count(); i++)
    {
        auto sighting = get(i).toMap();
        if (!sighting.value("submitted").toBool() && sighting.value("completed").toBool()) {
            canSubmit = true;
            break;
        }
    }

    update_canSubmit(canSubmit);
}
