#include "fxUtils.h"
#include "fxApplication.h"
#include "fxTranslate.h"
#include "ctElementTextEdit.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementTextEdit(pOwner, pParent);
}

CctControl_ElementTextEdit::CctControl_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Memo(pOwner, pParent), _retainState(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTEDIT);
    InitLockProperties("Result Element");

    _required = false;
    _oneLine = false;
    _maxLength = 0;

    InitXGuid(&_elementId);
	 
    RegisterRetainState(this, &_retainState);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementTextEdit::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementTextEdit::OnSessionNext);
}

CctControl_ElementTextEdit::~CctControl_ElementTextEdit()
{
    UnregisterRetainState(this);

    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);
}

void CctControl_ElementTextEdit::SetCaption(const char *Caption)
{
    CfxControl_Memo::SetCaption(Caption);

    if (_oneLine)
    {
        StringReplace(&_caption, "\n", " ");
        StringReplace(&_caption, "\r", "");
    }

    if (_maxLength != 0)
    {
        if (strlen(_caption) > _maxLength)
        {
            _caption[_maxLength] = '\0';            
        }
    }

    UpdateScrollBar();
}

void CctControl_ElementTextEdit::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_required && ((_caption == nullptr) || (strlen(_caption) == 0)))
    {
        GetSession(this)->ShowMessage(fxTranslate("Text is required"));
        *((bool *)pParams) = false;
    }
}

void CctControl_ElementTextEdit::OnSessionNext(CfxControl *pSender, void *pParams)
{
    GetCaption();

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId))
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);

        if (_caption && (strlen(_caption) > 0))
        {
            action.SetupAdd(_id, _elementId, dtPText, &_caption);
        }

        ((CctSession *)pSender)->ExecuteAction(&action);
    }
    else
    {
        ((CctSession *)pSender)->ShowMessage(fxTranslate("No Result: Text not saved"));
    }
}

void CctControl_ElementTextEdit::DefineProperties(CfxFiler &F)
{
    CfxControl_Memo::DefineProperties(F);
    _retainState.DefineProperties(F);

    F.DefineValue("MaxLength", dtInt32, &_maxLength, F_ZERO);
    F.DefineValue("OneLine", dtBoolean, &_oneLine, F_FALSE);
    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineXOBJECT("Element", &_elementId);

    HackFixResultElementLockProperties(&_lockProperties);
}

void CctControl_ElementTextEdit::ResetState()
{
    CfxControl_Memo::ResetState();

    MEM_FREE(_caption);
    _caption = nullptr;
}

void CctControl_ElementTextEdit::DefineState(CfxFiler &F)
{
    CfxControl_Memo::DefineState(F);
    _retainState.DefineState(F);

    F.DefineValue("Caption", dtPText, &_caption);
}

void CctControl_ElementTextEdit::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ElementTextEdit::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementTextEdit::OnPenClick(int X, int Y)
{
    GetSession(this)->ShowTextEditorPage(GetId());
}

void CctControl_ElementTextEdit::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if ((_caption == nullptr) || (strlen(_caption) == 0))
    {
        if (!_transparent)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(pRect);
        }

        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, fxTranslate("Tap to edit"));
            resource->ReleaseFont(font);
        }
    }
    else
    {
        CfxControl_Memo::OnPaint(pCanvas, pRect);
    }
}

void CctControl_ElementTextEdit::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Memo::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max(_height, (int)ScaleHitSize);
}
