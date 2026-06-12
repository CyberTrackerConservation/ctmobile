#include "fxApplication.h"
#include "fxUtils.h"
#include "fxMath.h"
#include "ctElementList.h"
#include "ctDialog_NumberEditor.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementList(pOwner, pParent);
}

//*************************************************************************************************
// CctControl_ElementList

CctControl_ElementList::CctControl_ElementList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent), _elements(), _links(), _checkStates(), _itemValues(), _filterItems(), _sortedItems(), _historyStates(), _retainState(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTLIST);
    InitLockProperties("Elements;Icon;Check boxes;Columns;Show caption;List mode");

    SetCaption("<No data: set Elements property>");

    _firstPaint = true;

    _aliasOverride = _aliasDetails = ElementAliasOverride::None;
    _attribute = _attributeDetails = ElementAttribute::Icon32;
    _showCaption = true;
    _saveResult = true;
    _hideLinks = false;
    _listMode = ElementListMode::Radio;
    _selectedIndex = -1;
    _digits = 3;
    _decimals = 0;
    _fraction = 0;
    _minValue = 0;
    _maxValue = 999999999;
    _minChecks = _maxChecks = 0;
    _keypadFormulaMode = false;
    _transparentImages = false;
    _radioSetGoto = false;
    _detailsOnDown = true;

    InitFont(_detailsFont, F_FONT_DEFAULT_M);

    _blockNext = true;
    InitXGuid(&_radioElement);
    
    _lastItemIndex = _detailsItemIndex = _detailsTimerItemIndex = _penDownItemIndex = _penOverItemIndex = -1;
    _detailsShown = false;
    
    _sorted = false;

    _outputChecks = true;
    _numberChecks = false;

    _rightToLeft = false;
    _centerText = false;

    _filterEnabled = false;
    _filterValid = _filterError = false;
    _filter = 0;

    _historyEnabled = _historyCanSelect = false;
    _historyTextColor = 0x808080;
    _historySortType = ElementHistorySortType::None;

    _setLat = _setLon = false;

    // Radio sound buttons
    _showRadioSound = false;
    _showRadioSoundStop = true;

    _playButton = new CfxControl_Button(pOwner, this);
    _playButton->SetComposite(true);
    _playButton->SetVisible(_showRadioSound);
    _playButton->SetBorder(ControlBorderStyle::None, 0, 0, _borderColor);
    _playButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementList::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementList::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, this);
    _stopButton->SetComposite(true);
    _stopButton->SetVisible(_showRadioSound);
    _stopButton->SetBorder(ControlBorderStyle::None, 0, 0, _borderColor);
    _stopButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementList::OnStopClick);
    _stopButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementList::OnStopPaint);

    // State retention
    _retainOnlyScrollState = false;
    _autoSelectIndex = 0;
    _autoRadioNext = false;

    // Global result
    _resultGlobalValue = nullptr;

    // Register for retain state
    RegisterRetainState(this, &_retainState);

    // Register for CanNext and Next events
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementList::OnSessionNext);
    RegisterSessionEvent(seSetJumpTarget, this, (NotifyMethod)&CctControl_ElementList::OnSessionSetJumpTarget);
}

CctControl_ElementList::~CctControl_ElementList()
{
    if (IsLive())
    {
        KillTimer();
        
        if (_showRadioSound)
        {
            GetHost(this)->StopSound();
        }
    }

    UnregisterRetainState(this);

    UnregisterSessionEvent(seSetJumpTarget, this);
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
    
    freeX(_filter);

    MEM_FREE(_resultGlobalValue);
	_resultGlobalValue = nullptr;
}

void CctControl_ElementList::SetRadioSelectedIndex(int Index)
{
    _selectedIndex = Index;

    // Set the global value
    if (IsLive() && (_resultGlobalValue != nullptr))
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, (Index == -1) ? 0 : Index + 1);
    }
}

void CctControl_ElementList::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    bool allow = *((bool *)pParams);

    if (!allow) return;

    if (IsCheckList() || (IsNumberList() && _numberChecks))
    {
        int checkCount = 0;
        char message[200];

        for (int i = 0; i < GetItemCount(); i++)
        {
            int index = GetRawItemIndex(i);
            if (GetCheckState(index) != 0) 
            {
                checkCount++;
            }
        }

        if ((_minChecks > 0) && (checkCount < _minChecks))
        {
            sprintf(message, "Check at least %lu", _minChecks);
            GetSession(this)->ShowMessage(message);
            allow = false;
        }

        if ((_maxChecks > 0) && (_maxChecks >= _minChecks) && (checkCount > _maxChecks))
        {
            sprintf(message, "Check at most %lu", _maxChecks);
            GetSession(this)->ShowMessage(message);
            allow = false;
        }
    }

    switch (_listMode)
    {
    case ElementListMode::Radio:
        if (_blockNext && (_selectedIndex == -1))
        {
            allow = false;
        }
        break;

    case ElementListMode::Check:
    case ElementListMode::CheckIcon:
        break;

    case ElementListMode::Number:
    case ElementListMode::NumberKeypad:
    case ElementListMode::NumberFastTap:
        break;

    case ElementListMode::Gps:
        if (_blockNext && (!_setLat || !_setLon))
        {
            allow = false;
        }
        break;
    }

    *((bool *)pParams) = allow;
}

bool CctControl_ElementList::GetPosition(FXGPS_POSITION *pPosition)
{
    int i;

    memset(pPosition, 0, sizeof(FXGPS_POSITION));
    if (_listMode != ElementListMode::Gps) return false;

    double lat = GetItemValue(0);
    for (i=0; i < GPS_LAT_DECIMALS; i++, lat/=10) {}

    double lon = GetItemValue(1);
    for (i=0; i < GPS_LON_DECIMALS; i++, lon/=10) {}

    double alt = GetItemValue(2);
    for (i=0; i < GPS_ALT_DECIMALS; i++, alt/=10) {}

    pPosition->Latitude = lat;
    pPosition->Longitude = lon;
    pPosition->Altitude = alt;
    pPosition->Quality = FXGPS_QUALITY::User3D;

    return true;
}

