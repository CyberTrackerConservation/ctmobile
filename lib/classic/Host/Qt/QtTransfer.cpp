#include "QtTransfer.h"
#include "fxUtils.h"
#include "fxApplication.h"
#include <Utils.h>
#include <cstdio>

//=====================================================================================================================

struct TransferItem
{
    QString fileNameOut;
    QString fileNameData;
    FXSENDDATA sendData;
};

QList<TransferItem> buildTransferItems(const QString& queuePath)
{
    auto result = QList<TransferItem>();

    auto dirToBeScanned = QDir(queuePath);
    auto filterExtensions = QStringList {{ "*.OUT" }};

    QFileInfoList fileInfoList(dirToBeScanned.entryInfoList(filterExtensions, QDir::Files));
    for (auto fileInfo: fileInfoList)
    {
        auto transferItem = TransferItem();

        auto filenameOUT = fileInfo.filePath();

        auto f = fopen(filenameOUT.toStdString().c_str(), "rb");
        if (f == nullptr)
        {
            qDebug() << "Failed to open " << filenameOUT;
            continue;
        }

        if (fread(&transferItem.sendData, sizeof(FXSENDDATA), 1, f) != 1)
        {
            qDebug() << "Failed to read " << filenameOUT;
            fclose(f);
            continue;
        }

        fclose(f);

        if (transferItem.sendData.Magic != MAGIC_SENDDATA)
        {
            qDebug() << "Bad OUT file " << fileInfo.path();
            continue;
        }

        switch (transferItem.sendData.Protocol)
        {
        case FXSENDDATA_PROTOCOL_UNC:
        case FXSENDDATA_PROTOCOL_FTP:
        case FXSENDDATA_PROTOCOL_BACKUP:
            transferItem.fileNameData = QString(filenameOUT).replace(".OUT", ".CTX");
            break;

        case FXSENDDATA_PROTOCOL_HTTP:
        case FXSENDDATA_PROTOCOL_HTTPS:
        case FXSENDDATA_PROTOCOL_HTTPZ:
        case FXSENDDATA_PROTOCOL_ESRI:
            transferItem.fileNameData = QString(filenameOUT).replace(".OUT", ".JSON");
            break;

        default:
            continue;
        }

        if (!QFile::exists(transferItem.fileNameData))
        {
            qDebug() << "No matching data file for OUT file";
            continue;
        }

        transferItem.fileNameOut = filenameOUT;
        result.append(transferItem);
    }

    return result;
}

//=====================================================================================================================
// Transfer thread.

class TransferThread: public QThread
{
public:
    TransferThread(QtTransfer* transfer, const QList<TransferItem>& items): QThread()
    {
        m_transfer = transfer;
        m_items = items;
    }

    void run() override
    {
        auto successCount = 0;
        auto failCount = 0;

        for (auto item: m_items)
        {
            if (!m_transfer->active())
            {
                break;
            }

            emit m_transfer->stateChange(FXSENDSTATEMODE::SS_SENDING);

            auto success = false;
            switch (item.sendData.Protocol)
            {
            case FXSENDDATA_PROTOCOL_UNC:
            //case FXSENDDATA_PROTOCOL_BACKUP:
                success = m_transfer->sendDataUNC(item.fileNameData, item.sendData.Url, item.sendData.UserName, item.sendData.Password);
                break;

            case FXSENDDATA_PROTOCOL_FTP:
                success = m_transfer->sendDataFTP(item.fileNameData, item.sendData.Url, item.sendData.UserName, item.sendData.Password);
                break;

            case FXSENDDATA_PROTOCOL_HTTP:
            case FXSENDDATA_PROTOCOL_HTTPS:
                success = m_transfer->sendDataHTTP(item.fileNameData, item.sendData.Url, item.sendData.UserName, item.sendData.Password, false);
                break;

            case FXSENDDATA_PROTOCOL_HTTPZ:
                success = m_transfer->sendDataHTTP(item.fileNameData, item.sendData.Url, item.sendData.UserName, item.sendData.Password, true);
                break;

            case FXSENDDATA_PROTOCOL_ESRI:
                {
                    auto secureUrl = QString(item.sendData.Url).replace("http://", "https://", Qt::CaseInsensitive);
                    success = m_transfer->sendDataESRI(item.fileNameData, secureUrl, item.sendData.UserName, item.sendData.Password);
                }
                break;

            default:
                continue;
            }

            if (success)
            {
                successCount++;
                QFile::remove(item.fileNameOut);
                QFile::remove(item.fileNameData);
            }
            else
            {
                failCount++;
                emit m_transfer->stateChange(FXSENDSTATEMODE::SS_FAILED_SEND);
            }

            Utils::mediaScan(item.fileNameOut);
            Utils::mediaScan(item.fileNameData);
        }

        emit m_transfer->stateChange(FXSENDSTATEMODE::SS_COMPLETED);
        emit m_transfer->stateChange(FXSENDSTATEMODE::SS_IDLE);
    }

private:
    QtTransfer* m_transfer;
    QList<TransferItem> m_items;
};

