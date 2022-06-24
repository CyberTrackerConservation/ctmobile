#pragma once
#include "pch.h"
#include "Form.h"

class FieldListModel: public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_READONLY_AUTO_PROPERTY (int, count)
    QML_WRITABLE_AUTO_PROPERTY (QString, recordUid)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, fieldUids)

public:
    enum Roles
    {
        RecordUidRole = Qt::UserRole + 1,
        FieldUidRole,
        SectionRole,
        VisibleRole,
        TitleVisibleRole,
        ValidRole,
        ConstraintMessageRole,
    };

    FieldListModel(QObject* parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    bool getVisible(int row) const;
    QVariant get(int row) const;
    int indexOfField(const QString& fieldUid) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    Form* m_form = nullptr;
    QMap<QString, bool> m_visibleMap;

    void onFieldValueChanged(const QString& recordUid, const QString& fieldUid, const QVariant& oldValue, const QVariant& newValue);
    void rebuild();
};

class FieldListProxyModel: public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_READONLY_AUTO_PROPERTY (int, count)
    QML_WRITABLE_AUTO_PROPERTY (QString, recordUid)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, filterFieldUids)
    QML_WRITABLE_AUTO_PROPERTY (QString, filterElementUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, filterTagName)
    QML_WRITABLE_AUTO_PROPERTY (QVariant, filterTagValue)

public:
    FieldListProxyModel(QObject* parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

    Q_INVOKABLE QVariant get(int row) const;
    Q_INVOKABLE int indexOfField(const QString& fieldUid) const;

private:
    FieldListModel* m_fieldListModel = nullptr;
    Form* m_form = nullptr;
    void rebuild();
};