void CctControl_ElementList::OnSessionNext(CfxControl *pSender, void *pParams)
{
    int i;

    //
    // Special case for Gps
    //

    if (_listMode == ElementListMode::Gps)
    {
        FXGPS_POSITION position;
        if (GetPosition(&position))
        {
            CctAction_SnapGPS action(GetOwner());
            action.Setup(&position);
            ((CctSession *)pSender)->ExecuteAction(&action);
        }

        return;
    }

    //
    // Append attributes
    //
    CfxResource *resource = GetResource();

    for (i=_elements.GetCount()-1; i>=0; i--)
    {
        if ((IsRadioList() && (i == _selectedIndex)) ||
            (IsCheckList() && GetCheckState(i)) ||
            IsNumberList())
        {
            if (IsNumberList())
            {
                if (_numberChecks)
                {
                    if (!GetCheckState(i)) continue;
                }
                else
                {
                    if (GetItemValue(i) == 0) continue;
                }
            }
            
            XGUID link = _links.Get(i * 2 + 1);
            if (!IsNullXGuid(&link))
            {
                CctAction_InsertScreen action(this);
                action.Setup(link);
                ((CctSession *)pSender)->ExecuteAction(&action);
            }

            if (_historyEnabled)
            {
                SetHistoryState(i, true);
                //CctAction_SetElementListHistory action(this);
                //action.Setup(_id, i, GetHistoryState(i), true);
                //((CctSession *)pSender)->ExecuteAction(&action);
            }
        }
    }

    CctAction_MergeAttribute action(this);
    action.SetControlId(_id);

    CfxStream resultStream;

    for (i=0; i<(int)_elements.GetCount(); i++)
    {
        XGUID elementId = _elements.Get(i);
        if (!IsNullXGuid(&elementId))
        {
            if (IsRadioList() && (i == _selectedIndex))
            {
                if (_radioSetGoto)
                {
                    FXGOTO *gotoPoint = (FXGOTO *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::Goto);
                    if (gotoPoint != nullptr)
                    {
                        GetEngine(this)->SetCurrentGoto(gotoPoint);
                        resource->ReleaseObject(gotoPoint);
                    }
                }

                if (IsNullXGuid(&_radioElement))
                {
                    action.SetupAdd(_id, elementId);
                }
                else
                {
                    CfxResource *resource = GetResource();
                    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::Name);
                    if (element)
                    {
                        FX_ASSERT(element->Magic == MAGIC_TEXT);
                        action.SetupAdd(_id, _radioElement, dtGuid, &element->Guid, elementId);
                        resource->ReleaseObject(element);
                    }
                }
                break;
            }
            else if (IsCheckList() && GetCheckState(i))
            {
                if (!IsNullXGuid(&_radioElement))
                {
                    if (resultStream.GetPosition() > 0)
                    {
                        resultStream.Write("; ");
                    }

                    FXJSONID *jsonId = (FXJSONID *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::JsonId);
                    if (jsonId && !QString(jsonId->Text).startsWith("__")) // Skip generated jsonIds.
                    {
                        FX_ASSERT(jsonId->Magic == MAGIC_JSONID);
                        resultStream.Write(jsonId->Text);
                        resource->ReleaseObject(jsonId);
                    }
                    else
                    {
                        FXTEXTRESOURCE* name = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::Name);
                        if (name)
                        {
                            FX_ASSERT(name->Magic == MAGIC_TEXT);
                            resultStream.Write(name->Text);
                            resource->ReleaseObject(name);
                        }
                    }
                }
                else if (_outputChecks)
                {
                    bool value = true;
                    action.SetupAdd(_id, elementId, dtBoolean, &value);
                }
                else
                {
                    action.SetupAdd(_id, elementId);
                }
            }
            else if (IsNumberList())
            {
                if ((_numberChecks && GetCheckState(i)) ||
                    (!_numberChecks && (GetItemValue(i) != 0)))
                {
                    if (_decimals == 0)
                    {
                        int value = GetItemValue(i);
                        action.SetupAdd(_id, elementId, dtInt32, &value);
                    }
                    else
                    {
                        double value = GetItemValue(i);

                        for (int j=0; j < _decimals; j++, value/=10) {}
                        action.SetupAdd(_id, elementId, dtDouble, &value);
                    }
                }
            }
        }
    }

    if (resultStream.GetPosition() > 0)
    {
        resultStream.Write("\0");
        action.SetupAdd(_id, _radioElement, dtText, resultStream.GetMemory());
    }

    if (action.IsValid() && _saveResult)
    {
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

void CctControl_ElementList::OnSessionSetJumpTarget(CfxControl *pSender, void *pParams)
{
    if (!IsRadioList()) return;

    for (int i=0; i<_elements.GetCount(); i++)
    {
        XGUID link = _links.Get(i * 2 + 1);
        if (CompareXGuid(&link, (XGUID *)pParams))
        {
            SetRadioSelectedIndex(i);
            break;
        }
    }
}

void CctControl_ElementList::OnPlayClick(CfxControl *pSender, void *pParams)
{
    FX_ASSERT(_selectedIndex != -1);

    CfxResource *resource = GetResource();
    FXSOUNDRESOURCE *sound = (FXSOUNDRESOURCE *)resource->GetObject(this, _elements.Get(_selectedIndex), (BYTE)ElementAttribute::Sound);
    if (sound)
    {
        GetHost(this)->PlaySound(sound);
        resource->ReleaseObject(sound);
    }
}

void CctControl_ElementList::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, pParams->Canvas->InvertColor(_color), pParams->Canvas->InvertColor(_borderColor), _playButton->GetDown()); 
}

void CctControl_ElementList::OnStopClick(CfxControl *pSender, void *pParams)
{
    GetHost(this)->StopSound();
}

void CctControl_ElementList::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawStop(pParams->Rect, pParams->Canvas->InvertColor(_color), pParams->Canvas->InvertColor(_borderColor), _stopButton->GetDown()); 
}

void CctControl_ElementList::OnPenDown(int X, int Y)
{
    CfxControl_List::OnPenDown(X, Y);

    int index;
    int offsetX, offsetY;
    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        index = GetRawItemIndex((int)index);

        // Track pen down item for Gps/NumberKeypad modes (need to know which item was initially touched)
        if ((_listMode == ElementListMode::Gps) || (_listMode == ElementListMode::NumberKeypad))
        {
            _penDownItemIndex = _penOverItemIndex = index;
        }

        // Start details hold timer on pen down
        bool detailsActivate = _detailsOnDown &&
                               (_listMode != ElementListMode::Gps) &&
                               (max(_attributeDetails, _attribute) >= ElementAttribute::Icon32) &&
                               ((_listMode == ElementListMode::CheckIcon) || PtInRect(&_iconRect, offsetX, offsetY) || PtInRect(&_textRect, offsetX, offsetY));

        if (detailsActivate)
        {
            _detailsTimerItemIndex = index;
            if (index != -1)
            {
                SetTimer(500);
            }
        }
    }
}

void CctControl_ElementList::OnPenUp(int X, int Y)
{
    _detailsItemIndex = -1;
    _detailsTimerItemIndex = -1;
    KillTimer();
    CfxControl_List::OnPenUp(X, Y);
}

void CctControl_ElementList::OnPenDrag(int StartX, int StartY, int X, int Y)
{
    CfxControl_List::OnPenDrag(StartX, StartY, X, Y);

    _penDownItemIndex = _penOverItemIndex = -1;
    _detailsTimerItemIndex = -1;
    KillTimer();
}

