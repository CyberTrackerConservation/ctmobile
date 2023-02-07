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
#include "fxControl.h"
#include "fxUtils.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctHistoryItem.h"
#include "ctElementContainer.h"
#include "ctDialog_SightingEditor.h"

//*************************************************************************************************
//
// The Sightings control has a phantom entry for the current sighting. These are helper routines
// that the control uses to simulate that entry which has Id = INVALID_DBID and Index = Count.
// 

INT SightingIdToIndex(CfxDatabase *pDatabase, DBID Id)
{
    FX_ASSERT(pDatabase->GetCount() > 0);

    if (Id == INVALID_DBID)
    {
        return pDatabase->GetCount();
    }
    else
    {
        INT index = -1;
        pDatabase->GetIndex(Id, &index);
        return index;
    }
}

DBID SightingIndexToId(CfxDatabase *pDatabase, INT Index)
{
    FX_ASSERT(pDatabase->GetCount() > 0);

    Index = max(Index, 1);
    Index = min(Index, (INT)(pDatabase->GetCount()));

    DBID id = INVALID_DBID;
    pDatabase->Enum(Index, &id);
    return id;
}

//*************************************************************************************************
// CctControl_Sightings

CfxControl *Create_Control_Sightings(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_Sightings(pOwner, pParent);
}

CctControl_Sightings::CctControl_Sightings(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent), _elements(), _itemMap()
{
    InitControl(&GUID_CONTROL_SIGHTINGS);

    _onChange.Object = NULL;

    _paintDatabase = NULL;
    _sighting = new CctSighting(pOwner);
    _itemHeight = 20;
    
    _selectedId = _markId = 0;
    _sightingCount = 0;

    _showUnsaved = TRUE;

    InitXGuid(&_valueElementId);
    InitFont(F_FONT_FIXED_S);

    _showEditButton = TRUE;
    _editButton = new CfxControl_Button(pOwner, this);
    _editButton->SetBorder(bsNone, 0, 0, _borderColor);
    _editButton->SetComposite(TRUE);
    _editButton->SetVisible(FALSE);
    _editButton->SetOnClick(this, (NotifyMethod)&CctControl_Sightings::OnEditButtonClick);
    _editButton->SetCaption("Edit");
}

CctControl_Sightings::~CctControl_Sightings()
{
    delete _sighting;
    _sighting = NULL;

    delete _paintDatabase;
    _paintDatabase = NULL;
}

XGUIDLIST *CctControl_Sightings::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != NULL ? elements : &_elements;
}

VOID CctControl_Sightings::Update()
{
    XGUIDLIST *elements = GetElements();
    CctSession *session = GetSession(this);
    CfxDatabase *database = session->GetSightingDatabase();
    DBID currentId = session->GetCurrentSightingId();

    _itemMap.SetCount(database->GetCount());

    for (UINT i=0; i<_itemMap.GetCount(); i++)
    {
        _itemMap.Put(i, i + 1);
    }

    if (elements->GetCount() == 0)
    {
        _selectedId = _markId = currentId;
        _sightingCount = database->GetCount() - 1;
    }
    else
    {
        ATTRIBUTES *attributes = session->GetCurrentAttributes();
        XGUID matchElementId;
        InitXGuid(&matchElementId);

        for (UINT j=0; j<elements->GetCount() && IsNullXGuid(&matchElementId); j++)
        {
            for (UINT i=0; i<attributes->GetCount(); i++)
            {
                if (CompareXGuid(elements->GetPtr(j), &attributes->GetPtr(i)->ElementId))
                {
                    matchElementId = elements->Get(j);
                    break;
                }
            }
        }
        
        _selectedId = _markId = INVALID_DBID;

        UINT index = 1;
        UINT mapIndex = 0;
        DBID id = INVALID_DBID;
        while (database->Enum(index, &id))
        {
            if (id == currentId)
            {
                _selectedId = _markId = id;
            }

            if (_sighting->Load(database, id))
            {
                ATTRIBUTES *attributes = _sighting->GetAttributes();
                for (UINT i=0; i<attributes->GetCount(); i++)
                {
                    if (CompareXGuid(&matchElementId, &attributes->GetPtr(i)->ElementId))
                    {
                        _itemMap.Put(mapIndex, index);
                        mapIndex++;
                        break;
                    }
                }
            }

            index++;
        }

        _sightingCount = mapIndex;
    }
    
    UpdateScrollBar();
    
    _editButton->SetVisible(_showEditButton);

    delete database;
}

VOID CctControl_Sightings::SetOnChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onChange.Object = pControl;
    _onChange.Method = OnClick;
}

VOID CctControl_Sightings::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_List::DefineResources(F);
    for (UINT i=0; i < _sighting->GetAttributes()->GetCount(); i++)
    {
        ATTRIBUTE *attribute = _sighting->GetAttributes()->GetPtr(i);
        F.DefineObject(attribute->ElementId, eaIcon32);
        F.DefineObject(attribute->ValueId, eaIcon32);
    }

    XGUIDLIST *elements = GetElements();
    UINT i = 0;
    while (i < elements->GetCount()) 
    {
        if (IsNullXGuid(elements->GetPtr(i)))
        {
            elements->Delete(i);
            i = 0;
        }
        else
        {
            F.DefineObject(elements->Get(i), eaName);
            i++;
        }
    }

    F.DefineObject(_valueElementId, eaName);
   
#endif
}

