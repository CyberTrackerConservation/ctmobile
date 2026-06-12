#include "Schema.h"
#include "App.h"

namespace CyberTracker {

namespace {
QMap<QString, QPair<Schema*, int>> s_schemaCache;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Schema

Schema::Schema(QObject* parent): QObject(parent)
{
    m_version = "1.0";
    m_languageIndex = -1;
    m_rootElement = new Element(this);
    m_rootField = new RecordField(this);
}

Schema::~Schema()
{
}

QQmlListProperty<QObject> Schema::items()
{
    return QQmlListProperty<QObject>(this, &m_items);
}

Element* Schema::rootElement() const
{
    return m_rootElement;
}

RecordField* Schema::rootField() const
{
    return m_rootField;
}

Schema* Schema::acquire(const QString& hash)
{
    if (!s_schemaCache.contains(hash))
    {
        auto qmlEngine = App::instance()->qmlEngine();
        qmlEngine->clearComponentCache();
        auto schema = qobject_cast<Schema*>(Utils::loadQmlFromFile(qmlEngine, App::instance()->getArchiveFilePath(hash)));
        qFatalIf(schema == nullptr, "Failed to load schema: " + hash);

        // Connect and validate the rootElement and rootField.
        auto items = schema->m_items;
        for (auto itemIt = items.constBegin(); itemIt != items.constEnd(); itemIt++)
        {
            auto item = *itemIt;

            if (qobject_cast<Element*>(item))
            {
                schema->m_rootElement = qobject_cast<Element*>(item);
            }
            else if (qobject_cast<RecordField*>(item))
            {
                schema->m_rootField = qobject_cast<RecordField*>(item);
            }
        }

        // Insert schema into cache.
        s_schemaCache.insert(hash, qMakePair(schema, 0));
    }

    // Bump the reference count.
    s_schemaCache[hash].second++;

    // Return the schema.
    return s_schemaCache[hash].first;
}

void Schema::release(const QString hash)
{
    auto value = s_schemaCache[hash];
    qFatalIf(value.second == 0, "Bad ref count on schema file cache");

    s_schemaCache[hash].second--;

    if (s_schemaCache.count() > 8)
    {
        garbageCollectCache();
    }
}

QStringList Schema::cached()
{
    auto result = QStringList();
    for (auto it = s_schemaCache.constKeyValueBegin(); it != s_schemaCache.constKeyValueEnd(); it++)
    {
        if (it->second.second > 0)
        {
            result.append(it->first);
        }
    }

    return result;
}

void Schema::garbageCollectCache()
{
    auto removeList = QStringList();
    for (auto it = s_schemaCache.constKeyValueBegin(); it != s_schemaCache.constKeyValueEnd(); it++)
    {
        if (it->second.second > 0)
        {
            continue;
        }

        delete it->second.first;
        removeList.append(it->first);
    }

    for (auto it = removeList.constBegin(); it != removeList.constEnd(); it++)
    {
        s_schemaCache.remove(*it);
    }
}

void Schema::saveToQmlFile(const QString& filePath) const
{
    QByteArray buffer;
    QTextStream stream(&buffer);
    stream.setEncoding(QStringConverter::Encoding::Utf8);
    stream.setGenerateByteOrderMark(true);
    stream << "import CyberTracker 1.0\n\n";

    Utils::writeQml(stream, 0, "Schema {");

    Utils::writeQml(stream, 1, "version", m_version);
    Utils::writeQml(stream, 1, "title", m_title);
    Utils::writeQml(stream, 1, "subtitle", m_subtitle);
    Utils::writeQml(stream, 1, "icon", m_icon);
    Utils::writeQml(stream, 1, "languages", m_languages);
    Utils::writeQml(stream, 1, "languageIndex", m_languageIndex, -1);
    Utils::writeQml(stream, 1, "settings", m_settings);
    Utils::writeQml(stream, 1, "rendererName", m_rendererName);

    if (m_rootField)
    {
        m_rootField->toQml(stream, 1);
    }

    if (m_rootElement)
    {
        Element::toQml(QFileInfo(filePath).path(), stream, m_rootElement, 1);
    }

    Utils::writeQml(stream, 0, "}");

    stream.flush();

    QFile file(filePath);
    file.remove();
    qFatalIf(!file.open(QIODevice::WriteOnly | QIODevice::Text), "Failed to create file");
    file.write(buffer.data(), buffer.length());
}

QVariant Schema::getSetting(const QString& name, const QVariant& defaultValue) const
{
    return m_settings.value(name, defaultValue);
}

void Schema::setSetting(const QString& name, const QVariant& value)
{
    m_settings.remove(name);
    m_settings.insert(name, value);
}

void Schema::setProjectInfo(const QString& title, const QString& subtitle, const QString& icon)
{
    m_title = title;
    m_subtitle = subtitle;
    m_icon = icon;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Element.

void Schema::appendElement(Element* element)
{
    m_rootElement->appendElement(element);
}

Element* Schema::getElement(const QString& elementUid) const
{
    if (elementUid.isEmpty())
    {
        return m_rootElement;
    }

    if (!m_elementMap.contains(elementUid))
    {
        m_elementMap.clear();
        populateElementMap(m_rootElement);
    }

    return m_elementMap.value(elementUid, nullptr);
}

void Schema::sortByIndex(QStringList& elementUids) const
{
    std::sort(elementUids.begin(), elementUids.end(), [&](const QString& e1, const QString& e2) -> bool
    {
        auto element1 = getElement(e1);
        auto element2 = getElement(e2);

        return element1->listIndex() < element2->listIndex();
    });
}

void Schema::enumElementChildren(const QString& parentElementUid, bool leavesOnly, const std::function<void(Element*)>& callback) const
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

QStringList Schema::getLeafElementUids(const QString& parentElementUid, bool ignoreHidden) const
{
    auto result = QStringList();

    enumElementChildren(parentElementUid, true, [&](Element* element)
    {
        if (ignoreHidden && element->hidden())
        {
            return;
        }

        result.append(element->uid());
    });

    return result;
}

QList<Element*> Schema::getLeafElements(const QString& parentElementUid, bool ignoreHidden) const
{
    auto result = QList<Element*>();

    enumElementChildren(parentElementUid, true, [&](Element* element)
    {
        if (ignoreHidden && element->hidden())
        {
            return;
        }

        result.append(element);
    });

    return result;
}

QString Schema::getElementName(const QString& elementUid, const QString& languageCode) const
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
        return translater(name);
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
            return translater(result.toString());
        }

