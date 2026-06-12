#pragma once

#include "pch.h"
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_ELEMENTLIST = {0x289461c2, 0xb3ee, 0x4075, {0x95, 0x38, 0x45, 0x15, 0x80, 0xbd, 0x4b, 0x38}};

const int ELEMENT_LIST_VERSION = 1;

const int MAGIC_ELEMENT_FILTER = 'FILT';

enum class ElementAliasOverride : int8_t {None=0, Alias1, Alias2, Alias3};
enum class ElementListMode : int8_t {Radio=0, Check, Number, CheckIcon, NumberFastTap, NumberKeypad, Gps};
enum class ElementHitPlace : int8_t {Icon=0, Check, Number, Text};
enum class ElementHistorySortType : int8_t {None=0, Bottom, Top};

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
    XGUID _radioElement = 0;
    bool _firstPaint = false;
    bool _blockNext = false;
    bool _radioSetGoto = false;
    ElementAliasOverride _aliasOverride = ElementAliasOverride::None, _aliasDetails = ElementAliasOverride::None;
    ElementAttribute _attribute = ElementAttribute::Name, _attributeDetails = ElementAttribute::Name;
    ElementListMode _listMode = ElementListMode::Radio;
    bool _showCaption = false, _saveResult = false;
    bool _hideLinks = false;
    int _selectedIndex = 0;
    BYTE _digits = 0, _decimals = 0, _fraction = 0;
    int _minValue = 0, _maxValue = 0;
    int _minChecks = 0, _maxChecks = 0;
    bool _keypadFormulaMode = false;
    bool _transparentImages = false;
    FXRECT _iconRect, _checkRect, _numberRect, _textRect, _minusRect;
    XFONT _detailsFont = 0;
    bool _sorted = false;
    bool _outputChecks = false;
    bool _rightToLeft = false;
    bool _centerText = false;
    bool _numberChecks = false;
    bool _detailsOnDown = false;

    bool _setLat = false, _setLon = false;

    XBINARY _filter = 0;
    bool _filterEnabled = false;
    bool _filterValid = false, _filterError = false;
    CctRetainState _retainState;
    bool _retainOnlyScrollState = false;
    int _autoSelectIndex = 0;
    bool _autoRadioNext = false;
    bool _historyEnabled = false, _historyCanSelect = false;
    COLOR _historyTextColor = 0;
    ElementHistorySortType _historySortType = ElementHistorySortType::None;

    int _lastItemIndex = 0, _detailsItemIndex = 0, _detailsTimerItemIndex = 0, _penDownItemIndex = 0, _penOverItemIndex = 0;
    bool _detailsShown = false;

    TList<BYTE> _checkStates;
    TList<int> _itemValues;
    TList<int> _filterItems;
    TList<int> _sortedItems;
    TList<BYTE> _historyStates;

    bool _showRadioSound = false, _showRadioSoundStop = false;
    CfxControl_Button *_playButton = nullptr, *_stopButton = nullptr;

    char *_resultGlobalValue = nullptr;

    void SetRadioSelectedIndex(int Index);

    int GetVersion() override { return ELEMENT_LIST_VERSION; }

    void ExecuteSort();

    void RebuildLinks();
    bool ExecuteFilter();

    void ValidateItemCount();

    bool GetCheckState(int Index);
    void SetCheckState(int Index, bool Value);
    void IncItemValue(int Index, BYTE Digit, int Delta);

    void DrawItemCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, bool Checked, bool Selected);
    void DrawItemIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, XGUID ElementId, bool Selected = false);
    void DrawItemText(CfxCanvas *pCanvas, FXRECT *pItemRect, FXRECT *pRect, FXFONTRESOURCE *pFont, XGUID ElementId, bool Selected = false);

    void PaintRadio(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index);
    void PaintCheck(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index);
    void PaintCheckIcon(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index);
    void PaintNumber(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index);
    void PaintGps(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index);

    ElementHitPlace HitTestItem(int OffsetX, int OffsetY);

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    void OnSessionSetJumpTarget(CfxControl *pSender, void *pParams);

    void OnPlayClick(CfxControl *pSender, void *pParams);
    void OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnStopClick(CfxControl *pSender, void *pParams);
    void OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    bool IsRadioList()  { return (_listMode == ElementListMode::Radio); }
    bool IsCheckList()  { return (_listMode == ElementListMode::Check) || (_listMode == ElementListMode::CheckIcon); }
    bool IsNumberList() { return (_listMode == ElementListMode::Number) || (_listMode == ElementListMode::NumberFastTap) || (_listMode == ElementListMode::NumberKeypad) || (_listMode == ElementListMode::Gps); }

public:
    CctControl_ElementList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementList();

    void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenDrag(int StartX, int StartY, int X, int Y) override;
    void OnPenClick(int X, int Y) override;

    void ResetState() override;

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void OnTimer() override;

    ElementListMode GetListMode()           { return _listMode;  }
    void SetListMode(ElementListMode Value) { _listMode = Value; }

    bool GetPosition(FXGPS_POSITION *pPosition);

    char *GetElementText(FXTEXTRESOURCE *Element);
    char *GetDetailsText(FXTEXTRESOURCE *Element);

    bool IsFilterValid()    { return _filterValid; }
    bool IsFilterEnabled()  { return _filterValid && _filterEnabled && _filter; }
    int GetRawItemIndex(int Index);
    int GetItemCount() override;
    XGUID GetItem(int Index);

    int GetItemValue(int Index);
    void SetItemValue(int Index, int Value);

    bool GetHistoryState(int Index);
    void SetHistoryState(int Index, bool Value);

    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;
};

extern CfxControl *Create_Control_ElementList(CfxPersistent *pOwner, CfxControl *pParent);

