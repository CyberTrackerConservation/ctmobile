#include "fxApplication.h"
#include "fxControl.h"
#include "fxUtils.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctHistoryItem.h"
#include "ctElementContainer.h"
#include "ctSightingEditor.h"

//*************************************************************************************************
//
// The Sightings control has a phantom entry for the current sighting. These are helper routines
// that the control uses to simulate that entry which has an empty uid and Index = Count.
//

int SightingIdToIndex(CctSightingDatabase *pDatabase, const QString &uid)
{
    FX_ASSERT(pDatabase->GetCount() > 0);

    if (uid.isEmpty())
    {
        return pDatabase->GetCount();
    }
    else
    {
        int index = -1;
        pDatabase->GetIndex(uid, &index);
        return index;
    }
}

QString SightingIndexToId(CctSightingDatabase *pDatabase, int Index)
{
    FX_ASSERT(pDatabase->GetCount() > 0);

    Index = max(Index, 0);
    Index = min(Index, (int)(pDatabase->GetCount()));

    QString uid;
    pDatabase->Enum(Index, &uid);
    return uid;
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

    _onChange.Object = nullptr;

    _paintDatabase = nullptr;
    _sighting = new CctSighting(pOwner);
    _itemHeight = 20;
    
    _selectedId = _markId = 0;
    _sightingCount = 0;

    _showUnsaved = true;

    InitXGuid(&_valueElementId);
    InitFont(F_FONT_FIXED_S);

    _showEditButton = true;
    _editButton = new CfxControl_Button(pOwner, this);
    _editButton->SetBorder(ControlBorderStyle::None, 0, 0, _borderColor);
    _editButton->SetComposite(true);
    _editButton->SetVisible(false);
    _editButton->SetOnClick(this, (NotifyMethod)&CctControl_Sightings::OnEditButtonClick);
    _editButton->SetCaption("Edit");
}

CctControl_Sightings::~CctControl_Sightings()
{
    delete _sighting;
    _sighting = nullptr;

    delete _paintDatabase;
    _paintDatabase = nullptr;
}

XGUIDLIST *CctControl_Sightings::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != nullptr ? elements : &_elements;
}

