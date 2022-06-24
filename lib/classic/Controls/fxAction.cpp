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

#include "fxAction.h"
#include "fxUtils.h"

//*************************************************************************************************
// CfxActionManager

CfxActionManager::CfxActionManager(CfxPersistent *pOwner): CfxPersistent(pOwner), _actions()
{
}

CfxActionManager::~CfxActionManager()
{
    Clear();
}

VOID CfxActionManager::Clear()
{
    for (UINT i=0; i<_actions.GetCount(); i++)
    {
        delete _actions.Get(i);
    }
   _actions.Clear();
}

VOID CfxActionManager::AddCloneAndExecute(CfxAction *pAction, CfxPersistent *pSender)
{
    // Clone the action
    CfxAction *action = ActionRegistry.CreateRegisteredAction(pAction->GetTypeId(), this);
    action->Assign(pAction);
    _actions.Add(action);
    action->Execute(pSender);
}

VOID CfxActionManager::Rollback(CfxPersistent *pSender)
{
    for (INT i=_actions.GetCount()-1; i>=0; i--)
    {
        _actions.Get(i)->Rollback(pSender);
    }
}

VOID CfxActionManager::Execute(CfxPersistent *pSender)
{
    for (UINT i=0; i<_actions.GetCount(); i++)
    {
        _actions.Get(i)->Execute(pSender);
    }
}

VOID CfxActionManager::ReplaceBySource(CfxActionManager *pSourceManager, XGUID *pSourceId)
{
    INT i = _actions.GetCount()-1;
    INT index = -1;
    while ((i >= 0) && ((INT)_actions.GetCount() > i))
    {
        CfxAction *item = _actions.Get(i);
        if (CompareXGuid(item->GetSourceId(), pSourceId))
        {
            index = i;
            delete item;
            _actions.Delete(i);
        }
        i--;
    }

    if (index == -1)
    {
        index = (INT)_actions.GetCount();
    }

    for (i=pSourceManager->_actions.GetCount()-1; i>=0; i--)
    {
        CfxAction *sourceAction = pSourceManager->_actions.Get(i);

        CfxAction *action = ActionRegistry.CreateRegisteredAction(sourceAction->GetTypeId(), this);
        action->Assign(sourceAction);
        _actions.Insert(index, action);
    }
}

VOID CfxActionManager::Filter(GUIDLIST *pFilterTypeIds)
{
    INT i = _actions.GetCount()-1;
    while ((i >= 0) && ((INT)_actions.GetCount() > i))
    {
        CfxAction *item = _actions.Get(i);
        if (pFilterTypeIds->IndexOf(*item->GetTypeId()) == -1)
        {
            delete item;
            _actions.Delete(i);
        }
        i--;
    }
}

VOID CfxActionManager::DefineProperties(CfxFiler &F)
{
    if (F.DefineBegin("Actions"))
    {
        if (F.IsWriter())
        {
            for (UINT i=0; i<_actions.GetCount(); i++)
            {
                F.DefineBegin("Action");
                F.DefineValue("Type", dtGuid, &_actions.Get(i)->_typeId);
                _actions.Get(i)->DefineProperties(F);
                F.DefineEnd();
            }
            F.ListEnd();
        }
        else
        {
            FX_ASSERT(F.IsReader());
            Clear();
      
            while (!F.ListEnd())
            {
                if (F.DefineBegin("Action"))
                {
                    GUID actionTypeId = {0};
                    F.DefineValue("Type", dtGuid, (VOID *)&actionTypeId);                    
          
                    CfxAction *action = ActionRegistry.CreateRegisteredAction(&actionTypeId, this);

                    if (action) // BUGBUG: action missing
                    {
                        action->DefineProperties(F);
                        _actions.Add(action);
                    }
                    F.DefineEnd();
                }
                else break;
            }
        }

        F.DefineEnd();
    }
}

VOID CfxActionManager::DefineState(CfxFiler &F)
{
    for (UINT i=0; i<_actions.GetCount(); i++)
    {
        _actions.Get(i)->DefineState(F);
    }
}

//*************************************************************************************************
// CfxAction

VOID CfxAction::InitAction(const GUID *pTypeId)
{
    InitGuid(&_typeId, pTypeId);
}

// Global control registry
CfxActionRegistry ActionRegistry;

//*************************************************************************************************
// CfxActionRegistry

CfxAction *CfxActionRegistry::CreateRegisteredAction(const GUID *pActionTypeId, CfxPersistent *pOwner)
{
    for (UINT i=0; i<_actions.GetCount(); i++)
    {
        GUID value = _actions.Get(i).TypeId;
        if (CompareGuid(pActionTypeId, &value))
        {
            return _actions.Get(i).FnFactory(pOwner);
        }
    }

    return NULL;
}

VOID CfxActionRegistry::RegisterAction(const GUID *pActionTypeId, CHAR *pActionName, Fn_Action_Factory FnFactory, BOOL UserVisible)
{
    RegisteredAction action = {0};

    action.TypeId = *pActionTypeId;
    strncpy(action.Name, pActionName, sizeof(action.Name)-1);
    action.UserVisible = UserVisible;
    action.FnFactory = FnFactory;

    _actions.Add(action);
}

BOOL CfxActionRegistry::EnumRegisteredActions(UINT Index, GUID *pActionTypeId, CHAR *pActionName)
{
    if (Index >= _actions.GetCount()) 
    {
        return FALSE;
    }

    UINT i = 0, j = 0;
    while (i < _actions.GetCount()) 
    {
        RegisteredAction action = _actions.Get(i);
        if (action.UserVisible)
        {
            if (Index == j)
            {
                memmove(pActionTypeId, &action.TypeId, sizeof(GUID));
                strncpy(pActionName, (CHAR *)&action.Name, sizeof(action.Name));
                return TRUE;
            }
            j++;
        }
        i++;
    }

    return FALSE;
}
