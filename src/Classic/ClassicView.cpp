#include "ClassicView.h"
#include "ClassicConnector.h"
#include "ClassicProvider.h"
#include "ctRegister.h"
#include "ctSession.h"
#include "ctSighting.h"
#include "ctElement.h"
#include "fxHost.h"
#include "fxMemo.h"
#include "App.h"

ClassicView::ClassicView(QQuickItem *parent): QQuickPaintedItem(parent), m_form(nullptr)
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptTouchEvents(false);
    setFlag(QQuickItem::ItemHasContents, true);
    m_initialWidth = m_initialHeight = 0;
}

ClassicView::~ClassicView()
{
    disconnectSession();
}

void ClassicView::classBegin()
{
    QQuickPaintedItem::classBegin();
}

void ClassicView::componentComplete()
{
    QQuickPaintedItem::componentComplete();

    m_form = Form::parentForm(this);
    m_projectUid = m_form->projectUid();
    m_projectApp = m_form->project()->connectorParams()["app"].toString();
    m_projectDir = Utils::classicRoot();

    setWidth(m_initialWidth);
    setHeight(m_initialHeight);

    m_dpi = qMax(96, App::instance()->dpi());
    qDebug() << "Width: " << width() << " Height: " << height() << " DPI: " << m_dpi;

    int autoScale = qMin(500, qMax(100, m_dpi));
    autoScale = App::instance()->scaleByFontSize(autoScale);

    m_scaleX = m_scaleY = m_scaleBorder = autoScale;
    m_scaleHitSize = (42 * autoScale) / 100;
    m_scaleFont = App::instance()->settings()->fontSize();

    connect(m_form, &Form::connectedChanged, this, [&]()
    {
        if (m_form && m_form->connected())
        {
            connectSession();
        }
    });

    connect(m_form, &Form::beforeDisconnect, this, [&]()
    {
        disconnectSession();
    });
}

void ClassicView::connectSession()
{
    if ((width() > 0.0) && (height() > 0.0) && !m_projectDir.isEmpty())
    {
        connectSession(m_projectDir.toStdString().c_str(), m_projectApp.toStdString().c_str(), width() + 1, height() + 1, m_dpi, m_scaleX, m_scaleY, m_scaleBorder, m_scaleFont, m_scaleHitSize, false);
    }
}

void ClassicView::connectSession(const char *pRoot, const char *Name, int Width, int Height, int Dpi, int ScaleX, int ScaleY, int ScaleBorder, int ScaleFont, int ScaleHitSize, bool UseResourceScale)
{
    disconnectSession();

    // Create the host
    FXPROFILE profile = {};

    profile.Magic = MAGIC_PROFILE;
    profile.Width = Width;
    profile.Height = Height;
    profile.BitDepth = 16;
    profile.DPI = Dpi;
    profile.ScaleX = ScaleX;
    profile.ScaleY = ScaleY;
    profile.ScaleBorder = ScaleBorder;
    profile.ScaleFont = ScaleFont;
    profile.ScaleHitSize = ScaleHitSize;
    profile.Zoom = 100;
    profile.AliasIndex = 0;

    CfxHost *host = new CfxHost(nullptr, this, &profile, pRoot);
    host->SetBounds(0, 0, profile.Width, profile.Height);
    host->SetUseResourceScale(UseResourceScale);

    RegisterCTControls();

    m_application = new CfxApplication(this, host, new CfxEngine(nullptr), new CfxScreen(nullptr, nullptr));

    FXRECT rect;
    host->GetBounds(&rect);
    m_application->GetScreen()->SetBounds(rect.Left, rect.Top, rect.Right - rect.Left, rect.Bottom - rect.Top);

    m_session = new CctSession(m_application, m_application->GetScreen());
    m_session->SetDock(ControlDock::Fill);

    // Strip extension
    char Name2[MAX_PATH];
    strcpy(Name2, Name);
    char *t = strrchr(Name2, '.');
    if (t != nullptr)
    {
        *t = '\0';
    }

    m_application->GetEngine()->Startup();
    m_session->Connect(Name2, 0);
    m_session->Run();

    m_session->SaveState();
    m_session->GetTransferManager()->Update();

    buildSources();

    auto mapLayers = QVariantList();
    if (!m_gotoPoints.isEmpty())
    {
        mapLayers.append(QVariantMap {
            { "id", "classicGotos" },
            { "name", tr("Gotos") },
            { "color", "#0000FF" },
            { "outlineColor", "#000000" },
            { "pointSize", 12 },
            { "points", m_gotoPoints },
        });
    }

    if (!m_historyLines.isEmpty())
    {
        mapLayers.append(QVariantMap {
            { "id", "classicHistoryTrack" },
            { "name", tr("History track") },
            { "color", "#FFFF00" },
            { "lineWidth", 2 },
            { "lines", m_historyLines },
        });
    }

    if (!m_historyPoints.isEmpty())
    {
        mapLayers.append(QVariantMap {
            { "id", "classicHistory" },
            { "name", tr("History") },
            { "color", "#00FFFF" },
            { "outlineColor", "#000000" },
            { "pointSize", 12 },
            { "points", m_historyPoints },
        });
    }

    m_form->set_mapLayers(mapLayers);

    static_cast<ClassicProvider*>(m_form->provider())->registerClassicView(this);
}