void CctControl_Sightings::Update()
{
    XGUIDLIST *elements = GetElements();
    CctSession *session = GetSession(this);
    auto database = session->CreateSightingDatabasePtr();
    auto currentUid = session->GetCurrentSightingUid();

    _itemMap.SetCount(database->GetCount());

    for (int i=0; i<_itemMap.GetCount(); i++)
    {
        _itemMap.Put(i, i + 1);
    }

    if (elements->GetCount() == 0)
    {
        _selectedId = _markId = currentUid;
        _sightingCount = database->GetCount();
    }
    else
    {
        ATTRIBUTES *attributes = session->GetCurrentAttributes();
        XGUID matchElementId;
        InitXGuid(&matchElementId);

        for (int j=0; j<elements->GetCount() && IsNullXGuid(&matchElementId); j++)
        {
            for (int i=0; i<attributes->GetCount(); i++)
            {
                if (CompareXGuid(elements->GetPtr(j), &attributes->GetPtr(i)->ElementId))
                {
                    matchElementId = elements->Get(j);
                    break;
                }
            }
        }

        _selectedId = _markId = QString();

        int index = 0;
        int mapIndex = 0;
        QString uid;
        while (database->Enum(index, &uid))
        {
            if (uid == currentUid)
            {
                _selectedId = _markId = uid;
            }

            if (database->Load(_sighting, uid))
            {
                ATTRIBUTES *attributes = _sighting->GetAttributes();
                for (int i=0; i<attributes->GetCount(); i++)
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
}

void CctControl_Sightings::SetOnChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onChange.Object = pControl;
    _onChange.Method = OnClick;
}

void CctControl_Sightings::DefineResources(CfxFilerResource &F)
{
}

void CctControl_Sightings::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineXOBJECTLIST("Elements", &_elements);
    F.DefineXOBJECT("ValueElementId", &_valueElementId);
    F.DefineValue("ShowEditButton",  dtBoolean, &_showEditButton, F_TRUE);
    F.DefineValue("ShowUnsaved",     dtBoolean, &_showUnsaved, F_TRUE);

    if (F.IsReader())
    {
        _editButton->AssignFont(&_font);
        _editButton->SetVisible(false);

        if (IsLive())
        {
            Update();
        }
    }
}

void CctControl_Sightings::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_Sightings::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;
    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        auto oldSelectedId = _selectedId;
        if (index < (int)_sightingCount)
        {
            auto database = GetSession(this)->CreateSightingDatabasePtr();
            _selectedId = SightingIndexToId(database.get(), _itemMap.Get(index));
        }
        else
        {
            _selectedId = QString();
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

void CctControl_Sightings::OnEditButtonClick(CfxControl *pSender, void *pParams)
{
    CctSession *session = GetSession(this);
    
    session->SaveState();

    auto originalId = session->GetCurrentSightingUid();
    auto selectedId = _selectedId;

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

void CctControl_Sightings::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive())
    {
        _editButton->SetVisible(false);

        auto paintDatabase = GetSession(this)->CreateSightingDatabasePtr();
        _paintDatabase = paintDatabase.get();
        CfxControl_List::OnPaint(pCanvas, pRect);
        _paintDatabase = nullptr;
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

int CctControl_Sightings::GetItemCount()
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

void CctControl_Sightings::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
{
    QString uid;
    if (Index < _sightingCount)
    {
        Index = _itemMap.Get(Index);
        _paintDatabase->Enum(Index, &uid);
    }

    bool selected = (uid == _selectedId);

    // Connect and load the sighting
    if (uid.isEmpty() || uid == _markId)
    {
        _sighting->Assign(GetSession(this)->GetCurrentSighting());
    }
    else
    {
        _paintDatabase->Load(_sighting, uid);
    }

    ATTRIBUTES *attributes = _sighting->GetAttributes();

    // Build the caption string
    char caption[32];
    FXDATETIME *time = _sighting->GetDateTime();
    if (time->Year == 0)
    {
        sprintf(caption, "%c%s", (_markId == uid) ? '*' : ' ', TEXT_UNSAVED);
    }
    else
    {
        sprintf(caption, "%c%02d-%02d:%02d:%02d", (_markId == uid) ? '*' : ' ', time->Day, time->Hour, time->Minute, time->Second);
    }

    FXRECT rect = *pRect;
    int itemHeight = rect.Bottom - rect.Top + 1;
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
        int xstart = pRect->Left + pCanvas->TextWidth(pFont, caption) + 8;
        for (int i=0; i<attributes->GetCount(); i++)
        {
            XGUID elementId = attributes->GetPtr(i)->ElementId;
            
            FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::Icon32);
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

        for (int i=0; i<attributes->GetCount(); i++)
        {
            ATTRIBUTE *attribute = attributes->GetPtr(i);

            if (!CompareXGuid(&_valueElementId, &attribute->ElementId))
            {
                continue;
            }

            QString valueText;
            GetAttributeText(this, attribute, valueText);

            pCanvas->DrawTextRect(pFont, &rect, TEXT_ALIGN_VCENTER, valueText);
        }
    }

    if (selected)
    {
        _editButton->SetVisible(false);
        if (_showEditButton && !uid.isEmpty() && !GetSession(this)->IsEditing() && !GetSession(this)->GetResourceHeader()->DisableEditing)
        {
            FXRECT rect = *pRect;
            int buttonWidth = pCanvas->TextWidth(pFont, "Edit");
            rect.Left = rect.Right - (buttonWidth + _oneSpace * 16);
            InflateRect(&rect, -_oneSpace, -_oneSpace);
            _editButton->SetVisible(true);

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
    _painter = nullptr;
}

void CctControl_Attributes::SetActiveId(const QString &Value)
{
    ((CctSighting_Painter *)_painter)->ConnectById(Value);
    UpdateScrollBar();
}

void CctControl_Attributes::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    if (F.IsReader() && IsLive())
    {
        SetActiveId(QString());
    }
}

void CctControl_Attributes::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_List::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
}

void CctControl_Attributes::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;

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
    InitBorder(ControlBorderStyle::None, 0);

    _buttonHeight = 16;

    _activeId = QString();
    _onChange.Object = nullptr;
    _onEditClick.Object = nullptr;

    _attributes = new CctControl_Attributes(pOwner, this);
    _attributes->SetComposite(true);
    _attributes->SetDock(ControlDock::Fill);
    _attributes->SetBorderStyle(ControlBorderStyle::None);
    _attributes->SetShowLines(false);

    _buttonPanel = new CfxControl_Panel(pOwner, this);
    _buttonPanel->SetComposite(true);
    _buttonPanel->SetBorderWidth(0);
    _buttonPanel->SetBorderStyle(ControlBorderStyle::Single);
    _buttonPanel->SetBorderLineWidth(1);
    _buttonPanel->SetDock(ControlDock::Top);
    _buttonPanel->SetMinHeight(_buttonHeight);
    _buttonPanel->SetBounds(0, 0, _buttonHeight, _buttonHeight);
    _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_AttributeInspector::OnButtonPanelResize);
    for (int i=0; i<5; i++)
    {
        _buttons[i] = new CfxControl_Button(pOwner, _buttonPanel);
        _buttons[i]->SetComposite(true);
    }
    _buttons[4]->SetCaption("Edit");
}