VOID CctControl_Sightings::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineXOBJECTLIST("Elements", &_elements);
    F.DefineXOBJECT("ValueElementId", &_valueElementId);
    F.DefineValue("ShowEditButton",  dtBoolean, &_showEditButton, F_TRUE);
    F.DefineValue("ShowUnsaved",     dtBoolean, &_showUnsaved, F_TRUE);

    if (F.IsReader())
    {
        _editButton->AssignFont(&_font);
        _editButton->SetVisible(FALSE);

        if (IsLive())
        {
            Update();
        }
    }
}

VOID CctControl_Sightings::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    //
    // We only want to allow selection of eaText1..eaText8
    //
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,     &_alignment,   items);

    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Dock",         dtByte,      &_dock,       "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Edit button",  dtBoolean,   &_showEditButton, F_FALSE);
    
    if (GetContainerElements(this) == NULL)
    {
        F.DefineValue("Elements", dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    }

    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Item height",  dtInt32,     &_itemHeight, "MIN(16);MAX(64)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum height", dtInt32,   &_minHeight, "MIN(0)");
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Unsaved sighting", dtBoolean, &_showUnsaved, F_TRUE);
    F.DefineValue("Width",        dtInt32,     &_width);

    F.DefineValue("Value Element", dtGuid,   &_valueElementId, "EDITOR(ScreenElementsEditor)");
#endif
}

VOID CctControl_Sightings::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;
    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        DBID oldSelectedId = _selectedId;
        if (index < (INT)_sightingCount)
        {
            CfxDatabase *database = GetSession(this)->GetSightingDatabase();
            _selectedId = SightingIndexToId(database, _itemMap.Get(index));
            delete database;
        }
        else
        {
            _selectedId = INVALID_DBID;
        }

        if (_showEditButton)
        {
            
        }

        // Run the change event
        if ((oldSelectedId != _selectedId) && _onChange.Object)
        {
            ExecuteEvent(&_onChange, this);
        }

        Repaint();
    }
}

VOID CctControl_Sightings::OnEditButtonClick(CfxControl *pSender, VOID *pParams)
{
    CctSession *session = GetSession(this);
    
    session->SaveState();

    DBID originalId = session->GetCurrentSightingId();
    DBID selectedId = _selectedId;

    // Must do this after getting the active id
    ClearControls();

    if (originalId == selectedId)
    {
		session->Reconnect(originalId);
    }
    else
    {
        session->DoStartEdit(selectedId);
    }
}

VOID CctControl_Sightings::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive())
    {
        _editButton->SetVisible(FALSE);
        _paintDatabase = GetSession(this)->GetSightingDatabase();
        CfxControl_List::OnPaint(pCanvas, pRect);
        delete _paintDatabase;
        _paintDatabase = NULL;
    }
    else
    {
        CfxResource *resource = GetResource();
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, "Sightings");
            resource->ReleaseFont(font);
        }
    }
}

UINT CctControl_Sightings::GetItemCount()
{
    if (_showUnsaved && IsLive() && !GetSession(this)->IsEditing())
    {
        return _sightingCount + 1;
    }
    else
    {
        return _sightingCount;
    }
}

VOID CctControl_Sightings::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
{
    DBID id = INVALID_DBID;
    if (Index < _sightingCount)
    {
        Index = _itemMap.Get(Index);
        _paintDatabase->Enum(Index, &id);
    }

    BOOL selected = (id == _selectedId);

    // Connect and load the sighting
    if ((id == INVALID_DBID) || (id == _markId))
    {
        _sighting->Assign(GetSession(this)->GetCurrentSighting());
    }
    else
    {
        _sighting->Load(_paintDatabase, id);
    }

    ATTRIBUTES *attributes = _sighting->GetAttributes();

    // Build the caption string
    CHAR caption[32];
    FXDATETIME *time = _sighting->GetDateTime();
    if (time->Year == 0)
    {
        sprintf(caption, "%c%s", (_markId == id ) ? '*' : ' ', TEXT_UNSAVED);
    }
    else
    {
        sprintf(caption, "%c%02d-%02d:%02d:%02d", (_markId == id ) ? '*' : ' ', time->Day, time->Hour, time->Minute, time->Second);
    }

    FXRECT rect = *pRect;
    UINT itemHeight = rect.Bottom - rect.Top + 1;
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, selected);
        pCanvas->FillRect(&rect);
    }
    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);
    if (pFont)
    {
        pCanvas->DrawText(pFont, rect.Left + 2, rect.Top + (itemHeight - pCanvas->FontHeight(pFont)) / 2, caption);
    }

    CfxResource *resource = GetResource();

    if (IsNullXGuid(&_valueElementId))
    {
        INT xstart = pRect->Left + pCanvas->TextWidth(pFont, caption) + 8;
        for (UINT i=0; i<attributes->GetCount(); i++)
        {
            XGUID elementId = attributes->GetPtr(i)->ElementId;
            
            FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, elementId, eaIcon32);
            if (bitmap)
            {
                FX_ASSERT(bitmap->Magic == MAGIC_BITMAP);
                pCanvas->StretchDraw(bitmap, xstart, pRect->Top + 1, itemHeight - 2, itemHeight - 2, selected);
                resource->ReleaseObject(bitmap);
                xstart += itemHeight;
            }
            if (xstart > pRect->Right) break;
        }
    }
    else
    {
        FXRECT rect = *pRect;
        rect.Left = rect.Left + (rect.Right - rect.Left + 1) / 2;

        for (UINT i=0; i<attributes->GetCount(); i++)
        {
            ATTRIBUTE *attribute = attributes->GetPtr(i);

            if (!CompareXGuid(&_valueElementId, &attribute->ElementId))
            {
                continue;
            }

            CfxString valueText;
            GetAttributeText(this, attribute, valueText);
            
            pCanvas->DrawTextRect(pFont, &rect, TEXT_ALIGN_VCENTER, valueText.Get());
        }
    }

    if (selected)
    {
        _editButton->SetVisible(FALSE);
        if (_showEditButton && (id != INVALID_DBID) && !GetSession(this)->IsEditing() && !GetSession(this)->GetResourceHeader()->DisableEditing)
        {
            FXRECT rect = *pRect;
            INT buttonWidth = pCanvas->TextWidth(pFont, "Edit");
            rect.Left = rect.Right - (buttonWidth + _oneSpace * 16);
            InflateRect(&rect, -_oneSpace, -_oneSpace);
            _editButton->SetVisible(TRUE);

            FXRECT absoluteRect;
            GetAbsoluteBounds(&absoluteRect);

            _editButton->SetBounds(rect.Left - absoluteRect.Left, rect.Top - absoluteRect.Top, rect.Right - rect.Left + 1, rect.Bottom - rect.Top + 1);
        }
    }
}

