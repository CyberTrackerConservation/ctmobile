/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page,    *
//   located at: http://www.            .com.                                                     *
//                                                                                                *
//   The contents of this file are used subject to the Mozilla Public License Version 1.1         *
//   (the "License"); you may not use this file except in compliance with the License. You may    *
//   obtain a copy of the License from http://www.mozilla.org/MPL.                                *
//                                                                                                *
//   Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY  *
//   OF ANY KIND, either express or implied. See the License for the specific language governing  *
//   rights and limitations under the License.                                                    *
//                                                                                                *
//************************************************************************************************/

#include "fxApplication.h"
#include "fxUtils.h"
#include "fxRegister.h"

//*************************************************************************************************
// CfxApplication

CfxApplication::CfxApplication(CfxHost *pHost, CfxEngine *pEngine, CfxScreen *pScreen): CfxPersistent(NULL), _eventManager()
{
	RegisterControls();

    _live = TRUE;
    _kioskMode = FALSE;
    _demoMode = FALSE;

    _host = pHost;
    _host->SetOwner(this);
    _host->CreateGuid(&_id);

    _engine = pEngine;
    _engine->SetOwner(this);

    _screen = pScreen;
    _screen->SetOwner(this);
}

CfxApplication::~CfxApplication()
{
    // Reset the kiosk mode
    SetKioskMode(FALSE);

    // Deleting the screen first so controls can still hit the engine and host as they die
    delete _screen;
    delete _engine;
    delete _host;
}

VOID CfxApplication::RegisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject, NotifyMethod Method)
{
    FXEVENT event = {Event, pObject, Method};
    _eventManager.Register(&event);
}

VOID CfxApplication::UnregisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject)
{
    _eventManager.Unregister(Event, pObject);
}

VOID CfxApplication::GetBounds(FXRECT *pBounds)
{
    pBounds->Left = 0;
    pBounds->Top = 0;
    pBounds->Right = _host->GetCanvas()->GetWidth() - 1;
    pBounds->Bottom = _host->GetCanvas()->GetHeight() - 1;
}

VOID CfxApplication::Startup()
{
    _eventManager.ExecuteType(aeStartup, this);
}

VOID CfxApplication::Shutdown()
{
    _eventManager.ExecuteType(aeShutdown, this);
}

VOID CfxApplication::ShowMenu()
{
    _eventManager.ExecuteType(aeShowMenu, this);
}

VOID CfxApplication::SetKioskMode(BOOL Value) 
{ 
    if ((_host != NULL) && (_kioskMode != Value))
    {
        _kioskMode = Value;
        _host->SetKioskMode(Value);
    }
}

//*************************************************************************************************
// Utilitiy functions

CfxApplication *GetApplication(CfxPersistent *pPersistent)
{
    // Must have an owner
    if (!pPersistent || !pPersistent->GetOwner())
    {
        return NULL;
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
    return application ? application->GetHost() : NULL;
}

CfxEngine *GetEngine(CfxPersistent *pPersistent)
{
    CfxApplication *application = GetApplication(pPersistent);
    return application ? application->GetEngine() : NULL;
}

CfxScreen *GetScreen(CfxPersistent *pPersistent)
{
    CfxApplication *application = GetApplication(pPersistent);
    return application ? application->GetScreen() : NULL;
}

CfxCanvas *GetCanvas(CfxPersistent *pPersistent)
{
    CfxHost *host = GetHost(pPersistent);
    return host ? host->GetCanvas() : NULL;
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
        return NULL;
    }
}

BOOL GetApplicationLive(CfxPersistent *pPersistent)
{
    CfxApplication * application = GetApplication(pPersistent);

    return (application != NULL) && 
           application->GetLive();
}
