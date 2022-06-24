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

#pragma once
#include "pch.h"
#include "fxHost.h"
#include "fxEngine.h"
#include "fxControl.h"


enum APPLICATIONEVENT
{
    aeStartup,
    aeShutdown,
    aeShowMenu
};

//
// CfxApplication
//
class CfxApplication: public CfxPersistent
{
protected:
    GUID _id;
    BOOL _live;
    BOOL _kioskMode;
    BOOL _demoMode;

    CfxEventManager _eventManager;
    CfxHost *_host;
    CfxEngine *_engine;
	CfxScreen *_screen;

public:
    CfxApplication(CfxHost *pHost, CfxEngine *pEngine, CfxScreen *pScreen);
    ~CfxApplication();

    VOID GetBounds(FXRECT *pBounds);

    VOID RegisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject, NotifyMethod Method);
    VOID UnregisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject);

    GUID *GetId()            { return &_id;   }

    BOOL GetLive()           { return _live;  }
    VOID SetLive(BOOL Value) { _live = Value; }

    BOOL GetKioskMode()      { return _kioskMode; }
    VOID SetKioskMode(BOOL Enabled);

    BOOL GetDemoMode()           { return _demoMode;  }
    VOID SetDemoMode(BOOL Value) { _demoMode = Value; }

    virtual VOID Startup();
    virtual VOID Shutdown();
    virtual VOID ShowMenu();

    inline CfxHost   *GetHost()   { return _host;   }
    inline CfxEngine *GetEngine() { return _engine; }
    inline CfxScreen *GetScreen() { return _screen; }
};

extern CfxApplication *GetApplication(CfxPersistent *pPersistent);
extern CfxHost *GetHost(CfxPersistent *pPersistent);
extern CfxEngine *GetEngine(CfxPersistent *pPersistent);
extern CfxScreen *GetScreen(CfxPersistent *pPersistent);
extern CfxCanvas *GetCanvas(CfxPersistent *pPersistent);
extern CfxScreen *GetParentScreen(CfxControl *pControl);
extern BOOL GetApplicationLive(CfxPersistent *pPersistent);
