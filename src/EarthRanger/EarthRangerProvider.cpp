#include "EarthRangerProvider.h"
#include "EarthRangerSchema.h"
#include "App.h"
#include "Form.h"

namespace CyberTracker {

EarthRangerProvider::EarthRangerProvider(Form* form) : Provider(form)
{
    update_name(PROVIDER_NAME);
}

bool EarthRangerProvider::connectToProject()
{
    connect(m_form, &Form::sightingSaved, this, [=](const QString& sightingUid)
    {
        m_form->setSightingFlag(sightingUid, Database::DataFlag::DB_UPLOADED_FLAG, false);
        sendSighting(m_form, sightingUid);
    });

    return true;
}

QString EarthRangerProvider::getStartPage() const
{
    if (m_form->editing())
    {
        return "qrc:/EarthRanger/AttributesPage.qml";
    }

    auto needReportedBy = m_project->reportedBy().isEmpty() && m_form->schema()->getElement(EarthRangerSchema::ELEMENT_REPORTED_BY)->elementCount() > 0;
    return needReportedBy ? "qrc:/EarthRanger/ReportedByPage.qml" : "qrc:/EarthRanger/StartPage.qml";
}

bool EarthRangerProvider::sendSighting(Form* form, const QString& sightingUid)
{
    auto projectUid = m_project->uid();
    auto sighting = form->createSightingPtr(sightingUid);

    auto reportUid = sighting->getRecord(sightingUid)->getFieldValue("reportUid").toString();
    auto element = form->getElement(reportUid);
    auto fieldUids = element->fieldUids();

    auto remapToExportUid = [=](const QString& input) -> QString
    {
        auto element = form->getElement(input);
        return element ? element->exportUid() : input;
    };

    auto remapToNameValue = [=](const QString& input) -> QVariantMap
    {
        auto element = form->getElement(input);
        return QVariantMap
        {
            { "name", element ? form->getElementName(input) : input },
            { "value", remapToExportUid(input) },
        };
    };

    // Append non-record fields.
    auto appendFlatFieldValue = [&](QVariantMap& currDetail, BaseField* field, const QVariant& fieldValue)
    {
        if (field->isRecordField())
        {
            return;
        }

        // Skip values that were not entered.
        if (!fieldValue.isValid())
        {
            return;
        }

        auto value = field->save(fieldValue);

        // Fixup the list elements with their export ids.
        // TODO(justin): should the infrastructure handle this?
        if (!field->listElementUid().isEmpty())
        {
            auto type = static_cast<QMetaType::Type>(value.typeId());
            if (!value.toString().isEmpty() && type != QMetaType::QStringList && type != QMetaType::QVariantList)
            {
                value = remapToExportUid(value.toString());
            }
            else if (!value.toList().isEmpty())
            {
                // Check lists expect an array of name+value pairs.
                auto useNameValue = qobject_cast<CheckField*>(field);
                auto valueArr = value.toList();
                for (auto i = 0; i < valueArr.count(); i++)
                {
                    valueArr[i] = useNameValue ? remapToNameValue(valueArr[i].toString()) : QVariant(remapToExportUid(valueArr[i].toString()));
                }
                value = valueArr;
            }
        }

        currDetail.insert(field->exportUid(), QVariant(value));
    };

    auto detail = QVariantMap();
    for (auto fieldIt = fieldUids.constBegin(); fieldIt != fieldUids.constEnd(); fieldIt++)
    {
        auto fieldUid = *fieldIt;

        // Skip location, photos and notes.
        if (fieldUid == EarthRangerSchema::FIELD_LOCATION || fieldUid == EarthRangerSchema::FIELD_PHOTOS || fieldUid == EarthRangerSchema::FIELD_NOTES)
        {
            continue;
        }

        auto field = form->getField(fieldUid);
        auto recordField = RecordField::fromField(field);

        // Add flat fields.
        if (!recordField)
        {
            appendFlatFieldValue(detail, field, sighting->getRecord(sightingUid)->getFieldValue(fieldUid));
            continue;
        }

        // Add records.
        auto recordUids = sighting->getRecord(sightingUid)->getFieldValue(fieldUid).toStringList();
        auto recordArray = QVariantList();
        for (auto recordIt = recordUids.constBegin(); recordIt != recordUids.constEnd(); recordIt++)
        {
            auto recordUid = *recordIt;
            auto childDetail = QVariantMap();

            sighting->getRecord(recordUid)->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
            {
                appendFlatFieldValue(childDetail, fieldValue.field(), fieldValue.value());
            });

            recordArray.append(childDetail);
        }

        if (recordArray.count() > 0)
        {
            detail.insert(field->exportUid(), recordArray);
        }
    }

    auto location = QVariantMap();
    auto locationValue = sighting->getRecord(sightingUid)->getFieldValue(EarthRangerSchema::FIELD_LOCATION).toMap();
    location["latitude"] = locationValue.value("y").toDouble();
    location["longitude"] = locationValue.value("x").toDouble();

    auto event = QVariantMap();
    event["event_type"] = reportUid;
    event["event_details"] = detail;
    event["time"] = sighting->createTime();
    event["location"] = location;

    auto reportedByElement = form->getElement(m_project->reportedBy());
    if (reportedByElement)
    {
        event["reported_by"] = reportedByElement->getTagValue("identity").toMap();
    }
    else
    {
        qDebug() << "warning: sighting reported_by not found (" << m_project->reportedBy() << ")";
    }

    m_database->transaction([&]() -> bool
    {
        auto queueItem = Database::DbQueueItem();
        queueItem.projectUid = projectUid;
        queueItem.dataUid = sighting->rootRecordUid();
        queueItem.type = "DATA";
        queueItem.url = m_project->getConnectorParam("server").toString();
        queueItem.summary = form->renderer()->getSightingSummaryText(sighting.get());
        queueItem.payload = Utils::variantMapToJson(event);
        queueItem.webhook = form->renderer()->getSightingWebhook(m_project, sighting.get());
        queueItem.attachments = sighting->getRecord(sightingUid)->getFieldValue(EarthRangerSchema::FIELD_PHOTOS).toStringList();
        m_uploadQueue->addItem(queueItem);

        auto queueState = QVariantMap();
        m_database->getDataQueueState(projectUid, queueItem.dataUid, &queueState);
        queueState.insert("note", sighting->getRecord(sightingUid)->getFieldValue(EarthRangerSchema::FIELD_NOTES).toString().trimmed());
        m_database->setDataQueueState(projectUid, queueItem.dataUid, queueState);

        registerBackupData(queueItem.payload, ".json");

        return true;
    });

    m_uploadQueue->kick();

    return true;
}

bool EarthRangerProvider::finalizePackage(const QString& packageFilesPath) const
{
    // Cleanup cached files.
    QFile::remove(packageFilesPath + "/Form.qml");

    return true;
}

QString EarthRangerProvider::banner() const
{
    auto result = QString();
    auto createTime = m_form->sighting()->createTime();
    if (!createTime.isEmpty())
    {
        auto timeManager = App::instance()->timeManager();
        auto dt = timeManager->getDateText(createTime);
        auto tt = timeManager->getTimeText(createTime);
        result = tr("Editing report saved on") + " " + dt + " " + tt;
    }

    return result;
}

} // namespace CyberTracker