void CctControl_ElementList::OnPenClick(int X, int Y)
{
    if (_detailsShown)
    {
        _detailsShown = false;
        return;
    }

    // Skip selection if details timer is pending (hold-to-show-details in progress)
    if (_detailsTimerItemIndex != -1) return;

    // Perform item selection (deferred from OnPenDown for finger scroll support)
    int index;
    int offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        index = GetRawItemIndex((int)index);

        if ((index != -1) &&
            _historyEnabled && !_historyCanSelect &&
            GetHistoryState(index))
        {
            return;
        }

        switch (_listMode)
        {
        case ElementListMode::Radio:
            SetRadioSelectedIndex(index);
            break;

        case ElementListMode::Check:
        case ElementListMode::CheckIcon:
            SetCheckState(index, !GetCheckState(index));
            break;

        case ElementListMode::Number:
            if (_numberChecks && PtInRect(&_checkRect, offsetX, offsetY))
            {
                SetCheckState(index, !GetCheckState(index));
            }
            else if (PtInRect(&_numberRect, offsetX, offsetY))
            {
                int columnWidth = (GetClientWidth() - GetScrollBarWidth()) / GetColumnCount();
                char valueText[32] = "";
                char oneValue[2]   = "0";
                NumberToText(GetItemValue(index), _digits, _decimals, _fraction, false, valueText, true);
                FXFONTRESOURCE *font = GetResource()->GetFont(this, _font);
                CfxCanvas *canvas = GetCanvas(this);
                BYTE digit = _digits - 1;

                int l = columnWidth - canvas->TextWidth(font, valueText) - (int)_itemHeight / 4;

                if (offsetX > l)
                for (int i=0; i<(int)strlen(valueText); i++)
                {
                    oneValue[0] = valueText[i];
                    l += canvas->TextWidth(font, oneValue);

                    bool isDigit = ((oneValue[0] >= '0') && (oneValue[0] <= '9'));

                    if (l > offsetX)
                    {
                        if (isDigit)
                        {
                            int delta = (offsetY > (int)(_itemHeight / 2)) ? -1 : 1;
                            IncItemValue(index, digit, delta);
                        }
                        break;
                    }
                    if (isDigit) digit--;
                }
                GetResource()->ReleaseFont(font);
            }
            break;

        case ElementListMode::Gps:
        case ElementListMode::NumberKeypad:
            if (_numberChecks && PtInRect(&_checkRect, offsetX, offsetY))
            {
                SetCheckState(index, !GetCheckState(index));
            }
            break;

        case ElementListMode::NumberFastTap:
            if (_numberChecks && PtInRect(&_checkRect, offsetX, offsetY))
            {
                SetCheckState(index, !GetCheckState(index));
            }
            else if (!_numberChecks || GetCheckState(index))
            {
                int value = GetItemValue(index);
                if (PtInRect(&_minusRect, offsetX, offsetY))
                {
                    if (value > _minValue)
                    {
                        SetItemValue(index, value - 1);
                    }
                }
                else if (value < _maxValue)
                {
                    SetItemValue(index, value + 1);
                }
            }
            break;

        default:
            FX_ASSERT(false);
        }

        Repaint();
    }

    // Autonext for radio lists
    if (IsRadioList())
    {
        if (_autoRadioNext && (_selectedIndex != -1))
        {
            GetEngine(this)->KeyPress(KEY_RIGHT, 100);
        }
    }
    else if (IsCheckList())
    {
    }
    else if (IsNumberList())
    {
        int index = ((_penDownItemIndex != -1) && (_penDownItemIndex == _penOverItemIndex)) ? _penDownItemIndex : -1;
        _penDownItemIndex = _penOverItemIndex = -1;

        if (index != -1)
        {
            NUMBEREDITOR_PARAMS params = {0};
            params.ControlId = GetId();
            params.Index = index;
            params.Digits = _digits;
            params.Decimals = _decimals;
            params.Fraction = _fraction;
            params.MaxValue = _maxValue;
            params.MinValue = _minValue;
            params.FormulaMode = _keypadFormulaMode;

            if (_listMode == ElementListMode::Gps)
            {
                params.Fraction = 0;
                switch (index)
                {
                case 0:
                    params.Digits = GPS_LAT_DIGITS;
                    params.Decimals = GPS_LAT_DECIMALS;
                    params.MinValue = -90;
                    params.MaxValue = 90;
                    _setLat = true;
                    strncpy(params.Title, "Enter Latitude", sizeof(params.Title)-1);
                    break;
                case 1:
                    params.Digits = GPS_LON_DIGITS;
                    params.Decimals = GPS_LON_DECIMALS;
                    params.MinValue = -180;
                    params.MaxValue = +180;
                    _setLon = true;
                    strncpy(params.Title, "Enter Longitude", sizeof(params.Title)-1);
                    break;
                case 2:
                    params.Digits = GPS_ALT_DIGITS;
                    params.Decimals = GPS_ALT_DECIMALS;
                    params.MinValue = -5000;
                    params.MaxValue = 100000;
                    strncpy(params.Title, "Enter Altitude", sizeof(params.Title)-1);
                    break;
                default:
                    FX_ASSERT(false);
                }
            }
            else if (_numberChecks)
            {
                SetCheckState(index, true);
            }

            // Get the value
            int itemValue = GetItemValue(index);
            params.Value = EncodeDouble(FxAbs(itemValue), params.Decimals, itemValue < 0);

            // Pick up the title from the Element
            if (strlen(params.Title) == 0)
            {
                CfxResource *resource = GetResource();
                XGUID elementId = _elements.Get(index);
                FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::Name);
                if (element)
                {
                    FX_ASSERT(element->Magic == MAGIC_TEXT);
                    strncpy(params.Title, GetElementText(element), sizeof(params.Title)-1);
                    resource->ReleaseObject(element);
                }
            }

            GetSession(this)->ShowDialog(&GUID_DIALOG_NUMBEREDITOR, nullptr, &params, sizeof(NUMBEREDITOR_PARAMS)); 
        }
        else
        {
            Repaint();
        }
    }
}

void CctControl_ElementList::OnTimer()
{
    KillTimer();

    _detailsItemIndex = _detailsTimerItemIndex;
    _detailsTimerItemIndex = -1;
    _detailsShown = true;

    _scrollBar->SetVisible(false);
    Repaint();
}

void CctControl_ElementList::RebuildLinks()
{
}

struct FILTERCATEGORY
{
    int Type = 0;
    int Count = 0;
    XGUID Items[1];
};

bool FilterMatch(int Type, BYTE *pSightingMask, BYTE *pElementMask, int ByteCount)
{
    switch (Type)
    {
    case 0:     // Subset
        {
            //bool atLeast1 = false;
            for (int i=0; i<ByteCount; i++)
            {
                BYTE match = (~(*pSightingMask) | (*pSightingMask & *pElementMask));
                if (match != 0xFF) return false;
                //atLeast1 = atLeast1 || (*pSightingMask != 0);
                pSightingMask++;
                pElementMask++;
            }
            return true;
        }
        break;

    case 1:     // Any
        {
            for (int i=0; i<ByteCount; i++)
            {
                if ((*pSightingMask & *pElementMask) != 0) return true;
                pSightingMask++; pElementMask++;
            }
            return false;
        }
        break;

    case 2:     // Exact
        {
            for (int i=0; i<ByteCount; i++)
            {
                if ((*pSightingMask & *pElementMask) != *pElementMask) return false;
                pSightingMask++; pElementMask++;
            }
            return true;
        }
        break;

    
    default:
        return false;
    }

    return false;
} 

void SetBit(BYTE *pBits, int Index)
{
    BYTE *value = pBits + (Index >> 3);
    BYTE mask = 0x80 >> (Index & 7);
    *value = (*value & ~mask) | mask;
}

XGUIDLIST *BuildSightingAttributes(CctSession *pSession)
{
    XGUIDLIST *list = new XGUIDLIST();
    ATTRIBUTES *attributes = pSession->GetCurrentSighting()->GetAttributes();
    for (int i=0; i<attributes->GetCount(); i++)
    {
        ATTRIBUTE *attribute = attributes->GetPtr(i);

        list->Add(attribute->ElementId);

        if (!IsNullXGuid(&attribute->ValueId))
        {
            list->Add(attribute->ValueId);
        }
    }
    return list;
}

