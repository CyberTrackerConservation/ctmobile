#include "Element.h"

namespace CyberTracker {

Element::Element(QObject* parent): QObject(parent)
{
    m_listIndex = -1;
    m_hidden = m_other = false;
}

Element::~Element()
{
    qDeleteAll(m_elements);
}

QQmlListProperty<Element> Element::elements()
{
    return QQmlListProperty<Element>(this, &m_elements);
}

QVariant Element::getTagValue(const QString& key, const QVariant& defaultValue) const
{
    return m_tag.value(key, defaultValue);
}

void Element::setTagValue(const QString& key, const QVariant& value)
{
    m_tag.insert(key, value);
}

bool Element::hasChildren() const
{
    return !m_elements.isEmpty() || !m_elementUids.isEmpty();
}

int Element::elementCount() const
{
    return m_elements.count();
}

Element* Element::elementAt(int index) const
{
    return m_elements.at(index);
}

Element* Element::parentElement() const
{
    return qobject_cast<Element*>(this->parent());
}

void Element::appendElement(Element* element)
{
    element->setParent(this);
    m_elements.append(element);
}

void Element::appendElementUid(const QString& elementUid)
{
    if (m_elementUids.indexOf(elementUid) == -1)
    {
        m_elementUids.append(elementUid);
    }
}

void Element::appendFieldUid(const QString& fieldUid)
{
    m_fieldUids.append(fieldUid);
}

void Element::removeFieldUid(const QString& fieldUid)
{
    m_fieldUids.removeOne(fieldUid);
}

void Element::addTag(const QString& key, const QVariant& value)
{
    m_tag[key] = value;
}

void Element::removeTag(const QString& key)
{
    m_tag.remove(key);
}

QString Element::safeExportUid()
{
    return !m_exportUid.isEmpty() ? m_exportUid : m_uid;
}

void Element::toQml(const QString& path, QTextStream& stream, Element* element, int depth)
{
    Utils::writeQml(stream, depth, "Element {");
    Utils::writeQml(stream, depth + 1, "uid", element->m_uid);
    Utils::writeQml(stream, depth + 1, "exportUid", element->m_exportUid);

    if (!element->m_name.isEmpty())
    {
        Utils::writeQml(stream, depth + 1, "name", element->m_name);
    }
    else if (!element->m_names.isEmpty())
    {
        Utils::writeQml(stream, depth + 1, "names", element->m_names);
    }

    Utils::writeQml(stream, depth + 1, "icon", element->m_icon);
    Utils::writeQml(stream, depth + 1, "audio", element->m_audio);
    Utils::writeQml(stream, depth + 1, "tag", element->m_tag);
    Utils::writeQml(stream, depth + 1, "fieldUids", element->m_fieldUids);
    Utils::writeQml(stream, depth + 1, "elementUids", element->m_elementUids);
    Utils::writeQml(stream, depth + 1, "aliases", element->m_aliases);
    Utils::writeQml(stream, depth + 1, "hidden", element->m_hidden, false);
    Utils::writeQml(stream, depth + 1, "other", element->m_other, false);

    for (int i = 0; i < element->elementCount(); i++)
    {
        toQml(path, stream, element->elementAt(i), depth + 1);
    }

    Utils::writeQml(stream, depth, "}");
}

} // namespace CyberTracker
