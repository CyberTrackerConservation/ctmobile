#include "EsriProvider.h"
#include "EsriConnector.h"
#include "App.h"
#include "Form.h"
#include "TrackFile.h"

namespace CyberTracker {

EsriProvider::EsriProvider(Form* form) : Provider(form)
{
    m_name = PROVIDER_NAME;
    m_locationServicePage = "qrc:/Esri/LocationServicePage.qml";
}

bool EsriProvider::connectToProject()
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
        if (!(flags & Database::DataFlag::DB_UPLOADED_FLAG))
        {
            return;
        }

        QTimer::singleShot(2000, m_form, &Form::deleteUploadedSightings);
    });

    // Last known location.
    connect(m_form, &Form::locationPoint, this, [=](Location* update)
    {
        sendLastLocation(update);
    });

    // Keep locationServiceUrl up to date.
    m_locationServiceUrl = m_project->locationServiceUrl();
    connect(m_project, &Project::locationServiceUrlChanged, this, [&]()
    {
        auto serviceUrl = m_project->locationServiceUrl();

        auto finalServiceUrl = serviceUrl.trimmed();
        if (finalServiceUrl.endsWith("?"))
        {
            finalServiceUrl.truncate(finalServiceUrl.length() - 1);
        }

        if (serviceUrl != finalServiceUrl)
        {
            m_project->set_locationServiceUrl(finalServiceUrl);
        }

        update_locationServiceUrl(finalServiceUrl);
    });

    // Cleanup uploaded data.
    connect(m_form, &Form::connectedChanged, this, [=]()
    {
        m_form->deleteUploadedSightings();
    });

    return true;
}

bool EsriProvider::requireUsername() const
{
    return m_form->schema()->getSetting("requireUsername").toBool() &&
           m_project->username().isEmpty() &&
           App::instance()->settings()->username().isEmpty();
}

bool EsriProvider::supportLocationTrackField() const
{
    return false;
}

bool EsriProvider::supportLocationTrackService() const
{
    return true;
}

QString EsriProvider::getStartPage() const
{
    auto parameters = m_form->schema()->getSetting("parameters").toMap();
    if (parameters.contains("startPage"))
    {
        auto startPagePath = getProjectFilePath(parameters["startPage"].toString());
        if (QFile::exists(startPagePath))
        {
            return QUrl::fromLocalFile(startPagePath).toString();
        }

        qDebug() << "Failed to load start page: " << startPagePath;
    }

    return "qrc:/ODK/StartPage.qml";
}

bool EsriProvider::ensureServiceState()
{
    if (m_project->locationServiceUrl().isEmpty())
    {
        return true;
    }

    auto serviceState = m_project->esriLocationServiceState();
    if (!serviceState.isEmpty())
    {
        return true;
    }

    auto response = Utils::esriInitLocationTracking(App::instance()->networkAccessManager(), m_project->locationServiceUrl(), m_project->accessToken(), &serviceState);
    if (response.success)
    {
        m_project->set_esriLocationServiceState(serviceState);
        return true;
    }

    emit App::instance()->showError(tr("Failed to initialize service"));
    return false;
}

