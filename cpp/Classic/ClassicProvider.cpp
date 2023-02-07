#include "ClassicProvider.h"
#include "ctSession.h"
#include "fxUtils.h"
#include "ctElement.h"

constexpr bool CACHE_ENABLE = false;

namespace CTSParser {

UINT64 PadOffset(UINT64 Value) { return (Value + 3) & ~3; }

void enumFields(FXRESOURCEHEADER* header, std::function<void(FXFIELDRESOURCE*)> callback)
{
    auto fieldOffset = header->FirstField;
    for (auto i = 0; i < (int)header->FieldCount; i++)
    {
        auto binary = (FXBINARYRESOURCE *)((UINT64)header + (UINT64)fieldOffset);
        if (binary->Magic == MAGIC_FIELD)
        {
            callback((FXFIELDRESOURCE*)binary);
        }

        fieldOffset += sizeof(FXFIELDRESOURCE);
        fieldOffset += 4;
    }
}

void enumObjects(FXRESOURCEHEADER* header, quint64 fileSize, std::function<void(FXOBJECTHEADER*)> callback)
{
    auto offset = (quint64)header + (quint64)header->FirstField - 4;
    auto endOfFile = (quint64)header + fileSize;
    while (offset < endOfFile)
    {
        quint32 size = *(quint32*)offset;
        offset += 4;
        quint32 magic = *(quint32*)offset;
        if (magic == MAGIC_OBJECT)
        {
            callback((FXOBJECTHEADER*)offset);
        }

        offset += size;
        offset = (offset + 3) & ~3;
    }
}

struct ClassicElement
{
    QString uid;
    QString name;
    QString jsonId;
};

void enumElements(FXRESOURCEHEADER* header, quint64 fileSize, std::function<void(ClassicElement&)> callback)
{
    enumObjects(header, fileSize, [&](FXOBJECTHEADER* objectHeader)
    {
        ClassicElement e;

        auto offset = (quint64)objectHeader;
        for (;;)
        {
            FXOBJECTHEADER* h = (FXOBJECTHEADER *)offset;

            // Done searching: resource not found
            if (h->Magic != MAGIC_OBJECT)
            {
                break;
            }

            // Check for a match
            if (h->Attribute == eaName)
            {
                FXTEXTRESOURCE* r = (FXTEXTRESOURCE *)(offset + sizeof(FXOBJECTHEADER));
                if (r->Magic != MAGIC_TEXT)
                {
                    // Not an Element.
                    break;
                }

                e.uid = GUIDToString(&r->Guid);
                e.name = QString(r->Text);
            }
            else if (h->Attribute == eaJsonId)
            {
                FXJSONID* r = (FXJSONID *)(offset + sizeof(FXOBJECTHEADER));
                if (r->Magic != MAGIC_JSONID)
                {
                    // Not an Element.
                    break;
                }

                e.jsonId = QString(r->Text);
            }

            if (h->Attribute > eaJsonId)
            {
                break;
            }

            // No match, so skip ahead
            offset = offset + sizeof(FXOBJECTHEADER) + h->Size;
            offset = (offset + 3) & ~3;
        }

        if (!e.uid.isEmpty())
        {
            callback(e);
        }
    });
}

void parse(const QString& formFilePath, const QString& targetFolder)
{
    FXFILEMAP fileMap = {};
    FXRESOURCEHEADER* header = nullptr;

    if (!FxMapFile(formFilePath.toStdString().c_str(), &fileMap))
    {
        return;
    }

    auto onExit = qScopeGuard([&] { FxUnmapFile(&fileMap); });

    header = (FXRESOURCEHEADER *)fileMap.BasePointer;
    if (header->FieldCount == 0)
    {
        qDebug() << "No fields in file";
        return;
    }

    FieldManager fieldManager;

    auto locationField = new LocationField();
    locationField->set_uid("location");
    locationField->set_hidden(true);
    fieldManager.rootField()->appendField(locationField);

    auto uidHash = QHash<QString, bool>();

    enumFields(header, [&](FXFIELDRESOURCE* fieldResource)
    {
        auto uid = GUIDToString(&fieldResource->ElementId);

        if (uidHash.contains(uid))
        {
            qDebug() << "Discard duplicate field: " << uid;
            return;
        }

        uidHash.insert(uid, true);

        BaseField* f = nullptr;
        switch ((FXDATATYPE)(fieldResource->DataType))
        {
        case dtGraphic:
            f = new PhotoField();
            break;

        case dtSound:
            f = new AudioField();
            break;

        default:
            f = new StringField();
        };

        f->set_uid(uid);
        f->set_nameElementUid(uid);
        f->set_exportUid(fieldResource->Name);
        fieldManager.rootField()->appendField(f);
    });

    auto targetFolderFinal = Utils::removeTrailingSlash(targetFolder);
    fieldManager.saveToQmlFile(targetFolderFinal + "/Fields.qml");

    ElementManager elementManager;

    uidHash.clear();

    enumElements(header, fileMap.FileSize, [&](ClassicElement& classicElement)
    {
        if (uidHash.contains(classicElement.uid))
        {
            qDebug() << "Discard duplicate element: " << classicElement.uid;
            return;
        }

        uidHash.insert(classicElement.uid, true);

        auto element = new Element();
        element->set_uid(classicElement.uid);
        element->set_name(classicElement.name);
        element->set_exportUid(classicElement.jsonId);
        elementManager.rootElement()->appendElement(element);
    });

    elementManager.saveToQmlFile(targetFolderFinal + "/Elements.qml");
}

}

ClassicProvider::ClassicProvider(QObject *parent) : Provider(parent)
{
    update_name(CLASSIC_PROVIDER);
}

int ClassicProvider::version()
{
    return RESOURCE_VERSION;
}

bool ClassicProvider::connectToProject(bool newBuild, bool* formChangedOut)
{
    auto formFilePath = Utils::classicRoot() + "/" + m_project->connectorParams()["app"].toString() + ".CTS";
    auto formLastModified = QFileInfo(formFilePath).lastModified().toMSecsSinceEpoch();
    auto elementsFilePath = getProjectFilePath("Elements.qml");
    auto fieldsFilePath = getProjectFilePath("Fields.qml");

    // Early out if possible.
    if (!CACHE_ENABLE || newBuild || formLastModified != m_project->formLastModified() || !QFile::exists(elementsFilePath) || !QFile::exists(fieldsFilePath))
    {
        CTSParser::parse(formFilePath, getProjectFilePath(""));
        m_project->set_formLastModified(formLastModified);
    }

    return true;
}

QUrl ClassicProvider::getStartPage()
{
    return QUrl("qrc:/Classic/StartPage.qml");
}

void ClassicProvider::getElements(ElementManager* elementManager)
{
    elementManager->loadFromQmlFile(getProjectFilePath("Elements.qml"));
}

void ClassicProvider::getFields(FieldManager* fieldManager)
{
    fieldManager->loadFromQmlFile(getProjectFilePath("Fields.qml"));
}