//=====================================================================================================================
// QtTransfer.

QtTransfer::QtTransfer(QObject* parent): QObject(parent), m_active(false)
{

}

QtTransfer::~QtTransfer()
{
    stop();
}

void QtTransfer::start(const QString& queuePath)
{
    stop();

    m_arcGisClient.reset();

    auto items = buildTransferItems(queuePath);
    if (items.isEmpty())
    {
        emit stateChange(FXSENDSTATEMODE::SS_COMPLETED);
        emit stateChange(FXSENDSTATEMODE::SS_IDLE);
        return;
    }

    m_active = true;
    emit stateChange(FXSENDSTATEMODE::SS_CONNECTING);
    m_thread = new TransferThread(this, items);
    m_thread->start();
}

void QtTransfer::stop()
{
    if (m_thread) {
        m_active = false;

        while (m_thread->isRunning())
        {
            QEventLoop eventLoop;
            QTimer::singleShot(100, &eventLoop, &QEventLoop::quit);
            eventLoop.exec();
        }

        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;

        emit stateChange(FXSENDSTATEMODE::SS_STOPPED);
    }
}

bool QtTransfer::sendDataHTTP(const QString& filename, const QString& url, const QString& username, const QString& password, bool compressed)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly))
    {
        return false;
    }

    auto payload = file.readAll();

    QNetworkAccessManager networkAccessManager;
    auto request = QNetworkRequest();
    QEventLoop eventLoop;
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");

    if (!username.isEmpty() && !password.isEmpty())
    {
        auto concatenated = username + ":" + password;
        auto data = concatenated.toLocal8Bit().toBase64();
        auto headerData = QString("Basic " + data).toLocal8Bit();
        request.setRawHeader("Authorization", headerData);
    }

    request.setRawHeader("Content-Type", "application/json; charset=utf-8");
    request.setUrl(url);

    if (compressed)
    {
        request.setRawHeader("Content-Encoding", "deflate");
    }

    auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager.post(request, payload));
    reply->ignoreSslErrors();

    QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    auto result = Utils::HttpResponse();

    result.success = reply->error() == QNetworkReply::NoError;
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
    result.location = reply->header(QNetworkRequest::LocationHeader).toString();
    result.errorString = reply->errorString();
    result.data = QByteArray(reply->readAll());

    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.errorString;
        qDebug() << "HTTP error: " << result.data;
    }

    return result.success;
}

QStringList buildFileList(const QString& path, const QString& prefix)
{
    auto result = QStringList();

    auto dir = QDir(path);
    auto fileInfoList = QFileInfoList(dir.entryInfoList({"*"}, QDir::Files));
    for (auto fileInfo: fileInfoList)
    {
        if (fileInfo.fileName().startsWith(prefix))
        {
            result.append(fileInfo.filePath());
        }
    }

    return result;
}