void ClassicView::buildSources()
{
    auto session = m_session;
    auto resource = session->GetResource();

    // Gotos layer.
    for (int i = 0; i < session->GetResourceHeader()->GotosCount; i++)
    {
        auto g = resource->GetGoto(nullptr, i);
        if (g && (g->X != 0 || g->Y != 0))
        {
            m_gotoPoints.append(QVariantMap { { "title", QString(g->Title) }, { "x", g->X }, { "y", g->Y } });
        }
    }

    // History layer.
    HISTORY_ITEM* historyItemDef;
    if (session->EnumHistoryInit(&historyItemDef))
    {
        // Collect column names from the def record.
        auto columnNames = QStringList();
        for (int c = 0; c < historyItemDef->ColumnCount; c++)
        {
            auto row = session->GetHistoryItemRow(historyItemDef, c);
            columnNames.append(QString::fromLatin1((const char*)row, strnlen((const char*)row, sizeof(HISTORY_ROW))));
        }

        HISTORY_ITEM* historyItem;
        while (session->EnumHistoryNext(&historyItem))
        {
            if (historyItem->X != 0 || historyItem->Y != 0)
            {
                auto properties = QVariantList();
                for (int c = 0; c < historyItem->ColumnCount; c++)
                {
                    auto row = session->GetHistoryItemRow(historyItem, c);
                    auto value = QString::fromLatin1((const char*)row, strnlen((const char*)row, sizeof(HISTORY_ROW)));
                    if (!value.isEmpty())
                    {
                        properties.append(QVariantMap { { "name", columnNames.value(c) }, { "value", value } });
                    }
                }

                m_historyPoints.append(QVariantMap { { "x", historyItem->X }, { "y", historyItem->Y }, { "properties", properties } });
            }
        }
        session->EnumHistoryDone();

        // History track waypoints (stored after history items in .HST file).
        HISTORY_ITEM* historyItemDef2;
        if (session->EnumHistoryInit(&historyItemDef2))
        {
            auto host = m_application->GetHost();
            auto historyItemSize = sizeof(HISTORY_ITEM) + historyItemDef2->ColumnCount * 32;
            if (host->EnumRecordSetIndex(historyItemDef2->RecordCount + 1, historyItemSize))
            {
                auto currentLine = QVariantList();
                HISTORY_WAYPOINT wp;
                while (host->EnumRecordNext(&wp, sizeof(wp)))
                {
                    if (wp.X == 0 && wp.Y == 0) continue;

                    if (wp.Flags == 1 && !currentLine.isEmpty())
                    {
                        m_historyLines.append(QVariant(currentLine));
                        currentLine.clear();
                    }
                    currentLine.append(QVariantMap { { "x", wp.X }, { "y", wp.Y } });
                }
                if (!currentLine.isEmpty())
                {
                    m_historyLines.append(QVariant(currentLine));
                }
            }
            session->EnumHistoryDone();
        }
    }
}

void ClassicView::disconnectSession()
{
    if (m_application == nullptr)
    {
        return;
    }

    // Classic disables the track on close.
    if (m_form->trackStreamer()->running())
    {
        m_form->trackStreamer()->stop();
        m_form->saveState();
        App::instance()->garbageCollectStreamers();
    }

    if (m_form)
    {
        static_cast<ClassicProvider*>(m_form->provider())->unregisterClassicView();
        m_form->set_mapLayers({});
    }

    m_application->Shutdown();
    delete m_session;
    m_session = nullptr;
    delete m_application;
    m_application = nullptr;

    m_form = nullptr;
}