        auto fallback = nameMap.contains("default") ? "default" : nameMap.keys().constFirst();
        return translater(nameMap[fallback].toString());
    }

    auto exportUid = element->exportUid();
    if (!exportUid.isEmpty())
    {
        return exportUid;
    }

    return "";
}

QString Schema::getElementIcon(const QString& elementUid, bool walkBack) const
{
    for (auto element = getElement(elementUid); element && element != m_rootElement; element = element->parentElement())
    {
        auto icon = element->icon();
        if (!icon.isEmpty())
        {
            return icon;
        }

        if (!walkBack)
        {
            break;
        }
    }

    return QString();
}

void Schema::populateElementMap(Element* element) const
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
        populateElementMap(e);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Field.


void Schema::appendField(BaseField* field)
{
    m_rootField->appendField(field);
}

BaseField* Schema::getField(const QString& fieldUid) const
{
    if (fieldUid.isEmpty())
    {
        return m_rootField;
    }

    if (!m_fieldMap.contains(fieldUid))
    {
        m_fieldMap.clear();
        populateFieldMap(m_rootField);
    }

    return m_fieldMap.value(fieldUid, nullptr);
}

QString Schema::getFieldName(const QString& fieldUid, const QString& languageCode) const
{
    auto field = getField(fieldUid);
    if (!field)
    {
        return "";
    }

    auto result = QString();

    if (!field->nameElementUid().isEmpty())
    {
        result = getElementName(field->nameElementUid(), languageCode);
    }
    else if (!field->name().isEmpty())
    {
        result = translater(field->name());
    }

    return result.isEmpty() ? field->uid() : result;
}