bool QtTransfer::sendDataESRI(const QString& filename, const QString& url, const QString& username, const QString& password)
{
    auto requireAuth = m_arcGisClient.accessToken().isEmpty() ||
                       m_arcGisClient.username() != username ||
                       m_arcGisClient.password() != password;
    if (requireAuth)
    {
        m_arcGisClient.set_username(username);
        m_arcGisClient.set_password(password);

        emit stateChange(FXSENDSTATEMODE::SS_AUTH_START);
        if (!m_arcGisClient.authorizeToken())
        {
            emit stateChange(FXSENDSTATEMODE::SS_AUTH_FAILURE);
            return false;
        }
    }

    emit stateChange(FXSENDSTATEMODE::SS_AUTH_SUCCESS);

    QFileInfo fileInfo(filename);

    auto startLayerId = filename.lastIndexOf('-');
    auto stopLayerId = filename.lastIndexOf('.');
    if ((startLayerId == -1) || (stopLayerId == -1) || (startLayerId >= stopLayerId))
    {
        qDebug() << "Bad layer id 1";
        return false;
    }

    auto layerId = filename.mid(startLayerId + 1, stopLayerId - startLayerId - 1);
    if (layerId.length() != 1)
    {
        qDebug() << "Bad layer id 2";
        return false;
    }

    auto objectId = 0;
    auto objectIdFilename = QString("");
    auto basePath = fileInfo.path();

    // Get object id if we already uploaded this sighting and still need to upload attachments
    auto mediaFiles = buildFileList(basePath, fileInfo.baseName() + "-");

    if (mediaFiles.count() > 0)
    {
        auto objectIdFiles = buildFileList(basePath, fileInfo.fileName() + ".OID.");
        if (objectIdFiles.count() > 0)
        {
            objectIdFilename = objectIdFiles[0];
            auto objectIdString = objectIdFilename.mid(objectIdFilename.lastIndexOf('.') + 1);
            objectId = objectIdString.toInt();
            if (objectId == 0)
            {
                qDebug() << "Bad object id file";
                return false;
            }
        }
    }

    emit stateChange(FXSENDSTATEMODE::SS_SENDING);

    // Upload the sighting if we have not already done so.
    if (objectId == 0)
    {
        auto response = m_arcGisClient.sendData(url, layerId.toInt(), filename);
        if (!response.success)
        {
            qDebug() << "Failed to send: " + response.errorMessage;
            return false;
        }

        objectId = response.results[0].objectId;
        if (objectId == 0)
        {
            qDebug() << "Bad object id returned";
            return false;
        }
    }

    // Upload attachments.
    if ((objectId > 0) && (mediaFiles.count() > 0))
    {
        objectIdFilename = filename + ".OID." + QString::number(objectId);
        {
            QFile objectIdFile(objectIdFilename);
            if (!objectIdFile.open(QIODevice::WriteOnly))
            {
                qDebug() << "Failed to write marker file";
                return false;
            }
        }

        for (int i = 0; i < mediaFiles.count(); i++)
        {
            auto mediaFile = mediaFiles[i];
            auto response = m_arcGisClient.sendAttachment(url, layerId.toInt(), objectId, mediaFile);
            if (!response.success)
            {
                qDebug() << "Failed to add attachment: " + response.errorMessage;
                return false;
            }

            if (!QFile::remove(mediaFile))
            {
                qDebug() << "Failed to delete attachment file: " << mediaFiles[i];
                return false;
            }
        }

        if (!objectIdFilename.isEmpty())
        {
            QFile::remove(objectIdFilename);
        }
    }

    return true;
}

bool QtTransfer::sendDataFTP(const QString& filename, const QString& url, const QString& username, const QString& password)
{
    if (url.isEmpty())
    {
        return false;
    }

    auto finalUrl = url;
    if (finalUrl[finalUrl.length() - 1] != '/')
    {
        finalUrl += "/";
    }

    QFile file(filename);
    QFileInfo fileInfo(file);

    auto serverUrl = QUrl(finalUrl + fileInfo.fileName());
    serverUrl.setUserName(username);
    serverUrl.setPassword(password);

    if (finalUrl.startsWith("ftps:"))
    {
        serverUrl.setPort(22);
    }
    else
    {
        serverUrl.setPort(21);
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QNetworkAccessManager manager;
    auto request = QNetworkRequest();
    request.setUrl(serverUrl);

    auto reply = manager.put(request, &file);
    reply->ignoreSslErrors();

    QEventLoop eventLoop;
    auto uploadSuccess = false;
    auto errorCode = 404;
    connect(reply, &QNetworkReply::finished, this, [&]()
    {
        errorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        uploadSuccess = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        eventLoop.exit();
    });

    eventLoop.exec();

    return uploadSuccess;
}

bool QtTransfer::sendDataUNC(const QString& filename, const QString& url, const QString& /*username*/, const QString& /*password*/)
{
    if (url.isEmpty())
    {
        return false;
    }

    auto finalUrl = url;
    finalUrl.replace('\\', '/');
    if (finalUrl[finalUrl.length() - 1] != '/')
    {
        finalUrl += "/";
    }

    return QFile::copy(filename, finalUrl + QFileInfo(filename).fileName());
}