bool ClassicView::canSubmitData() const
{
    if (m_application == nullptr) return false;

    auto transferManager = m_session->GetTransferManager();
    return transferManager->HasTransferData() ||
           transferManager->HasOutgoingData() ||
           transferManager->HasExportData();
}

bool ClassicView::submitData()
{
    if (m_application == nullptr) return false;

    auto transferManager = m_session->GetTransferManager();

    if (!canSubmitData())
    {
        return false;
    }

    auto result = false;

    if (transferManager->HasTransferData() || transferManager->HasExportData())
    {
        result = transferManager->CreateTransfer();
    }
    else if (transferManager->HasOutgoingData())
    {
        result = true;
    }

    if (result)
    {
        m_session->Repaint();
    }

    emit App::instance()->sightingsChanged(m_form->projectUid());

    return result;
}

void ClassicView::loadState()
{
    auto provider = static_cast<ClassicProvider*>(m_form->provider());

    session()->LoadStateBlob(provider->stateBlob());
}

void ClassicView::saveState()
{
    auto provider = static_cast<ClassicProvider*>(m_form->provider());

    auto attachments = QStringList();
    provider->set_stateBlob(session()->SaveStateBlob(&attachments));
    provider->set_stateAttachments(attachments);

    m_form->saveState();
}

QVariantList buildClassicData(CctSighting* pSighting, CfxResource* pResource)
{
    auto app = App::instance();
    auto result = QVariantList();

    auto attributes = pSighting->GetAttributes();
    for (auto i = 0; i < (int)attributes->GetCount(); i++)
    {
        ATTRIBUTE* attribute = attributes->GetPtr(i);

        auto value = QString();
        if (!IsNullXGuid(&attribute->ValueId))
        {
            if (pResource)
            {
                FXTEXTRESOURCE* element = (FXTEXTRESOURCE*)pResource->GetObject(nullptr, attribute->ValueId, (BYTE)ElementAttribute::Name);
                if (element != nullptr)
                {
                    value = GUIDToString(&element->Guid);
                    pResource->ReleaseObject(element);
                }
            }
        }
        else
        {
            Type_VariantToText(attribute->Value, value);
        }

        auto map = QVariantMap
        {
            { "elementUid", GUIDToString(&attribute->ElementGuid) },
            { "type", (int)attribute->Value->Type },
            { "value", value },
        };

        auto filename = QString(attribute->GetMediaName());
        if (!filename.isEmpty())
        {
            auto modernPath = app->getMediaFilePath(filename);

            // Bootstrap: legacy sightings reference files still in classicRoot/Media/.
            // Move without renaming so classicBlob's reference stays valid.
            if (!QFile::exists(modernPath))
            {
                auto classicPath = Utils::classicRoot() + "/Media/" + filename;
                if (QFile::exists(classicPath))
                {
                    QFile::rename(classicPath, modernPath);
                }
            }

            auto mimeTypePrefix = Utils::mimeDatabase()->mimeTypeForFile(modernPath).name().split("/").constFirst();
            map["attachment"] = filename;

            if (mimeTypePrefix == "image")
            {
                map["fieldValue"] = PhotoField::createValue(filename);
            }
            else if (mimeTypePrefix == "audio")
            {
                auto v = AudioField::createValue(filename).toMap();
                v["duration"] = Utils::waveFileComputeDuration(modernPath);
                map["fieldValue"] = v;
            }
            else
            {
                qDebug() << "Error: unknown media type - " << mimeTypePrefix;
            }
        }

        result.append(map);
    }

    return result;
}

