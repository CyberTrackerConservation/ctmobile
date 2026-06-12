#include "ClassicProvider.h"
#include "ClassicConnector.h"
#include "ctSession.h"
#include "Form.h"

namespace CyberTracker {

ClassicProvider::ClassicProvider(Form* form) : Provider(form), m_classicView(nullptr), m_currentSighting(nullptr)
{
    update_name(PROVIDER_NAME);

    // BUGBUG: Force a rebuild.
    form->project()->set_lastBuildString("");
}

ClassicProvider::~ClassicProvider()
{
    delete m_currentSighting;
    m_currentSighting = nullptr;
}

int ClassicProvider::version()
{
    return RESOURCE_VERSION;
}

void ClassicProvider::registerClassicView(ClassicView* classicView)
{
    m_classicView = classicView;
}

void ClassicProvider::unregisterClassicView()
{
    m_classicView = nullptr;
}

Sighting* ClassicProvider::currentSighting()
{
    delete m_currentSighting;
    m_currentSighting = nullptr;
    update_currentSightingSummaryText({});

    if (m_classicView)
    {
        m_currentSighting = m_classicView->makeSighting(
            m_form,
            m_classicView->session()->GetCurrentSighting(),
            m_classicView->session()->GetResource());

        update_currentSightingSummaryText(m_form->renderer()->getSightingSummaryText(m_currentSighting));
    }

    return m_currentSighting;
}

bool ClassicProvider::supportLocationTrackField() const
{
    return true;
}

bool ClassicProvider::connectToProject()
{
    // Bootstrap ESRI OAuth tokens from resource.
    ClassicConnector().bootstrapEsriLogin(m_form->project());

    return true;
}

QVariantMap ClassicProvider::saveState(QStringList* attachments)
{
    *attachments += m_stateAttachments;

    return QVariantMap {{ "stateBlob", m_stateBlob }};
}

void ClassicProvider::loadState(const QVariantMap& data)
{
    m_stateBlob = data.value("stateBlob").toByteArray();
    m_stateAttachments.clear();
}

bool ClassicProvider::resetPageStackOnConnect() const
{
    return true;
}

QString ClassicProvider::getStartPage() const
{
    return "qrc:/Classic/StartPage.qml";
}

bool ClassicProvider::canSnapTrackToNewSighting() const
{
    return false;
}

bool ClassicProvider::canSubmitData() const
{
    if (m_classicView)
    {
        return m_classicView->canSubmitData();
    }

    return false;
}

void ClassicProvider::submitData()
{
    if (m_classicView)
    {
        m_classicView->submitData();
    }
}

void ClassicProvider::editSighting(const QString& /*stateSpace*/, const QString& sightingUid) const
{
    if (m_classicView)
    {
        m_classicView->editSighting(sightingUid);
        m_form->popPagesToStart();
    }
}

} // namespace CyberTracker