//*************************************************************************************************
// CctControl_Attributes

CfxControl *Create_Control_Attributes(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_Attributes(pOwner, pParent);
}

CctControl_Attributes::CctControl_Attributes(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ATTRIBUTES);
    InitFont(F_FONT_DEFAULT_S);

    _itemHeight = 18;
    _painter = new CctSighting_Painter(pOwner, this);
}

CctControl_Attributes::~CctControl_Attributes()
{
    delete _painter;
    _painter = NULL;
}

VOID CctControl_Attributes::SetActiveId(DBID Value)
{
    ((CctSighting_Painter *)_painter)->ConnectById(Value);
    UpdateScrollBar();
}

VOID CctControl_Attributes::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    if (F.IsReader() && IsLive())
    {
        SetActiveId(INVALID_DBID);
    }
}

VOID CctControl_Attributes::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_List::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
}

VOID CctControl_Attributes::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    if (!HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        return;
    }

    ((CctSighting_Painter *)_painter)->SetSelectedIndex(index);
}

//*************************************************************************************************
// CctDialog_AttributeInspector

CfxControl *Create_Control_AttributeInspector(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_AttributeInspector(pOwner, pParent);
}

CctControl_AttributeInspector::CctControl_AttributeInspector(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ATTRIBUTEINSPECTOR);
    InitBorder(bsNone, 0);

    _buttonHeight = 16;

    _activeId = INVALID_DBID; 
    _onChange.Object = NULL;
    _onEditClick.Object = NULL;

    _attributes = new CctControl_Attributes(pOwner, this);
    _attributes->SetComposite(TRUE);
    _attributes->SetDock(dkFill);
    _attributes->SetBorderStyle(bsNone);
    _attributes->SetShowLines(FALSE);

    _buttonPanel = new CfxControl_Panel(pOwner, this);
    _buttonPanel->SetComposite(TRUE);
    _buttonPanel->SetBorderWidth(0);
    _buttonPanel->SetBorderStyle(bsSingle);
    _buttonPanel->SetBorderLineWidth(1);
    _buttonPanel->SetDock(dkTop);
    _buttonPanel->SetMinHeight(_buttonHeight);
    _buttonPanel->SetBounds(0, 0, _buttonHeight, _buttonHeight);
    _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_AttributeInspector::OnButtonPanelResize);
    for (UINT i=0; i<5; i++)
    {
        _buttons[i] = new CfxControl_Button(pOwner, _buttonPanel);
        _buttons[i]->SetComposite(TRUE);
    }
    _buttons[4]->SetCaption("Edit");
}

CctControl_AttributeInspector::~CctControl_AttributeInspector()
{
}

VOID CctControl_AttributeInspector::OnButtonClick(CfxControl *pSender, VOID *pParams)
{
    UINT buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;

    CfxDatabase *database = GetSession(this)->GetSightingDatabase();

    DBID oldActiveId = _activeId;

    switch (buttonIndex)
    {
    case 0: 
        _activeId = SightingIndexToId(database, 1);
        break;

    case 1:
        _activeId = SightingIndexToId(database, SightingIdToIndex(database, _activeId) - 1);
        break;

    case 2: 
        _activeId = SightingIndexToId(database, SightingIdToIndex(database, _activeId) + 1);
        break;

    case 3: 
        _activeId = SightingIndexToId(database, database->GetCount() + 1);
        break;
    }
    delete database;

    // Fire change event
    if ((oldActiveId != _activeId) && _onChange.Object)
    {
        ExecuteEvent(&_onChange, this);
        _attributes->SetActiveId(_activeId);
    }

    // Repaint
    Repaint();
}

VOID CctControl_AttributeInspector::OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    UINT buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;
    BOOL down = _buttons[buttonIndex]->GetDown();

    pParams->Canvas->State.LineWidth = GetBorderLineWidth();

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -_oneSpace, -_oneSpace);

    switch (buttonIndex)
    {
    case 0: 
        pParams->Canvas->DrawFirstArrow(&rect, _color, _borderColor, down); 
        break;

    case 1:
        pParams->Canvas->DrawBackArrow(&rect, _color, _borderColor, down); 
        break;

    case 2: 
        pParams->Canvas->DrawNextArrow(&rect, _color, _borderColor, down); 
        break;

    case 3: 
        pParams->Canvas->DrawLastArrow(&rect, _color, _borderColor, down); 
        break;
    }
}

