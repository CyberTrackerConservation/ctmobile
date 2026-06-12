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
    CfxEventManager _eventManager;
    CfxHost *_host = nullptr;
    CfxEngine *_engine = nullptr;
    CfxScreen *_screen = nullptr;

public:
    CfxApplication(QObject *pParent, CfxHost *pHost, CfxEngine *pEngine, CfxScreen *pScreen);
    ~CfxApplication();

    void GetBounds(FXRECT *pBounds);

    void RegisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject, NotifyMethod Method);
    void UnregisterEvent(APPLICATIONEVENT Event, CfxPersistent *pObject);

    void Startup();
    void Shutdown();
    void ShowMenu();

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
