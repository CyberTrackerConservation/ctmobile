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
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_ELEMENTLIST = {0x289461c2, 0xb3ee, 0x4075, {0x95, 0x38, 0x45, 0x15, 0x80, 0xbd, 0x4b, 0x38}};

const UINT ELEMENT_LIST_VERSION = 1;

const UINT MAGIC_ELEMENT_FILTER = 'FILT';

enum ElementAliasOverride {eaoNone, eaoAlias1, eaoAlias2, eaoAlias3};
enum ElementListMode {elmRadio=0, elmCheck, elmNumber, elmCheckIcon, elmNumberFastTap, elmNumberKeypad, elmGps};
enum ElementHitPlace {ehpIcon=0, ehpCheck, ehpNumber, ehpText};
enum ElementHistorySortType {ehsNone=0, ehsBottom, ehsTop};

// For ListMode == elmGps
const BYTE GPS_LAT_DIGITS = 8;
const BYTE GPS_LAT_DECIMALS = 5;

const BYTE GPS_LON_DIGITS = 8;
const BYTE GPS_LON_DECIMALS = 5;

const BYTE GPS_ALT_DIGITS = 6;
const BYTE GPS_ALT_DECIMALS = 0;

//
// CctControl_ElementList
//
class CctControl_ElementList: public CfxControl_List
{
protected:
    XGUIDLIST _elements, _links;
    XGUID _radioElement;
    BOOL _firstPaint;
    BOOL _blockNext;
    BOOL _radioSetGoto;
    ElementAliasOverride _aliasOverride, _aliasDetails;
    ElementAttribute _attribute, _attributeDetails;
    ElementListMode _listMode;
    BOOL _showCaption, _saveResult;
    BOOL _hideLinks;
    INT _selectedIndex;
    BYTE _digits, _decimals, _fraction;
    INT _minValue, _maxValue;
    UINT _minChecks, _maxChecks;
    BOOL _keypadFormulaMode;
    BOOL _transparentImages;
    FXRECT _iconRect, _checkRect, _numberRect, _textRect, _minusRect;
    XFONT _detailsFont;
    BOOL _sorted;
    BOOL _outputChecks;
    BOOL _rightToLeft;
    BOOL _centerText;
    BOOL _numberChecks;
    BOOL _detailsOnDown;

    BOOL _setLat, _setLon;

    XBINARY _filter;
    BOOL _filterEnabled;
    BOOL _filterValid, _filterError;
    CctRetainState _retainState;
    BOOL _retainOnlyScrollState;
    UINT _autoSelectIndex;
    BOOL _autoRadioNext;
    BOOL _historyEnabled, _historyCanSelect;
    COLOR _historyTextColor;
    ElementHistorySortType _historySortType;

    INT _lastItemIndex, _detailsItemIndex, _detailsTimerItemIndex, _penDownItemIndex, _penOverItemIndex;
    BOOL _detailsShown;

    TList<BYTE> _checkStates;
    TList<INT> _itemValues;
    TList<INT> _filterItems;
    TList<INT> _sortedItems;
    TList<BYTE> _historyStates;

    BOOL _showRadioSound, _showRadioSoundStop;
    CfxControl_Button *_playButton, *_stopButton;

    CHAR *_resultGlobalValue;

    VOID SetRadioSelectedIndex(INT Index);

    UINT GetVersion() { return ELEMENT_LIST_VERSION; }

    VOID ExecuteSort();

    VOID RebuildLinks();
    BOOL ExecuteFilter();

    VOID ValidateItemCount();

    BOOL GetCheckState(UINT Index);
    VOID SetCheckState(UINT Index, BOOL Value);
    VOID IncItemValue(UINT Index, BYTE Digit, INT Delta);

    VOID DrawItemCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, BOOL Checked, BOOL Selected);
    VOID DrawItemIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, XGUID ElementId, BOOL Selected = FALSE);
    VOID DrawItemText(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, FXFONTRESOURCE *pFont, XGUID ElementId, BOOL Selected = FALSE);

    VOID PaintRadio(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);
    VOID PaintCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);
    VOID PaintCheckIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);
    VOID PaintNumber(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);
    VOID PaintGps(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);

    ElementHitPlace HitTestItem(INT OffsetX, INT OffsetY);

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionSetJumpTarget(CfxControl *pSender, VOID *pParams);

    VOID OnPlayClick(CfxControl *pSender, VOID *pParams);
    VOID OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnStopClick(CfxControl *pSender, VOID *pParams);
    VOID OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    BOOL IsRadioList()  { return (_listMode == elmRadio); }
    BOOL IsCheckList()  { return (_listMode == elmCheck) || (_listMode == elmCheckIcon); }
    BOOL IsNumberList() { return (_listMode == elmNumber) || (_listMode == elmNumberFastTap) || (_listMode == elmNumberKeypad) || (_listMode == elmGps); }

public:
    CctControl_ElementList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementList();

    VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
    VOID OnPenMove(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);

    VOID ResetState();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnTimer();

    ElementListMode GetListMode()           { return _listMode;  }
    VOID SetListMode(ElementListMode Value) { _listMode = Value; }

    BOOL GetPosition(FXGPS_POSITION *pPosition);

    CHAR *GetElementText(FXTEXTRESOURCE *Element);
    CHAR *GetDetailsText(FXTEXTRESOURCE *Element);

    BOOL IsFilterValid()    { return _filterValid; }
    BOOL IsFilterEnabled()  { return _filterValid && _filterEnabled && _filter; }
    UINT GetRawItemIndex(UINT Index);
    UINT GetItemCount();
    XGUID GetItem(UINT Index);

    INT GetItemValue(UINT Index);
    VOID SetItemValue(UINT Index, INT Value);

    BOOL GetHistoryState(UINT Index);
    VOID SetHistoryState(UINT Index, BOOL Value);

    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);
};

extern CfxControl *Create_Control_ElementList(CfxPersistent *pOwner, CfxControl *pParent);

