#include "VariantListModel.h"

VariantListModel::VariantListModel(QObject* parent): QAbstractListModel(parent)
{
}

VariantListModel::~VariantListModel()
{
    clear();
}

int VariantListModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_items.count();
}

QVariant VariantListModel::data(const QModelIndex& index, int role) const
{
    auto ret = QVariant();
    auto idx = index.row();
    if (idx >= 0 && idx < count() && role == Qt::UserRole)
    {
        ret = m_items.value(idx);
    }

    return ret;
}

QHash<int, QByteArray> VariantListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::UserRole] = QByteArrayLiteral("qtVariant");

    return roles;
}

bool VariantListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    auto ret = false;
    auto idx = index.row();
    if (idx >= 0 && idx < count() && role == Qt::UserRole)
    {
        m_items.replace(idx, value);
        auto item = QAbstractListModel::index(idx);
        emit dataChanged(item, item);
        ret = true;
    }

    return ret;
}

int VariantListModel::count() const
{
    return m_items.size();
}

bool VariantListModel::isEmpty() const
{
    return m_items.isEmpty();
}

void VariantListModel::clear()
{
    if (!m_items.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, count() -1);
        m_items.clear();
        endRemoveRows();
        updateCounter();
    }
}

void VariantListModel::append(const QVariant& item)
{
    int pos = m_items.count();
    beginInsertRows(QModelIndex(), pos, pos);
    m_items.append(item);
    endInsertRows();
    updateCounter();
}

void VariantListModel::prepend(const QVariant& item)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(item);
    endInsertRows();
    updateCounter();
}

void VariantListModel::insert(int index, const QVariant& item)
{
    beginInsertRows (QModelIndex(), index, index);
    m_items.insert (index, item);
    endInsertRows();
    updateCounter();
}

void VariantListModel::replace(int pos, const QVariant& item)
{
    if (pos >= 0 && pos < count())
    {
        m_items.replace(pos, item);
        auto index = QAbstractListModel::index(pos);
        emit dataChanged(index, index);
    }
}

void VariantListModel::change(int pos)
{
    if (pos >= 0 && pos < count())
    {
        auto index = QAbstractListModel::index(pos);
        emit dataChanged(index, index);
    }
}

void VariantListModel::appendList(const QVariantList& itemList)
{
    if (!itemList.isEmpty())
    {
        int pos = m_items.count();
        beginInsertRows(QModelIndex(), pos, pos + itemList.count() -1);
        m_items.append(itemList);
        endInsertRows();
        updateCounter();
    }
}

void VariantListModel::prependList(const QVariantList& itemList)
{
    if (!itemList.isEmpty())
    {
        beginInsertRows(QModelIndex(), 0, itemList.count() -1);
        int offset = 0;
        for (auto item: itemList)
        {
            m_items.insert(offset, item);
            offset++;
        }
        endInsertRows();
        updateCounter();
    }
}

void VariantListModel::insertList(int index, const QVariantList& itemList)
{
    if (!itemList.isEmpty())
    {
        beginInsertRows(QModelIndex(), index, index + itemList.count() -1);
        auto offset = 0;
        for (auto item: itemList)
        {
            m_items.insert(index + offset, item);
            offset++;
        }
        endInsertRows();
        updateCounter();
    }
}

void VariantListModel::move(int index, int pos)
{
    if (index != pos)
    {
        beginRemoveRows(QModelIndex(), index, index);
        beginInsertRows(QModelIndex(), pos, pos);
        m_items.move(index, pos);
        endRemoveRows();
        endInsertRows();
    }
}

void VariantListModel::remove(int index)
{
    if (index >= 0 && index < m_items.size())
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_items.removeAt(index);
        endRemoveRows();
        updateCounter();
    }
}

QVariant VariantListModel::get(int index) const
{
    auto ret = QVariant();
    if (index >= 0 && index < m_items.size())
    {
        ret = m_items.value(index);
    }
    return ret;
}

QVariantList VariantListModel::list() const
{
    return m_items;
}

void VariantListModel::updateCounter()
{
    if (m_count != m_items.count())
    {
        m_count = m_items.count();
        emit countChanged (m_count);
    }
}
