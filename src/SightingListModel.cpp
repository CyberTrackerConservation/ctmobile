#include "SightingListModel.h"
#include "App.h"
#include "TrackFile.h"

namespace CyberTracker {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SightingListModel

SightingListModel::SightingListModel(QObject* parent): QAbstractListModel(parent), QQmlParserStatus()
{
    m_active = true;
    m_count = 0;
    m_canSubmit = false;
    m_complete = false;
    m_projectUid = m_stateSpace = "*";
}

void SightingListModel::classBegin()
{
}

void SightingListModel::componentComplete()
{
    connect(this, &SightingListModel::activeChanged, this, &SightingListModel::rebuild);
    connect(this, &SightingListModel::projectUidChanged, this, &SightingListModel::rebuild);
    connect(this, &SightingListModel::stateSpaceChanged, this, &SightingListModel::rebuild);
    connect(this, &SightingListModel::rowsInserted, this, &SightingListModel::updateExtras);
    connect(this, &SightingListModel::rowsRemoved, this, &SightingListModel::updateExtras);
    connect(this, &SightingListModel::dataChanged, this, &SightingListModel::updateExtras);

    auto app = App::instance();
    connect(app, &App::sightingSaved, this, &SightingListModel::sightingChanged);
    connect(app, &App::sightingModified, this, &SightingListModel::sightingChanged);
    connect(app, &App::sightingFlagsChanged, this, &SightingListModel::sightingChanged);
    connect(app, &App::sightingsChanged, this, &SightingListModel::rebuild);

    connect(app->settings(), &Settings::languageCodeChanged, this, &SightingListModel::rebuild);

    connect(app, &App::sightingDeleted, this, [&](const QString& /*projectUid*/, const QString& sightingUid)
    {
        remove(indexOf(sightingUid));
    });

    m_form = Form::parentForm(this);
    if (m_form)
    {
        connect(m_form, &Form::languageCodeChanged, this, &SightingListModel::rebuild);

        connect(m_form, &Form::connectedChanged, this, [&]()
        {
            if (!m_form->connected())
            {
                clear();
            }
            else if (m_items.isEmpty())
            {
                rebuild();
            }
        });
    }

    m_complete = true;

    rebuild();
}

void SightingListModel::clear()
{
    if (m_items.isEmpty())
    {
        return;
    }

    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    m_items.clear();
    endRemoveRows();
}

int SightingListModel::indexOf(const QString& sightingUid) const
{
    auto matchList = match(index(0, 0), (int)SightingListModel::Roles::SightingUid, sightingUid, 1, Qt::MatchExactly);
    return matchList.isEmpty() ? -1 : matchList.constFirst().row();
}

void SightingListModel::remove(int pos)
{
    if (pos < 0 || pos >= m_items.count())
    {
        return;
    }

    beginRemoveRows(QModelIndex(), pos, pos);
    m_items.removeAt(pos);
    endRemoveRows();
}

void SightingListModel::replace(int pos, const Item& item)
{
    if (pos < 0 || pos >= m_items.count())
    {
        return;
    }

    m_items.replace(pos, item);
    auto idx = QAbstractListModel::index(pos);
    emit dataChanged(idx, idx);
}

void SightingListModel::append(const Item& item)
{
    int pos = m_items.count();
    beginInsertRows(QModelIndex(), pos, pos);
    m_items.append(item);
    endInsertRows();
}

int SightingListModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_items.count();
}

QVariant SightingListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_items.count())
    {
        return QVariant();
    }

    const auto item = m_items[index.row()];

    auto proxyModel = qobject_cast<SightingListProxyModel*>(parent());
    if (proxyModel)
    {
        switch ((Roles)role)
        {
        case Roles::Section:
            return proxyModel->getSection(item);

        case Roles::Expanded:
            return proxyModel->getExpanded(proxyModel->getSection(item));

        case Roles::Checked:
            return proxyModel->getChecked(item);

        default:
            break;
        }
    }

    return item.value((Roles)role);
}