std::unique_ptr<Sighting> buildSighting(Form* form, CctSighting* pSighting, const QVariantList& classicData, const QByteArray& classicBlob)
{
    auto app = App::instance();

    auto sightingUid = GUIDToString(pSighting->GetUniqueId());
    auto sighting = form->createSightingPtr();
    sighting->recordManager()->rootRecord()->setRecordUid(sightingUid);
    sighting->recalculate();

    // Set the timestamp.
    auto dateTime = pSighting->GetDateTime();
    auto isoTimestamp = QString();
    if (dateTime->Year != 0)
    {
        auto timestamp = CfxHost::GetDateStringUTC(dateTime, false);
        isoTimestamp = QString(timestamp);
        MEM_FREE(timestamp);
        timestamp = nullptr;
    }
    else
    {
        isoTimestamp = app->timeManager()->currentDateTimeISO();
    }

    sighting->set_createTime(isoTimestamp);
    sighting->set_updateTime(isoTimestamp);

    // Set the location.
    auto gps = pSighting->GetGPS();
    if (gps->Quality > FXGPS_QUALITY::None)
    {
        auto location = Location();
        location.set_x(gps->Longitude);
        location.set_y(gps->Latitude);
        location.set_z(gps->Altitude);
        location.set_speed(gps->Speed);
        location.set_accuracy(gps->Accuracy * 10);
        location.set_timestamp(isoTimestamp);

        sighting->setRootFieldValue("location", location.toMap());
    }

    // Fill in the root record fields from the enriched attribute list.
    for (auto attributeIt = classicData.constBegin(); attributeIt != classicData.constEnd(); attributeIt++)
    {
        auto attribute = attributeIt->toMap();
        auto fieldUid = attribute["elementUid"].toString();
        auto dataType = (FXDATATYPE)attribute["type"].toInt();
        auto fieldValue = attribute["fieldValue"];

        if (fieldValue.isValid())
        {
            sighting->setRootFieldValue(fieldUid, fieldValue);
        }
        else if (dataType == dtNull)
        {
            sighting->setRootFieldValue(fieldUid, " ");
        }
        else
        {
            sighting->setRootFieldValue(fieldUid, attribute["value"].toString());
        }
    }

    sighting->set_classicData(classicData);
    sighting->set_classicBlob(classicBlob);

    sighting->recalculate();

    return sighting;
}

Sighting* ClassicView::makeSighting(Form* form, CctSighting* pSighting, CfxResource* resource)
{
    auto classicData = buildClassicData(pSighting, resource);
    auto classicBlob = QByteArray();
    pSighting->SaveBlob(classicBlob);

    auto sighting = buildSighting(form, pSighting, classicData, classicBlob);

    return sighting.release();
}

void ClassicView::saveSighting(CctSighting* pSighting)
{
    auto sighting = std::unique_ptr<Sighting>(makeSighting(m_form, pSighting, m_session->GetResource()));

    auto sightingUid = sighting->rootRecordUid();

    // If the track is running, push the sighting's GPS fix into the stream so
    // the sighting lands on the track line. Skip drafts, and skip manually
    // entered locations (LocationPage leaves altitude and accuracy at zero).
    if (m_form->trackStreamer()->running())
    {
        auto gps = pSighting->GetGPS();
        if (gps->Quality > FXGPS_QUALITY::None && (gps->Altitude != 0 || gps->Accuracy != 0))
        {
            auto location = Location();
            location.set_x(gps->Longitude);
            location.set_y(gps->Latitude);
            location.set_z(gps->Altitude);
            location.set_accuracy(gps->Accuracy);
            location.set_timestamp(sighting->createTime());

            m_form->trackStreamer()->pushUpdate(QUuid::createUuid().toString(QUuid::Id128), &location);
        }
    }

    m_form->saveSighting(sighting.get(), true);

    // Mark completed: draft is incomplete.
    m_form->setSightingFlag(sightingUid, Database::DataFlag::DB_COMPLETED_FLAG, true);

    // A save (including edits) invalidates any previous upload — the row needs
    // to be re-sent on the next transfer.
    m_form->setSightingFlag(sightingUid, Database::DataFlag::DB_UPLOADED_FLAG, false);

    // Send web hook — skip for draft snapshots.
    auto app = App::instance();
    if (!app->settings()->webhookUrl().isEmpty())
    {
        auto project = app->projectManager()->loadPtr(m_form->projectUid());

        auto queueItem = Database::DbQueueItem();
        queueItem.projectUid = project->uid();
        queueItem.dataUid = sightingUid;
        queueItem.summary = QString("%1: %2").arg(QObject::tr("Webhook"), sighting->summaryText());
        queueItem.type = ClassicConnector::UPLOAD_WEBHOOK_ONLY;
        queueItem.webhook = m_form->renderer()->getSightingWebhook(project.get(), sighting.get());
        app->uploadQueue()->addItem(queueItem);
    }
}

void ClassicView::editSighting(const QString& sightingUid)
{
    if (m_application == nullptr) return;

    auto session = m_session;
    session->SaveState();
    session->EditSighting(sightingUid);
}

