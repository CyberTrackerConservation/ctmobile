#pragma once
#include "pch.h"
#include "Form.h"

namespace CyberTracker {

class SightingListModel: public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY(bool, active)
    QML_READONLY_AUTO_PROPERTY(int, count)
    QML_READONLY_AUTO_PROPERTY(bool, canSubmit)
    QML_WRITABLE_AUTO_PROPERTY(QString, projectUid)
    QML_WRITABLE_AUTO_PROPERTY(QString, stateSpace)

public:
    enum class Roles
    {
        Type = Qt::UserRole + 1,
        ProjectUid,
        SightingUid,
        CreateTime,
        UpdateTime,
        StateSpace,
        Username,
        Transcribing,
        Exported,
        Uploaded,
        Submitted,
        Readonly,
        Completed,
        Archived,
        CanEdit,
        CanDelete,
        CanSubmit,
        GroupText,
        SummaryText,
        SummaryIcon,
        StatusIcon,
        TrackOnly,
        TrackFileName,
        TrackFileSummary,
        ProjectTitle,
        SearchText,
        Note,
        Location,
        Section,    // Proxy role
        Expanded,   // Proxy role
        Checked,    // Proxy role
    };

    Q_ENUM(Roles)

    typedef QMap<Roles, QVariant> Item;

    SightingListModel(QObject* parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const Item& getItem(int index) const;
    Q_INVOKABLE QVariant get(int row) const;

    void init(const QString& projectUid, const QString& stateSpace);

private slots:
    void sightingChanged(const QString& projectUid, const QString& sightingUid);
    void updateExtras();

private:
    bool m_complete = false;
    Form* m_form = nullptr;
    QVector<Item> m_items;

    void clear();
    int indexOf(const QString& sightingUid) const;
    void remove(int pos);
    void replace(int pos, const Item& item);
    void append(const Item& item);

    Item makeItem(const QString& projectUid, Sighting* sighting, Database::DataFlags flags) const;

    void rebuild();
};

class SightingFilter : public QObject
{
    Q_OBJECT
    QML_WRITABLE_AUTO_PROPERTY(bool, sourceArchive)
    QML_WRITABLE_AUTO_PROPERTY(bool, sourceProject)
    QML_WRITABLE_AUTO_PROPERTY(bool, completedOnly)
    QML_WRITABLE_AUTO_PROPERTY(int, dateRangeDays)
    QML_WRITABLE_AUTO_PROPERTY(QString, projectUid)
    QML_WRITABLE_AUTO_PROPERTY(QString, stateSpace)
    QML_WRITABLE_AUTO_PROPERTY(QString, dateFrom)
    QML_WRITABLE_AUTO_PROPERTY(QString, dateTo)
    QML_WRITABLE_AUTO_PROPERTY(QString, searchText)

    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)

public:
    explicit SightingFilter(QObject *parent = nullptr);

    bool loading() const;

    Q_INVOKABLE QVariantMap save() const;
    Q_INVOKABLE void load(const QVariantMap& data);

    void matchInit();

    bool matchTimestamp(const QString& timestamp) const;
    bool matchTimestamp(qint64 t) const;
    bool matchSearchText(const QString& text) const;
    bool matchProject(const QString& projectUid) const;
    bool matchSighting(Sighting* sighting) const;
    bool matchSighting(const SightingListModel::Item& item) const;

    bool active() const;
    QString summary() const;

signals:
    void changed();

private:
    bool m_loading = false;
    qint64 m_cacheDateFrom = -1;
    qint64 m_cacheDateTo = -1;
    QStringList m_wordsToInclude, m_wordsToExclude;
};

class SightingListProxyModel: public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

public:
    enum class GroupBy
    {
        None = 0,
        ProjectTitle,
        GroupText,
        Day,
        Month,
        Year
    };

    Q_ENUM(GroupBy)

    QML_WRITABLE_AUTO_PROPERTY(bool, active)
    QML_READONLY_AUTO_PROPERTY(int, count)
    QML_READONLY_AUTO_PROPERTY(bool, canSubmit)
    QML_READONLY_AUTO_PROPERTY(SightingListModel*, model)
    QML_READONLY_AUTO_PROPERTY(SightingFilter*, filter)
    QML_READONLY_AUTO_PROPERTY(GroupBy, groupBy)
    QML_WRITABLE_AUTO_PROPERTY(bool, showNullGroup)
    QML_READONLY_AUTO_PROPERTY(bool, sortReverse)
    QML_READONLY_AUTO_PROPERTY(QVariantMap, expands)
    QML_READONLY_AUTO_PROPERTY(QVariantMap, checked)
    QML_READONLY_AUTO_PROPERTY(int, checkedCount)

public:
    SightingListProxyModel(QObject* parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QString getSection(const SightingListModel::Item& item) const;
    Q_INVOKABLE bool getExpanded(const QString& section) const;
    Q_INVOKABLE void setExpanded(const QString& section, bool expanded);

    bool getChecked(const SightingListModel::Item& item) const;
    Q_INVOKABLE void setChecked(int row, bool checked);

    Q_INVOKABLE bool getSectionChecked(const QString& section) const;
    Q_INVOKABLE void setSectionChecked(const QString& section, bool checked);

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    Q_INVOKABLE QVariant get(int row) const;

    Q_INVOKABLE void clearChecked();
    Q_INVOKABLE void deleteChecked();
    Q_INVOKABLE QStringList getChecked() const;

signals:
    void changed();

private slots:
    void updateExtras();

private:
    void rebuild();
};

} // namespace CyberTracker