bool CctControl_ElementList::ExecuteFilter()
{
	_filterError = false;
    _filterItems.Clear();
    SetCaption("No matching Elements");

    // Validate the resource
    CfxResource *resource = GetResource();
    FXBINARYRESOURCE *filter = resource->GetBinary(this, _filter);
    if (!filter) return false;
    FX_ASSERT(filter->Magic == MAGIC_BINARY);
    FX_ASSERT(filter->BinaryMagic == MAGIC_ELEMENT_FILTER);

    //
    // Setup before the match
    //
    int i, j;
    BYTE *data = &filter->Data[0];

    // Parse the structure and build the list of categories
    int categoryCount = *(int *)data; data += sizeof(int);
    FILTERCATEGORY **categories = (FILTERCATEGORY **)MEM_ALLOC(categoryCount * sizeof(FILTERCATEGORY *));
    int maskByteCount = 0;
    for (i=0; i<categoryCount; i++)
    {
        FILTERCATEGORY *category = (FILTERCATEGORY *)data;
        categories[i] = category;
        data += (int)sizeof(FILTERCATEGORY) + (category->Count - 1) * (int)sizeof(XGUID);
        maskByteCount += (category->Count + 7) / 8;
    }
    int elementCount = *(int *)data; data += sizeof(int);
    BYTE *firstElement = data;

    // Build the sightingMask
    XGUIDLIST *attributes = BuildSightingAttributes(GetSession(this));
    BYTE *sightingMask = (BYTE *)MEM_ALLOC(maskByteCount);
    memset(sightingMask, 0, maskByteCount);
    BYTE *currentMask = sightingMask;
    for (i=0; i<categoryCount; i++)
    {
        FILTERCATEGORY *category = categories[i];
        for (j=0; j<category->Count; j++)
        {
            if (attributes->IndexOf(category->Items[j]) != -1)
            {
                SetBit(currentMask, j);
            }
        }
        currentMask += (category->Count + 7) / 8;
    }

    //
    // Run matching system
    //
    if (_elements.GetCount() > 0) 
    { 
        XGUID *elementListId = _elements.GetPtr(0);
        BYTE *elementData = firstElement;
        for (i=0; i<_elements.GetCount(); i++, elementListId++)
        {
            if (!CompareXGuid(elementListId, (XGUID *)elementData))
            {
                SetCaption("Filter out of date");
                _filterError = true;
                break;
            }
            elementData += sizeof(XGUID);

            // Iterate over all the categories and match the element to each
            bool match = true;
            BYTE *currentSightingMask = sightingMask;
            for (j=0; j<categoryCount; j++)
            {
                FILTERCATEGORY *category = categories[j];
                int byteCount = (category->Count + 7) / 8;
                match = match && FilterMatch(category->Type, currentSightingMask, elementData, byteCount);
                
                elementData += byteCount;
                currentSightingMask += byteCount;
            }

            // If it matches then add it to the filterItem list
            if (match)
            {   
                _filterItems.Add(i);
            }
        }
    }

    MEM_FREE(categories);
    MEM_FREE(sightingMask);
    delete attributes;

    resource->ReleaseBinary(filter);

    if (!_filterError && IsLive() && (GetSession(this)->GetCurrentSighting()->GetPathCount() == 1)) 
    {
        return false;
    }

    return true;
}

void CctControl_ElementList::ValidateItemCount()
{
    if (IsNumberList())
    {
        if (_listMode == ElementListMode::Gps)
        {
            _itemValues.SetCount(3);
        }
        else
        {
            _itemValues.SetCount(_elements.GetCount());
        }
    }
}

void CctControl_ElementList::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementList::DefineProperties(CfxFiler &F)
{
    bool legacyUseKeypad = false;

    _firstPaint = true;

    CfxControl_List::DefineProperties(F);
    _retainState.DefineProperties(F);

    // ELEMENT_LIST_VERSION
    F.DefineValue("Version",            dtInt32,   &_version);

    F.DefineValue("AutoSelectIndex",    dtInt32,   &_autoSelectIndex, F_ZERO);
    F.DefineValue("AutoRadioNext",      dtBoolean, &_autoRadioNext, F_FALSE);
    F.DefineValue("SaveResult",         dtBoolean, &_saveResult, F_TRUE);
    F.DefineValue("ShowCaption",        dtBoolean, &_showCaption, F_TRUE);
    F.DefineValue("ShowRadioSound",     dtBoolean, &_showRadioSound, F_FALSE);
    F.DefineValue("ShowRadioSoundStop", dtBoolean, &_showRadioSoundStop, F_TRUE);
    F.DefineValue("RadioSetGoto",       dtBoolean, &_radioSetGoto, F_FALSE);
    F.DefineValue("RadioBlockNext",     dtBoolean, &_blockNext, F_TRUE);
    F.DefineValue("AliasDetails",       dtInt8,    &_aliasDetails, F_ZERO);
    F.DefineValue("AliasOverride",      dtInt8,    &_aliasOverride, F_ZERO);
    F.DefineValue("Attribute",          dtInt8,    &_attribute, "9");
    F.DefineValue("AttributeDetails",   dtInt8,    &_attributeDetails, "9");
    F.DefineValue("DetailsOnDown",      dtBoolean, &_detailsOnDown, F_TRUE);
    F.DefineValue("ListMode",           dtInt8,    &_listMode, F_ZERO);
    F.DefineValue("Digits",             dtByte,    &_digits, "3");
    F.DefineValue("Decimals",           dtByte,    &_decimals, F_ZERO);
    F.DefineValue("Fraction",           dtByte,    &_fraction, F_ZERO);
    F.DefineValue("TransparentImages",  dtBoolean, &_transparentImages, F_FALSE);
    F.DefineValue("FilterEnabled",      dtBoolean, &_filterEnabled, F_FALSE);
    F.DefineValue("RetainOnlyScrollState", dtBoolean, &_retainOnlyScrollState, F_FALSE);
    F.DefineValue("HideLinks",          dtBoolean, &_hideLinks, F_FALSE);
    F.DefineValue("HistoryEnabled",     dtBoolean, &_historyEnabled, F_FALSE);
    F.DefineValue("HistoryTextColor",   dtColor,   &_historyTextColor, F_COLOR_GRAY);
    F.DefineValue("HistoryCanSelect",   dtBoolean, &_historyCanSelect, F_FALSE);
    F.DefineValue("HistorySortType",    dtByte,    &_historySortType, F_ZERO);
    F.DefineValue("Sorted",             dtBoolean, &_sorted, F_FALSE);
    F.DefineValue("UseKeypadEditor",    dtBoolean, &legacyUseKeypad, F_FALSE);
    F.DefineValue("KeypadFormulaMode",  dtBoolean, &_keypadFormulaMode, F_FALSE);
    F.DefineValue("KeypadMinValue",     dtInt32,   &_minValue, F_ZERO);
    F.DefineValue("KeypadMaxValue",     dtInt32,   &_maxValue, "999999999");
    F.DefineValue("MinChecks",          dtInt32,   &_minChecks, F_ZERO);
    F.DefineValue("MaxChecks",          dtInt32,   &_maxChecks, F_ZERO);
    F.DefineValue("OutputChecks",       dtBoolean, &_outputChecks, F_TRUE);
    F.DefineValue("NumberChecks",       dtBoolean, &_numberChecks, F_FALSE);
    F.DefineValue("RightToLeft",        dtBoolean, &_rightToLeft, F_FALSE);
    F.DefineValue("CenterText",         dtBoolean, &_centerText, F_FALSE);

    F.DefineXOBJECTLIST("Elements", &_elements);
    F.DefineXOBJECTLIST("Links",    &_links);
    F.DefineXFONT("DetailsFont",    &_detailsFont, F_FONT_DEFAULT_M);
    F.DefineXBINARY("Filter",       &_filter);
    F.DefineXOBJECT("RadioElement", &_radioElement);

    F.DefineValue("ResultGlobalValue", dtPText, &_resultGlobalValue);


    RebuildLinks();

    if (F.IsReader())
    {
        CctSession *session = IsLive() ? GetSession(this) : nullptr;

        if (_version < ELEMENT_LIST_VERSION)
        {
            _blockNext = true;
            _outputChecks = false;
            _rightToLeft = false;
            _centerText = false;
        }

        if (session && _historyEnabled) 
        {
            STOREITEM *item = session->GetStoreItem(ConstructGuid(*session->GetCurrentScreenId()), 
                                                    (int)(-(int)_id));
            if (item)
            {
                CfxStream stream(item->Data);
                _historyStates.LoadFromStream(stream);
            }
        }

        _filterValid = session && _filterEnabled && ExecuteFilter();

        _minItemHeight = max(16, _minItemHeight);
        _minItemWidth  = max(32, _minItemWidth);

        SetRadioSelectedIndex(-1);
        _checkStates.Clear();
        _itemValues.Clear();

        _decimals = min(_decimals, _digits-1);
        if (_fraction > 1)
        {
            _decimals = 3;
        }
       
        _stopButton->SetVisible(false);
        _playButton->SetVisible(false);

        UpdateScrollBar();

        if (IsRadioList())
        {
            if ((_autoSelectIndex > 0) && (_autoSelectIndex <= GetItemCount()))
            {
                int index = GetRawItemIndex(_autoSelectIndex - 1);
                SetRadioSelectedIndex(index);
            }
        }
        else if (IsCheckList())
        {
            if (_listMode == ElementListMode::CheckIcon)
            {
                _showCaption = false;
            }
        }
        else if (IsNumberList())
        {
            if ((legacyUseKeypad || (_fraction > 1)) && (_listMode != ElementListMode::Gps))
            {
                _listMode = ElementListMode::NumberKeypad;
            }
        }
        
        _setLat = _setLon = false;

        _minValue = min(_minValue, _maxValue);
        _maxValue = max(_maxValue, _minValue);

        /*#ifdef CLIENT_DLL
            //
            // Hack to add "Radio element" to the lock properties for radio lists.
            //
            const char *kRadioElement = "Radio element";
            if ((_listMode == ElementListMode::Radio) && (strstr(_lockProperties, kRadioElement) == 0))
            {
                char *newLockProperties = (char *)MEM_ALLOC(strlen(_lockProperties) + strlen(kRadioElement) + 2);
                strcpy(newLockProperties, _lockProperties);
                strcat(newLockProperties, ";");
                strcat(newLockProperties, kRadioElement);

                MEM_FREE(_lockProperties);
                _lockProperties = newLockProperties;
            }
        #endif*/
    }
}

