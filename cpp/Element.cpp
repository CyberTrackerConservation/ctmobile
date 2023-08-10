#include "Element.h"
#include "App.h"

namespace
{
    Form* form(const ElementManager* elementManager)
    {
        return qobject_cast<Form*>(elementManager->parent());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Element

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

QVariant Element::getTagValue(const QString& key, const QVariant& defaultValue)
{
    return m_tag.value(key, defaultValue);
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

void Element::toQml(QTextStream& stream, Element* element, int depth)
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
    Utils::writeQml(stream, depth + 1, "hidden", element->m_hidden, false);
    Utils::writeQml(stream, depth + 1, "other", element->m_other, false);

    for (int i = 0; i < element->elementCount(); i++)
    {
        toQml(stream, element->elementAt(i), depth + 1);
    }

    Utils::writeQml(stream, depth, "}");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ElementManager

ElementManager::ElementManager(QObject* parent): QObject(parent)
{
    m_rootElement = new Element();
}

ElementManager::~ElementManager()
{
    if (!m_cacheKey.isEmpty())
    {
        App::instance()->releaseQml(m_cacheKey);
    }
    else
    {
        delete m_rootElement;
    }
}

Element* ElementManager::getElement(const QString& elementUid) const
{
    return m_elementMap.value(elementUid, nullptr);
}

Element* ElementManager::rootElement() const
{
    return m_rootElement;
}

void ElementManager::appendElement(Element* parent, Element* element)
{
    if (parent == nullptr)
    {
        parent = rootElement();
    }

    parent->appendElement(element);
    populateMap(element);
}

void ElementManager::enumChildren(const QString& parentElementUid, bool leavesOnly, const std::function<void(Element*)>& callback) const
{
    auto parentElement = getElement(parentElementUid);
    QHash<Element*, bool> visitedHash;

    // No recurse.
    if (!leavesOnly)
    {
        // Enumerate child Elements.
        for (auto i = 0; i < parentElement->elementCount(); i++)
        {
            auto element = parentElement->elementAt(i);
            visitedHash.insert(element, true);
            callback(element);
        }

        // Enumerate child references.
        auto elementUids = parentElement->elementUids();
        for (auto it = elementUids.constBegin(); it != elementUids.constEnd(); it++)
        {
            auto element = getElement(*it);
            if (visitedHash.contains(element))
            {
                qDebug() << "Duplicate Element detected in references: " << *it;
                continue;
            }

            callback(element);
        }

        return;
    }

    // Leaves (Elements without children) only.
    std::function<void(Element*)> depthFirstSearch = [&](Element* element)
    {
        if (visitedHash.contains(element))
        {
            return;
        }

        visitedHash.insert(element, true);

        // If no children, then it is a leaf.
        if (!element->hasChildren())
        {
            callback(element);
            return;
        }

        // Enumerate child Elements.
        for (auto i = 0; i < element->elementCount(); i++)
        {
            depthFirstSearch(element->elementAt(i));
        }

        // Enumerate child references.
        if (!element->elementUids().isEmpty())
        {
            auto elementUids = element->elementUids();
            for (auto it = elementUids.constBegin(); it != elementUids.constEnd(); it++)
            {
                depthFirstSearch(getElement(*it));
            }
        }
    };

    depthFirstSearch(parentElement);
}

QStringList ElementManager::getLeafElementUids(const QString& parentElementUid, bool ignoreHidden) const
{
    auto result = QStringList();

    enumChildren(parentElementUid, true, [&](Element* element)
    {
        if (ignoreHidden && element->hidden())
        {
            return;
        }

        result.append(element->uid());
    });

    return result;
}

QList<Element*> ElementManager::getLeafElements(const QString& parentElementUid, bool ignoreHidden) const
{
    auto result = QList<Element*>();

    enumChildren(parentElementUid, true, [&](Element* element)
    {
        if (ignoreHidden && element->hidden())
        {
            return;
        }

        result.append(element);
    });

    return result;
}

QString ElementManager::getElementName(const QString& elementUid) const
{
    return getElementName(elementUid, form(this)->languageCode());
}

QString ElementManager::getElementName(const QString& elementUid, const QString& languageCode) const
{
    if (elementUid.isEmpty())
    {
        return "";
    }

    auto element = getElement(elementUid);
    if (!element)
    {
        return "";
    }

    auto name = element->name();
    if (!name.isEmpty())
    {
        // Simple lookup to allow for common names.
        auto map = QVariantMap
        {
            { "tr:None",  tr("None")  },
            { "tr:Yes",   tr("Yes")   },
            { "tr:No",    tr("No")    },
            { "tr:Other", tr("Other") },
            { "tr:Empty", tr("Empty") },
        };

        return map.contains(name) ? map[name].toString() : name;
    }

    auto nameMap = element->names();
    if (!nameMap.isEmpty())
    {
        auto result = nameMap.value(languageCode);
        if (result.isString())
        {
            // Language found!
            return result.toString();
        }

        result = nameMap.value(languageCode.left(2));
        if (result.isString())
        {
            return result.toString();
        }

        auto fallback = nameMap.contains("default") ? "default" : nameMap.keys().constFirst();
        return nameMap[fallback].toString();
    }

    return "";
}

QUrl ElementManager::getElementIcon(const QString& elementUid, bool walkBack) const
{
    auto projectManager = form(this)->projectManager();
    auto projectUid = form(this)->projectUid();

    for (auto element = getElement(elementUid); element && element != rootElement(); element = element->parentElement())
    {
        auto icon = element->icon();
        if (!icon.isEmpty())
        {
            if (icon.startsWith("qrc:/"))
            {
                return QUrl(icon);
            }

            auto filePath = projectManager->getFilePath(projectUid, icon);
            if (QFile::exists(filePath))
            {
                return QUrl::fromLocalFile(filePath);
            }
            else
            {
                qDebug() << "Icon not found: " << icon;
            }
        }

        if (!walkBack)
        {
            break;
        }
    }

    return QUrl();
}

void ElementManager::populateMap(Element* element)
{
    // Insert this element.
    m_elementMap.insert(element->uid(), element);

    // Insert the children.
    for (int i = 0; i < element->elementCount(); i++)
    {
        auto e = element->elementAt(i);
        e->set_listIndex(i);
        qFatalIf(m_elementMap.contains(e->uid()), "Duplicate element: " + e->uid());
        m_elementMap.insert(e->uid(), e);
        populateMap(e);
    }
}

void ElementManager::loadFromQmlFile(const QString& filePath)
{
    m_elementMap.clear();

    if (!m_cacheKey.isEmpty())
    {
        App::instance()->releaseQml(m_cacheKey);
        m_cacheKey.clear();
    }
    else
    {
        delete m_rootElement;
    }

    m_rootElement = nullptr;

    if (filePath.isEmpty())
    {
        return;
    }

    if (!QFile::exists(filePath))
    {
        qDebug() << "QML file not found: " << filePath;
        return;
    }

    m_rootElement = qobject_cast<Element*>(App::instance()->acquireQml(filePath, &m_cacheKey));
    qFatalIf(!m_rootElement, "Failed to load Elements QML: " + filePath);
    populateMap(m_rootElement);
}

void ElementManager::saveToQmlFile(const QString& filePath)
{
    QFile file(filePath);
    file.remove();
    qFatalIf(!file.open(QIODevice::WriteOnly | QIODevice::Text), "Failed to create file");

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(true);
    stream << "import CyberTracker 1.0\n\n";
    Element::toQml(stream, m_rootElement, 0);
}

void ElementManager::completeUpdate()
{
    m_elementMap.clear();
    populateMap(m_rootElement);
}

void ElementManager::sortByIndex(QStringList& elementUids) const
{
    std::sort(elementUids.begin(), elementUids.end(), [&](const QString& e1, const QString& e2) -> bool
    {
        auto element1 = getElement(e1);
        auto element2 = getElement(e2);

        return element1->listIndex() < element2->listIndex();
    });
}