VOID CctControl_AttributeInspector::OnButtonPanelResize(CfxControl *pSender, VOID *pParams)
{
    UINT buttonCount = IsLive() && GetSession(this)->GetResourceHeader()->DisableEditing ? 4 : 5;
    UINT buttonW = _buttonPanel->GetClientWidth() / buttonCount;
    UINT buttonH = _buttonPanel->GetClientHeight();
    UINT tx = 0;

    for (UINT i=0; i<4; i++)
    {
        _buttons[i]->SetBounds(tx, 0, buttonW + _oneSpace, buttonH);
        tx += buttonW;
    }

    // Make sure the last button fits properly
    if (buttonCount > 4)
    {
        _buttons[4]->SetBounds(tx, 0, GetClientWidth() - tx, buttonH);
        _buttons[4]->SetVisible(TRUE);
    }
    else
    {
        _buttons[4]->SetVisible(FALSE);
    }
}

VOID CctControl_AttributeInspector::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _buttons[4]->DefineResources(F);
    _attributes->DefineResources(F);
#endif
}

VOID CctControl_AttributeInspector::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.IsReader())
    {
        _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_AttributeInspector::OnButtonPanelResize);
        for (UINT i=0; i<4; i++)
        {
            _buttons[i]->SetOnClick(this, (NotifyMethod)&CctControl_AttributeInspector::OnButtonClick);
            _buttons[i]->SetOnPaint(this, (NotifyMethod)&CctControl_AttributeInspector::OnButtonPaint);
        }
        _buttons[4]->SetOnClick((CfxControl *)(_onEditClick.Object), _onEditClick.Method);
        _buttons[4]->SetCaption("Edit");
    }

    // For edit button text
    if (F.DefineBegin("EditButton"))
    {
        _buttons[4]->DefineProperties(F);
        F.DefineEnd();
    }

    // For attributes list text
    if (F.DefineBegin("Attributes"))
    {
        _attributes->DefineProperties(F);
        F.DefineEnd();
    }
}

VOID CctControl_AttributeInspector::SetOnChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onChange.Object = pControl;
    _onChange.Method = OnClick;
}

VOID CctControl_AttributeInspector::SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick)
{
    _onEditClick.Object = pControl;
    _onEditClick.Method = OnClick;
}

VOID CctControl_AttributeInspector::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonHeight = max(ScaleHitSize, _buttonHeight);
    _buttonPanel->SetMinHeight(_buttonHeight);
    //_attributes->Scale(ScaleX, ScaleY, ScaleBorder, 0);
}

//*************************************************************************************************
// CctControl_SendData

CfxControl *Create_Control_SendData(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_SendData(pOwner, pParent);
}

CctControl_SendData::CctControl_SendData(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Button(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SENDDATA);
    _onAfterSend.Object = NULL;
    _hideState = FALSE;
    _packaging = FALSE;
    _state = sdsNoData;
    SetCaption(NULL);
}

CctControl_SendData::~CctControl_SendData()
{
    if (IsLive())
    {
        GetEngine(this)->RemoveTransferListener(this);
    }
}

VOID CctControl_SendData::SetOnAfterSend(CfxControl *pControl, NotifyMethod OnEvent)
{
    _onAfterSend.Object = pControl;
    _onAfterSend.Method = OnEvent;
}

VOID CctControl_SendData::InitState()
{
    if (!IsLive())
    {
        _state = sdsIdle;
        return;
    }

    FXSENDSTATE sendState;
    GetEngine(this)->GetTransferState(&sendState);

    SetEnabled(TRUE);
    if (sendState.Mode != SS_IDLE)
    {
        _state = sdsBusy;
    }
    else
    {
        if (GetSession(this)->GetTransferManager()->HasTransferData() || 
            GetSession(this)->GetTransferManager()->HasOutgoingData() ||
            GetSession(this)->GetTransferManager()->HasExportData())
        {
            _state = sdsIdle;
        }
        else
        {
            _state = sdsNoData;
            SetEnabled(FALSE);
        }
    }
}

VOID CctControl_SendData::OnPenClick(INT X, INT Y)
{
    FXSENDDATA sendData = {0};
    CHAR fileName[MAX_PATH] = {0};
    BOOL buildSuccess;
    CctSession *session = GetSession(this);

    //
    // Cancel send
    //
    FXSENDSTATE sendState;
    GetEngine(this)->GetTransferState(&sendState);
    if (sendState.Mode != SS_IDLE)
    {
        GetEngine(this)->CancelTransfers();
        goto cleanup;
    }

    //
    // No data check: required if a previous send has failed.
    //
    if (!session->GetTransferManager()->HasTransferData() && 
        !session->GetTransferManager()->HasOutgoingData() &&
        !session->GetTransferManager()->HasExportData())
    {
        session->ShowMessage("No data to send");
        goto cleanup;
    }

    // 
    // Normal send
    //

    _packaging = TRUE;
    Repaint();

    if (session->GetTransferManager()->HasTransferData() ||
        session->GetTransferManager()->HasExportData())
    {
        buildSuccess = session->GetTransferManager()->CreateTransfer();
    }
    else if (session->GetTransferManager()->HasOutgoingData())
    {
        buildSuccess = TRUE;
    }
    else
    {
        session->ShowMessage("No data to send");
        goto cleanup;
    }

    if (buildSuccess) 
    {
        if (_onAfterSend.Object)
        {
            ExecuteEvent(&_onAfterSend, this);
        }        

        SetEnabled(FALSE);
        GetEngine(this)->StartTransfer(this, session->GetOutgoingPath());
    }

cleanup:
    _packaging = FALSE;
    Repaint();
}

