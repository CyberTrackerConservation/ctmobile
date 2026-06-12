#include "fxApplication.h"
#include "fxUtils.h"
#include "fxRegister.h"

//*************************************************************************************************
// CfxApplication

CfxApplication::CfxApplication(QObject *pParent, CfxHost *pHost, CfxEngine *pEngine, CfxScreen *pScreen): CfxPersistent(nullptr), _eventManager()
{
    setParent(pParent);

	RegisterControls();

    _host = pHost;
    _host->SetOwner(this);

    _engine = pEngine;
    _engine->SetOwner(this);

    _screen = pScreen;
    _screen->SetOwner(this);
}

CfxApplication::~CfxApplication()
{
    // Deleting the screen first so controls can still hit the engine and host as they die
    delete _screen;
    _screen = nullptr;

    delete _engine;
    _engine = nullptr;

    delete _host;
    _host = nullptr;
}

void CfxApplication::RegisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject, NotifyMethod Method)
{
    FXEVENT event = {(BYTE)Event, pObject, Method};
    _eventManager.Register(&event);
}

void CfxApplication::UnregisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject)
{
    _eventManager.Unregister(Event, pObject);
}

void CfxApplication::GetBounds(FXRECT *pBounds)
{
    pBounds->Left = 0;
    pBounds->Top = 0;
    pBounds->Right = _host->GetCanvas()->GetWidth() - 1;
    pBounds->Bottom = _host->GetCanvas()->GetHeight() - 1;
}

void CfxApplication::Startup()
{
    _eventManager.ExecuteType(aeStartup, this);
}

void CfxApplication::Shutdown()
{
    _eventManager.ExecuteType(aeShutdown, this);
}

void CfxApplication::ShowMenu()
{
    _eventManager.ExecuteType(aeShowMenu, this);
}

//*************************************************************************************************
// Utilitiy functions

CfxApplication *GetApplication(CfxPersistent *pPersistent)
{
    // Must have an owner
    if (!pPersistent || !pPersistent->GetOwner())
    {
        return nullptr;
    }

    CfxPersistent *p = pPersistent;
    while (p->GetOwner())
    {
        p = p->GetOwner();
    }

    FX_ASSERT(p && !p->GetOwner());

    return (CfxApplication *)p;
}

CfxHost *GetHost(CfxPersistent *pPersistent)
{
    CfxApplication *application = GetApplication(pPersistent);
    return application ? application->GetHost() : nullptr;
}

CfxEngine *GetEngine(CfxPersistent *pPersistent)
{
    CfxApplication *application = GetApplication(pPersistent);
    return application ? application->GetEngine() : nullptr;
}

CfxScreen *GetScreen(CfxPersistent *pPersistent)
{
    CfxApplication *application = GetApplication(pPersistent);
    return application ? application->GetScreen() : nullptr;
}

CfxCanvas *GetCanvas(CfxPersistent *pPersistent)
{
    CfxHost *host = GetHost(pPersistent);
    return host ? host->GetCanvas() : nullptr;
}

CfxScreen *GetParentScreen(CfxControl *pControl)
{
    CfxControl *p = pControl->GetParent();
    while (p && !CompareGuid(p->GetTypeId(), &GUID_0))
    {
        p = p->GetParent();
    }

    if (p)
    {
        return (CfxScreen *)p;
    }
    else
    {
        return nullptr;
    }
}