void CctControl_ElementList::ResetState()
{
    CfxControl_List::ResetState();

    _selectedIndex = -1;
    _checkStates.Clear();
    _itemValues.Clear();
    _historyStates.Clear();

    if (IsLive())
    {
        CctSession *session = GetSession(this);
        session->SetStoreItem(ConstructGuid(*session->GetCurrentScreenId()), (int)(-(int)_id), nullptr);    
    }

    _setLat = _setLon = false;

    ValidateItemCount();
}

void CctControl_ElementList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    _retainState.DefineState(F);

    if (!_retainOnlyScrollState) 
    {
        F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
        F.DefineValue("CheckStates", dtPBinary, _checkStates.GetMemoryPtr());
        F.DefineValue("ItemValues", dtPBinary, _itemValues.GetMemoryPtr());
        F.DefineValue("HasSetLat", dtBoolean, &_setLat);
        F.DefineValue("HasSetLon", dtBoolean, &_setLon);

        if (F.IsReader())
        {
            SetRadioSelectedIndex(_selectedIndex);
        }
    }

    ValidateItemCount();
}

void CctControl_ElementList::DefinePropertiesUI(CfxFilerUI &F) 
{
}

CfxResource *g_sortResource;
CctControl_ElementList *g_sortControl;
XGUIDLIST *g_sortElements;
bool g_sortNames;
ElementHistorySortType g_sortHistory;

int SortCompare(const void *pItem1, const void *pItem2)
{
    int ret = 0;
    int index1, index2;
    int s1, s2;

    index1 = *(int *)pItem1;
    XGUID elementId1 = g_sortElements->Get(index1);
    FXTEXTRESOURCE *element1 = (FXTEXTRESOURCE *)g_sortResource->GetObject(g_sortControl, elementId1, (BYTE)ElementAttribute::Name);

    index2 = *(int *)pItem2;
    XGUID elementId2 = g_sortElements->Get(index2);
    FXTEXTRESOURCE *element2 = (FXTEXTRESOURCE *)g_sortResource->GetObject(g_sortControl, elementId2, (BYTE)ElementAttribute::Name);

    if (element1 && element2)
    {
        s1 = s2 = 0;
        if (g_sortHistory != ElementHistorySortType::None)
        {
            s1 = (int)g_sortControl->GetHistoryState(index1);
            s2 = (int)g_sortControl->GetHistoryState(index2);

            if (g_sortHistory == ElementHistorySortType::Bottom)
            {
                ret = SIGN(s1 - s2);
            }
            else
            {
                ret = SIGN(s2 - s1);
            }
        }

        if (s1 == s2)
        {
            if (g_sortNames)
            {
                ret = strcmp(g_sortControl->GetElementText(element1), g_sortControl->GetElementText(element2));
            }
            else
            {
                s1 = g_sortElements->IndexOf(elementId1);
                s2 = g_sortElements->IndexOf(elementId2);
                ret = SIGN(s1 - s2);
            }
        }
    }

    if (element1)
    {
        g_sortResource->ReleaseObject(element1);
    }

    if (element2)
    {
        g_sortResource->ReleaseObject(element2);
    }

    return ret;
}

void CctControl_ElementList::ExecuteSort()
{
    // Setup the sort list
    int count = GetItemCount(); 
    _sortedItems.SetCount(count);
    for (int i=0; i<count; i++)
    {
        _sortedItems.Put(i, _filterValid ? _filterItems.Get(i) : i);
    }
    
    g_sortResource = GetResource();
    g_sortControl = this;
    g_sortElements = &_elements;
    g_sortNames = this->_sorted;
    g_sortHistory = this->_historySortType;

    // Run the sort
    if (g_sortResource)
    {
        qsort(_sortedItems.GetMemory(), count, sizeof(int), SortCompare);
    }

    g_sortResource = nullptr;
    g_sortControl = nullptr;
    g_sortElements = nullptr;
}

int CctControl_ElementList::GetRawItemIndex(int Index)
{
    if (_sorted || (_historySortType != ElementHistorySortType::None))
    {
        if (_sortedItems.GetCount() != GetItemCount())
        {
            ExecuteSort();
        }

        return _sortedItems.Get(Index);
    }
    else if (_filterValid)
    {
        return _filterItems.Get(Index);
    }
    else
    {
        return Index;
    }
}

int CctControl_ElementList::GetItemCount()
{
    if (_listMode == ElementListMode::Gps)
    {
        return 3;
    }

    if (_filterValid)
    {
        return _filterError ? 0 : _filterItems.GetCount();
    }
    else
    {
        return _elements.GetCount();
    }
}

XGUID CctControl_ElementList::GetItem(int Index)
{
    if (_listMode == ElementListMode::Gps)
    {
        XGUID x;
        InitXGuid(&x);
        return x;
    }

    Index = GetRawItemIndex(Index);
    return _elements.Get(Index);
}

char *CctControl_ElementList::GetElementText(FXTEXTRESOURCE *Element)
{
    int i;
    char *result = Element->Text;

    for (i = 0; i < (int)_aliasOverride; i++)
    {
        result += strlen(result) + 1;
    }

    if (strlen(result) == 0)
    {
        result = Element->Text;
    }

    return result;
}

