#include "ClassicSchema.h"
#include "fxUtils.h"
#include "fxClasses.h"
#include "fxControl.h"
#include "ctElement.h"
#include "Schema.h"
#include "App.h"

namespace CyberTracker {

namespace {
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



}

ClassicSchema::ClassicSchema(QObject *parent): QObject{parent}
{
}

bool ClassicSchema::parse(Project* project, const QString& formFilePath, Schema* schema)
{
    FXFILEMAP fileMap = {};
    FXRESOURCEHEADER* header = nullptr;

    if (!FxMapFile(formFilePath.toStdString().c_str(), &fileMap))
    {
        qDebug() << "File not found: " << formFilePath;
        return false;
    }

    auto onExit = qScopeGuard([&] { FxUnmapFile(&fileMap); });

    header = (FXRESOURCEHEADER *)fileMap.BasePointer;
    if (header->FieldCount == 0)
    {
        qDebug() << "No fields in file";
        //return false;
    }

    auto locationField = new LocationField();
    locationField->set_uid("location");
    locationField->set_hidden(true);
    schema->appendField(locationField);

    auto trackFileField = new FileField();
    trackFileField->set_uid(TRACK_FIELD_UID);
    trackFileField->set_hidden(true);
    trackFileField->set_format("kmz");
    schema->appendField(trackFileField);

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
            auto sf = new StringField();
            sf->set_decodeValue(true); // decode element uid values.
            f = sf;
        };

        f->set_uid(uid);
        f->set_nameElementUid(uid);
        f->set_exportUid(fieldResource->Name);
        schema->appendField(f);
   });

    uidHash.clear();

    // Walk the file chunk by chunk to find elements.
    // File layout: [header][chunks...] where each chunk is [4-byte size][data of that size][padding to 4-byte align].
    // Element objects are OBJH chunks starting with attribute 0 (Name) containing MAGIC_TEXT data.
    // Additional attributes (icons, etc.) follow within the same chunk.
    {
        auto baseAddr = (const char*)header;
        auto offset = ((quint64)header->Size + 3) & ~3;
        while (offset + 8 < fileMap.FileSize)
        {
            auto chunkSize = *(quint32*)(baseAddr + offset);
            if (chunkSize == 0 || offset + 4 + chunkSize > fileMap.FileSize)
            {
                break;
            }

            auto chunkData = baseAddr + offset + 4;
            auto chunkEnd = chunkData + chunkSize;
            auto chunkMagic = *(quint32*)chunkData;

            // Check for OBJH chunks starting with attribute 0 (Name) containing TEXT data.
            if (chunkMagic == MAGIC_OBJECT && chunkSize >= sizeof(FXOBJECTHEADER) + sizeof(int))
            {
                auto firstObj = (FXOBJECTHEADER*)chunkData;
                if (firstObj->Attribute == (int)ElementAttribute::Name)
                {
                    auto textData = (FXTEXTRESOURCE*)(chunkData + sizeof(FXOBJECTHEADER));
                    if (textData->Magic == MAGIC_TEXT)
                    {
                        auto uid = GUIDToString(&textData->Guid);
                        if (!uid.isEmpty() && !uidHash.contains(uid))
                        {
                            uidHash.insert(uid, true);

                            auto element = new Element();
                            element->set_uid(uid);
                            element->set_name(QString(textData->Text));

                            // Walk sub-attributes for icons (prefer Icon100 > Icon50 > Icon32).
                            FXBITMAPRESOURCE* bestBitmap = nullptr;
                            auto subOffset = (const char*)chunkData;
                            while (subOffset + sizeof(FXOBJECTHEADER) < chunkEnd)
                            {
                                auto h = (FXOBJECTHEADER*)subOffset;
                                if (h->Magic != MAGIC_OBJECT) break;

                                if (h->Attribute == (int)ElementAttribute::Icon100 ||
                                    h->Attribute == (int)ElementAttribute::Icon50 ||
                                    h->Attribute == (int)ElementAttribute::Icon32)
                                {
                                    auto bitmap = (FXBITMAPRESOURCE*)(subOffset + sizeof(FXOBJECTHEADER));
                                    if (bitmap->Magic == MAGIC_BITMAP)
                                    {
                                        if (!bestBitmap || h->Attribute > ((FXOBJECTHEADER*)((const char*)bestBitmap - sizeof(FXOBJECTHEADER)))->Attribute)
                                        {
                                            bestBitmap = bitmap;
                                        }
                                    }
                                }

                                subOffset += sizeof(FXOBJECTHEADER) + h->Size;
                                subOffset = (const char*)(((quint64)subOffset + 3) & ~3);
                            }

                            if (bestBitmap)
                            {
                                auto format = (bestBitmap->BitDepth == 16) ? QImage::Format_RGB16 : QImage::Format_RGB32;
                                QImage image(&bestBitmap->Pixels[0], bestBitmap->Width, bestBitmap->Height, bestBitmap->RowBytes, format);
                                auto iconFileName = QString("Element_%1.png").arg(uid);
                                auto iconFilePath = App::instance()->projectManager()->getFilePath(project->uid(), iconFileName);
                                image.save(iconFilePath, "PNG");
                                element->set_icon(iconFileName);
                            }

                            schema->appendElement(element);
                        }
                    }
                }
            }

            offset += 4 + chunkSize;
            offset = (offset + 3) & ~3;
        }
    }

    return true;
}

} // namespace CyberTracker