CctControl_AttributeInspector::~CctControl_AttributeInspector()
{
}

void CctControl_AttributeInspector::OnButtonClick(CfxControl *pSender, void *pParams)
{
    int buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;

    auto databasePtr = GetSession(this)->CreateSightingDatabasePtr();
    auto database = databasePtr.get();

    auto oldActiveId = _activeId;

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

    // Fire change event
    if ((oldActiveId != _activeId) && _onChange.Object)
    {
        ExecuteEvent(&_onChange, this);
        _attributes->SetActiveId(_activeId);
    }

    // Repaint
    Repaint();
}

void CctControl_AttributeInspector::OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    int buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;
    bool down = _buttons[buttonIndex]->GetDown();

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

void CctControl_AttributeInspector::OnButtonPanelResize(CfxControl *pSender, void *pParams)
{
    int buttonCount = IsLive() && GetSession(this)->GetResourceHeader()->DisableEditing ? 4 : 5;
    int buttonW = _buttonPanel->GetClientWidth() / buttonCount;
    int buttonH = _buttonPanel->GetClientHeight();
    int tx = 0;

    for (int i=0; i<4; i++)
    {
        _buttons[i]->SetBounds(tx, 0, buttonW + _oneSpace, buttonH);
        tx += buttonW;
    }

    // Make sure the last button fits properly
    if (buttonCount > 4)
    {
        _buttons[4]->SetBounds(tx, 0, GetClientWidth() - tx, buttonH);
        _buttons[4]->SetVisible(true);
    }
    else
    {
        _buttons[4]->SetVisible(false);
    }
}

void CctControl_AttributeInspector::DefineResources(CfxFilerResource &F)
{
}

void CctControl_AttributeInspector::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.IsReader())
    {
        _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_AttributeInspector::OnButtonPanelResize);
        for (int i=0; i<4; i++)
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

void CctControl_AttributeInspector::SetOnChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onChange.Object = pControl;
    _onChange.Method = OnClick;
}

void CctControl_AttributeInspector::SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick)
{
    _onEditClick.Object = pControl;
    _onEditClick.Method = OnClick;
}

void CctControl_AttributeInspector::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
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
    _onAfterSend.Object = nullptr;
    _hideState = false;
    _packaging = false;
    _state = SendDataState::NoData;
    SetCaption(nullptr);
}

CctControl_SendData::~CctControl_SendData()
{
}

void CctControl_SendData::SetOnAfterSend(CfxControl *pControl, NotifyMethod OnEvent)
{
    _onAfterSend.Object = pControl;
    _onAfterSend.Method = OnEvent;
}

void CctControl_SendData::InitState()
{
    if (!IsLive())
    {
        _state = SendDataState::Idle;
        return;
    }

    auto transferManager = GetSession(this)->GetTransferManager();

    if (transferManager->HasOutgoingData() || GetHost(this)->IsUploadQueueBusy())
    {
        _state = SendDataState::Busy;
        SetEnabled(false);
    }
    else if (transferManager->HasTransferData() || transferManager->HasExportData())
    {
        _state = SendDataState::Idle;
        SetEnabled(true);
    }
    else
    {
        _state = SendDataState::NoData;
        SetEnabled(false);
    }
}

void CctControl_SendData::OnPenClick(int X, int Y)
{
    GetSession(this)->SubmitData();
    Repaint();
}