char *CctControl_ElementList::GetDetailsText(FXTEXTRESOURCE *Element)
{
    int i;
    char *result = Element->Text;

    for (i = 0; i < (int)_aliasDetails; i++)
    {
        result += strlen(result) + 1;
    }

    if (strlen(result) == 0)
    {
        result = Element->Text;
    }

    return result;
}

bool CctControl_ElementList::GetCheckState(int Index)
{
    _checkStates.SetCount((_elements.GetCount() + 7) >> 3);
    BYTE byte = _checkStates.Get(Index >> 3);
    
    byte = (byte >> (Index & 7)) & 1;

    return byte != 0;
}

void CctControl_ElementList::SetCheckState(int Index, bool Value)
{
    _checkStates.SetCount((_elements.GetCount() + 7) >> 3);

    BYTE byte = _checkStates.Get(Index >> 3);
    BYTE shift = (char)(Index & 7);

    byte = (byte & ~(1 << shift)) | ((Value & 1) << shift);
    _checkStates.Put(Index >> 3, byte);
}

bool CctControl_ElementList::GetHistoryState(int Index)
{
    _historyStates.SetCount((_elements.GetCount() + 7) >> 3);
    BYTE byte = _historyStates.Get(Index >> 3);
    
    byte = (byte >> (Index & 7)) & 1;

    return byte != 0;
}

void CctControl_ElementList::SetHistoryState(int Index, bool Value)
{
    _historyStates.SetCount((_elements.GetCount() + 7) >> 3);

    BYTE byte = _historyStates.Get(Index >> 3);
    BYTE shift = (char)(Index & 7);

    byte = (byte & ~(1 << shift)) | ((Value & 1) << shift);
    _historyStates.Put(Index >> 3, byte);

    CfxStream stream;
    _historyStates.SaveToStream(stream);
    stream.GetMemory();

    CctSession *session = GetSession(this);
    session->SetStoreItem(ConstructGuid(*session->GetCurrentScreenId()), (int)(-(int)_id), stream.CloneMemory());
}

int CctControl_ElementList::GetItemValue(int Index)
{
    ValidateItemCount();
    return _itemValues.Get(Index);
}

void CctControl_ElementList::SetItemValue(int Index, int Value)
{
    ValidateItemCount();
    _itemValues.Put(Index, Value);
}

void CctControl_ElementList::IncItemValue(int Index, BYTE Digit, int Delta)
{
    int offset = 1;
    for (int i=0; i<Digit; i++, offset*=10) {}

    int itemValue = GetItemValue(Index);

    int digitValue = (itemValue / offset) % 10;

    if (digitValue + Delta > 9)
    {
        itemValue = itemValue - 9 * offset;
    }
    else if (digitValue + Delta < 0)
    {
        itemValue = itemValue + 9 * offset;
    }
    else
    {
        itemValue = itemValue + offset * Delta;
    }
    SetItemValue(Index, itemValue);
}

void CctControl_ElementList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
{
    COLOR lastTextColor = _textColor;
    COLOR lastBorderColor = _borderColor;

    Index = GetRawItemIndex(Index);

    if (_historyEnabled && GetHistoryState(Index))
    {
        _textColor = _historyTextColor;
        _borderColor = _historyTextColor;
    }

    switch (_listMode)
    {
    case ElementListMode::Radio:
        PaintRadio(pCanvas, pRect, pFont, Index);
        break;

    case ElementListMode::Check:
        PaintCheck(pCanvas, pRect, pFont, Index);
        break;

    case ElementListMode::CheckIcon:
        PaintCheckIcon(pCanvas, pRect, pFont, Index);
        break;
    
    case ElementListMode::Number:
    case ElementListMode::NumberFastTap:
    case ElementListMode::NumberKeypad:
        PaintNumber(pCanvas, pRect, pFont, Index);
        break;

    case ElementListMode::Gps:
        PaintGps(pCanvas, pRect, pFont, Index);
        break;
    }

    _textColor = lastTextColor;
    _borderColor = lastBorderColor;
}

void CctControl_ElementList::DrawItemCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, bool Checked, bool Selected)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    int itemHeight = itemRect.Bottom - itemRect.Top + 1;

    FXRECT rect = *pItemRect;
    rect.Right = rect.Left + itemHeight - 1;

    _checkRect = rect;
    OffsetRect(&_checkRect, -rect.Left, -rect.Top);

    pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Selected);
    pCanvas->FillRect(&rect);

    pCanvas->State.LineColor = pCanvas->InvertColor(_textColor, Selected);

    InflateRect(&rect, -_oneSpace * 2, -_oneSpace * 2);
    pCanvas->State.LineWidth = 2 * _oneSpace;
    pCanvas->FrameRect(&rect);

    if (Checked)
    {
        InflateRect(&rect, -4 * _oneSpace, -4 * _oneSpace);
        pCanvas->State.BrushColor = pCanvas->InvertColor(_textColor, Selected);
        pCanvas->FillRect(&rect);
    }

    pRect->Left += itemHeight - 2 * _oneSpace;
}

void CctControl_ElementList::DrawItemIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, XGUID ElementId, bool Selected)
{
    CfxResource *resource = GetResource();
    FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, ElementId, (BYTE)_attribute);
    if (bitmap)
    {
        FX_ASSERT(bitmap->Magic == MAGIC_BITMAP);

        FXRECT rect = *pRect;
        int itemHeight = rect.Bottom - rect.Top + 1;
        int itemWidth = rect.Right - rect.Left + 1;
        int scaleTo = min(itemHeight, itemWidth);

        if (_showCaption || (_detailsItemIndex != -1))
        {
            if (_rightToLeft)
            {
                rect.Right = rect.Left + scaleTo;
            }
            else
            {
                rect.Left = rect.Right - scaleTo;
            }
        }
        else
        {
            rect.Left = rect.Left + (itemWidth - scaleTo) / 2;
            rect.Top = rect.Top + (itemHeight - scaleTo) / 2;
        }

        //rect.Left = max(pRect->Left, rect.Left);

        FXRECT stretchRect;
        GetBitmapRect(bitmap, scaleTo - 2 * _oneSpace, scaleTo - 2 * _oneSpace, true, true, true, &stretchRect);
        OffsetRect(&stretchRect, rect.Left + _oneSpace, rect.Top + _oneSpace);
        pCanvas->StretchDraw(bitmap, stretchRect.Left, stretchRect.Top, stretchRect.Right - stretchRect.Left + 1, stretchRect.Bottom - stretchRect.Top + 1, Selected, _transparentImages);
        resource->ReleaseObject(bitmap);

        _iconRect = rect;
        OffsetRect(&_iconRect, -pItemRect->Left, -pItemRect->Top);
        _iconRect.Right = _iconRect.Left + scaleTo;
        _iconRect.Bottom = _iconRect.Top + scaleTo;
    }
}

void CctControl_ElementList::DrawItemText(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, FXFONTRESOURCE *pFont, XGUID ElementId, bool Selected)
{
    CfxResource *resource = GetResource();

    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, ElementId, (BYTE)ElementAttribute::Name);
    if (element)
    {
        FX_ASSERT(element->Magic == MAGIC_TEXT);

        int flags = TEXT_ALIGN_VCENTER;
        FXRECT rect = *pRect;

        if (_rightToLeft)
        {
            flags |= TEXT_ALIGN_RIGHT;
            rect.Right -= 4 * _oneSpace;
        }
        else if (_centerText)
        {
            flags |= TEXT_ALIGN_HCENTER;
        }
        else
        {
            rect.Left += 4 * _oneSpace;
        }

        pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, Selected);
        pCanvas->DrawTextRect(pFont, &rect, flags, GetElementText(element));
        resource->ReleaseObject(element);

        _textRect = *pRect;
        OffsetRect(&_textRect, -pItemRect->Left, -pItemRect->Top);
    }
}

