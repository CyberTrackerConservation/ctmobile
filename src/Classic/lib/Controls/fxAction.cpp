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

void CfxActionManager::Clear()
{
    for (int i=0; i<_actions.GetCount(); i++)
    {
        delete _actions.Get(i);
    }
   _actions.Clear();
}

void CfxActionManager::AddCloneAndExecute(CfxAction *pAction, CfxPersistent *pSender)
{
    // Clone the action
    CfxAction *action = ActionRegistry.CreateRegisteredAction(pAction->GetTypeId(), this);
    action->Assign(pAction);
    _actions.Add(action);
    action->Execute(pSender);
}

void CfxActionManager::Rollback(CfxPersistent *pSender)
{
    for (int i=_actions.GetCount()-1; i>=0; i--)
    {
        _actions.Get(i)->Rollback(pSender);
    }
}

void CfxActionManager::Execute(CfxPersistent *pSender)
{
    for (int i=0; i<_actions.GetCount(); i++)
    {
        _actions.Get(i)->Execute(pSender);
    }
}

void CfxActionManager::ReplaceBySource(CfxActionManager *pSourceManager, XGUID *pSourceId)
{
    int i = _actions.GetCount()-1;
    int index = -1;
    while ((i >= 0) && ((int)_actions.GetCount() > i))
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
        index = (int)_actions.GetCount();
    }

    for (i=pSourceManager->_actions.GetCount()-1; i>=0; i--)
    {
        CfxAction *sourceAction = pSourceManager->_actions.Get(i);

        CfxAction *action = ActionRegistry.CreateRegisteredAction(sourceAction->GetTypeId(), this);
        action->Assign(sourceAction);
        _actions.Insert(index, action);
    }
}

void CfxActionManager::Filter(GUIDLIST *pFilterTypeIds)
{
    int i = _actions.GetCount()-1;
    while ((i >= 0) && ((int)_actions.GetCount() > i))
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

void CfxActionManager::DefineProperties(CfxFiler &F)
{
    if (F.DefineBegin("Actions"))
    {
        if (F.IsWriter())
        {
            for (int i=0; i<_actions.GetCount(); i++)
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
                    F.DefineValue("Type", dtGuid, (void *)&actionTypeId);                    
          
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

void CfxActionManager::DefineState(CfxFiler &F)
{
    for (int i=0; i<_actions.GetCount(); i++)
    {
        _actions.Get(i)->DefineState(F);
    }
}

//*************************************************************************************************
// CfxAction

void CfxAction::InitAction(const GUID *pTypeId)
{
    InitGuid(&_typeId, pTypeId);
}

// Global control registry
CfxActionRegistry ActionRegistry;

//*************************************************************************************************
// CfxActionRegistry

CfxAction *CfxActionRegistry::CreateRegisteredAction(const GUID *pActionTypeId, CfxPersistent *pOwner)
{
    for (int i=0; i<_actions.GetCount(); i++)
    {
        GUID value = _actions.Get(i).TypeId;
        if (CompareGuid(pActionTypeId, &value))
        {
            return _actions.Get(i).FnFactory(pOwner);
        }
    }

    return nullptr;
}

void CfxActionRegistry::RegisterAction(const GUID *pActionTypeId, const char *pActionName, Fn_Action_Factory FnFactory, bool UserVisible)
{
    RegisteredAction action = {0};

    action.TypeId = *pActionTypeId;
    strncpy(action.Name, pActionName, sizeof(action.Name)-1);
    action.UserVisible = UserVisible;
    action.FnFactory = FnFactory;

    _actions.Add(action);
}

bool CfxActionRegistry::EnumRegisteredActions(int Index, GUID *pActionTypeId, char *pActionName)
{
    if (Index >= _actions.GetCount()) 
    {
        return false;
    }

    int i = 0, j = 0;
    while (i < _actions.GetCount()) 
    {
        RegisteredAction action = _actions.Get(i);
        if (action.UserVisible)
        {
            if (Index == j)
            {
                memmove(pActionTypeId, &action.TypeId, sizeof(GUID));
                strncpy(pActionName, (char *)&action.Name, sizeof(action.Name));
                return true;
            }
            j++;
        }
        i++;
    }

    return false;
}
