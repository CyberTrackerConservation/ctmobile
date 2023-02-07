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

    _firstPaint = TRUE;

    _aliasOverride = _aliasDetails = eaoNone;
    _attribute = _attributeDetails = eaIcon32;
    _showCaption = TRUE;
    _saveResult = TRUE;
    _hideLinks = FALSE;
    _listMode = elmRadio;
    _selectedIndex = -1;
    _digits = 3;
    _decimals = 0;
    _fraction = 0;
    _minValue = 0;
    _maxValue = 999999999;
    _minChecks = _maxChecks = 0;
    _keypadFormulaMode = FALSE;
    _transparentImages = FALSE;
    _radioSetGoto = FALSE;
    _detailsOnDown = TRUE;

    InitFont(_detailsFont, F_FONT_DEFAULT_M);

    _blockNext = TRUE;
    InitXGuid(&_radioElement);
    
    _lastItemIndex = _detailsItemIndex = _detailsTimerItemIndex = _penDownItemIndex = _penOverItemIndex = -1;
    _detailsShown = FALSE;
    
    _sorted = FALSE;

    _outputChecks = TRUE;
    _numberChecks = FALSE;

    _rightToLeft = FALSE;
    _centerText = FALSE;

    _filterEnabled = FALSE;
    _filterValid = _filterError = FALSE;
    _filter = 0;

    _historyEnabled = _historyCanSelect = FALSE;
    _historyTextColor = 0x808080;
    _historySortType = ehsNone;

    _setLat = _setLon = FALSE;

    // Radio sound buttons
    _showRadioSound = FALSE;
    _showRadioSoundStop = TRUE;

    _playButton = new CfxControl_Button(pOwner, this);
    _playButton->SetComposite(TRUE);
    _playButton->SetVisible(_showRadioSound);
    _playButton->SetBorder(bsNone, 0, 0, _borderColor);
    _playButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementList::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementList::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, this);
    _stopButton->SetComposite(TRUE);
    _stopButton->SetVisible(_showRadioSound);
    _stopButton->SetBorder(bsNone, 0, 0, _borderColor);
    _stopButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementList::OnStopClick);
    _stopButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementList::OnStopPaint);

    // State retention
    _retainOnlyScrollState = FALSE;
    _autoSelectIndex = 0;
    _autoRadioNext = FALSE;

    // Global result
    _resultGlobalValue = NULL;

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
	_resultGlobalValue = NULL;
}

VOID CctControl_ElementList::SetRadioSelectedIndex(INT Index)
{
    _selectedIndex = Index;

    // Set the global value
    if (IsLive() && (_resultGlobalValue != NULL))
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, (Index == -1) ? 0 : Index + 1);
    }
}

VOID CctControl_ElementList::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    BOOL allow = *((BOOL *)pParams);

    if (!allow) return;

    if (IsCheckList() || (IsNumberList() && _numberChecks))
    {
        UINT checkCount = 0;
        CHAR message[200];

        for (UINT i = 0; i < GetItemCount(); i++)
        {
            UINT index = GetRawItemIndex(i);
            if (GetCheckState(index) != 0) 
            {
                checkCount++;
            }
        }

        if ((_minChecks > 0) && (checkCount < _minChecks))
        {
            sprintf(message, "Check at least %lu", _minChecks);
            GetSession(this)->ShowMessage(message);
            allow = FALSE;
        }

        if ((_maxChecks > 0) && (_maxChecks >= _minChecks) && (checkCount > _maxChecks))
        {
            sprintf(message, "Check at most %lu", _maxChecks);
            GetSession(this)->ShowMessage(message);
            allow = FALSE;
        }
    }

    switch (_listMode)
    {
    case elmRadio:
        if (_blockNext && (_selectedIndex == -1))
        {
            allow = FALSE;
        }
        break;

    case elmCheck:
    case elmCheckIcon:
        break;

    case elmNumber:
    case elmNumberKeypad:
    case elmNumberFastTap:
        break;

    case elmGps:
        if (_blockNext && (!_setLat || !_setLon))
        {
            allow = FALSE;
        }
        break;
    }

    *((BOOL *)pParams) = allow;
}

BOOL CctControl_ElementList::GetPosition(FXGPS_POSITION *pPosition)
{
    INT i;

    memset(pPosition, 0, sizeof(FXGPS_POSITION));
    if (_listMode != elmGps) return FALSE;

    DOUBLE lat = GetItemValue(0);
    for (i=0; i < GPS_LAT_DECIMALS; i++, lat/=10) {}

    DOUBLE lon = GetItemValue(1);
    for (i=0; i < GPS_LON_DECIMALS; i++, lon/=10) {}

    DOUBLE alt = GetItemValue(2);
    for (i=0; i < GPS_ALT_DECIMALS; i++, alt/=10) {}

    pPosition->Latitude = lat;
    pPosition->Longitude = lon;
    pPosition->Altitude = alt;
    pPosition->Quality = fqUser3D;

    return TRUE;
}