VOID CctControl_SendData::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    CfxControl_Button::OnPaint(pCanvas, pRect);
    BOOL invert = GetDown();
    BOOL filled = TRUE;
    BOOL drawState = !_hideState;
    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, _font);
    if (!font) return;

    // Paint the upload icon
    FXRECT rect = *pRect;
    if (drawState)
    {
        rect.Bottom -= pCanvas->FontHeight(font) * 2;
        if (rect.Bottom < rect.Top)
        {
            drawState = FALSE;
            rect = *pRect;
        }
    }

    // Paint the state
    CHAR caption[32] = {0};
    if (_packaging)
    {
        strcpy(caption, "Packaging");
    }
    else 
    {
        FXSENDSTATE sendState;
        GetEngine(this)->GetTransferState(&sendState);

        if (!IsLive() || GetSession(this)->HasMessage())
        {
            strcpy(caption, "Pending");
        }
        else 
        {
            switch (sendState.Mode)
            {
            case SS_IDLE:
                if (GetEnabled())
                {
                    strcpy(caption, "Ready");
                }
                else 
                {
                    strcpy(caption, "No data");
                    filled = FALSE;
                }
                break;

            case SS_CONNECTING:
                strcpy(caption, "Connecting");
                break;

            case SS_FAILED_CONNECT:
                strcpy(caption, "Connect failed");
                break;

            case SS_SENDING:
                strcpy(caption, "Sending");
                break;

            case SS_FAILED_SEND:
                strcpy(caption, "Send failed");
                break;

            case SS_COMPLETED:
                strcpy(caption, "Completed");
                break;

            case SS_AUTH_START:
                strcpy(caption, "Signing in");
                break;

            case SS_AUTH_SUCCESS:
                strcpy(caption, "Sign in success");
                break;

            case SS_AUTH_FAILURE:
                strcpy(caption, "Sign in failed");
                break;

            case SS_NO_WIFI:
                strcpy(caption, "No wifi");
                break;
            }
        }
    }

    if (drawState && (strlen(caption) > 0))
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, invert);
        pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, invert);
        pRect->Top = pRect->Bottom - pCanvas->FontHeight(font) * 2;
        pCanvas->DrawTextRect(font, pRect, _alignment | TEXT_ALIGN_VCENTER, caption);
    }

    pCanvas->State.LineWidth = _borderLineWidth;
    pCanvas->DrawUpload(filled, &rect, _color, _borderColor, invert, _transparent);

    resource->ReleaseFont(font);
}

VOID CctControl_SendData::OnTransfer(FXSENDSTATEMODE Mode)
{
    switch (Mode)
    {
    case SS_FAILED_CONNECT:
        GetSession(this)->ShowMessage("Failed to connect");
        break;

    case SS_FAILED_SEND:
        GetSession(this)->ShowMessage("Failed to send");
        break;
    }

    InitState();

    Repaint();
}

VOID CctControl_SendData::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("HideState", dtBoolean, &_hideState, F_FALSE);

    if (F.IsReader())
    {
        SetCaption(NULL);
        InitState();
    }
}

VOID CctControl_SendData::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,    &_alignment,   items);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,       "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Hide state",   dtBoolean, &_hideState);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight, "MIN(0)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Use screen name", dtBoolean, &_useScreenName);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

//*************************************************************************************************
// CctControl_ShareData

CfxControl *Create_Control_ShareData(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ShareData(pOwner, pParent);
}

CctControl_ShareData::CctControl_ShareData(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Button(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SHAREDATA);
    InitLockProperties("Caption;Center;Format;Stretch;Proportional;Image");

    _center = _proportional = TRUE;
    _stretch = _transparentImage = FALSE;
    _bitmap = 0;
    _format = 0;
}

CctControl_ShareData::~CctControl_ShareData()
{
    freeX(_bitmap);
}

VOID CctControl_ShareData::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    if (_width != _height && _bitmap == 0 && (_caption == NULL || (strlen(_caption) == 0)))
    {
        if (_dock == dkLeft || _dock == dkRight)
        {
            _width = _height;
        }
        else if (_dock == dkTop || _dock == dkBottom)
        {
            _height = _width;
        }
        else if (_dock == dkNone)
        {
            _width = _height = min(_width, _height);
        }
        else
        {
            return;
        }

        CfxEngine *engine = GetEngine(this);
        if (engine)
        {
            engine->RequestRealign();
        }
    }
}

VOID CctControl_ShareData::DefineProperties(CfxFiler &F) 
{ 
    CfxControl_Button::DefineProperties(F);

    F.DefineValue("Transparent",  dtBoolean,  &_transparent, F_FALSE);

    F.DefineValue("Center",       dtBoolean, &_center, F_TRUE);
    F.DefineValue("Stretch",      dtBoolean, &_stretch, F_FALSE);
    F.DefineValue("Proportional", dtBoolean, &_proportional, F_TRUE);
    F.DefineValue("TransparentImage", dtBoolean, &_transparentImage, F_FALSE);
    F.DefineXBITMAP("Bitmap", &_bitmap);
    F.DefineValue("Format", dtByte, &_format, F_ZERO);
}

VOID CctControl_ShareData::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Button::DefineResources(F);
    F.DefineBitmap(_bitmap);
#endif
}