void CctControl_SendData::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    SetPaintTimer(1000);
    InitState();

    CfxControl_Button::OnPaint(pCanvas, pRect);
    bool invert = GetDown();
    bool filled = true;
    bool drawState = !_hideState;
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
            drawState = false;
            rect = *pRect;
        }
    }

    // Paint the state
    char caption[32] = {0};
    if (_packaging)
    {
        strcpy(caption, "Packaging");
    }
    else if (_state == SendDataState::Busy)
    {
        strcpy(caption, "Sending");
    }
    else
    {
        if (!IsLive() || GetSession(this)->HasMessage())
        {
            strcpy(caption, "Pending");
        }
        else if (GetEnabled())
        {
            strcpy(caption, "Ready");
        }
        else
        {
            strcpy(caption, "No data");
            filled = false;
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

void CctControl_SendData::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("HideState", dtBoolean, &_hideState, F_FALSE);

    if (F.IsReader())
    {
        SetCaption(nullptr);
        InitState();
    }
}

void CctControl_SendData::DefinePropertiesUI(CfxFilerUI &F) 
{
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

    _center = _proportional = true;
    _stretch = _transparentImage = false;
    _bitmap = 0;
    _format = 0;
}

CctControl_ShareData::~CctControl_ShareData()
{
    freeX(_bitmap);
}

void CctControl_ShareData::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    if (_width != _height && _bitmap == 0 && (_caption == nullptr || (strlen(_caption) == 0)))
    {
        if (_dock == ControlDock::Left || _dock == ControlDock::Right)
        {
            _width = _height;
        }
        else if (_dock == ControlDock::Top || _dock == ControlDock::Bottom)
        {
            _height = _width;
        }
        else if (_dock == ControlDock::None)
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

void CctControl_ShareData::DefineProperties(CfxFiler &F) 
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

void CctControl_ShareData::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ShareData::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ShareData::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    bool invert = GetDown();

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
        GetSession(this)->SetShareDataEnabled(true);
    }
}

void CctControl_ShareData::OnPenClick(int X, int Y)
{
    GetSession(this)->ShareData();
}

//*************************************************************************************************
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
    _painter = nullptr;
}

void CctControl_HistoryList::SetActiveIndex(int Value)
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
    _history->SetComposite(true);
    _history->SetDock(ControlDock::Fill);
    _history->SetBorderStyle(ControlBorderStyle::None);
    _history->SetShowLines(false);
    _history->SetTransparent(true);
    _history->SetCaption("-");

    _buttonPanel = new CfxControl_Panel(pOwner, this);
    _buttonPanel->SetComposite(true);
    _buttonPanel->SetTransparent(true);
    _buttonPanel->SetBorderWidth(0);
    _buttonPanel->SetBorderStyle(ControlBorderStyle::Single);
    _buttonPanel->SetBorderLineWidth(1);
    _buttonPanel->SetDock(ControlDock::Top);
    _buttonPanel->SetHeight(_buttonHeight);
    _buttonPanel->SetBounds(0, 0, _buttonHeight, _buttonHeight);
    _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_HistoryInspector::OnButtonPanelResize);
    for (int i=0; i<5; i++)
    {
        _buttons[i] = new CfxControl_Button(pOwner, _buttonPanel);
        _buttons[i]->SetComposite(true);
        _buttons[i]->SetTransparent(true);
    }
    _buttons[4]->SetEnabled(false);
    _buttons[4]->SetCaption("0");

    _itemCount = 0;
    if (IsLive())
    {
        if (GetSession(this)->EnumHistoryInit(nullptr))
        {
            _itemCount = GetSession(this)->GetHistoryItemCount();
            GetSession(this)->EnumHistoryDone();
        }
    }
}

CctControl_HistoryInspector::~CctControl_HistoryInspector()
{
}

void CctControl_HistoryInspector::OnButtonClick(CfxControl *pSender, void *pParams)
{
    int buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;

    int oldActiveIndex = _activeIndex;

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
    char caption[8];
    sprintf(caption, "%lu", _itemCount == 0 ? 0 : _activeIndex + 1);
    _buttons[4]->SetCaption(caption);

    // Repaint
    Repaint();
}

void CctControl_HistoryInspector::OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    int buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;
    bool down = _buttons[buttonIndex]->GetDown();

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

void CctControl_HistoryInspector::OnButtonPanelResize(CfxControl *pSender, void *pParams)
{
    int buttonW = _buttonPanel->GetClientWidth() / 5;
    int buttonH = _buttonPanel->GetClientHeight();
    int tx = 0;

    for (int i=0; i<4; i++)
    {
        _buttons[i]->SetBounds(tx, 0, buttonW + _oneSpace, buttonH);
        tx += buttonW;
    }

    // Make sure the last button fits properly
    _buttons[4]->SetBounds(tx, 0, GetClientWidth() - tx, buttonH);
}

void CctControl_HistoryInspector::DefineResources(CfxFilerResource &F)
{
}

void CctControl_HistoryInspector::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("ButtonHeight", dtInt32, &_buttonHeight, "20");
    F.DefineValue("ItemHeight",  dtInt32, &_itemHeight, "20");

    if (F.IsReader())
    {
        _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_HistoryInspector::OnButtonPanelResize);
        for (int i=0; i<4; i++)
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
            if (session->EnumHistoryInit(nullptr))
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

void CctControl_HistoryInspector::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_HistoryInspector::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonHeight = max(ScaleHitSize, _buttonHeight);
    _buttonPanel->SetMinHeight(_buttonHeight);
}

//*************************************************************************************************