QVariantList EsriProvider::buildSightingPayload(Sighting* sighting, QVariantMap* attachmentsMapOut)
{
    // Build the per-record payload.
    struct RecordData
    {
        QString recordUid;
        int serviceId = -1;
        QVariantMap payload;
    };

    attachmentsMapOut->clear();
    auto recordDataList = QVector<RecordData>();
    auto schema = m_form->schema();

    // Iterate over the records and build a RecordData structure for each service table.
    auto queueRecordUids = QStringList { sighting->rootRecordUid() };

    while (!queueRecordUids.isEmpty())
    {
        auto currRecordUid = queueRecordUids.first();
        queueRecordUids.removeFirst();
        auto recordField = sighting->getRecord(currRecordUid)->recordField();

        auto geometry = QVariantMap();
        auto geometryType = recordField->getTagValue("geometryType").toString();
        if (!geometryType.isEmpty())
        {
            geometry = QVariantMap { { "geometryType", geometryType }, { "spatialReference", QVariantMap { { "wkid", 4326 } } } };
        }

        auto attributes = QVariantMap();
        attributes.insert("globalid", Utils::addBracesToUuid(currRecordUid));

        auto parentRecordUid = sighting->getRecord(currRecordUid)->parentRecordUid();
        if (!parentRecordUid.isEmpty())
        {
            attributes.insert("parentglobalid", Utils::addBracesToUuid(parentRecordUid));
        }

        auto attachments = QStringList();

        // Enumerate the fields in this record and child groups.
        sighting->recordManager()->enumFieldValues(currRecordUid, true,
            [&](const Record& record, bool* skipRecordOut) // recordBeginCallback.
            {
                // Always use the record we are scanning.
                if (record.recordUid() == currRecordUid)
                {
                    return;
                }

                // Flatten groups.
                if (record.recordField()->group())
                {
                    return;
                }

                // Skip this record.
                *skipRecordOut = true;

                // Add complex records with a service as pending records.
                if (recordField->tag().contains("serviceId"))
                {
                    queueRecordUids.append(record.recordUid());
                }
            },
            [&](const FieldValue& fieldValue) // fieldValueCallback.
            {
                auto field = fieldValue.field();

                // Skip record fields:
                //   If it is a group, enum will enumerate all the children next
                //   If it is a complex record, the recordBeginCallback will track it for later.
                if (field->isRecordField())
                {
                    return;
                }

                auto fieldIsList = !field->listElementUid().isEmpty();
                auto finalFieldValue = fieldValue.value();

                // Single select.
                if (fieldIsList && qobject_cast<StringField*>(field))
                {
                    if (!fieldValue.value().toString().isEmpty())
                    {
                        finalFieldValue = schema->getElement(fieldValue.value().toString())->safeExportUid();
                    }
                }
                // Check list.
                else if (fieldIsList && qobject_cast<CheckField*>(field))
                {
                    auto fieldValueMap = fieldValue.value().toMap();
                    auto fieldValues = QStringList();

                    for (auto it = fieldValueMap.constKeyValueBegin(); it != fieldValueMap.constKeyValueEnd(); it++)
                    {
                        if (!it->second.toBool())
                        {
                            continue;
                        }

                        fieldValues.append(schema->getElement(it->first)->safeExportUid());
                    }

                    finalFieldValue = fieldValues.join(',');
                }
                // Location.
                else if (qobject_cast<LocationField*>(field))
                {
                    auto locationValue = fieldValue.value().toMap();
                    geometry.insert("x", locationValue["x"]);
                    geometry.insert("y", locationValue["y"]);
                    geometry.insert("z", locationValue["z"]);
                    return;
                }
                else if (qobject_cast<LineField*>(field))
                {
                    auto points = fieldValue.value().toList();
                    auto path = QVariantList();

                    for (auto it = points.constBegin(); it != points.constEnd(); it++)
                    {
                        auto point = it->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        auto z = point[2].toDouble();
                        path.append(QVariant(QVariantList { x, y, z }));
                    }

                    if (!path.isEmpty())
                    {
                        geometry.insert("paths", QVariantList { QVariant(path) });
                    }

                    return;
                }
                else if (qobject_cast<AreaField*>(field))
                {
                    auto points = fieldValue.value().toList();
                    auto path = QVariantList();

                    for (auto it = points.constBegin(); it != points.constEnd(); it++)
                    {
                        auto point = it->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        auto z = point[2].toDouble();
                        path.append(QVariant(QVariantList { x, y, z }));
                    }

                    if (!path.isEmpty())
                    {
                        path.append(path.constFirst()); // Last point same as first point.
                        geometry.insert("rings", QVariantList { QVariant(path) });
                    }

                    return;
                }

                // Attachments.
                else if (qobject_cast<PhotoField*>(field) ||
                         qobject_cast<AudioField*>(field) ||
                         qobject_cast<SketchField*>(field) ||
                         qobject_cast<FileField*>(field))
                {
                    attachments.append(fieldValue.attachments());
                    return;
                }

                // Get ESRI field type for custom conversions.
                auto esriFieldType = field->getTagValue("type").toString();
                if (esriFieldType.isEmpty())
                {
                    // Some calculation fields are only used internally to the form.
                    qWarning() << "Missing esriFieldType: " << fieldValue.fieldUid();
                }

                // Convert date to required format.
                if (esriFieldType == "esriFieldTypeDate")
                {
                    finalFieldValue = Utils::decodeTimestampMSecs(fieldValue.value().toString());
                }
                else if (esriFieldType == "esriFieldTypeInteger")
                {
                    finalFieldValue = fieldValue.value().toInt();
                }

                // Fill in the attribute.
                attributes.insert(field->safeExportUid(), finalFieldValue);
            },
            nullptr
        );

        // Add to the list.
        RecordData recordData;
        recordData.recordUid = currRecordUid;
        recordData.serviceId = recordField->getTagValue("serviceId", -1).toInt();
        recordData.payload["attributes"] = attributes;

        if (!geometry.isEmpty())
        {
            recordData.payload["geometry"] = geometry;
        }

        recordDataList.append(recordData);

        if (attachments.count() > 0)
        {
            attachmentsMapOut->insert(currRecordUid, attachments);
        }
    }

    // Sort by layer id.
    auto orderRecordUids = sighting->recordManager()->recordUids();
    std::sort(recordDataList.begin(), recordDataList.end(), [&](const RecordData& rd1, const RecordData& rd2) -> bool
    {
        // Order by original list order if the service ids are the same.
        if (rd1.serviceId == rd2.serviceId)
        {
            return orderRecordUids.indexOf(rd1.recordUid) < orderRecordUids.indexOf(rd2.recordUid);
        }

        // Order by service id.
        return rd1.serviceId < rd2.serviceId;
    });

    // Build the final output.
    auto result = QVariantList();

    auto lastServiceId = -1;
    auto lastServiceAdds = QVariantList();
    for (auto it = recordDataList.constBegin(); it != recordDataList.constEnd(); it++)
    {
        if (lastServiceId != -1 && it->serviceId != lastServiceId)
        {
            result.append(QVariantMap { { "id", lastServiceId }, { "adds", lastServiceAdds } });
            lastServiceAdds.clear();
        }

        lastServiceId = it->serviceId;
        lastServiceAdds.append(it->payload);
    }

    result.append(QVariantMap { { "id", lastServiceId }, { "adds", lastServiceAdds } });

    return result;
}