VOID CctControl_ShareData::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Format",       dtByte,     &_format,      "LIST(\"CSV zip\")");
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(1024);HINTHEIGHT(1024)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_ShareData::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    BOOL invert = GetDown();

    CfxResource *resource = GetResource();
    FXBITMAPRESOURCE *bitmap = resource->GetBitmap(this, _bitmap);
    if (bitmap)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, invert);
        pCanvas->FillRect(pRect);

        FXRECT rect;
        GetBitmapRect(bitmap, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1, _center, _stretch, _proportional, &rect);
        pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, invert, _transparentImage);
        resource->ReleaseBitmap(bitmap);
    }
    else if (_caption && (strlen(_caption) > 0))
    {
        CfxControl_Button::OnPaint(pCanvas, pRect);         
    }
    else
    {
        pCanvas->DrawShare(pRect, _color, _borderColor, invert, _transparent); 
    }

    // Enable data share feature.
    if (IsLive())
    {
        GetSession(this)->SetShareDataEnabled(TRUE);
    }
}

VOID CctControl_ShareData::OnPenClick(INT X, INT Y)
{
    GetHost(this)->ShareData();
}

//*************************************************************************************************
// CctDialog_SightingEditor

CfxDialog *Create_Dialog_SightingEditor(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_SightingEditor(pOwner, pParent);
}

CctDialog_SightingEditor::CctDialog_SightingEditor(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_SIGHTINGEDITOR);
    InitBorder(_borderStyle, 0);

    _dock = dkFill;

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetDock(dkTop);
    _titleBar->SetCaption("Options");
    _titleBar->SetShowOkay(TRUE);
    _titleBar->SetShowExit(TRUE);
    _titleBar->SetBounds(0, 0, 24, 24);

    _spacer = new CfxControl_Panel(pOwner, this);
    _spacer->SetBorderStyle(bsNone);
    _spacer->SetDock(dkTop);
    _spacer->SetBounds(0, 1, 24, 4);

    _noteBook = new CfxControl_Notebook(pOwner, this);
    _noteBook->SetDock(dkFill);
    _noteBook->SetCaption("All Sightings;Active;History;Send Data");
    _noteBook->ProcessPages();

    _sightings = new CctControl_Sightings(pOwner, _noteBook->GetPage(0));
    _sightings->SetDock(dkFill);
    _sightings->SetBorderStyle(bsNone);
    _sightings->SetOnChange(this, (NotifyMethod)&CctDialog_SightingEditor::OnSightingsChange);

    _attributeInspector = new CctControl_AttributeInspector(pOwner, _noteBook->GetPage(1));
    _attributeInspector->SetDock(dkFill);
    _attributeInspector->SetBorderStyle(bsNone);
	_attributeInspector->SetOnChange(this, (NotifyMethod)&CctDialog_SightingEditor::OnAttributeInspectorChange);
    _attributeInspector->SetOnEditClick(this, (NotifyMethod)&CctDialog_SightingEditor::OnSightingEditClick);

    _sendData = new CctControl_SendData(pOwner, this);
    _sendData->SetComposite(TRUE);
    _sendData->SetDock(dkFill);
    _sendData->SetBorderStyle(bsNone);
    _sendData->SetOnAfterSend(this, (NotifyMethod)&CctDialog_SightingEditor::OnAfterSend);

    _showExports = new CfxControl_Button(pOwner, this);
    _showExports->SetComposite(TRUE);
    _showExports->SetCaption("Show Exported Data");
    _showExports->SetDock(dkBottom);
    _showExports->SetOnClick(this, (NotifyMethod)&CctDialog_SightingEditor::OnShowExports);

    _historyInspector = new CctControl_HistoryInspector(pOwner, this);
    _historyInspector->SetDock(dkFill);
    _historyInspector->SetBorder(bsNone, 0, 0, _borderColor);
}

VOID CctDialog_SightingEditor::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{
    CctSession *session = GetSession(this);

    DBID currentSightingId = session->GetCurrentSightingId();
    _sightings->SetSelectedId(currentSightingId);
    _sightings->SetMarkId(currentSightingId);

    // Turn off the exit button if in kiosk mode.
#ifdef QTBUILD
    _titleBar->SetShowExit(TRUE);
#else
    _titleBar->SetShowExit(!GetApplication(this)->GetKioskMode());
#endif
    _titleBar->SetShowBattery(FALSE);

	_attributeInspector->SetActiveId(currentSightingId);

    //
    // Setup the tabs to adjust for send/no-send tab
    //
    BOOL sendDataTab, historyTab, editTab;

    sendDataTab = IsLive() && GetResource() && 
                  ((strlen(session->GetResourceHeader()->SendData.Url) > 0) ||
                   GetSession(this)->GetTransferManager()->HasTransferData() || 
                   GetSession(this)->GetTransferManager()->HasOutgoingData());

    editTab = TRUE;
    sendDataTab = sendDataTab && (session->GetResourceHeader()->AllowManualSend || 
                                  GetSession(this)->GetTransferManager()->HasTransferData() ||
                                  GetSession(this)->GetTransferManager()->HasOutgoingData());

    // Export if no protocol selected.
    BOOL exportData = GetSession(this)->GetResourceHeader()->SendData.Protocol == FXSENDDATA_PROTOCOL_UNKNOWN;

    if (!sendDataTab && exportData)
    {
        sendDataTab = TRUE;
    }

    historyTab = IsLive() && session->EnumHistoryInit(NULL);
    if (historyTab) session->EnumHistoryDone();

    INT tabIndex;
    INT allTabIndex, editTabIndex, historyTabIndex, sendDataTabIndex, activeTabIndex;
    allTabIndex = editTabIndex = historyTabIndex = sendDataTabIndex = -1;
    activeTabIndex = 1;

    tabIndex = 0;
    CHAR pages[128] = "";

    if (editTab)
    {
        strcpy(pages, "All Sightings;Active");
        allTabIndex = tabIndex;
        tabIndex++;
        editTabIndex = tabIndex;
        tabIndex++;
        activeTabIndex = editTabIndex + 1;
    }

    if (historyTab)
    {
        strcat(pages, ";History");
        historyTabIndex = tabIndex;
        tabIndex++;
    }

    if (sendDataTab)
    {
        strcat(pages, exportData ? ";Export data" : ";Send data");
        sendDataTabIndex = tabIndex;
        tabIndex++;
    }

    _sightings->SetParent(this);
    _attributeInspector->SetParent(this);
    _historyInspector->SetParent(this);
    _sendData->SetParent(this);
    _showExports->SetParent(this);

    _noteBook->SetCaption(pages);
    _noteBook->ProcessPages();

    if (editTab)
    {
        _sightings->SetParent(_noteBook->GetPage(allTabIndex));
        _attributeInspector->SetParent(_noteBook->GetPage(editTabIndex));
    }

    if (historyTab)
    {
        _historyInspector->SetParent(_noteBook->GetPage(historyTabIndex));
        _historyInspector->SetActiveIndex(0);
        _historyInspector->SetVisible(TRUE);
    }
    else
    {
        _historyInspector->SetVisible(FALSE);
    }

    _sendData->SetVisible(FALSE);
    _showExports->SetVisible(FALSE);
    if (sendDataTab) 
    {
        _sendData->SetParent(_noteBook->GetPage(sendDataTabIndex));
        _sendData->SetVisible(TRUE);
        _noteBook->GetPage(sendDataTabIndex)->SetBorderWidth(16);

        _sendData->InitState();
        _sendData->SetOnAfterSend(this, (NotifyMethod)&CctDialog_SightingEditor::OnAfterSend);
    
        if (exportData)
        {
            _showExports->SetParent(_noteBook->GetPage(sendDataTabIndex));
            _showExports->SetVisible(TRUE);
            _showExports->AssignFont(_sendData->GetDefaultFont());
        }
    }

    _noteBook->SetActivePageIndex(activeTabIndex);
}