VOID CctControl_ElementList::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    INT i;

    //
    // Special case for Gps
    //

    if (_listMode == elmGps)
    {
        FXGPS_POSITION position;
        if (GetPosition(&position))
        {
            CctAction_SnapGPS action(_owner);
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
                SetHistoryState(i, TRUE);
                //CctAction_SetElementListHistory action(this);
                //action.Setup(_id, i, GetHistoryState(i), TRUE);
                //((CctSession *)pSender)->ExecuteAction(&action);
            }
        }
    }

    CctAction_MergeAttribute action(this);
    action.SetControlId(_id);

    CfxStream resultStream;

    for (i=0; i<(INT)_elements.GetCount(); i++)
    {
        XGUID elementId = _elements.Get(i);
        if (!IsNullXGuid(&elementId))
        {
            if (IsRadioList() && (i == _selectedIndex))
            {
                if (_radioSetGoto)
                {
                    FXGOTO *gotoPoint = (FXGOTO *)resource->GetObject(this, elementId, eaGoto);
                    if (gotoPoint != NULL)
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
                    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, eaName);
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

                    FXJSONID *jsonId = (FXJSONID *)resource->GetObject(this, elementId, eaJsonId);
                    if (jsonId)
                    {
                        FX_ASSERT(jsonId->Magic == MAGIC_JSONID);
                        resultStream.Write(jsonId->Text);
                        resource->ReleaseObject(jsonId);
                    }
                    else
                    {
                        FXTEXTRESOURCE* name = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, eaName);
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
                    BOOL value = TRUE;
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
                        INT value = GetItemValue(i);
                        action.SetupAdd(_id, elementId, dtInt32, &value);
                    }
                    else
                    {
                        DOUBLE value = GetItemValue(i);

                        for (INT j=0; j < _decimals; j++, value/=10) {}
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

VOID CctControl_ElementList::OnSessionSetJumpTarget(CfxControl *pSender, VOID *pParams)
{
    if (!IsRadioList()) return;

    for (UINT i=0; i<_elements.GetCount(); i++)
    {
        XGUID link = _links.Get(i * 2 + 1);
        if (CompareXGuid(&link, (XGUID *)pParams))
        {
            SetRadioSelectedIndex(i);
            break;
        }
    }
}

VOID CctControl_ElementList::OnPlayClick(CfxControl *pSender, VOID *pParams)
{
    FX_ASSERT(_selectedIndex != -1);

    CfxResource *resource = GetResource();
    FXSOUNDRESOURCE *sound = (FXSOUNDRESOURCE *)resource->GetObject(this, _elements.Get(_selectedIndex), eaSound);
    if (sound)
    {
        GetHost(this)->PlaySound(sound);
        resource->ReleaseObject(sound);
    }
}

VOID CctControl_ElementList::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, pParams->Canvas->InvertColor(_color), pParams->Canvas->InvertColor(_borderColor), _playButton->GetDown()); 
}

VOID CctControl_ElementList::OnStopClick(CfxControl *pSender, VOID *pParams)
{
    GetHost(this)->StopSound();
}

VOID CctControl_ElementList::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawStop(pParams->Rect, pParams->Canvas->InvertColor(_color), pParams->Canvas->InvertColor(_borderColor), _stopButton->GetDown()); 
}

VOID CctControl_ElementList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;
    INT detailsActivate;

    if (!HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        //_lastItemIndex = -1;
        return;
    }

    index = GetRawItemIndex((UINT)index);

    detailsActivate = _detailsOnDown &&
                      (_listMode != elmGps) && 
                      (max(_attributeDetails, _attribute) >= eaIcon32)  &&
                      ((_listMode == elmCheckIcon) || PtInRect(&_iconRect, offsetX, offsetY) || PtInRect(&_textRect, offsetX, offsetY));

    if ((index != -1) && !detailsActivate && 
        _historyEnabled && !_historyCanSelect &&
        GetHistoryState(index))
    {
        return;
    }

    switch (_listMode)
    {
    case elmRadio:
        SetRadioSelectedIndex(index);
        break;
    
    case elmCheck:
    case elmCheckIcon:
        SetCheckState(index, !GetCheckState(index));
        break;

    case elmNumber:
        if (_numberChecks && PtInRect(&_checkRect, offsetX, offsetY))
        {
            SetCheckState(index, !GetCheckState(index));
        }
        else if (PtInRect(&_numberRect, offsetX, offsetY))
        {
            INT columnWidth = (GetClientWidth() - GetScrollBarWidth()) / GetColumnCount();
            CHAR valueText[32] = "";
            CHAR oneValue[2]   = "0";
            NumberToText(GetItemValue(index), _digits, _decimals, _fraction, FALSE, valueText, TRUE);
            FXFONTRESOURCE *font = GetResource()->GetFont(this, _font);
            CfxCanvas *canvas = GetCanvas(this);
            BYTE digit = _digits - 1;

            INT l = columnWidth - canvas->TextWidth(font, valueText) - (INT)_itemHeight / 4;

            if (offsetX > l)
            for (INT i=0; i<(INT)strlen(valueText); i++)
            {
                oneValue[0] = valueText[i];
                l += canvas->TextWidth(font, oneValue);

                BOOL isDigit = ((oneValue[0] >= '0') && (oneValue[0] <= '9'));

                if (l > offsetX) 
                {
                    if (isDigit)
                    {
                        INT delta = (offsetY > (INT)(_itemHeight / 2)) ? -1 : 1;
                        IncItemValue(index, digit, delta);
                    }
                    break;
                }
                if (isDigit) digit--;
            }
            GetResource()->ReleaseFont(font);
        }
        break;

    case elmGps:
    case elmNumberKeypad:
        if (_numberChecks && PtInRect(&_checkRect, offsetX, offsetY))
        {
            SetCheckState(index, !GetCheckState(index));
        }
        else 
        {
            _penDownItemIndex = _penOverItemIndex = index;
        }
        break;

    case elmNumberFastTap:
        if (_numberChecks && PtInRect(&_checkRect, offsetX, offsetY))
        {
            SetCheckState(index, !GetCheckState(index));
        }
        else if (!_numberChecks || GetCheckState(index))
        {
            INT value = GetItemValue(index);
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
        FX_ASSERT(FALSE);
    }

    if (detailsActivate)
    {
        _detailsTimerItemIndex = index;
        if (index != -1)
        {
            SetTimer(500);
        }
        else
        {
            KillTimer();
        }

        Repaint();
        return;
    }

    //_lastItemIndex = index;

    Repaint();
}

VOID CctControl_ElementList::OnPenUp(INT X, INT Y)
{
    _detailsItemIndex = -1;
    _detailsTimerItemIndex = -1; 
    KillTimer();
    UpdateScrollBar();
    Repaint();
}

VOID CctControl_ElementList::OnPenMove(INT X, INT Y)
{
    if (_penDownItemIndex == -1) return;

    INT index;
    INT offsetX, offsetY;
    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        index = GetRawItemIndex((UINT)index);
    }
    _penOverItemIndex = index;

    Repaint();
}