QString Schema::getFieldIcon(const QString& fieldUid) const
{
    auto field = getField(fieldUid);
    if (!field)
    {
        return QString();
    }

    auto nameElementUid = field->nameElementUid();
    return nameElementUid.isEmpty() ? QString() : getElementIcon(nameElementUid, false);
}

QString Schema::getFieldType(const QString& fieldUid) const
{
    constexpr char PREFIX[] = "CyberTracker::";
    constexpr int PREFIX_LEN = sizeof(PREFIX) - 1;

    auto result = QString(getField(fieldUid)->metaObject()->className());
    if (result.startsWith(PREFIX))
    {
        result = result.mid(PREFIX_LEN);
    }

    return result;
}

QString Schema::translater(const QString& trName) const
{
    auto knowns = QMap<QString, QString> {
        { TR_NONE,           tr("None")  },
        { TR_YES,            tr("Yes")   },
        { TR_NO,             tr("No")    },
        { TR_OTHER,          tr("Other") },
        { TR_EMPTY,          tr("Empty") },
        { TR_LOCATION,       tr("Location") },
        { TR_PHOTOS,         tr("Photos") },
        { TR_NOTES,          tr("Notes") },
        { TR_DISTANCE,       tr("Distance") + " (m)" },
        { TR_BEARING,        tr("Bearing") + " (°)" },
        { TR_AUDIO_NOTE,     tr("Audio note") },
        { TR_SAMPLING_UNIT,  tr("Sampling unit") },
        { TR_OBSERVER,       tr("Observer") },
        { TR_CATEGORIES,     tr("Categories") },
        { TR_TRANSPORT_TYPE, tr("Transport type") },
        { TR_IS_ARMED,       tr("Is armed") },
        { TR_TEAM,           tr("Team") },
        { TR_STATION,        tr("Station") },
        { TR_MANDATE,        tr("Mandate") },
        { TR_OBJECTIVE,      tr("Objective") },
        { TR_COMMENT,        tr("Comment") },
        { TR_EMPLOYEES,      tr("Employees") },
        { TR_LEADER,         tr("Leader") },
        { TR_PILOT,          tr("Pilot") },
        { TR_INCIDENT_TYPE,  tr("Incident type") },
    };

    return knowns.value(trName, trName);
}

void Schema::populateFieldMap(BaseField* field) const
{
    // Insert this field.
    m_fieldMap.insert(field->uid(), field);

    // Insert the children.
    if (!field->isRecordField())
    {
        return;
    }

    auto recordField = RecordField::fromField(field);
    for (int i = 0; i < recordField->fieldCount(); i++)
    {
        auto f = recordField->field(i);
        qFatalIf(m_fieldMap.contains(f->uid()), "Duplicate field: " + f->uid());
        m_fieldMap.insert(f->uid(), f);
        populateFieldMap(f);
    }
}

QStringList Schema::buildFieldList(const QString& recordFieldUid, std::function<bool(BaseField* field)> filter) const
{
    auto result = QStringList();
    auto recordField = RecordField::fromField(getField(recordFieldUid));
    qFatalIf(!recordField, "Field must be a record field");

    for (int i = 0; i < recordField->fieldCount(); i++)
    {
        auto field = recordField->field(i);
        if (filter(field))
        {
            result.append(field->uid());
        }
    }

    return result;
}

QStringList Schema::buildFieldList(const QString& recordFieldUid, const QString& filterTagName, const QVariant& filterTagValue) const
{
    return buildFieldList(recordFieldUid, [&](BaseField* field) -> bool
    {
        if (field->hidden())
        {
            return false;
        }

        if (!filterTagName.isEmpty())
        {
            auto fieldTag = field->tag();
            if (fieldTag.isEmpty())
            {
                return false;
            }

            if (!fieldTag.contains(filterTagName))
            {
                return false;
            }

            if (fieldTag.value(filterTagName) != filterTagValue)
            {
                return false;
            }
        }

        return true;
    });
}

} // namespace CyberTracker