VOID CctDialog_SightingEditor::OnShowExports(CfxControl *pSender, VOID *pParams)
{
    GetHost(this)->ShowExports();
}

VOID CctDialog_SightingEditor::OnSightingsChange(CfxControl *pSender, VOID *pParams)
{
	_attributeInspector->SetActiveId(_sightings->GetSelectedId());
}

VOID CctDialog_SightingEditor::OnAttributeInspectorChange(CfxControl *pSender, VOID *pParams)
{
	_sightings->SetSelectedId(_attributeInspector->GetActiveId());
}

VOID CctDialog_SightingEditor::OnAfterSend(CfxControl *pSender, VOID *pParams)
{
    _sightings->Update();
    _attributeInspector->SetActiveId(GetSession(this)->GetCurrentSightingId());
}

VOID CctDialog_SightingEditor::OnCloseDialog(BOOL *pHandled)
{
    *pHandled = TRUE;

    // Disconnects the _list database 
    ClearControls();
    CctSession *session = GetSession(this);
    session->Reconnect(session->GetCurrentSightingId());
}

VOID CctDialog_SightingEditor::OnSightingEditClick(CfxControl *pSender, VOID *pParams)
{
    CctSession *session = GetSession(this);
    DBID originalId = session->GetCurrentSightingId();
    DBID selectedId = _attributeInspector->GetActiveId();

    // Must do this after getting the active id
    ClearControls();

    if (originalId == selectedId)
    {
		session->Reconnect(originalId);
    }
    else
    {
        session->DoStartEdit(selectedId);
    }
}

VOID CctDialog_SightingEditor::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
    _noteBook->DefineResources(F);
    _sightings->DefineResources(F);
    _attributeInspector->DefineResources(F);
    _sendData->DefineResources(F);
#endif
}

VOID CctDialog_SightingEditor::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Spacer"))
    {
        _spacer->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Notebook"))
    {
        _noteBook->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("History"))
    {
        _historyInspector->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("SendData"))
    {
        _sendData->DefineProperties(F);
        F.DefineEnd();
    }
}

VOID CctDialog_SightingEditor::DefineState(CfxFiler &F)
{
    _noteBook->DefineState(F);
}

//*************************************************************************************************
// CctControl_HistoryList

CfxControl *Create_Control_HistoryList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_HistoryList(pOwner, pParent);
}

CctControl_HistoryList::CctControl_HistoryList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_HISTORY_LIST);
    InitFont(F_FONT_DEFAULT_S);

    _itemHeight = 18;

    _painter = new CctHistory_Painter(pOwner, this);
}

CctControl_HistoryList::~CctControl_HistoryList()
{
    delete _painter;
    _painter = NULL;
}

VOID CctControl_HistoryList::SetActiveIndex(UINT Value)
{
    ((CctHistory_Painter *)_painter)->Connect(Value);
    UpdateScrollBar();
}

//*************************************************************************************************
// CctDialog_HistoryInspector

CfxControl *Create_Control_HistoryInspector(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_HistoryInspector(pOwner, pParent);
}