VOID CctControl_ElementList::OnPenClick(INT X, INT Y)
{
    if (_detailsShown)
    {
        _detailsShown = FALSE;
        return;
    }

    // Autonext for radio lists
    if (IsRadioList())
    {
        if (_autoRadioNext && (_selectedIndex != -1))
        {
            GetEngine(this)->KeyPress(KEY_RIGHT);
        }
    }
    else if (IsCheckList())
    {
    }
    else if (IsNumberList())
    {
        INT index = ((_penDownItemIndex != -1) && (_penDownItemIndex == _penOverItemIndex)) ? _penDownItemIndex : -1;
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

            if (_listMode == elmGps)
            {
                params.Fraction = 0;
                switch (index)
                {
                case 0:
                    params.Digits = GPS_LAT_DIGITS;
                    params.Decimals = GPS_LAT_DECIMALS;
                    params.MinValue = -90;
                    params.MaxValue = 90;
                    _setLat = TRUE;
                    strncpy(params.Title, "Enter Latitude", sizeof(params.Title)-1);
                    break;
                case 1:
                    params.Digits = GPS_LON_DIGITS;
                    params.Decimals = GPS_LON_DECIMALS;
                    params.MinValue = -180;
                    params.MaxValue = +180;
                    _setLon = TRUE;
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
                    FX_ASSERT(FALSE);
                }
            }
            else if (_numberChecks)
            {
                SetCheckState(index, TRUE);
            }

            // Get the value
            INT itemValue = GetItemValue(index);
            params.Value = EncodeDouble(FxAbs(itemValue), params.Decimals, itemValue < 0);

            // Pick up the title from the Element
            if (strlen(params.Title) == 0)
            {
                CfxResource *resource = GetResource();
                XGUID elementId = _elements.Get(index);
                FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, eaName);
                if (element)
                {
                    FX_ASSERT(element->Magic == MAGIC_TEXT);
                    strncpy(params.Title, GetElementText(element), sizeof(params.Title)-1);
                    resource->ReleaseObject(element);
                }
            }

            GetSession(this)->ShowDialog(&GUID_DIALOG_NUMBEREDITOR, NULL, &params, sizeof(NUMBEREDITOR_PARAMS)); 
        }
        else
        {
            Repaint();
        }
    }
}

VOID CctControl_ElementList::OnTimer()
{
    KillTimer();

    _detailsItemIndex = _detailsTimerItemIndex;
    _detailsTimerItemIndex = -1;
    _detailsShown = TRUE;

    _scrollBar->SetVisible(FALSE);
    Repaint();
}

VOID CctControl_ElementList::RebuildLinks()
{
    #ifdef CLIENT_DLL

    // Fast test for dirty
    UINT i, c, elementCount = _elements.GetCount();
    UINT emptyElements = 0;
    if ((elementCount > 0) && (_links.GetCount() == elementCount * 2))
    {
        XGUID *pElement = _elements.GetPtr(0);
        XGUID *pOldLink = _links.GetPtr(0);
        BOOL dirty = FALSE;

        for (i=0; i<elementCount; i++)
        {
            if (*pElement != *pOldLink)
            {
                dirty = TRUE;
                break;
            }
            emptyElements += (UINT) IsNullXGuid(pElement);
            pElement++;
            pOldLink+=2;
        }

        if (emptyElements == elementCount)
        {
            _elements.Clear();
            _links.Clear();
            return;
        }

        if (!dirty && (emptyElements == 0)) 
        {
            return;
        }
    }
    
    // Slow rebuild the list of links to match the Elements
    XGUIDLIST newLinks;
    newLinks.SetCount((elementCount - emptyElements) * 2);
    c = 0;
    for (i=0; i<elementCount; i++)
    {
        if (IsNullXGuid(_elements.GetPtr(i))) 
        {
            continue;
        }

        UINT indexElement = c * 2;
        UINT indexLink    = indexElement + 1;
        XGUID element = _elements.Get(i);
        
        newLinks.Put(indexElement, element);
        _elements.Put(c, element);
        c++;

        for (UINT j=0; j<_links.GetCount(); j+=2)
        {
            if (_links.Get(j) == element)
            {
                newLinks.Put(indexLink, _links.Get(j+1));
                break;
            }
        }
    }

    //
    // Note that assign will just do a memcpy if the list sizes are the same. This is important because it
    // means that using the filer will not affect UI connections to underlying control data.
    //
    _links.Assign(&newLinks);
    _elements.SetCount(c);

    UpdateScrollBar();

    #endif
}

struct FILTERCATEGORY
{
    UINT Type;
    UINT Count;
    XGUID Items[1];
};

BOOL FilterMatch(UINT Type, BYTE *pSightingMask, BYTE *pElementMask, UINT ByteCount)
{
    switch (Type)
    {
    case 0:     // Subset
        {
            //BOOL atLeast1 = FALSE;
            for (UINT i=0; i<ByteCount; i++)
            {
                BYTE match = (~(*pSightingMask) | (*pSightingMask & *pElementMask));
                if (match != 0xFF) return FALSE;
                //atLeast1 = atLeast1 || (*pSightingMask != 0);
                pSightingMask++;
                pElementMask++;
            }
            return TRUE;
        }
        break;

    case 1:     // Any
        {
            for (UINT i=0; i<ByteCount; i++)
            {
                if ((*pSightingMask & *pElementMask) != 0) return TRUE;
                pSightingMask++; pElementMask++;
            }
            return FALSE;
        }
        break;

    case 2:     // Exact
        {
            for (UINT i=0; i<ByteCount; i++)
            {
                if ((*pSightingMask & *pElementMask) != *pElementMask) return FALSE;
                pSightingMask++; pElementMask++;
            }
            return TRUE;
        }
        break;

    
    default:
        return FALSE;
    }

    return FALSE;
} 

VOID SetBit(BYTE *pBits, UINT Index)
{
    BYTE *value = pBits + (Index >> 3);
    BYTE mask = 0x80 >> (Index & 7);
    *value = (*value & ~mask) | mask;
}

