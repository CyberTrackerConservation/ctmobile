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
    VOID Clear();
    VOID AddCloneAndExecute(CfxAction *pAction, CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
    VOID Execute(CfxPersistent *pSender);
    VOID ReplaceBySource(CfxActionManager *pSourceManager, XGUID *pSourceId);
    VOID Filter(GUIDLIST *pFilterTypeIds);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
};

//
// CfxAction
//
class CfxAction: public CfxPersistent
{
friend VOID CfxActionManager::DefineProperties(CfxFiler &F);
protected:
    GUID _typeId;
    XGUID _sourceId;
    VOID InitAction(const GUID *pTypeId);
public:
    CfxAction(CfxPersistent *pOwner): CfxPersistent(pOwner) { InitXGuid(&_sourceId); }
    GUID *GetTypeId()                                 { return &_typeId;     }
    XGUID *GetSourceId()                              { return &_sourceId;   }
    VOID SetSourceId(XGUID *pValue)                   { _sourceId = *pValue; }
    virtual VOID DefineState(CfxFiler &F)             { }
    virtual VOID DefineProperties(CfxFiler &F)        { F.DefineXOBJECT("Source", &_sourceId); }
    virtual VOID DefineResources(CfxFilerResource &F) {}
    virtual VOID Execute(CfxPersistent *pSender)= 0;
    virtual VOID Rollback(CfxPersistent *pSender)= 0;
};

//
// CfxActionRegistry
//
typedef CfxAction* (*Fn_Action_Factory)(CfxPersistent *pOwner);

struct RegisteredAction
{
    GUID TypeId;
    CHAR Name[32];
    BOOL UserVisible;
    Fn_Action_Factory FnFactory;
};

class CfxActionRegistry 
{
protected:
    TList<RegisteredAction> _actions;
public:
    CfxAction *CreateRegisteredAction(const GUID *pActionTypeId, CfxPersistent *pOwner);
    VOID RegisterAction(const GUID *pActionTypeId, CHAR *pActionName, Fn_Action_Factory FnFactory, BOOL UserVisible=TRUE);
    BOOL EnumRegisteredActions(UINT Index, GUID *pActionTypeId, CHAR *pActionName);
};

// Global registry
extern CfxActionRegistry ActionRegistry;