QHash<int, QByteArray> SightingListModel::roleNames() const
{
    auto result = QHash<int, QByteArray>();

    auto metaEnum = QMetaEnum::fromType<Roles>();
    for (auto i = 0; i < metaEnum.keyCount(); i++)
    {
        auto roleId = i + Qt::UserRole + 1;
        auto roleName = QString(metaEnum.valueToKey(roleId));
        roleName.replace(0, 1, roleName[0].toLower());
        result.insert(roleId, roleName.toLatin1());
    }

    return result;
}

SightingListModel::Item SightingListModel::makeItem(const QString& projectUid, Sighting* sighting, Database::DataFlags flags) const
{
    auto app = App::instance();

    auto trackFileSummary = QString();
    if (!sighting->trackFile().isEmpty())
    {
        trackFileSummary = TrackFile(app->getMediaFilePath(sighting->trackFile())).summary();
    }

    auto archived = projectUid == ProjectManager::PROJECT_ARCHIVE;
    auto renderer = app->createRendererPtr(sighting->schema()->rendererName());
    auto sameForm = m_form && m_form->connected() && m_form->project()->uid() == projectUid;

    auto resolveIconUrl = [&](Sighting* sighting, const QString& icon) -> QString
    {
        return app->resolveArchiveUrl(sighting->getIconHash(icon));
    };

    if (archived)
    {
        flags = Database::DataFlag::DB_NO_FLAG;
    }

    auto location = sighting->locationPtr();

    return SightingListModel::Item
    {
        { Roles::Type, "sighting" },
        { Roles::ProjectUid, projectUid },
        { Roles::SightingUid, sighting->rootRecordUid() },
        { Roles::CreateTime, sighting->createTime() },
        { Roles::UpdateTime, sighting->updateTime() },
        { Roles::StateSpace, sighting->stateSpace() },
        { Roles::Username, sighting->username() },
        { Roles::Transcribing, flags.testFlag(Database::DataFlag::DB_TRANSCRIBING_FLAG) },
        { Roles::Exported, flags.testFlag(Database::DataFlag::DB_EXPORTED_FLAG) },
        { Roles::Uploaded, flags.testFlag(Database::DataFlag::DB_UPLOADED_FLAG) },
        { Roles::Submitted, flags.testFlag(Database::DataFlag::DB_SUBMITTED_FLAG) },
        { Roles::Readonly, flags.testFlag(Database::DataFlag::DB_READONLY_FLAG) },
        { Roles::Completed, flags.testFlag(Database::DataFlag::DB_COMPLETED_FLAG) },
        { Roles::Archived, archived },
        { Roles::CanEdit, archived ? false : sameForm && renderer->getSightingCanEdit(sighting, flags) },
        { Roles::CanDelete, archived ? true : sameForm && renderer->getSightingCanDelete(sighting, flags) },
        { Roles::CanSubmit, archived ? false : sameForm && m_form->canSubmitData() && flags.testFlag(Database::DataFlag::DB_COMPLETED_FLAG) && !flags.testFlag(Database::DataFlag::DB_SUBMITTED_FLAG) },
        { Roles::GroupText, renderer->getSightingGroupText(sighting) },
        { Roles::SummaryText, renderer->getSightingSummaryText(sighting) },
        { Roles::SummaryIcon, resolveIconUrl(sighting, renderer->getSightingSummaryIcon(sighting)) },
        { Roles::StatusIcon, archived || !sameForm ? ""/*"qrc:/icons/archive_outline.svg"*/ : resolveIconUrl(sighting, renderer->getSightingStatusIcon(sighting, flags)) },
        { Roles::TrackOnly, sighting->trackOnly() },
        { Roles::TrackFileName, sighting->trackFile() },
        { Roles::TrackFileSummary, trackFileSummary },
        { Roles::ProjectTitle, sighting->schema()->title() },
        { Roles::SearchText, sighting->searchText() },
        { Roles::Note, sighting->note() },
        { Roles::Location, location->isValid() ? QVariantList { location->x(), location->y() } : QVariant() },
    };
}