QVariantMap EsriProvider::buildLocationPayload(const QString& locationUid, Location* location) const
{
    auto attributes = QVariantMap
    {
        { "globalid", Utils::addBracesToUuid(locationUid) },
        { "deviceid", Utils::addBracesToUuid(App::instance()->settings()->deviceId()) },
        { "appid", App::instance()->config()["title"] },
        { "counter", location->counter() },
        { "timestamp", Utils::decodeTimestampMSecs(location->timestamp()) },
        { "accuracy", location->accuracy() },
        { "altitude", location->z() },
        { "speed", location->speed() },
        { "course", location->direction() },
        { "interval", location->interval() },
        { "distance_filter", location->distanceFilter() },
        { "accuracy_filter", location->accuracyFilter() },
        { "full_name", m_project->esriLocationServiceState().value("fullName", "Not Found") },
    };

    auto geometry = QVariantMap
    {
        { "spatialReference", QVariantMap { { "wkid", 4326 } } },
        { "x", location->x() },
        { "y", location->y() },
        { "z", location->z() },
    };

    return QVariantMap
    {
        { "attributes", attributes },
        { "geometry", geometry },
    };
}

bool EsriProvider::sendSighting(Sighting* sighting)
{
    // Send the sighting and attachments.
    if (!sighting->trackOnly())
    {
        auto attachments = QVariantMap();
        auto features = buildSightingPayload(sighting, &attachments);

        auto payload = QVariantMap
        {
            { "attachments", attachments },
            { "features", features },
        };

        auto queueItem = Database::DbQueueItem();
        queueItem.projectUid = m_project->uid();
        queueItem.dataUid = sighting->rootRecordUid();
        queueItem.type = EsriConnector::UPLOAD_DATA;
        queueItem.summary = m_form->renderer()->getSightingSummaryText(sighting);
        queueItem.url = m_project->connectorParams()["serviceUrl"].toString();
        queueItem.payload = Utils::variantMapToJson(payload);
        queueItem.webhook = m_form->renderer()->getSightingWebhook(m_project, sighting);

        for (auto attachmentsIt = attachments.constKeyValueBegin(); attachmentsIt != attachments.constKeyValueEnd(); attachmentsIt++)
        {
            queueItem.attachments.append(attachmentsIt->second.toStringList());
        }

        m_uploadQueue->addItem(queueItem);

        registerBackupData(queueItem.payload, ".json");
    }

    // Send the track.
    if (!sighting->trackFile().isEmpty())
    {
        sendTrackFile(sighting);
    }

    return true;
}