XGUIDLIST *BuildSightingAttributes(CctSession *pSession)
{
    XGUIDLIST *list = new XGUIDLIST();
    ATTRIBUTES *attributes = pSession->GetCurrentSighting()->GetAttributes();
    for (UINT i=0; i<attributes->GetCount(); i++)
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

BOOL CctControl_ElementList::ExecuteFilter()
{
	_filterError = FALSE;
    _filterItems.Clear();
    SetCaption("No matching Elements");

    // Validate the resource
    CfxResource *resource = GetResource();
    FXBINARYRESOURCE *filter = resource->GetBinary(this, _filter);
    if (!filter) return FALSE;
    FX_ASSERT(filter->Magic == MAGIC_BINARY);
    FX_ASSERT(filter->BinaryMagic == MAGIC_ELEMENT_FILTER);

    //
    // Setup before the match
    //
    UINT i, j;
    BYTE *data = &filter->Data[0];

    // Parse the structure and build the list of categories
    UINT categoryCount = *(UINT *)data; data += sizeof(UINT);
    FILTERCATEGORY **categories = (FILTERCATEGORY **)MEM_ALLOC(categoryCount * sizeof(FILTERCATEGORY *));
    UINT maskByteCount = 0;
    for (i=0; i<categoryCount; i++)
    {
        FILTERCATEGORY *category = (FILTERCATEGORY *)data;
        categories[i] = category;
        data += (UINT)sizeof(FILTERCATEGORY) + (category->Count - 1) * (UINT)sizeof(XGUID);
        maskByteCount += (category->Count + 7) / 8;
    }
    UINT elementCount = *(UINT *)data; data += sizeof(UINT);
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
                _filterError = TRUE;
                break;
            }
            elementData += sizeof(XGUID);

            // Iterate over all the categories and match the element to each
            BOOL match = TRUE;
            BYTE *currentSightingMask = sightingMask;
            for (j=0; j<categoryCount; j++)
            {
                FILTERCATEGORY *category = categories[j];
                UINT byteCount = (category->Count + 7) / 8;
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
        return FALSE;
    }

    return TRUE;
}

VOID CctControl_ElementList::ValidateItemCount()
{
    if (IsNumberList())
    {
        if (_listMode == elmGps)
        {
            _itemValues.SetCount(3);
        }
        else
        {
            _itemValues.SetCount(_elements.GetCount());
        }
    }
}

VOID CctControl_ElementList::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_List::DefineResources(F);

    UINT i = 0;

    // Test Elements
    RebuildLinks();

    FX_ASSERT(_links.GetCount() == _elements.GetCount() * 2);
    while (i < _elements.GetCount()) 
    {
        XGUID elementId = _elements.Get(i);

        if (IsNullXGuid(&elementId))
        {
            _elements.Delete(i);
            _links.Delete(i * 2);
            _links.Delete(i * 2);
            i = 0;
        }
        else
        {
            F.DefineObject(elementId, eaName);
            F.DefineObject(elementId, _attribute);
            F.DefineObject(elementId, _attributeDetails);

            if (_radioSetGoto)
            {
                F.DefineObject(elementId, eaGoto);
            }

            // Define sound if required
            if (_showRadioSound && IsRadioList())
            {
                F.DefineObject(elementId, eaSound);
            }

            if (IsNullXGuid(_links.GetPtr(i * 2 + 1)))
            {
                _links.Put(i * 2 + 1, GUID_0);
            }
            else
            {
                F.DefineObject(_links.Get(i * 2 + 1));
            }
            i++;
        }
    }

    F.DefineFont(_detailsFont);
    F.DefineBinary(_filter);
    F.DefineObject(_radioElement);

    // Fields
    if ((_listMode == elmRadio || _listMode == elmCheck || _listMode == elmCheckIcon) && !IsNullXGuid(&_radioElement))
    {
        F.DefineField(_radioElement, dtText);
    }
    else
    {
        for (i = 0; i < _elements.GetCount(); i++)
        {
            XGUID elementId = _elements.Get(i);

            switch (_listMode)
            {
            case elmRadio:
                F.DefineField(elementId, dtNull);
                break;

            case elmCheck:
            case elmCheckIcon:
                F.DefineField(elementId, dtBoolean);
                break;

            case elmNumber:
            case elmNumberKeypad:
            case elmNumberFastTap:
                F.DefineField(elementId, _decimals == 0 ? dtInt32 : dtDouble);
                break;

            case elmGps:
                break;
            }
        }
    }
#endif
}