void SightingListModel::sightingChanged(const QString& projectUid, const QString& sightingUid)
{
    if (m_projectUid != "*" && m_projectUid != projectUid)
    {
        return;
    }

    auto database = App::instance()->database();
    if (!database->testData(projectUid, sightingUid))
    {
        auto matchList = match(index(0, 0), (int)SightingListModel::Roles::SightingUid, sightingUid, 1, Qt::MatchExactly);
        if (!matchList.isEmpty())
        {
            remove(matchList.constFirst().row());
        }
        return;
    }

    auto dbSighting = Database::DbData();
    database->loadData(projectUid, sightingUid, &dbSighting);

    if (m_stateSpace != "*" && m_stateSpace != dbSighting.stateSpace)
    {
        return;
    }

    // Only interested in sightings for now.
    if (!dbSighting.flags.testFlag(Database::DataFlag::DB_SIGHTING_FLAG))
    {
        return;
    }

    auto languageCode = m_form ? m_form->languageCode() : App::instance()->settings()->languageCode();
    auto sighting = std::unique_ptr<Sighting>(new Sighting(dbSighting.data, dbSighting.stateSpace, Schema::acquire(dbSighting.schemaHash), languageCode));

    auto item = makeItem(projectUid, sighting.get(), dbSighting.flags);
    auto index = indexOf(sightingUid);
    if (index != -1)
    {
        replace(index, item);
    }
    else
    {
        append(item);
    }

    Schema::release(dbSighting.schemaHash);

    updateExtras();
}

void SightingListModel::rebuild()
{
    if (!m_complete)
    {
        return;
    }

    clear();

    if (!m_active)
    {
        return;
    }

    auto app = App::instance();
    auto database = app->database();
    auto languageCode = m_form ? m_form->languageCode() : app->settings()->languageCode();

    database->enumData(m_projectUid, m_stateSpace, Database::DataFlag::DB_SIGHTING_FLAG,
        [&](const QString& projectUid, const QString& /*sightingUid*/, const Database::DbData& dbSighting)
        {
            if (dbSighting.schemaHash.isEmpty())
            {
                return;
            }

            auto sighting = std::unique_ptr<Sighting>(new Sighting(dbSighting.data, dbSighting.stateSpace, Schema::acquire(dbSighting.schemaHash), languageCode));
            m_items.append(makeItem(projectUid, sighting.get(), dbSighting.flags));
            Schema::release(dbSighting.schemaHash);
        });

    if (!m_items.isEmpty())
    {
        beginInsertRows(QModelIndex(), 0, m_items.count() - 1);
        endInsertRows();
    }
}

const SightingListModel::Item& SightingListModel::getItem(int index) const
{
    return m_items[index];
}

QVariant SightingListModel::get(int row) const
{
    auto result = QVariantMap();
    auto modelIndex = index(row, 0);

    auto roles = roleNames();
    for (auto roleIt = roles.constKeyValueBegin(); roleIt != roles.constKeyValueEnd(); roleIt++)
    {
        result[roleIt->second] = data(modelIndex, roleIt->first);
    }

    return result;
}

void SightingListModel::init(const QString& projectUid, const QString& stateSpace)
{
    m_projectUid = projectUid;
    m_stateSpace = stateSpace;
}

