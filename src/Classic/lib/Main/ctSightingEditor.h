#pragma once
#include "pch.h"
#include "fxPanel.h"
#include "fxTitleBar.h"
#include "fxNotebook.h"
#include "fxList.h"
#include "ctElement.h"
#include "fxDatabase.h"
#include "ctSighting.h"
#include "ctSession.h"

//*************************************************************************************************

const GUID GUID_CONTROL_SIGHTINGS = {0xaec302bd, 0x2d98, 0x4206, {0xab, 0x37, 0x22, 0x19, 0x7c, 0x18, 0x80, 0xda}};

//
// CControl_SightingList
//
class CctControl_Sightings: public CfxControl_List
{
protected:
    CctSightingDatabase *_paintDatabase = nullptr;
    CctSighting *_sighting = nullptr;
    QString _selectedId, _markId;
    int _sightingCount = 0;
    bool _showEditButton = false, _showUnsaved = false;
    FXEVENT _onChange;
    XGUIDLIST _elements;
    XGUID _valueElementId = 0;
    TList<int> _itemMap;
    XGUIDLIST *GetElements();
    CfxControl_Button *_editButton = nullptr;
    void OnEditButtonClick(CfxControl *pSender, void *pParams);
public:
    CctControl_Sightings(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_Sightings();

    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;

    void OnPenClick(int X, int Y) override;

	QString GetMarkId()                       { return _markId;      }
	void SetMarkId(const QString &Value)      { _markId = Value;     }
	QString GetSelectedId()                   { return _selectedId;  }
	void SetSelectedId(const QString &Value)  { _selectedId = Value; }

    void SetOnChange(CfxControl *pControl, NotifyMethod OnClick);

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;

    void Update();
};

extern CfxControl *Create_Control_Sightings(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_ATTRIBUTES = {0xb9f46f85, 0x7f7b, 0x429c, {0xa3, 0x7, 0x90, 0xa3, 0xf4, 0xe1, 0x61, 0xf2}};

//
// CctControl_Attributes
//
class CctControl_Attributes: public CfxControl_List
{
public:
    CctControl_Attributes(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_Attributes();
    void DefineProperties(CfxFiler &F) override;
    void SetActiveId(const QString &Value);
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_Attributes(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_ATTRIBUTEINSPECTOR = {0xa8f3af8c, 0x7444, 0x4313, {0x86, 0x19, 0x1a, 0x11, 0xf8, 0x40, 0x8c, 0xd3}};

//
// CctControl_AttributeInspector
//
class CctControl_AttributeInspector: public CfxControl
{
protected:
    QString _activeId;
    int _buttonHeight = 0;

    CctControl_Attributes *_attributes = nullptr;
    FXEVENT _onChange, _onEditClick;

    CfxControl_Panel *_buttonPanel = nullptr;
    CfxControl_Button *_buttons[5] = {};

    void OnButtonClick(CfxControl *pSender, void *pParams);
    void OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnButtonPanelResize(CfxControl *pSender, void *pParams);
public:
    CctControl_AttributeInspector(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AttributeInspector();

    void DefineResources(CfxFilerResource &F) override;
	void DefineProperties(CfxFiler &F) override;

    CctControl_Attributes *GetAttributes() { return _attributes; }

    QString GetActiveId()                       { return _activeId;                                   }
    void SetActiveId(const QString &Value)      { _activeId = Value; _attributes->SetActiveId(Value); }

    void SetOnChange(CfxControl *pControl, NotifyMethod OnClick);
    void SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick);

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
};

extern CfxControl *Create_Control_AttributeInspector(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_SENDDATA = {0x893605b3, 0xe98c, 0x42fa, {0x88, 0xad, 0xb6, 0x9f, 0x7a, 0xdc, 0xbe, 0x70}};

enum class SendDataState : int8_t {NoData=0, Idle, Busy};

class CctControl_SendData: public CfxControl_Button
{
protected:
    FXEVENT _onAfterSend;
    bool _hideState = false;
    bool _packaging = false;
    SendDataState _state = SendDataState::NoData;
public:
    CctControl_SendData(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SendData();

    void InitState();

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;

    void SetOnAfterSend(CfxControl *pControl, NotifyMethod OnEvent);
};

extern CfxControl *Create_Control_SendData(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_SHAREDATA = {0x893805b3, 0xe98c, 0x42fa, {0x88, 0xad, 0xb6, 0x9f, 0x7a, 0xdc, 0xbe, 0x70}};

class CctControl_ShareData: public CfxControl_Button
{
protected:
    bool _center = false, _stretch = false, _proportional = false;
    bool _transparentImage = false;
    XBITMAP _bitmap = 0;
    BYTE _format = 0;

public:
    CctControl_ShareData(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ShareData();

    void SetBounds(int Left, int Top, int Width, int Height) override;

    void DefineProperties(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_ShareData(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_HISTORY_LIST = {0x1e7c30a5, 0x1440, 0x4daa, {0xa5, 0x6c, 0xc0, 0x33, 0x7d, 0x69, 0x9f, 0x1d}};

//
// CctControl_HistoryList
//
class CctControl_HistoryList: public CfxControl_List
{
public:
    CctControl_HistoryList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_HistoryList();
    void SetActiveIndex(int Value);
};

extern CfxControl *Create_Control_HistoryList(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_HISTORY_INSPECTOR = {0xfcfe1d93, 0xcc11, 0x4ac3, {0x83, 0xbe, 0x10, 0x70, 0xe8, 0x51, 0x48, 0x9a}};

//
// CctControl_HistoryInspector
//
class CctControl_HistoryInspector: public CfxControl_Panel
{
protected:
    int _activeIndex = 0;
    int _itemCount = 0;
    int _buttonHeight = 0, _itemHeight = 0;

    CctControl_HistoryList *_history = nullptr;

    CfxControl_Panel *_buttonPanel = nullptr;
    CfxControl_Button *_buttons[5] = {};

    void OnButtonClick(CfxControl *pSender, void *pParams);
    void OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnButtonPanelResize(CfxControl *pSender, void *pParams);
public:
    CctControl_HistoryInspector(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_HistoryInspector();

    void DefineResources(CfxFilerResource &F) override;
	void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    CctControl_HistoryList *GetHistory()  { return _history; }

    int GetActiveIndex()           { return _activeIndex; }
    void SetActiveIndex(int Value) { _activeIndex = Value; _history->SetActiveIndex(Value); }
};

extern CfxControl *Create_Control_HistoryInspector(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