VOID CctControl_ElementList::DefineProperties(CfxFiler &F)
{
    BOOL legacyUseKeypad = FALSE;

    _firstPaint = TRUE;

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

    #ifdef CLIENT_DLL
    if (F.IsReader())
    {
        StringReplace(&_lockProperties, "Attribute", "Icon");    
        StringReplace(&_lockProperties, "Attribute for details", "Icon details");    
        StringReplace(&_lockProperties, "Radio element", "Result Element");
        StringReplace(&_lockProperties, "Hide links", "");  // unsafe
        StringReplace(&_lockProperties, "Auto radio next", "Auto next");
        
        /*if (_stricmp(_font, "MS Sans Serif,10,") == 0) 
        {
            strcpy(_font, "Arial,10,");
        }

        if (_stricmp(_font, "MS Sans Serif,10,B") == 0) 
        {
            strcpy(_font, "Arial,10,B");
        }

        if (_stricmp(_font, "MS Sans Serif,12,") == 0) 
        {
            strcpy(_font, "Arial,12,");
        }

        if (_stricmp(_font, "MS Sans Serif,12,B") == 0) 
        {
            strcpy(_font, "Arial,12,B");
        }*/
    }
    #endif

    RebuildLinks();

    if (F.IsReader())
    {
        CctSession *session = IsLive() ? GetSession(this) : NULL;

        if (_version < ELEMENT_LIST_VERSION)
        {
            _blockNext = TRUE;
            _outputChecks = FALSE;
            _rightToLeft = FALSE;
            _centerText = FALSE;
        }

        if (session && _historyEnabled) 
        {
            STOREITEM *item = session->GetStoreItem(ConstructGuid(*session->GetCurrentScreenId()), 
                                                    (UINT)(-(INT)_id));
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
       
        _stopButton->SetVisible(FALSE);
        _playButton->SetVisible(FALSE);

        UpdateScrollBar();

        if (IsRadioList())
        {
            if ((_autoSelectIndex > 0) && (_autoSelectIndex <= GetItemCount()))
            {
                UINT index = GetRawItemIndex(_autoSelectIndex - 1);
                SetRadioSelectedIndex(index);
            }
        }
        else if (IsCheckList())
        {
            if (_listMode == elmCheckIcon)
            {
                _showCaption = FALSE;
            }
        }
        else if (IsNumberList())
        {
            if ((legacyUseKeypad || (_fraction > 1)) && (_listMode != elmGps))
            {
                _listMode = elmNumberKeypad;
            }
        }
        
        _setLat = _setLon = FALSE;

        _minValue = min(_minValue, _maxValue);
        _maxValue = max(_maxValue, _minValue);

        /*#ifdef CLIENT_DLL
            //
            // Hack to add "Radio element" to the lock properties for radio lists.
            //
            const CHAR *kRadioElement = "Radio element";
            if ((_listMode == elmRadio) && (strstr(_lockProperties, kRadioElement) == 0))
            {
                CHAR *newLockProperties = (CHAR *)MEM_ALLOC(strlen(_lockProperties) + strlen(kRadioElement) + 2);
                strcpy(newLockProperties, _lockProperties);
                strcat(newLockProperties, ";");
                strcat(newLockProperties, kRadioElement);

                MEM_FREE(_lockProperties);
                _lockProperties = newLockProperties;
            }
        #endif*/
    }
}

VOID CctControl_ElementList::ResetState()
{
    CfxControl_List::ResetState();

    _selectedIndex = -1;
    _checkStates.Clear();
    _itemValues.Clear();
    _historyStates.Clear();

    if (IsLive())
    {
        CctSession *session = GetSession(this);
        session->SetStoreItem(ConstructGuid(*session->GetCurrentScreenId()), (UINT)(-(INT)_id), NULL);    
    }

    _setLat = _setLon = FALSE;

    ValidateItemCount();
}

VOID CctControl_ElementList::DefineState(CfxFiler &F)
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

VOID CctControl_ElementList::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];

    sprintf(items, "LIST(None=%d \"Scientific name\"=%d \"Alias 1\"=%d \"Alias 2\"=%d)", eaoNone, eaoAlias1, eaoAlias2, eaoAlias3);
    F.DefineValue("Alias details",      dtByte,      &_aliasDetails, items);
    F.DefineValue("Auto height",		dtBoolean,   &_autoHeight);
    F.DefineValue("Auto next",          dtBoolean,   &_autoRadioNext);
    F.DefineValue("Alias override",     dtByte,      &_aliasOverride, items);
    sprintf(items, "LIST(None=%d Icon32=%d Icon50=%d Icon100=%d Image1=%d Image2=%d Image3=%d Image4=%d Image5=%d Image6=%d Image7=%d Image8=%d)", eaName, eaIcon32, eaIcon50, eaIcon100, eaImage1, eaImage2, eaImage3, eaImage4, eaImage5, eaImage6, eaImage7, eaImage8);
    F.DefineValue("Auto select index",  dtInt32,     &_autoSelectIndex, "MIN(0);MAX(10000)");
    F.DefineValue("Block next",         dtBoolean,   &_blockNext);
    F.DefineValue("Border color",       dtColor,     &_borderColor);
    F.DefineValue("Border line width",  dtInt32,     &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style",       dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width",       dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Center text",        dtBoolean,   &_centerText);
    F.DefineValue("Color",              dtColor,     &_color);
    F.DefineValue("Columns",            dtInt32,     &_columnCount, "MIN(1);MAX(8)");
    F.DefineValue("Decimals",           dtByte,      &_decimals,    "MIN(0);MAX(8)");
    F.DefineValue("Details font",       dtFont,      &_detailsFont);
    F.DefineValue("Details on press",   dtBoolean,   &_detailsOnDown);
    F.DefineValue("Digits",             dtByte,      &_digits,      "MIN(1);MAX(9)");
    F.DefineValue("Dock",               dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Elements",           dtPGuidList, &_elements,    "EDITOR(ScreenElementsEditor);REQUIRED");

    CHAR filterParams[256];
    sprintf(filterParams, "EDITOR(ScreenElementsFilterEditor);ELEMENTS(%d)", &_elements);
    F.DefineValue("Filter",             dtPBinary,   &_filter,      filterParams);
    F.DefineValue("Filter enabled",     dtBoolean,   &_filterEnabled);

    F.DefineValue("Font",               dtFont,       &_font);
    F.DefineValue("Fraction",           dtByte,       &_fraction,       "MIN(1);MAX(9)");
    F.DefineValue("Height",             dtInt32,      &_height);
    F.DefineValue("Hide links",         dtBoolean,    &_hideLinks);
    F.DefineValue("History can select", dtBoolean,    &_historyCanSelect);
    F.DefineValue("History enabled",    dtBoolean,    &_historyEnabled);
    F.DefineValue("History sort mode",  dtByte,       &_historySortType,  "LIST(None Bottom Top)");
    F.DefineValue("History text color", dtColor,      &_historyTextColor);
    F.DefineValue("Icon",               dtByte,       &_attribute, items);
    F.DefineValue("Icon details",       dtByte,       &_attributeDetails, items);
    F.DefineValue("Item height",        dtInt32,      &_itemHeight,     "MIN(16);MAX(320)");
    F.DefineValue("Keypad formula mode", dtBoolean,   &_keypadFormulaMode);
    F.DefineValue("Left",               dtInt32,      &_left);
    sprintf(items, "LIST(\"Single select\"=%d \"Multi select\"=%d \"Multi select icons\"=%d \"Number digits\"=%d \"Number fast-tap\"=%d \"Number keypad\"=%d)", elmRadio, elmCheck, elmCheckIcon, elmNumber, elmNumberFastTap, elmNumberKeypad);
    F.DefineValue("List mode",          dtByte,       &_listMode, items);
    F.DefineValue("Minimum checks",     dtInt32,      &_minChecks,      "MIN(0);MAX(999999999)");
    F.DefineValue("Maximum checks",     dtInt32,      &_maxChecks,      "MIN(0);MAX(999999999)");
    F.DefineValue("Minimum item height",dtInt32,      &_minItemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Minimum item width", dtInt32,      &_minItemWidth,   "MIN(32);MAX(640)");
    F.DefineValue("Minimum value",      dtInt32,      &_minValue,       "MIN(-999999999);MAX(999999999)");
    F.DefineValue("Maximum value",      dtInt32,      &_maxValue,       "MIN(-999999999);MAX(999999999)");
    F.DefineValue("Number list checks", dtBoolean,    &_numberChecks);
    F.DefineValue("Output checks",      dtBoolean,    &_outputChecks);
    
    CHAR radioElementString[256];
    // Add required on radio lists
    sprintf(radioElementString, "EDITOR(ScreenElementsEditor)%s", _listMode == elmRadio ? "" : "");
    F.DefineValue("Result Element",     dtGuid,       &_radioElement,   radioElementString);
    F.DefineValue("Result global value", dtPText,     &_resultGlobalValue);

    F.DefineValue("Retain only scroll state", dtBoolean, &_retainOnlyScrollState);
    _retainState.DefinePropertiesUI(F);
    F.DefineValue("Right to left",      dtBoolean,    &_rightToLeft);
    F.DefineValue("Save result",        dtBoolean,    &_saveResult);
    F.DefineValue("Scale for touch",    dtBoolean,    &_scaleForHitSize);
    F.DefineValue("Scroll width",       dtInt32,      &_scrollBarWidth, "MIN(7);MAX(40)");
    F.DefineValue("Set current goto",   dtBoolean,    &_radioSetGoto);
    F.DefineValue("Show caption",       dtBoolean,    &_showCaption);
    F.DefineValue("Show lines",         dtBoolean,    &_showLines);
    F.DefineValue("Show radio sound",   dtBoolean,    &_showRadioSound);
    F.DefineValue("Show radio sound stop", dtBoolean, &_showRadioSoundStop);
    F.DefineValue("Sorted",             dtBoolean,    &_sorted);
    F.DefineValue("Text color",         dtColor,      &_textColor);
    F.DefineValue("Top",                dtInt32,      &_top);
    F.DefineValue("Transparent",        dtBoolean,    &_transparent);
    F.DefineValue("Transparent images", dtBoolean,    &_transparentImages);
    F.DefineValue("Width",              dtInt32,      &_width);

    RebuildLinks();

    // Define the links
    if (!_hideLinks)
    {
        for (UINT i=0; i<_links.GetCount(); i+=2)
        {
            F.DefineDynamicLink(_links.GetPtr(i), _links.GetPtr(i+1));
        }
    }
#endif
}

CfxResource *g_sortResource;
CctControl_ElementList *g_sortControl;
XGUIDLIST *g_sortElements;
BOOL g_sortNames;
ElementHistorySortType g_sortHistory;

int SortCompare(const void *pItem1, const void *pItem2)
{
    int ret = 0;
    UINT index1, index2;
    int s1, s2;

    index1 = *(UINT *)pItem1;
    XGUID elementId1 = g_sortElements->Get(index1);
    FXTEXTRESOURCE *element1 = (FXTEXTRESOURCE *)g_sortResource->GetObject(g_sortControl, elementId1, eaName);

    index2 = *(UINT *)pItem2;
    XGUID elementId2 = g_sortElements->Get(index2);
    FXTEXTRESOURCE *element2 = (FXTEXTRESOURCE *)g_sortResource->GetObject(g_sortControl, elementId2, eaName);

    if (element1 && element2)
    {
        s1 = s2 = 0;
        if (g_sortHistory != ehsNone)
        {
            s1 = (int)g_sortControl->GetHistoryState(index1);
            s2 = (int)g_sortControl->GetHistoryState(index2);

            if (g_sortHistory == ehsBottom)
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

VOID CctControl_ElementList::ExecuteSort()
{
    // Setup the sort list
    UINT count = GetItemCount(); 
    _sortedItems.SetCount(count);
    for (UINT i=0; i<count; i++)
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
        qsort(_sortedItems.GetMemory(), count, sizeof(INT), SortCompare);
    }

    g_sortResource = NULL;
    g_sortControl = NULL;
    g_sortElements = NULL;
}

UINT CctControl_ElementList::GetRawItemIndex(UINT Index)
{
    if (_sorted || (_historySortType != ehsNone))
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

UINT CctControl_ElementList::GetItemCount()
{
    if (_listMode == elmGps)
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

XGUID CctControl_ElementList::GetItem(UINT Index)
{
    if (_listMode == elmGps)
    {
        XGUID x;
        InitXGuid(&x);
        return x;
    }

    Index = GetRawItemIndex(Index);
    return _elements.Get(Index);
}

CHAR *CctControl_ElementList::GetElementText(FXTEXTRESOURCE *Element)
{
    INT i;
    CHAR *result = Element->Text;

    for (i = 0; i < _aliasOverride; i++)
    {
        result += strlen(result) + 1;
    }

    if (strlen(result) == 0)
    {
        result = Element->Text;
    }

    return result;
}

CHAR *CctControl_ElementList::GetDetailsText(FXTEXTRESOURCE *Element)
{
    INT i;
    CHAR *result = Element->Text;

    for (i = 0; i < _aliasDetails; i++)
    {
        result += strlen(result) + 1;
    }

    if (strlen(result) == 0)
    {
        result = Element->Text;
    }

    return result;
}

BOOL CctControl_ElementList::GetCheckState(UINT Index)
{
    _checkStates.SetCount((_elements.GetCount() + 7) >> 3);
    BYTE byte = _checkStates.Get(Index >> 3);
    
    byte = (byte >> (Index & 7)) & 1;

    return byte != 0;
}

VOID CctControl_ElementList::SetCheckState(UINT Index, BOOL Value)
{
    _checkStates.SetCount((_elements.GetCount() + 7) >> 3);

    BYTE byte = _checkStates.Get(Index >> 3);
    BYTE shift = (CHAR)(Index & 7);

    byte = (byte & ~(1 << shift)) | ((Value & 1) << shift);
    _checkStates.Put(Index >> 3, byte);
}

BOOL CctControl_ElementList::GetHistoryState(UINT Index)
{
    _historyStates.SetCount((_elements.GetCount() + 7) >> 3);
    BYTE byte = _historyStates.Get(Index >> 3);
    
    byte = (byte >> (Index & 7)) & 1;

    return byte != 0;
}

VOID CctControl_ElementList::SetHistoryState(UINT Index, BOOL Value)
{
    _historyStates.SetCount((_elements.GetCount() + 7) >> 3);

    BYTE byte = _historyStates.Get(Index >> 3);
    BYTE shift = (CHAR)(Index & 7);

    byte = (byte & ~(1 << shift)) | ((Value & 1) << shift);
    _historyStates.Put(Index >> 3, byte);

    CfxStream stream;
    _historyStates.SaveToStream(stream);
    stream.GetMemory();

    CctSession *session = GetSession(this);
    session->SetStoreItem(ConstructGuid(*session->GetCurrentScreenId()), (UINT)(-(INT)_id), stream.CloneMemory());
}

INT CctControl_ElementList::GetItemValue(UINT Index)
{
    ValidateItemCount();
    return _itemValues.Get(Index);
}

VOID CctControl_ElementList::SetItemValue(UINT Index, INT Value)
{
    ValidateItemCount();
    _itemValues.Put(Index, Value);
}

VOID CctControl_ElementList::IncItemValue(UINT Index, BYTE Digit, INT Delta)
{
    UINT offset = 1;
    for (INT i=0; i<Digit; i++, offset*=10) {}

    INT itemValue = GetItemValue(Index);

    INT digitValue = (itemValue / offset) % 10;

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

VOID CctControl_ElementList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
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
    case elmRadio:
        PaintRadio(pCanvas, pRect, pFont, Index);
        break;

    case elmCheck:
        PaintCheck(pCanvas, pRect, pFont, Index);
        break;

    case elmCheckIcon:
        PaintCheckIcon(pCanvas, pRect, pFont, Index);
        break;
    
    case elmNumber:
    case elmNumberFastTap:
    case elmNumberKeypad:
        PaintNumber(pCanvas, pRect, pFont, Index);
        break;

    case elmGps:
        PaintGps(pCanvas, pRect, pFont, Index);
        break;
    }

    _textColor = lastTextColor;
    _borderColor = lastBorderColor;
}

VOID CctControl_ElementList::DrawItemCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, BOOL Checked, BOOL Selected)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    INT itemHeight = itemRect.Bottom - itemRect.Top + 1;

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

VOID CctControl_ElementList::DrawItemIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, XGUID ElementId, BOOL Selected)
{
    CfxResource *resource = GetResource();
    FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, ElementId, _attribute);
    if (bitmap)
    {
        FX_ASSERT(bitmap->Magic == MAGIC_BITMAP);

        FXRECT rect = *pRect;
        INT itemHeight = rect.Bottom - rect.Top + 1;
        INT itemWidth = rect.Right - rect.Left + 1;
        INT scaleTo = min(itemHeight, itemWidth);

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
        GetBitmapRect(bitmap, scaleTo - 2 * _oneSpace, scaleTo - 2 * _oneSpace, TRUE, TRUE, TRUE, &stretchRect);
        OffsetRect(&stretchRect, rect.Left + _oneSpace, rect.Top + _oneSpace);
        pCanvas->StretchDraw(bitmap, stretchRect.Left, stretchRect.Top, stretchRect.Right - stretchRect.Left + 1, stretchRect.Bottom - stretchRect.Top + 1, Selected, _transparentImages);
        resource->ReleaseObject(bitmap);

        _iconRect = rect;
        OffsetRect(&_iconRect, -pItemRect->Left, -pItemRect->Top);
        _iconRect.Right = _iconRect.Left + scaleTo;
        _iconRect.Bottom = _iconRect.Top + scaleTo;
    }
}

VOID CctControl_ElementList::DrawItemText(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, FXFONTRESOURCE *pFont, XGUID ElementId, BOOL Selected)
{
    CfxResource *resource = GetResource();

    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, ElementId, eaName);
    if (element)
    {
        FX_ASSERT(element->Magic == MAGIC_TEXT);

        UINT flags = TEXT_ALIGN_VCENTER;
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

VOID CctControl_ElementList::PaintRadio(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CfxResource *resource = GetResource();
    INT itemHeight = pItemRect->Bottom - pItemRect->Top + 1;

    // Cleanup the selected index if it has already been set
    if (_historyEnabled && (_selectedIndex != -1) && !_historyCanSelect && GetHistoryState(_selectedIndex))
    {
        _selectedIndex = -1;
    }

    // Get the element
    XGUID elementId = _elements.Get(Index);

    // Paint if selected
    BOOL selected = ((INT)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);

        // Turn on the radio sound buttons if a sound exists
        if (_showRadioSound)
        {
            FXSOUNDRESOURCE *sound = (FXSOUNDRESOURCE *)resource->GetObject(this, elementId, eaSound);
            if (sound)
            {
                resource->ReleaseObject(sound);

                // Move and display the buttons
                FXRECT absoluteRect;
                GetAbsoluteBounds(&absoluteRect);
                INT tx = pItemRect->Right - absoluteRect.Left;
                INT ty = pItemRect->Top - absoluteRect.Top;

                // Stop button
                if (_showRadioSoundStop)
                {
                    pItemRect->Right -= itemHeight;
                    tx -= itemHeight;
                    _stopButton->SetVisible(TRUE);
                    _stopButton->SetBounds(tx + 1, ty, itemHeight, itemHeight);
                    
                }

                // Play button
                pItemRect->Right -= itemHeight;
                tx -= itemHeight;
                _playButton->SetVisible(TRUE);
                _playButton->SetBounds(tx, ty, itemHeight, itemHeight);

            }
            else
            {
                _playButton->SetVisible(FALSE);
                _stopButton->SetVisible(FALSE);
            }
        }
    }

    // Draw the caption
    if (_showCaption)
    {
        DrawItemText(pCanvas, pItemRect, pItemRect, pFont, elementId, selected);
    }

    // Draw the bitmap
    if (_attribute >= eaIcon32)
    {
        DrawItemIcon(pCanvas, pItemRect, pItemRect, elementId, selected);
    }
}

VOID CctControl_ElementList::PaintCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    INT itemHeight = itemRect.Bottom - itemRect.Top + 1;

    // Get the element
    XGUID elementId = _elements.Get(Index);

    // Draw the check
    DrawItemCheck(pCanvas, pItemRect, &itemRect, GetCheckState(Index), FALSE);

    // Draw the caption
    if (_showCaption)
    {
        DrawItemText(pCanvas, pItemRect, &itemRect, pFont, elementId);
    }

    // Draw the bitmap
    if (_attribute >= eaIcon32)
    {
        DrawItemIcon(pCanvas, pItemRect, &itemRect, elementId);
    }
}

VOID CctControl_ElementList::PaintCheckIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    INT itemHeight = itemRect.Bottom - itemRect.Top + 1;

    // Get the element
    XGUID elementId = _elements.Get(Index);

    FXRECT rect = *pItemRect;
    INT checkThickness = 5 * _oneSpace;

    // Draw the bitmap
    InflateRect(&itemRect, -checkThickness, -checkThickness);
    if (_attribute >= eaIcon32)
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

VOID CctControl_ElementList::PaintNumber(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    INT itemHeight = itemRect.Bottom - itemRect.Top + 1;

    // Get the element
    XGUID elementId = _elements.Get(Index);

    // Invert if needed
    BOOL selected = ((INT)Index == _penDownItemIndex) && ((INT)Index == _penOverItemIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the numbers    
    {
        INT value = GetItemValue(Index); 
        CHAR valueText[32] = "-";
        BOOL showNumber = FALSE;
        if ((_numberChecks == FALSE) || GetCheckState(Index))
        {
            NumberToText(FxAbs(value), _digits, _decimals, _fraction, value < 0, valueText, _listMode == elmNumber);
            showNumber = TRUE;
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
    if (_listMode == elmNumberFastTap)
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
    if (_attribute >= eaIcon32)
    {
        DrawItemIcon(pCanvas, pItemRect, &itemRect, elementId, selected);
    }
}

VOID CctControl_ElementList::PaintGps(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CfxResource *resource = GetResource();
    FXRECT itemRect = *pItemRect;
    INT itemHeight = itemRect.Bottom - itemRect.Top + 1;
    static const CHAR MAX_NUMBER_TEXT[] = "-180.88888";
    INT maxNumberWidth = pCanvas->TextWidth(pFont, (CHAR *)MAX_NUMBER_TEXT);

    // Invert if needed
    BOOL selected = ((INT)Index == _penDownItemIndex) && ((INT)Index == _penOverItemIndex);
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
            FX_ASSERT(FALSE);
        }

        INT value = GetItemValue(Index); 
        CHAR valueText[32] = "";
        NumberToText(FxAbs(value), digits, decimals, 0, value < 0, valueText, FALSE);

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
        CHAR *text = NULL;
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
            FX_ASSERT(FALSE);
        }

        UINT flags = TEXT_ALIGN_VCENTER;
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
        INT itemHeight = rect.Bottom - rect.Top + 1;
        INT itemWidth = rect.Right - rect.Left + 1;
        INT scaleTo = min(itemHeight, itemWidth);

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

        UINT w = (itemRect.Bottom - itemRect.Top);
        FXRECT iconRect = {0, 0, w, w};
        InflateRect(&iconRect, -_oneSpace * 2, -_oneSpace * 2);
        OffsetRect(&iconRect, rect.Left, rect.Top);

        UINT radius = (iconRect.Bottom - iconRect.Top + 1) / 2;
        UINT y = iconRect.Top;
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
                FXPOINT triangle[] = {{radius, 0}, {radius*2, radius*2}, {0, radius*2}};
                POLYGON(pCanvas, triangle, iconRect.Left, y);
            }
            break;

        default:
            FX_ASSERT(FALSE);
        }

        _iconRect = iconRect;
        OffsetRect(&_iconRect, -itemRect.Left, -itemRect.Top);
        _iconRect.Right = _iconRect.Left + scaleTo;
        _iconRect.Bottom = _iconRect.Top + scaleTo;
    }
}