void EsriProvider::sendLastLocation(Location* location)
{
    if (!ensureServiceState())
    {
        return;
    }

    auto serviceState = m_project->esriLocationServiceState();

    auto attributes = QVariantMap
    {
        { "globalid", serviceState.value("globalid") },
        { "deviceid", Utils::addBracesToUuid(App::instance()->settings()->deviceId()) },
        { "appid", App::instance()->config()["title"] },
        { "timestamp", Utils::decodeTimestampMSecs(location->timestamp()) },
        { "accuracy", location->accuracy() },
        { "altitude", location->z() },
        { "speed", location->speed() },
        { "course", location->direction() },
        { "interval", location->interval() },
        { "full_name", serviceState.value("fullName", "Not Found") },
    };

    auto geometry = QVariantMap
    {
        { "spatialReference", QVariantMap { { "wkid", 4326 } } },
        { "x", location->x() },
        { "y", location->y() },
        { "z", location->z() },
    };

    auto feature = QVariantMap
    {
        { "attributes", attributes },
        { "geometry", geometry },
    };

    auto queueItem = Database::DbQueueItem();
    queueItem.projectUid = m_project->uid();
    queueItem.type = EsriConnector::UPLOAD_LAST_LOCATION;
    queueItem.summary = "Send last location";
    queueItem.url = m_project->locationServiceUrl();
    queueItem.flags = Database::QueueFlag::DB_AUTO_DELETE;
    queueItem.payload = Utils::variantMapToJson(feature);
    m_uploadQueue->addItem(queueItem);

    m_uploadQueue->kick();
}

void EsriProvider::sendTrackFile(Sighting* sighting)
{
    auto queueItem = Database::DbQueueItem();
    queueItem.projectUid = m_project->uid();
    queueItem.dataUid = sighting->rootRecordUid();
    queueItem.type = EsriConnector::UPLOAD_TRACK;
    queueItem.summary = tr("Location track");;
    queueItem.url = m_project->locationServiceUrl();

    auto trackFile = TrackFile(App::instance()->getMediaFilePath(sighting->trackFile()));

    // Locations.
    {
        auto batchLocationUids = QStringList();
        auto batchFeatures = QVariantList();
        auto lastFeature = QVariantMap();
        auto featuresList = QVariantList();

        trackFile.enumLocations([&](const QString& locationUid, Location* location)
        {
            lastFeature = buildLocationPayload(locationUid, location);
            batchLocationUids.append(locationUid);
            batchFeatures.append(lastFeature);

            if (batchLocationUids.count() == 1024)
            {
                featuresList.append(QVariant(batchFeatures));
                batchLocationUids.clear();
                batchFeatures.clear();
            }
        });

        // Leftovers.
        if (!batchLocationUids.isEmpty())
        {
            featuresList.append(QVariant(batchFeatures));
        }

        auto locationsFilename = Utils::makeUniqueFilename(queueItem.uid + "_", App::instance()->timeManager()->currentDateTime(), "_locations.json");
        auto locationsFilePath = App::instance()->getMediaFilePath(locationsFilename);
        Utils::variantListToJsonFile(locationsFilePath, featuresList, QJsonDocument::Compact);
        queueItem.attachments.append(locationsFilename);
    }

    // Lines.
    {
        auto serviceState = m_project->esriLocationServiceState();
        auto features = trackFile.createEsriFeatures(queueItem.dataUid, serviceState.value("fullName").toString());

        auto linesFilename = Utils::makeUniqueFilename(queueItem.uid + "_", App::instance()->timeManager()->currentDateTime(), "_lines.json");
        auto linesFilePath = App::instance()->getMediaFilePath(linesFilename);
        Utils::variantListToJsonFile(linesFilePath, features, QJsonDocument::Compact);
        queueItem.attachments.append(linesFilename);
    }

    m_uploadQueue->addItem(queueItem);
}

void EsriProvider::submitData()
{
    if (!ensureServiceState())
    {
        return;
    }

    auto projectUid = m_project->uid();

    // Backup database.
    App::instance()->backupDatabase("ESRI: submit data");

    // Submit data to the queue.
    m_database->enumData(projectUid, m_form->stateSpace(), Database::DataFlag::DB_SIGHTING_FLAG | Database::DataFlag::DB_COMPLETED_FLAG /* ON */, Database::DataFlag::DB_SUBMITTED_FLAG /* OFF */,
        [&](const QString& /*projectUid*/, const QString& sightingUid, const Database::DbData& dbSighting)
    {
        auto sighting = m_form->createSightingPtr(dbSighting.data);
        sendSighting(sighting.get());

        m_database->setDataFlags(projectUid, sightingUid, Database::DataFlag::DB_SUBMITTED_FLAG);

        emit App::instance()->sightingFlagsChanged(projectUid, sightingUid);
    });

    m_uploadQueue->kick();
}

bool EsriProvider::finalizePackage(const QString& packageFilesPath) const
{
    QFile::remove(packageFilesPath + "/Form.qml");

    return true;
}

} // namespace CyberTracker