#pragma once
#include "pch.h"

class VariantListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY (int count READ count NOTIFY countChanged)

public:
    VariantListModel(QObject* parent = nullptr);
    ~VariantListModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void clear(void);
    int count(void) const;
    bool isEmpty(void) const;
    void append(const QVariant &item);
    void prepend(const QVariant &item);
    void insert(int idx, const QVariant& item);
    void appendList(const QVariantList& itemList);
    void prependList(const QVariantList& itemList);
    void replace(int pos, const QVariant& item);
    void change(int pos);
    void insertList(int idx, const QVariantList& itemList);
    void move(int idx, int pos);
    void remove(int idx);
    QVariant get(int idx) const;
    QVariantList list() const;

signals:
    void countChanged(int count);

protected:
    void updateCounter();

private:
    int m_count = 0;
    QVariantList m_items;
};