void CctControl_ElementList::PaintRadio(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    CfxResource *resource = GetResource();
    int itemHeight = pItemRect->Bottom - pItemRect->Top + 1;

    // Cleanup the selected index if it has already been set
    if (_historyEnabled && (_selectedIndex != -1) && !_historyCanSelect && GetHistoryState(_selectedIndex))
    {
        _selectedIndex = -1;
    }

    // Get the element
    XGUID elementId = _elements.Get(Index);

    // Paint if selected
    bool selected = ((int)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);

        // Turn on the radio sound buttons if a sound exists
        if (_showRadioSound)
        {
            FXSOUNDRESOURCE *sound = (FXSOUNDRESOURCE *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::Sound);
            if (sound)
            {
                resource->ReleaseObject(sound);

                // Move and display the buttons
                FXRECT absoluteRect;
                GetAbsoluteBounds(&absoluteRect);
                int tx = pItemRect->Right - absoluteRect.Left;
                int ty = pItemRect->Top - absoluteRect.Top;

                // Stop button
                if (_showRadioSoundStop)
                {
                    pItemRect->Right -= itemHeight;
                    tx -= itemHeight;
                    _stopButton->SetVisible(true);
                    _stopButton->SetBounds(tx + 1, ty, itemHeight, itemHeight);
                    
                }

                // Play button
                pItemRect->Right -= itemHeight;
                tx -= itemHeight;
                _playButton->SetVisible(true);
                _playButton->SetBounds(tx, ty, itemHeight, itemHeight);

            }
            else
            {
                _playButton->SetVisible(false);
                _stopButton->SetVisible(false);
            }
        }
    }

    // Draw the caption
    if (_showCaption)
    {
        DrawItemText(pCanvas, pItemRect, pItemRect, pFont, elementId, selected);
    }

    // Draw the bitmap
    if (_attribute >= ElementAttribute::Icon32)
    {
        DrawItemIcon(pCanvas, pItemRect, pItemRect, elementId, selected);
    }
}

void CctControl_ElementList::PaintCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    int itemHeight = itemRect.Bottom - itemRect.Top + 1;

    // Get the element
    XGUID elementId = _elements.Get(Index);

    // Draw the check
    DrawItemCheck(pCanvas, pItemRect, &itemRect, GetCheckState(Index), false);

    // Draw the caption
    if (_showCaption)
    {
        DrawItemText(pCanvas, pItemRect, &itemRect, pFont, elementId);
    }

    // Draw the bitmap
    if (_attribute >= ElementAttribute::Icon32)
    {
        DrawItemIcon(pCanvas, pItemRect, &itemRect, elementId);
    }
}

void CctControl_ElementList::PaintCheckIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    int itemHeight = itemRect.Bottom - itemRect.Top + 1;

    // Get the element
    XGUID elementId = _elements.Get(Index);

    FXRECT rect = *pItemRect;
    int checkThickness = 5 * _oneSpace;

    // Draw the bitmap
    InflateRect(&itemRect, -checkThickness, -checkThickness);
    if (_attribute >= ElementAttribute::Icon32)
    {
        DrawItemIcon(pCanvas, pItemRect, &itemRect, elementId);
    }

    pCanvas->State.LineColor = _textColor;

    // Draw the check
    if (GetCheckState(Index))
    {
        pCanvas->State.LineWidth = checkThickness;
        pCanvas->FrameRect(&rect);
    }
    else
    {
        InflateRect(&rect, -checkThickness + _borderLineWidth, -checkThickness + _borderLineWidth);
        pCanvas->State.LineWidth = _borderLineWidth;
        pCanvas->FrameRect(&rect);
    }
}