void ClassicView::deleteSighting(const QString& sightingUid)
{
    if (m_application == nullptr) return;

    auto session = m_session;
    session->DeleteSighting(sightingUid);
}

QString ClassicView::getTextEditValue(int controlId)
{
    if (m_application == nullptr) return {};

    auto session = m_session;
    auto sourceControl = (CfxControl_Memo *)(session->GetWindow()->FindControl(controlId));
    if (sourceControl)
    {
        auto caption = sourceControl->GetCaption();
        return QString::fromUtf8(caption ? caption : "");
    }

    return {};
}

void ClassicView::setTextEditValue(int controlId, const QString& text)
{
    if (m_application == nullptr) return;

    auto session = m_session;

    auto trimmedText = text.trimmed();
    char *clonedText = AllocString(trimmedText.toUtf8().constData());

    session->Reconnect(QString(), false);

    auto sourceControl = (CfxControl_Memo *)(session->GetWindow()->FindControl(controlId));
    if (sourceControl)
    {
        sourceControl->SetCaption(clonedText);
    }
    MEM_FREE(clonedText);

    session->SaveState();
    session->Repaint();
}

void ClassicView::confirmResult(int controlId, bool confirmed)
{
    if (m_application == nullptr) return;

    auto session = m_session;
    auto sourceControl = session->GetWindow()->FindControl(controlId);
    if (sourceControl)
    {
        GUID typeId = {};
        sourceControl->OnDialogResult(&typeId, (void *)(intptr_t)(confirmed ? 1 : 0));
    }
}

void ClassicView::setImage(QImage* image)
{
    m_imageLock.lock();
    m_image = *image;
    m_imageLock.unlock();
}

void ClassicView::paint(QPainter *painter)
{
    if (!m_application)
    {
        return;
    }

    if (m_imageLock.tryLock())
    {
        painter->drawImage(0, 0, m_image);
        m_imageLock.unlock();
    }
}

void ClassicView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);

    if (!m_application)
    {
        return;
    }

    if (newGeometry.size() != oldGeometry.size())
    {
        int w = newGeometry.width() + 1;
        int h = newGeometry.height() + 1;
        auto host = m_application->GetHost();
        host->SetBounds(0, 0, w, h);
        m_application->GetScreen()->SetBounds(0, 0, w, h);
        m_application->GetEngine()->AlignControls(m_application->GetScreen());
        host->Paint();
    }
}

void ClassicView::touchEvent(QTouchEvent *event)
{
    QQuickPaintedItem::touchEvent(event);
}

void ClassicView::mousePressEvent(QMouseEvent *event)
{
    QQuickPaintedItem::mousePressEvent(event);
    if (event->button() == Qt::LeftButton)
    {
        if (m_application)
        {
            event->accept();
            m_application->GetEngine()->PenDown(event->pos().x(), event->pos().y());
        }
    }
}

void ClassicView::mouseMoveEvent(QMouseEvent *event)
{
    QQuickPaintedItem::mouseMoveEvent(event);
    if (m_application)
    {
        event->accept();
        m_application->GetEngine()->PenMove(event->pos().x(), event->pos().y());
    }
}

void ClassicView::mouseReleaseEvent(QMouseEvent *event)
{
    QQuickPaintedItem::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton)
    {
        if (m_application)
        {
            event->accept();
            m_application->GetEngine()->PenUp(event->pos().x(), event->pos().y());
        }
    }
}

void ClassicView::wheelEvent(QWheelEvent *event)
{
    QQuickPaintedItem::wheelEvent(event);
}

void ClassicView::hoverMoveEvent(QHoverEvent *event)
{
    QQuickPaintedItem::hoverMoveEvent(event);
}

void ClassicView::keyPressEvent(QKeyEvent * event)
{
    QQuickPaintedItem::keyPressEvent(event);
    if (m_application)
    {
        m_application->GetEngine()->KeyDown(event->key());
    }
}

void ClassicView::keyReleaseEvent(QKeyEvent * event)
{
    QQuickPaintedItem::keyReleaseEvent(event);

    if (m_application)
    {
        m_application->GetEngine()->KeyUp(event->key());
    }
}

ClassicView *GetClassicView(CfxPersistent *pPersistent)
{
    CfxApplication *application = GetApplication(pPersistent);
    return application ? qobject_cast<ClassicView*>(application->parent()) : nullptr;
}