VOID CctControl_ElementList::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_firstPaint)
    {
        _firstPaint = FALSE;
        if (_sorted || (_historySortType != ehsNone))
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
        FXBITMAPRESOURCE *bitmap = NULL;
        ElementAttribute attributeDetails = max(_attribute, _attributeDetails);
        if (attributeDetails >= eaIcon32)
        {
            bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, elementId, attributeDetails);
        }

        UINT fontHeight = pCanvas->FontHeight(font);
        INT spacer = _borderLineWidth * 2;
        UINT w = _oneSpace * 2 *(bitmap ? bitmap->Width : 32);
        UINT h = _oneSpace * 2 * (bitmap ? bitmap->Height : 32);
        UINT clientW = max(0, (INT)GetClientWidth() - spacer);
        UINT clientH = max(0, (INT)GetClientHeight() - (INT)fontHeight - 8 - spacer);

        FXRECT bitmapRect = {0};

        if ((w > clientW) || (h > clientH))
        {
            if (bitmap)
            {
                GetBitmapRect(bitmap, clientW, clientH, TRUE, TRUE, TRUE, &bitmapRect);
            }
            else
            {
                bitmapRect.Top = bitmapRect.Bottom = (INT)-4 * _oneSpace;
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
        FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, elementId, eaName);
        if (element)
        {
            FX_ASSERT(element->Magic == MAGIC_TEXT);

            FXRECT rect = *pRect;
            rect.Top = min(bitmapRect.Bottom + 8 * _oneSpace, rect.Bottom - (INT)fontHeight - 4 * _oneSpace);
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