void CctControl_ElementList::PaintNumber(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    int itemHeight = itemRect.Bottom - itemRect.Top + 1;

    // Get the element
    XGUID elementId = _elements.Get(Index);

    // Invert if needed
    bool selected = ((int)Index == _penDownItemIndex) && ((int)Index == _penOverItemIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the numbers    
    {
        int value = GetItemValue(Index); 
        char valueText[32] = "-";
        bool showNumber = false;
        if ((_numberChecks == false) || GetCheckState(Index))
        {
            NumberToText(FxAbs(value), _digits, _decimals, _fraction, value < 0, valueText, _listMode == ElementListMode::Number);
            showNumber = true;
        }


        FXRECT rect = itemRect;
        rect.Left = rect.Right - pCanvas->TextWidth(pFont, valueText) - itemHeight / 4;
        pCanvas->State.BrushColor = selected ? pCanvas->InvertColor(_color) : _color;
        pCanvas->FillRect(&rect);

        pCanvas->State.TextColor = selected ? pCanvas->InvertColor(_textColor) : _textColor;
        pCanvas->DrawTextRect(pFont, &rect, TEXT_ALIGN_VCENTER, valueText);

        if (showNumber)
        {
            _numberRect = rect;
            OffsetRect(&_numberRect, -pItemRect->Left, -pItemRect->Top);
        }

        itemRect.Right = rect.Left - 4;
    }

    // Draw the check
    if (_numberChecks)
    {
        DrawItemCheck(pCanvas, pItemRect, &itemRect, GetCheckState(Index), selected);
    }

    // Draw the minus
    if (_listMode == ElementListMode::NumberFastTap)
    {
        FXRECT rect = itemRect;
        rect.Right = rect.Left + itemHeight / 2;

        _minusRect = rect;
        _minusRect.Right = rect.Left + (itemRect.Right - itemRect.Left + 1) / 3;
        OffsetRect(&_minusRect, -pItemRect->Left, -pItemRect->Top);

        pCanvas->State.BrushColor = _textColor;

        rect.Left += _oneSpace * 2;
        rect.Right -= _oneSpace * 2;
        rect.Top = rect.Top + _itemHeight / 2 - _oneSpace;
        rect.Bottom = rect.Top + _oneSpace * 2;
        pCanvas->FillRect(&rect);

        itemRect.Left += itemHeight / 2;
    }
   
    // Draw the caption
    if (_showCaption)
    {
        DrawItemText(pCanvas, pItemRect, &itemRect, pFont, elementId, selected);
    }

    // Draw the bitmap
    if (_attribute >= ElementAttribute::Icon32)
    {
        DrawItemIcon(pCanvas, pItemRect, &itemRect, elementId, selected);
    }
}

void CctControl_ElementList::PaintGps(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    int itemHeight = itemRect.Bottom - itemRect.Top + 1;
    static const char MAX_NUMBER_TEXT[] = "-180.88888";
    int maxNumberWidth = pCanvas->TextWidth(pFont, (char *)MAX_NUMBER_TEXT);

    // Invert if needed
    bool selected = ((int)Index == _penDownItemIndex) && ((int)Index == _penOverItemIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the numbers
    {
        BYTE digits, decimals;

        switch (Index)
        {
        case 0:
            digits = GPS_LAT_DIGITS;
            decimals = GPS_LAT_DECIMALS;
            break;

        case 1:
            digits = GPS_LON_DIGITS;
            decimals = GPS_LON_DECIMALS;
            break;

        case 2:
            digits = GPS_ALT_DIGITS;
            decimals = GPS_ALT_DECIMALS;
            break;

        default:
            FX_ASSERT(false);
        }

        int value = GetItemValue(Index); 
        char valueText[32] = "";
        NumberToText(FxAbs(value), digits, decimals, 0, value < 0, valueText, false);

        FXRECT rect = itemRect;
        rect.Left = rect.Right - pCanvas->TextWidth(pFont, valueText) - itemHeight / 4;
        pCanvas->State.BrushColor = selected ? pCanvas->InvertColor(_color) : _color;
        pCanvas->FillRect(&rect);

        pCanvas->State.TextColor = selected ? pCanvas->InvertColor(_textColor) : _textColor;
        pCanvas->DrawTextRect(pFont, &rect, TEXT_ALIGN_VCENTER, valueText);

        itemRect.Right = itemRect.Right - maxNumberWidth - itemHeight / 4 - _oneSpace * 2;
    }

    // Draw the caption
    if (_showCaption)
    {
        const char *text = nullptr;
        switch (Index)
        {
        case 0:
            text = "Latitude";
            break;

        case 1:
            text = "Longitude";
            break;

        case 2:
            text = "Altitude";
            break;

        default:
            FX_ASSERT(false);
        }

        int flags = TEXT_ALIGN_VCENTER;
        FXRECT rect = itemRect;

        if (_rightToLeft)
        {
            flags |= TEXT_ALIGN_RIGHT;
            rect.Right -= 2;
        }
        else if (_centerText)
        {
            flags |= TEXT_ALIGN_HCENTER;
        }
        else
        {
            rect.Left += 2;
        }

        pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);
        pCanvas->DrawTextRect(pFont, &rect, flags, text);

        _textRect = itemRect;
        OffsetRect(&_textRect, -pItemRect->Left, -pItemRect->Top);
    }

    // Draw the bitmap
    {
        FXRECT rect = itemRect;
        int itemHeight = rect.Bottom - rect.Top + 1;
        int itemWidth = rect.Right - rect.Left + 1;
        int scaleTo = min(itemHeight, itemWidth);

        if (_showCaption || (_detailsItemIndex != -1))
        {
            if (_rightToLeft)
            {
                rect.Right = rect.Left + scaleTo;
            }
            else
            {
                rect.Left = rect.Right - scaleTo;
            }
        }
        else
        {
            rect.Left = rect.Left + (itemWidth - scaleTo) / 2;
            rect.Top = rect.Top + (itemHeight - scaleTo) / 2;
        }

        int w = (itemRect.Bottom - itemRect.Top);
        FXRECT iconRect = {0, 0, (int)w, (int)w};
        InflateRect(&iconRect, -_oneSpace * 2, -_oneSpace * 2);
        OffsetRect(&iconRect, rect.Left, rect.Top);

        int radius = (iconRect.Bottom - iconRect.Top + 1) / 2;
        int y = iconRect.Top;
        pCanvas->State.LineColor = pCanvas->InvertColor(_borderColor, selected);
        pCanvas->State.LineWidth = _oneSpace;

        switch (Index)
        {
        case 0: // Latitude
            pCanvas->Ellipse(iconRect.Left + radius, y + radius, radius, radius);
            pCanvas->Line(iconRect.Left, y + radius, iconRect.Left + radius*2, y + radius);
            break;

        case 1: // Longitude
            pCanvas->Ellipse(iconRect.Left + radius, y + radius, radius, radius);
            pCanvas->Line(iconRect.Left + radius, y, iconRect.Left + radius, y + radius*2);
            break;

        case 2: // Altitude
            {
                FXPOINTLIST polygon;
                FXPOINT triangle[] = {{(int)radius, 0}, {(int)radius*2, (int)radius*2}, {0, (int)radius*2}};
                POLYGON(pCanvas, triangle, iconRect.Left, y);
            }
            break;

        default:
            FX_ASSERT(false);
        }

        _iconRect = iconRect;
        OffsetRect(&_iconRect, -itemRect.Left, -itemRect.Top);
        _iconRect.Right = _iconRect.Left + scaleTo;
        _iconRect.Bottom = _iconRect.Top + scaleTo;
    }
}

void CctControl_ElementList::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_firstPaint)
    {
        _firstPaint = false;
        if (_sorted || (_historySortType != ElementHistorySortType::None))
        {
            ExecuteSort();
        }
    }

    if (_detailsItemIndex == -1)
    {
        // Tracking for hittest
        memset(&_iconRect, 0, sizeof(FXRECT));
        memset(&_textRect, 0, sizeof(FXRECT));
        memset(&_checkRect, 0, sizeof(FXRECT)); 
        memset(&_minusRect, 0, sizeof(FXRECT)); 
        memset(&_numberRect, 0, sizeof(FXRECT));

        // Normal list paint: paint the items
        CfxControl_List::OnPaint(pCanvas, pRect);

        return;
    }

    // Draw item view
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 
    XGUID elementId = _elements.Get(_detailsItemIndex);

    FXFONTRESOURCE *font = resource->GetFont(this, _detailsFont);
    if (font)
    {
        FXBITMAPRESOURCE *bitmap = nullptr;
        ElementAttribute attributeDetails = max(_attribute, _attributeDetails);
        if (attributeDetails >= ElementAttribute::Icon32)
        {
            bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, elementId, (BYTE)attributeDetails);
        }

        int fontHeight = pCanvas->FontHeight(font);
        int spacer = _borderLineWidth * 2;
        int w = _oneSpace * 2 *(bitmap ? bitmap->Width : 32);
        int h = _oneSpace * 2 * (bitmap ? bitmap->Height : 32);
        int clientW = max(0, (int)GetClientWidth() - spacer);
        int clientH = max(0, (int)GetClientHeight() - (int)fontHeight - 8 - spacer);

        FXRECT bitmapRect = {0};

        if ((w > clientW) || (h > clientH))
        {
            if (bitmap)
            {
                GetBitmapRect(bitmap, clientW, clientH, true, true, true, &bitmapRect);
            }
            else
            {
                bitmapRect.Top = bitmapRect.Bottom = (int)-4 * _oneSpace;
            }
            OffsetRect(&bitmapRect, pRect->Left + spacer, pRect->Top + spacer);
        }
        else
        {
            bitmapRect.Left = pRect->Left + (clientW - w) / 2;
            bitmapRect.Top = pRect->Top + (clientH - h) / 2;
            bitmapRect.Right = bitmapRect.Left + w;
            bitmapRect.Bottom = bitmapRect.Top + h;
        }

        // Draw Icon
        if (bitmap)
        {
            FX_ASSERT(bitmap->Magic == MAGIC_BITMAP);
            pCanvas->StretchDraw(bitmap, bitmapRect.Left, bitmapRect.Top, bitmapRect.Right - bitmapRect.Left + 1, bitmapRect.Bottom - bitmapRect.Top + 1);
            resource->ReleaseObject(bitmap);
        }
        else
        {
            pCanvas->State.LineColor = _borderColor;
            pCanvas->State.LineWidth = GetBorderLineWidth();
            pCanvas->FrameRect(&bitmapRect);
        }

        // Draw Text
        FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, (BYTE)ElementAttribute::Name);
        if (element)
        {
            FX_ASSERT(element->Magic == MAGIC_TEXT);

            FXRECT rect = *pRect;
            rect.Top = min(bitmapRect.Bottom + 8 * _oneSpace, rect.Bottom - (int)fontHeight - 4 * _oneSpace);
            rect.Bottom = rect.Top + fontHeight + 8 * _oneSpace;

            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(&rect);

            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_HCENTER, GetDetailsText(element));

            resource->ReleaseObject(element);
        }

        resource->ReleaseFont(font);
    }
}
