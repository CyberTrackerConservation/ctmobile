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
#include "fxPanel.h"
#include "fxTitleBar.h"
#include "fxNotebook.h"
#include "fxList.h"
#include "ctElement.h"
#include "fxDatabase.h"
#include "ctSighting.h"

//*************************************************************************************************

const GUID GUID_CONTROL_SIGHTINGS = {0xaec302bd, 0x2d98, 0x4206, {0xab, 0x37, 0x22, 0x19, 0x7c, 0x18, 0x80, 0xda}};

//
// CControl_SightingList
//
class CctControl_Sightings: public CfxControl_List
{
protected:
    CfxDatabase *_paintDatabase;
    CctSighting *_sighting;
    DBID _selectedId, _markId; 
    UINT _sightingCount;
    BOOL _showEditButton, _showUnsaved;
    FXEVENT _onChange;
    XGUIDLIST _elements;
    XGUID _valueElementId;
    TList<INT> _itemMap;
    XGUIDLIST *GetElements();
    CfxControl_Button *_editButton;
    VOID OnEditButtonClick(CfxControl *pSender, VOID *pParams);
public:
    CctControl_Sightings(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_Sightings();

    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);

    VOID OnPenDown(INT X, INT Y);

	DBID GetMarkId()               { return _markId;      }
	VOID SetMarkId(DBID Value)     { _markId = Value;     }
	DBID GetSelectedId()           { return _selectedId;  }
	VOID SetSelectedId(DBID Value) { _selectedId = Value; }

    VOID SetOnChange(CfxControl *pControl, NotifyMethod OnClick);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);

    VOID Update();
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
    VOID DefineProperties(CfxFiler &F);
    VOID SetActiveId(DBID Value);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    VOID OnPenDown(INT X, INT Y);
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
    DBID _activeId; 
    UINT _buttonHeight;

    CctControl_Attributes *_attributes;
    FXEVENT _onChange, _onEditClick;

    CfxControl_Panel *_buttonPanel;
    CfxControl_Button *_buttons[5];

    VOID OnButtonClick(CfxControl *pSender, VOID *pParams);
    VOID OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnButtonPanelResize(CfxControl *pSender, VOID *pParams);
public:
    CctControl_AttributeInspector(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AttributeInspector();

    VOID DefineResources(CfxFilerResource &F);
	VOID DefineProperties(CfxFiler &F);

    CctControl_Attributes *GetAttributes() { return _attributes; }

    DBID GetActiveId()           { return _activeId;                                   }
    VOID SetActiveId(DBID Value) { _activeId = Value; _attributes->SetActiveId(Value); }

    VOID SetOnChange(CfxControl *pControl, NotifyMethod OnClick);
    VOID SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
};

extern CfxControl *Create_Control_AttributeInspector(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_SENDDATA = {0x893605b3, 0xe98c, 0x42fa, {0x88, 0xad, 0xb6, 0x9f, 0x7a, 0xdc, 0xbe, 0x70}};

enum SendDataState {sdsNoData, sdsIdle, sdsBusy};

class CctControl_SendData: public CfxControl_Button
{
protected:
    FXEVENT _onAfterSend;
    BOOL _hideState;
    BOOL _packaging;
    SendDataState _state;
public:
    CctControl_SendData(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SendData();

    VOID InitState();

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F); 

    VOID OnTransfer(FXSENDSTATEMODE Mode);

    VOID SetOnAfterSend(CfxControl *pControl, NotifyMethod OnEvent);
};

extern CfxControl *Create_Control_SendData(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_SHAREDATA = {0x893805b3, 0xe98c, 0x42fa, {0x88, 0xad, 0xb6, 0x9f, 0x7a, 0xdc, 0xbe, 0x70}};

class CctControl_ShareData: public CfxControl_Button
{
protected:
    BOOL _center, _stretch, _proportional;
    BOOL _transparentImage;
    XBITMAP _bitmap;
    BYTE _format;

public:
    CctControl_ShareData(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ShareData();

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID OnPenClick(INT X, INT Y);
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
    VOID SetActiveIndex(UINT Value);
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
    UINT _activeIndex; 
    UINT _itemCount;
    UINT _buttonHeight, _itemHeight;

    CctControl_HistoryList *_history;

    CfxControl_Panel *_buttonPanel;
    CfxControl_Button *_buttons[5];

    VOID OnButtonClick(CfxControl *pSender, VOID *pParams);
    VOID OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnButtonPanelResize(CfxControl *pSender, VOID *pParams);
public:
    CctControl_HistoryInspector(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_HistoryInspector();

    VOID DefineResources(CfxFilerResource &F);
	VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    CctControl_HistoryList *GetHistory()  { return _history; }

    UINT GetActiveIndex()           { return _activeIndex; }
    VOID SetActiveIndex(UINT Value) { _activeIndex = Value; _history->SetActiveIndex(Value); }
};

extern CfxControl *Create_Control_HistoryInspector(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_DIALOG_SIGHTINGEDITOR = {0x9854dec9, 0x80fa, 0x4084, {0xae, 0xc9, 0xaf, 0xf2, 0x4f, 0x0e, 0xc5, 0x82}};

//
// CctDialog_SightingEditor
//
class CctDialog_SightingEditor: public CfxDialog
{
protected:
    CfxControl *_sender;

    CfxControl_TitleBar *_titleBar;
    CfxControl_Panel *_spacer;
    CfxControl_Notebook *_noteBook;

    CctControl_Sightings *_sightings;
    CctControl_AttributeInspector *_attributeInspector;
    CctControl_HistoryInspector *_historyInspector;
    CctControl_SendData *_sendData;
    CfxControl_Button *_showExports;

    VOID OnSightingEditClick(CfxControl *pSender, VOID *pParams);

    VOID OnSightingsChange(CfxControl *pSender, VOID *pParams);
    VOID OnAttributeInspectorChange(CfxControl *pSender, VOID *pParams);
    VOID OnAfterSend(CfxControl *pSender, VOID *pParams);
    VOID OnShowExports(CfxControl *pSender, VOID *pParams);

public:
    CctDialog_SightingEditor(CfxPersistent *pOwner, CfxControl *pParent);

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);

    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    
    VOID OnCloseDialog(BOOL *pHandled);
};

extern CfxDialog *Create_Dialog_SightingEditor(CfxPersistent *pOwner, CfxControl *pParent);

