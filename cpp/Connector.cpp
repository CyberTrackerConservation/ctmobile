#include "Connector.h"
#include "App.h"

Connector::Connector(QObject* parent): QObject(parent)
{
    m_database = App::instance()->database();
    m_projectManager = App::instance()->projectManager();
}

bool Connector::canExportData(Project* /*project*/) const
{
    return false;
}

bool Connector::canSharePackage(Project* /*project*/) const
{
    return false;
}

bool Connector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap Connector::getShareData(Project* /*project*/, bool /*auth*/) const
{
    return QVariantMap();
}

bool Connector::canLogin(Project* /*project*/) const
{
    return false;
}

bool Connector::loggedIn(Project* /*project*/) const
{
    return true;
}

bool Connector::login(Project* /*project*/, const QString& /*server*/, const QString& /*username*/, const QString& /*password*/)
{
    return true;
}

void Connector::logout(Project* /*project*/)
{
}

ApiResult Connector::bootstrap(const QVariantMap& /*params*/)
{
    return Failure(tr("Not implemented"));
}

bool Connector::refreshAccessToken(QNetworkAccessManager* /*networkAccessManager*/, Project* /*project*/)
{
    return false;
}

ApiResult Connector::hasUpdate(QNetworkAccessManager* /*networkAccessManager*/, Project* /*project*/)
{
    return Failure(tr("Not implemented"));
}

bool Connector::canUpdate(Project* /*project*/) const
{
    return false;
}

ApiResult Connector::update(Project* /*project*/)
{
    return Failure(tr("Not implemented"));
}

void Connector::reset(Project* /*project*/)
{
}

void Connector::remove(Project* /*project*/)
{
}

ApiResult Connector::ExpectedError(const QString& errorString)
{
    return ApiResult { false, errorString, true };
}

ApiResult Connector::Failure(const QString& errorString)
{
    return ApiResult { false, errorString, false };
}

ApiResult Connector::Success(const QString& errorString)
{
    return ApiResult { true, errorString, true };
}

ApiResult Connector::AlreadyUpToDate()
{
    return ApiResult { false, tr("Up to date"), true };
}

ApiResult Connector::AlreadyConnected()
{
    return ApiResult { false, tr("Already connected"), true };
}

ApiResult Connector::UpdateAvailable()
{
    return ApiResult { true, tr("Update available"), true };
}

ApiResult Connector::bootstrapUpdate(const QString& projectUid)
{
    auto result = ApiResult::fromMap(m_projectManager->update(projectUid, false));
    if (!result.success)
    {
        m_projectManager->remove(projectUid);
        return Failure(result.errorString);
    }

    return Success();
}
