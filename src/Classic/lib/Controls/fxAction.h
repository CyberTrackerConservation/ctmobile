#pragma once
#include "pch.h"
#include "fxControl.h"
#include "fxUtils.h"

class CfxAction;

//
// CfxActionManager
//
class CfxActionManager: public CfxPersistent
{
protected:
    TList<CfxAction *> _actions;
public:
    CfxActionManager(CfxPersistent *pOwner);
    ~CfxActionManager();
    void Clear();
    void AddCloneAndExecute(CfxAction *pAction, CfxPersistent *pSender);
    void Rollback(CfxPersistent *pSender);
    void Execute(CfxPersistent *pSender);
    void ReplaceBySource(CfxActionManager *pSourceManager, XGUID *pSourceId);
    void Filter(GUIDLIST *pFilterTypeIds);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
};

//
// CfxAction
//
class CfxAction: public CfxPersistent
{
friend void CfxActionManager::DefineProperties(CfxFiler &F);
protected:
    GUID _typeId;
    XGUID _sourceId;
    void InitAction(const GUID *pTypeId);
public:
    CfxAction(CfxPersistent *pOwner): CfxPersistent(pOwner) { InitXGuid(&_sourceId); }
    GUID *GetTypeId()                                 { return &_typeId;     }
    XGUID *GetSourceId()                              { return &_sourceId;   }
    void SetSourceId(XGUID *pValue)                   { _sourceId = *pValue; }
    virtual void DefineState(CfxFiler &F)             { }
    virtual void DefineProperties(CfxFiler &F)        { F.DefineXOBJECT("Source", &_sourceId); }
    virtual void DefineResources(CfxFilerResource &F) {}
    virtual void Execute(CfxPersistent *pSender)= 0;
    virtual void Rollback(CfxPersistent *pSender)= 0;
};

//
// CfxActionRegistry
//
typedef CfxAction* (*Fn_Action_Factory)(CfxPersistent *pOwner);

struct RegisteredAction
{
    GUID TypeId;
    char Name[32] = {};
    bool UserVisible = false;
    Fn_Action_Factory FnFactory = nullptr;
};

class CfxActionRegistry 
{
protected:
    TList<RegisteredAction> _actions;
public:
    CfxAction *CreateRegisteredAction(const GUID *pActionTypeId, CfxPersistent *pOwner);
    void RegisterAction(const GUID *pActionTypeId, const char *pActionName, Fn_Action_Factory FnFactory, bool UserVisible=true);
    bool EnumRegisteredActions(int Index, GUID *pActionTypeId, char *pActionName);
};

// Global registry
extern CfxActionRegistry ActionRegistry;