CctControl_HistoryInspector::CctControl_HistoryInspector(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_HISTORY_INSPECTOR);

    _activeIndex = INVALID_INDEX; 
    _buttonHeight = 20;
    _itemHeight = 20;

    _history = new CctControl_HistoryList(pOwner, this);
    _history->SetComposite(TRUE);
    _history->SetDock(dkFill);
    _history->SetBorderStyle(bsNone);
    _history->SetShowLines(FALSE);
    _history->SetTransparent(TRUE);
    _history->SetCaption("-");

    _buttonPanel = new CfxControl_Panel(pOwner, this);
    _buttonPanel->SetComposite(TRUE);
    _buttonPanel->SetTransparent(TRUE);
    _buttonPanel->SetBorderWidth(0);
    _buttonPanel->SetBorderStyle(bsSingle);
    _buttonPanel->SetBorderLineWidth(1);
    _buttonPanel->SetDock(dkTop);
    _buttonPanel->SetHeight(_buttonHeight);
    _buttonPanel->SetBounds(0, 0, _buttonHeight, _buttonHeight);
    _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_HistoryInspector::OnButtonPanelResize);
    for (UINT i=0; i<5; i++)
    {
        _buttons[i] = new CfxControl_Button(pOwner, _buttonPanel);
        _buttons[i]->SetComposite(TRUE);
        _buttons[i]->SetTransparent(TRUE);
    }
    _buttons[4]->SetEnabled(FALSE);
    _buttons[4]->SetCaption("0");

    _itemCount = 0;
    if (IsLive())
    {
        if (GetSession(this)->EnumHistoryInit(NULL))
        {
            _itemCount = GetSession(this)->GetHistoryItemCount();
            GetSession(this)->EnumHistoryDone();
        }
    }
}

CctControl_HistoryInspector::~CctControl_HistoryInspector()
{
}

VOID CctControl_HistoryInspector::OnButtonClick(CfxControl *pSender, VOID *pParams)
{
    UINT buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;

    UINT oldActiveIndex = _activeIndex;

    switch (buttonIndex)
    {
    case 0: 
        _activeIndex = 0;
        break;

    case 1:
        if (_activeIndex > 0)
        {
            _activeIndex = _activeIndex - 1;
        }
        break;

    case 2: 
        if (_activeIndex < _itemCount - 1)
        {
            _activeIndex++;
        }
        break;

    case 3: 
        _activeIndex = _itemCount - 1;
        break;
    }

    // Fire change event
    if (oldActiveIndex != _activeIndex)
    {
        _history->SetActiveIndex(_activeIndex);
    }

    // Caption
    CHAR caption[8];
    sprintf(caption, "%lu", _itemCount == 0 ? 0 : _activeIndex + 1);
    _buttons[4]->SetCaption(caption);

    // Repaint
    Repaint();
}

VOID CctControl_HistoryInspector::OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    UINT buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;
    BOOL down = _buttons[buttonIndex]->GetDown();

    pParams->Canvas->State.LineWidth = GetBorderLineWidth();

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -_oneSpace, -_oneSpace);

    switch (buttonIndex)
    {
    case 0: 
        pParams->Canvas->DrawFirstArrow(&rect, _color, _borderColor, down); 
        break;

    case 1:
        pParams->Canvas->DrawBackArrow(&rect, _color, _borderColor, down); 
        break;

    case 2: 
        pParams->Canvas->DrawNextArrow(&rect, _color, _borderColor, down); 
        break;

    case 3: 
        pParams->Canvas->DrawLastArrow(&rect, _color, _borderColor, down); 
        break;
    }
}

VOID CctControl_HistoryInspector::OnButtonPanelResize(CfxControl *pSender, VOID *pParams)
{
    UINT buttonW = _buttonPanel->GetClientWidth() / 5;
    UINT buttonH = _buttonPanel->GetClientHeight();
    UINT tx = 0;

    for (UINT i=0; i<4; i++)
    {
        _buttons[i]->SetBounds(tx, 0, buttonW + _oneSpace, buttonH);
        tx += buttonW;
    }

    // Make sure the last button fits properly
    _buttons[4]->SetBounds(tx, 0, GetClientWidth() - tx, buttonH);
}

VOID CctControl_HistoryInspector::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    _buttons[4]->DefineResources(F);
    _history->DefineResources(F);
#endif
}

VOID CctControl_HistoryInspector::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("ButtonHeight", dtInt32, &_buttonHeight, "20");
    F.DefineValue("ItemHeight",  dtInt32, &_itemHeight, "20");

    if (F.IsReader())
    {
        _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_HistoryInspector::OnButtonPanelResize);
        for (UINT i=0; i<4; i++)
        {
            _buttons[i]->SetOnClick(this, (NotifyMethod)&CctControl_HistoryInspector::OnButtonClick);
            _buttons[i]->SetOnPaint(this, (NotifyMethod)&CctControl_HistoryInspector::OnButtonPaint);
        }

        _buttonPanel->SetHeight(_buttonHeight);
        _history->SetItemHeight(_itemHeight);
    }

    // For edit button text
    if (F.DefineBegin("DashButton"))
    {
        _buttons[4]->DefineProperties(F);
        _buttons[4]->SetCaption("0");
        _buttons[4]->SetTextColor(_borderColor);
        F.DefineEnd();
    }

    // For attributes list text
    if (F.DefineBegin("History"))
    {
        _history->DefineProperties(F);

        if (F.IsReader() && IsLive())
        {
            CctSession *session = GetSession(this);
            if (session->EnumHistoryInit(NULL))
            {
                session->EnumHistoryDone();
                _history->SetActiveIndex(0);
                _history->SetTextColor(_textColor);
                _activeIndex = 0;
                _buttons[4]->SetCaption("1");
            }
        }

        F.DefineEnd();
    }
}

VOID CctControl_HistoryInspector::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,    &_alignment,   items);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button row height", dtInt32, &_buttonHeight, "MIN(16);MAX(64)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,       "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Item height",  dtInt32,    &_itemHeight, "MIN(16);MAX(64)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight, "MIN(0)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_HistoryInspector::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonHeight = max(ScaleHitSize, _buttonHeight);
    _buttonPanel->SetMinHeight(_buttonHeight);
}

//*************************************************************************************************
