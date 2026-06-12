#include "GoogleProvider.h"
#include "App.h"
#include "Form.h"

namespace CyberTracker {

GoogleProvider::GoogleProvider(Form* form) : Provider(form)
{
    update_name(PROVIDER_NAME);
}

bool GoogleProvider::connectToProject()
{
    auto projectUid = m_project->uid();

    // Delete uploaded sightings after 2 seconds.
    connect(m_form, &Form::sightingFlagsChanged, this, [=](const QString& sightingUid)
    {
        if (!m_database->testData(projectUid, sightingUid))
        {
            return;
        }

        auto flags = m_database->getDataFlags(projectUid, sightingUid);
        if (!flags.testFlag(Database::DataFlag::DB_UPLOADED_FLAG))
        {
            return;
        }

        QTimer::singleShot(2000, m_form, &Form::deleteUploadedSightings);
    });

    // Cleanup uploaded data.
    connect(m_form, &Form::connectedChanged, this, [=]()
    {
        if (!m_form->connected())
        {
            return;
        }

        m_form->deleteUploadedSightings();

        // Show a message if we are missing a file folder for uploads.
        auto settings = m_form->schema()->settings();
        if (settings.value("driveFolderId").toString().isEmpty() && settings.value("needsFileFolder").toBool())
        {
            emit App::instance()->showError(QString(tr("Missing %1 attribute")).arg("#fileFolder"));
        }
    });

    return true;
}

bool GoogleProvider::requireUsername() const
{
    return m_form->schema()->settings()["requireUsername"].toBool() &&
           m_project->username().isEmpty() &&
           App::instance()->settings()->username().isEmpty();
}

QString GoogleProvider::getStartPage() const
{
    if (m_form->editing())
    {
        return "qrc:/ODK/FormPage.qml";
    }

    if (m_form->schema()->settings()["hasFileUploadItem"].toBool())
    {
        return "qrc:/Google/UnsupportedPage.qml";
    }

    return "qrc:/ODK/StartPage.qml";
}

QUrlQuery GoogleProvider::buildSighting(Sighting* sighting) const
{
    auto result = QUrlQuery();
    auto sections = QStringList { "0" };

    auto addDate = [&](const QString& exportUid, const QString& isoDateTime, bool hideYear)
    {
        auto date = Utils::decodeTimestamp(isoDateTime).date();

        if (!hideYear)
        {
            result.addQueryItem(exportUid + "_year", QString::number(date.year()));
        }

        result.addQueryItem(exportUid + "_month", QString::number(date.month()));
        result.addQueryItem(exportUid + "_day", QString::number(date.day()));
    };

    auto addTime = [&](const QString& exportUid, const QString& isoDateTime)
    {
        auto time = Utils::decodeTimestamp(isoDateTime).time();

        result.addQueryItem(exportUid + "_hour", QString::number(time.hour()));
        result.addQueryItem(exportUid + "_minute", QString::number(time.minute()));
    };

    sighting->recordManager()->enumFieldValues(
        sighting->rootRecordUid(), true,
        nullptr, // recordBeginCallback.
        [&](const FieldValue& fieldValue) // fieldValueCallback.
        {
            auto field = fieldValue.field();
            auto exportUid = fieldValue.exportUid();
            auto valueString = fieldValue.value().toString();

            auto parentField = field->parentField();
            auto sectionIndex = parentField ? parentField->getTagValue("sectionIndex").toString() : QString();
            if (!sectionIndex.isEmpty() && !sections.contains(sectionIndex))
            {
                sections.append(sectionIndex);
            }

            // String and single select.
            auto stringField = qobject_cast<StringField*>(field);
            if (stringField && !valueString.isEmpty())
            {
                if (!stringField->listElementUid().isEmpty())
                {
                    auto valueElement = m_form->schema()->getElement(valueString);
                    result.addQueryItem(exportUid, valueElement->exportUid());
                    return;
                }

                if (stringField->tag().value("duration", false).toBool())
                {
                    auto values = valueString.split(':');
                    qFatalIf(values.length() != 3, "Bad duration");
                    result.addQueryItem(exportUid + "_hour", values[0]);
                    result.addQueryItem(exportUid + "_minute", values[1]);
                    result.addQueryItem(exportUid + "_second", values[2]);
                    return;
                }

                result.addQueryItem(exportUid, valueString);
                return;
            }

            // Check list.
            auto checkField = qobject_cast<CheckField*>(field);
            if (checkField)
            {
                auto valueMap = fieldValue.value().toMap();
                if (valueMap.isEmpty())
                {
                    return;
                }

                m_form->schema()->enumElementChildren(checkField->listElementUid(), true, [&](Element* element)
                {
                    auto elementUid = element->uid();
                    if (valueMap.contains(elementUid))
                    {
                        result.addQueryItem(exportUid, element->safeExportUid());
                    }
                });

                return;
            }

            // Date.
            auto dateField = qobject_cast<DateField*>(field);
            if (dateField && !valueString.isEmpty())
            {
                addDate(exportUid, valueString, dateField->hideYear());
                return;
            }

            // Time.
            auto timeField = qobject_cast<TimeField*>(field);
            if (timeField && !valueString.isEmpty())
            {
                addTime(exportUid, valueString);
                return;
            }

            // Datetime.
            auto dateTimeField = qobject_cast<DateTimeField*>(field);
            if (dateTimeField && !valueString.isEmpty())
            {
                addDate(exportUid, valueString, dateTimeField->hideYear());
                addTime(exportUid, valueString);
                return;
            }

            // Location.
            auto locationField = qobject_cast<LocationField*>(field);
            if (locationField)
            {
                auto valueMap = fieldValue.value().toMap();
                if (valueMap.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, QString("%1 %2").arg(valueMap["y"].toString(), valueMap["x"].toString()));
                return;
            }

            // Line field.
            auto lineField = qobject_cast<LineField*>(field);
            if (lineField)
            {
                auto value = fieldValue.xmlValue();
                if (value.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, value);
                return;
            }

            // Area field.
            auto areaField = qobject_cast<AreaField*>(field);
            if (areaField)
            {
                auto value = fieldValue.xmlValue();
                if (value.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, value);
                return;
            }

            // Photo.
            auto photoField = qobject_cast<PhotoField*>(field);
            if (photoField)
            {
                auto valueList = fieldValue.value().toStringList();
                if (valueList.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, valueList.join(", "));
                return;
            }

            // Audio.
            auto audioField = qobject_cast<AudioField*>(field);
            if (audioField)
            {
                auto valueMap = fieldValue.value().toMap();
                auto filename = valueMap["filename"].toString();
                if (filename.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, filename);
                return;
            }

            // Sketch.
            auto sketchField = qobject_cast<SketchField*>(field);
            if (sketchField)
            {
                auto valueMap = fieldValue.value().toMap();
                auto filename = valueMap["filename"].toString();
                if (filename.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, filename);
                return;
            }

            // File.
            auto fileField = qobject_cast<FileField*>(field);
            if (fileField)
            {
                auto filename = fieldValue.value().toString();
                if (filename.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, filename);
                return;
            }

            // Everything else.
            if (!valueString.isEmpty())
            {
                result.addQueryItem(exportUid, valueString);
                return;
            }
        },
        nullptr // recordEndCallback.
    );

    result.addQueryItem("pageHistory", sections.join(','));
    result.addQueryItem("submit", "Submit");

    return result;
}

void GoogleProvider::submitData()
{
    auto projectUid = m_project->uid();

    // Backup database.
    App::instance()->backupDatabase("GF: submit data");

    // Submit data to the queue.
    m_database->transaction([&]() -> bool
    {
        m_database->enumData(projectUid, m_form->stateSpace(), Database::DataFlag::DB_SIGHTING_FLAG | Database::DataFlag::DB_COMPLETED_FLAG /* ON */, Database::DataFlag::DB_SUBMITTED_FLAG /* OFF */,
            [&](const QString& projectUid, const QString& sightingUid, const Database::DbData& dbSighting)
        {
            auto sighting = m_form->createSightingPtr(dbSighting.data);

            auto queueItem = Database::DbQueueItem();
            queueItem.projectUid = projectUid;
            queueItem.dataUid = sightingUid;
            queueItem.summary = m_form->renderer()->getSightingSummaryText(sighting.get());
            queueItem.url = m_project->getConnectorParam("publishedUrl").toString();
            queueItem.payload = buildSighting(sighting.get()).toString(QUrl::FullyEncoded).toUtf8();
            queueItem.webhook = m_form->renderer()->getSightingWebhook(m_project, sighting.get());
            queueItem.attachments = sighting->recordManager()->attachments();

            m_uploadQueue->addItem(queueItem);
            m_database->setDataFlags(projectUid, sightingUid, Database::DataFlag::DB_SUBMITTED_FLAG);

            registerBackupData(queueItem.payload, ".txt");

            emit App::instance()->sightingFlagsChanged(projectUid, sightingUid);
        });

        return true;
    });

    m_uploadQueue->kick();
}

} // namespace CyberTracker