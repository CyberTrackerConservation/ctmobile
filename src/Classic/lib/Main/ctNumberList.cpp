#include "fxApplication.h"
#include "fxUtils.h"
#include "ctNumberList.h"
#include "ctActions.h"

CfxControl *Create_Control_NumberList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NumberList(pOwner, pParent);
}

//*************************************************************************************************
// CctControl_NumberList

CctControl_NumberList::CctControl_NumberList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent), _retainState(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NUMBERLIST);
    InitLockProperties("Result Element;First value;Last value");

    _autoNext = false;
    _startValue = 1;
    _endValue = 50;
    _interval = 1;
    _selectedIndex = -1;
    InitXGuid(&_elementId);

    _minItemWidth = 8;
    _autoSelectIndex = 0;

    // Global result
    _resultGlobalValue = nullptr;

    // Register for next event
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_NumberList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_NumberList::OnSessionNext);

    RegisterRetainState(this, &_retainState);
}

CctControl_NumberList::~CctControl_NumberList()
{
    UnregisterRetainState(this);

    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_resultGlobalValue);
	_resultGlobalValue = nullptr;
}

void CctControl_NumberList::SetSelectedIndex(int Index)
{
    _selectedIndex = Index;

    // Set the global value
    if (IsLive() && (_resultGlobalValue != nullptr))
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, (Index == -1) ? 0 : Index + _startValue);
    }
}

void CctControl_NumberList::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_selectedIndex == -1)
    {
        *((bool *)pParams) = false;
    }
}

void CctControl_NumberList::OnSessionNext(CfxControl *pSender, void *pParams)
{
    if (_selectedIndex == -1) return;

    //
    // Add the Attribute for this Element
    //
    int value = _selectedIndex * _interval + _startValue;
    if (!IsNullXGuid(&_elementId) && (_selectedIndex >= 0))
    {
        CctAction_MergeAttribute action(this);
        action.SetupAdd(_id, _elementId, dtInt32, &value);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

void CctControl_NumberList::DefineResources(CfxFilerResource &F)
{
}

void CctControl_NumberList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);
    _retainState.DefineProperties(F);

    F.DefineValue("AutoNext",        dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("AutoSelectIndex", dtInt32,   &_autoSelectIndex, F_ZERO);
    F.DefineValue("StartValue",      dtInt32,   &_startValue, F_ONE);
    F.DefineValue("EndValue",        dtInt32,   &_endValue, "50");
    F.DefineValue("Interval",        dtInt32,   &_interval, "1");
    F.DefineValue("ResultGlobalValue", dtPText, &_resultGlobalValue);

    F.DefineXOBJECT("Element", &_elementId);

    if (F.IsReader())
    {
        _minItemWidth = 8;
        _endValue = max(_startValue + 1, _endValue);
        UpdateScrollBar();

        if ((_autoSelectIndex > 0) && (_autoSelectIndex <= GetItemCount()))
        {
            SetSelectedIndex(_autoSelectIndex - 1);
        }

        HackFixResultElementLockProperties(&_lockProperties);
    }
}

void CctControl_NumberList::ResetState()
{
    CfxControl_List::ResetState();
    _selectedIndex = -1;
}

void CctControl_NumberList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    _retainState.DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

void CctControl_NumberList::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_NumberList::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl_List::SetBounds(Left, Top, Width, Height);

    UpdateScrollBar();
}

void CctControl_NumberList::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        SetSelectedIndex((index != -1) ? index : _selectedIndex);
        Repaint();
    }

    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT, 100);
    }
}

int CctControl_NumberList::GetItemCount()
{
    return (_endValue - _startValue + 1) / max(1, _interval);
}

void CctControl_NumberList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
{
    char caption[10];
    bool selected = ((int)Index == _selectedIndex);
    sprintf(caption, "%ld", (int)(_startValue + Index * _interval));

    pCanvas->State.BrushColor = pCanvas->InvertColor(_color, selected);
    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);

    if (selected)
    {
        pCanvas->FillRect(pRect);
    }

    int w = pRect->Right - pRect->Left + 1;
    int h = pRect->Bottom - pRect->Top + 1;
    int x = (w - pCanvas->TextWidth(pFont, caption)) / 2;
    int y = (h - pCanvas->FontHeight(pFont)) / 2;

    pCanvas->DrawText(pFont, pRect->Left + x, pRect->Top + y, caption);
}