void SightingListModel::updateExtras()
{
    update_count(m_items.count());
    update_canSubmit(!match(index(0, 0), (int)Roles::CanSubmit, true, 1, Qt::MatchExactly).isEmpty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SightingFilter

SightingFilter::SightingFilter(QObject *parent): QObject{parent}
{
    m_sourceArchive = m_sourceProject = true;
    m_completedOnly = false;
    m_dateRangeDays = 0;
    m_projectUid = m_stateSpace = "*";

    connect(this, &SightingFilter::sourceArchiveChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::sourceProjectChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::completedOnlyChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::dateRangeDaysChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::projectUidChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::stateSpaceChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::dateFromChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::dateToChanged, this, &SightingFilter::changed);
    connect(this, &SightingFilter::searchTextChanged, this, &SightingFilter::changed);
    connect(App::instance()->settings(), &Settings::languageCodeChanged, this, &SightingFilter::changed);

    m_cacheDateFrom = m_cacheDateTo = -1;
}

bool SightingFilter::loading() const
{
    return m_loading;
}

QVariantMap SightingFilter::save() const
{
    return QVariantMap {
        { "sourceArchive", m_sourceArchive },
        { "sourceProject", m_sourceProject },
        { "completedOnly", m_completedOnly },
        { "dateRangeDays", m_dateRangeDays },
        { "projectUid", m_projectUid },
        { "stateSpace", m_stateSpace },
        { "dateFrom", m_dateFrom },
        { "dateTo", m_dateTo },
        { "searchText", m_searchText },
    };
}

void SightingFilter::load(const QVariantMap& data)
{
    m_loading = true;
    set_sourceArchive(data.value("sourceArchive", true).toBool());
    set_sourceProject(data.value("sourceProject", true).toBool());
    set_completedOnly(data.value("completedOnly", false).toBool());
    set_dateRangeDays(data.value("dateRangeDays", 0).toInt());
    set_projectUid(data.value("projectUid", "*").toString());
    set_stateSpace(data.value("stateSpace", "*").toString());
    set_dateFrom(data.value("dateFrom", "").toString());
    set_dateTo(data.value("dateTo", "").toString());
    set_searchText(data.value("searchText", "").toString());
    m_loading = false;

    emit changed();
}

void SightingFilter::matchInit()
{
    m_cacheDateFrom = m_cacheDateTo = -1;

    if (m_dateRangeDays != 0)
    {
        auto now = App::instance()->timeManager()->currentDateTime();
        m_cacheDateFrom = now.date().endOfDay(now.timeZone()).addDays(m_dateRangeDays).toMSecsSinceEpoch();
    }
    else
    {
        if (!m_dateFrom.isEmpty())
        {
            if (m_cacheDateFrom == -1)
            {
                m_cacheDateFrom = Utils::decodeTimestampMSecs(m_dateFrom);
            }
        }

        if (!m_dateTo.isEmpty())
        {
            if (m_cacheDateTo == -1)
            {
                m_cacheDateTo = Utils::decodeTimestampMSecs(m_dateTo);
            }
        }
    }

    m_wordsToInclude.clear();
    m_wordsToExclude.clear();

    if (!m_searchText.isEmpty())
    {
        Utils::parseSearchText(m_searchText, &m_wordsToInclude, &m_wordsToExclude);
    }
}

bool SightingFilter::matchTimestamp(const QString& timestamp) const
{
    auto ts = Utils::decodeTimestampMSecs(timestamp);

    if (ts < m_cacheDateFrom)
    {
        return false;
    }

    if (m_cacheDateTo != -1 && ts > m_cacheDateTo)
    {
        return false;
    }

    return true;
}

bool SightingFilter::matchTimestamp(qint64 t) const
{
    if (t < m_cacheDateFrom)
    {
        return false;
    }

    if (m_cacheDateTo != -1 && t > m_cacheDateTo)
    {
        return false;
    }

    return true;
}

bool SightingFilter::matchSearchText(const QString& text) const
{
    // Check for required words
    for (const auto& word: m_wordsToInclude)
    {
        if (!text.contains(word, Qt::CaseInsensitive))
        {
            return false;
        }
    }

    // Check for excluded words
    for (const auto& word: m_wordsToExclude)
    {
        if (text.contains(word, Qt::CaseInsensitive))
        {
            return false;
        }
    }

    return true;
}

bool SightingFilter::matchProject(const QString& projectUid) const
{
    auto projectIsArchive = projectUid == ProjectManager::PROJECT_ARCHIVE;

    if (!m_sourceArchive && projectIsArchive)
    {
        return false;
    }

    if (!m_sourceProject && !projectIsArchive)
    {
        return false;
    }

    return true;
}

bool SightingFilter::matchSighting(Sighting* sighting) const
{
    if (!matchTimestamp(sighting->createTime()))
    {
        return false;
    }

    if (!m_searchText.isEmpty())
    {
        return matchSearchText(sighting->searchText());
    }

    return true;
}

bool SightingFilter::matchSighting(const SightingListModel::Item& item) const
{
    auto archived = item[SightingListModel::Roles::Archived].toBool();
    if (!m_sourceArchive && archived)
    {
        return false;
    }

    if (!m_sourceProject && !archived)
    {
        return false;
    }

    if (m_completedOnly && !archived && !item[SightingListModel::Roles::Completed].toBool())
    {
        return false;
    }

    if (!matchTimestamp(item[SightingListModel::Roles::CreateTime].toString()))
    {
        return false;
    }

    if (!m_searchText.isEmpty())
    {
        return matchSearchText(item[SightingListModel::Roles::SearchText].toString());
    }

    return true;
}

bool SightingFilter::active() const
{
    return !m_sourceArchive || !m_sourceProject || m_completedOnly || m_dateRangeDays != 0 || !m_searchText.trimmed().isEmpty();
}

QString SightingFilter::summary() const
{
    auto source = QString();
    if (m_sourceArchive && m_sourceProject)
    {
        source = tr("All data");
    }
    else if (m_sourceArchive)
    {
        source = tr("Archive only");
    }
    else if (m_sourceProject)
    {
        source = QString(tr("%1 only")).arg(App::instance()->alias_Projects());
    }

    auto dateRange = QString();
    if (m_dateRangeDays == 0)
    {
        dateRange = tr("all days");
    }
    else if (m_dateRangeDays == -1)
    {
        dateRange = tr("today");
    }
    else
    {
        dateRange = QString(tr("last %1 days")).arg(QString::number(-m_dateRangeDays));
    }

    return QString("%1: %2%3").arg(source, dateRange, m_searchText.isEmpty() ? "" : ", \"" + m_searchText + "\"");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SightingListProxyModel

SightingListProxyModel::SightingListProxyModel(QObject* parent): QSortFilterProxyModel(parent), QQmlParserStatus()
{
    m_showNullGroup = false;
    m_active = false;
    m_count = 0;
    m_canSubmit = false;
    m_groupBy = GroupBy::None;
    m_sortReverse = false;
    m_expands = App::instance()->settings()->sightingSectionExpands();
    m_checkedCount = 0;
    m_filter = new SightingFilter(this);
    m_model = new SightingListModel(this);
    m_model->set_active(m_active);
}

void SightingListProxyModel::classBegin()
{
    m_model->classBegin();
}

void SightingListProxyModel::componentComplete()
{
    m_filter->matchInit();
    m_model->init(m_filter->projectUid(), m_filter->stateSpace());
    m_model->set_active(m_active);
    m_model->componentComplete();
    setSourceModel(m_model);
    sort(0);
    updateExtras();

    connect(this, &SightingListProxyModel::activeChanged, this, &SightingListProxyModel::rebuild);

    connect(m_filter, &SightingFilter::changed, this, &SightingListProxyModel::rebuild);
    connect(App::instance()->settings(), &Settings::sightingGroupByChanged, this, &SightingListProxyModel::rebuild);
    connect(App::instance()->settings(), &Settings::sightingSortReverseChanged, this, &SightingListProxyModel::rebuild);

    connect(this, &SightingListProxyModel::rowsInserted, this, &SightingListProxyModel::updateExtras);
    connect(this, &SightingListProxyModel::rowsRemoved, this, &SightingListProxyModel::updateExtras);
    connect(this, &SightingListProxyModel::dataChanged, this, &SightingListProxyModel::updateExtras);
}

QVariant SightingListProxyModel::get(int row) const
{
    return m_model->get(mapToSource(index(row, 0)).row());
}

QString SightingListProxyModel::getSection(const SightingListModel::Item& item) const
{
    switch (m_groupBy)
    {
    case GroupBy::None:
        return m_showNullGroup ? QString(tr("Data")) : QString();

    case GroupBy::ProjectTitle:
        return item.value(SightingListModel::Roles::ProjectTitle).toString();

    case GroupBy::GroupText:
        return item.value(SightingListModel::Roles::GroupText).toString();

    case GroupBy::Day:
        {
            auto dateTime = Utils::decodeTimestamp(item.value(SightingListModel::Roles::CreateTime).toString());
            return App::instance()->locale().toString(dateTime, "yyyy/MM/dd - dddd");
        }

    case GroupBy::Month:
        {
            auto dateTime = Utils::decodeTimestamp(item.value(SightingListModel::Roles::CreateTime).toString());
            return App::instance()->locale().toString(dateTime, "yyyy MMMM");
        }

    case GroupBy::Year:
        {
            auto dateTime = Utils::decodeTimestamp(item.value(SightingListModel::Roles::CreateTime).toString());
            return App::instance()->locale().toString(dateTime, "yyyy");
        }

    default:
        return QString();
    }
}

bool SightingListProxyModel::getExpanded(const QString& section) const
{
    return m_expands.value(section, true).toBool();
}

void SightingListProxyModel::setExpanded(const QString& section, bool expanded)
{
    if (section.isEmpty())
    {
        return;
    }

    if (expanded)
    {
        m_expands.remove(section);
    }
    else
    {
        m_expands.insert(section, expanded);
    }

    App::instance()->settings()->set_sightingSectionExpands(m_expands);

    auto matchList = match(index(0, 0), (int)SightingListModel::Roles::Section, section, -1, Qt::MatchExactly);
    if (!matchList.isEmpty())
    {
        emit dataChanged(matchList.constFirst(), matchList.constLast(), QVector<int> { (int)SightingListModel::Roles::Expanded });
    }
}

bool SightingListProxyModel::getChecked(const SightingListModel::Item& item) const
{
    return m_checked.contains(item[SightingListModel::Roles::SightingUid].toString());
}

void SightingListProxyModel::setChecked(int row, bool checked)
{
    auto index = this->index(row, 0);
    auto sightingUid = data(index, (int)SightingListModel::Roles::SightingUid).toString();
    if (!checked)
    {
        m_checked.remove(sightingUid);
    }
    else
    {
        auto projectUid = data(index, (int)SightingListModel::Roles::ProjectUid).toString();
        m_checked.insert(sightingUid, projectUid);
    }

    updateExtras();
}

bool SightingListProxyModel::getSectionChecked(const QString& section) const
{
    auto matchList = match(index(0, 0), (int)SightingListModel::Roles::Section, section, -1, Qt::MatchExactly);
    if (matchList.isEmpty())
    {
        return false;
    }

    for (auto matchIt = matchList.constBegin(); matchIt != matchList.constEnd(); matchIt++)
    {
        auto sightingUid = matchIt->data((int)SightingListModel::Roles::SightingUid).toString();
        if (!m_checked.contains(sightingUid))
        {
            return false;
        }
    }

    return true;
}

void SightingListProxyModel::setSectionChecked(const QString& section, bool checked)
{
    auto matchList = match(index(0, 0), (int)SightingListModel::Roles::Section, section, -1, Qt::MatchExactly);
    if (!matchList.isEmpty())
    {
        for (auto matchIt = matchList.constBegin(); matchIt != matchList.constEnd(); matchIt++)
        {
            setChecked(matchIt->row(), checked);
        }

        emit dataChanged(matchList.constFirst(), matchList.constLast(), QVector<int> { (int)SightingListModel::Roles::Checked });
    }
}

bool SightingListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& /*sourceParent*/) const
{
    return m_filter->matchSighting(m_model->getItem(sourceRow));
}

bool SightingListProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto item1 = m_model->getItem(left.row());
    auto item2 = m_model->getItem(right.row());

    if (m_groupBy == GroupBy::None)
    {
        auto compare = [&](SightingListModel::Roles role) -> int
        {
            return (int)item1[role].toBool() - (int)item2[role].toBool();
        };

        auto v0 = compare(SightingListModel::Roles::Archived);
        if (v0 != 0)
        {
            return v0 < 0;
        }

        auto v1 = compare(SightingListModel::Roles::Completed);
        if (v1 != 0)
        {
            return v1 < 0;
        }

        auto v2 = compare(SightingListModel::Roles::Submitted);
        if (v2 != 0)
        {
            return v2 < 0;
        }

        auto v3 = compare(SightingListModel::Roles::Exported);
        if (v3 != 0)
        {
            return v3 < 0;
        }
    }
    else
    {
        auto data1 = getSection(item1);
        auto data2 = getSection(item2);
        if (data1 != data2)
        {
            auto result = data1 < data2;
            return m_sortReverse ? !result : result;
        }
    }

    // Sort by time.
    auto result = item1[SightingListModel::Roles::UpdateTime].toString() < item2[SightingListModel::Roles::UpdateTime].toString();
    return m_sortReverse ? !result : result;
}

void SightingListProxyModel::rebuild()
{
    m_model->set_active(m_active);
    if (!m_active)
    {
        return;
    }

    if (m_filter->loading())
    {
        return;
    }

    beginResetModel();

    m_groupBy = (GroupBy)App::instance()->settings()->sightingGroupBy();
    m_sortReverse = App::instance()->settings()->sightingSortReverse();

    m_filter->matchInit();
    m_model->init(m_filter->projectUid(), m_filter->stateSpace());
    sort(0);

    endResetModel();

    update_count(rowCount());
}

void SightingListProxyModel::updateExtras()
{
    update_count(rowCount());
    update_checkedCount(m_checked.count());
    update_canSubmit(!match(index(0, 0), (int)SightingListModel::Roles::CanSubmit, true, 1, Qt::MatchExactly).isEmpty());
}

void SightingListProxyModel::clearChecked()
{
    m_checked.clear();

    if (rowCount() > 0)
    {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
    }

    updateExtras();
}

void SightingListProxyModel::deleteChecked()
{
    auto database = App::instance()->database();
    auto count = 0;
    auto checked = m_checked; // UpdateExtras modifies m_checked.

    for (auto itemIt = checked.constKeyValueBegin(); itemIt != checked.constKeyValueEnd(); itemIt++)
    {
        auto projectUid = itemIt->second.toString();
        auto sightingUid = itemIt->first;

        auto archived = itemIt->second == ProjectManager::PROJECT_ARCHIVE;
        auto readonly = database->getDataFlags(projectUid, sightingUid).testFlag(Database::DataFlag::DB_READONLY_FLAG);

        if (archived || !readonly)
        {
            App::instance()->deleteSighting(itemIt->second.toString(), itemIt->first, true);
            count++;
        }
    }

    if (count != checked.count())
    {
        emit App::instance()->showError(tr("Cannot delete read-only data"));
    }
    else
    {
        auto message = QString("%1 %2 %3").arg(QString::number(count), count == 1 ? tr("sighting") : tr("sightings"), tr("deleted"));
        emit App::instance()->showToast(message);
    }

    clearChecked();
}

QStringList SightingListProxyModel::getChecked() const
{
    return m_checked.keys();
}

} // namespace CyberTracker
