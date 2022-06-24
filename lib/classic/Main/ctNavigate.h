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
#include "fxControl.h"
#include "fxButton.h"
#include "ctActions.h"

class CctControl_Navigate;

//*************************************************************************************************
// CctControl_NavButton

class CctControl_NavButton: public CfxControl_Button
{
friend class CctControl_Navigate;
protected:
    BOOL _center, _stretch, _proportional;
    BOOL _transparentImage;
    XBITMAP _bitmap;
    virtual VOID InternalDefineProperties(CfxFiler &F)                         {}
    virtual VOID InternalDefineResources(CfxFilerResource &F)                  {}
    virtual VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent) {}
public:
    CctControl_NavButton(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NavButton();

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};
extern CfxControl *Create_Control_NavButton(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Back

const GUID GUID_CONTROL_NAVBUTTON_BACK = {0xd4eec6a5, 0x3288, 0x4ae4, {0xa9, 0x41, 0x8f, 0xe3, 0x46, 0x8f, 0xf2, 0x46}};

class CctControl_NavButton_Back: public CctControl_NavButton
{
protected:
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_Back(CfxPersistent *pOwner, CfxControl *pParent);
    VOID OnPenClick(INT X, INT Y);
    VOID OnKeyUp(BYTE KeyCode, BOOL *pHandled);
};
extern CfxControl *Create_Control_NavButton_Back(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Next

const GUID GUID_CONTROL_NAVBUTTON_NEXT = {0x644e799f, 0x7737, 0x4cdd, {0xaa, 0x16, 0xa, 0x2e, 0x88, 0xbc, 0xf7, 0xec}};

class CctControl_NavButton_Next: public CctControl_NavButton
{
protected:
    XGUID _nextScreenId;
    VOID InternalDefineProperties(CfxFiler &F);
    VOID InternalDefineResources(CfxFilerResource &F);
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_NavButton_Next(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NavButton_Next();
    VOID Setup(XGUID NextScreenId) { _nextScreenId = NextScreenId; }
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID OnPenClick(INT X, INT Y);
    VOID OnKeyUp(BYTE KeyCode, BOOL *pHandled);
};
extern CfxControl *Create_Control_NavButton_Next(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Home

const GUID GUID_CONTROL_NAVBUTTON_HOME = {0x7e9d4bd9, 0xd472, 0x49c4, { 0xbe, 0x6e, 0xe4, 0xd, 0x96, 0x82, 0x88, 0xe5}};

class CctControl_NavButton_Home: public CctControl_NavButton
{
protected:
    XGUID _homeScreenId;
    VOID InternalDefineProperties(CfxFiler &F);
    VOID InternalDefineResources(CfxFilerResource &F);
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_Home(CfxPersistent *pOwner, CfxControl *pParent);
    VOID Setup(XGUID HomeScreenId) { _homeScreenId = HomeScreenId; }
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_Home(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Skip

const GUID GUID_CONTROL_NAVBUTTON_SKIP = {0x683dc482, 0xbbce, 0x455b, {0x90, 0x23, 0x15, 0x26, 0x33, 0xbc, 0xba, 0x60}};

class CctControl_NavButton_Skip: public CctControl_NavButton
{
protected:
    XGUID _skipScreenId;
    BOOL _filterExecuted;
    CHAR *_filterCaption;
    VOID ExecuteFilter();
    VOID InternalDefineProperties(CfxFiler &F);
    VOID InternalDefineResources(CfxFilerResource &F);
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_Skip(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NavButton_Skip();
    VOID ClearFilterText();
    VOID Setup(XGUID SkipScreenId);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_Skip(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Save

const GUID GUID_CONTROL_NAVBUTTON_SAVE = {0xee341cf4, 0x7a4c, 0x47ab, {0xb8, 0xf0, 0x6e, 0x8f, 0x9a, 0x49, 0xba, 0x4e}};

class CctControl_NavButton_Save: public CctControl_NavButton
{
protected:
    XGUID _saveScreenId;
    BOOL _takeGPS;
    INT _gpsSkipTimeout;
    INT _saveNumber;
    VOID InternalDefineProperties(CfxFiler &F);
    VOID InternalDefineResources(CfxFilerResource &F);
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_Save(CfxPersistent *pOwner, CfxControl *pParent);

    VOID SetSaveNumber(INT Value) { _saveNumber = Value; }
    VOID Setup(BOOL TakeGPS, INT GpsSkipTimeout, XGUID SaveScreenId);

    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_Save(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Options

const GUID GUID_CONTROL_NAVBUTTON_OPTIONS = {0xe41c6e5b, 0x7a0d, 0x4805, {0x9a, 0x64, 0xa2, 0x8c, 0x6, 0x48, 0x87, 0x35}};

class CctControl_NavButton_Options: public CctControl_NavButton
{
protected:
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_Options(CfxPersistent *pOwner, CfxControl *pParent);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_Options(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_GPS

const GUID GUID_CONTROL_NAVBUTTON_GPS = {0x46befad1, 0x2a97, 0x4cca, {0x82, 0xcc, 0xa4, 0x2c, 0xb5, 0xe9, 0x37, 0x8d}};

class CctControl_NavButton_GPS: public CctControl_NavButton
{
protected:
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_GPS(CfxPersistent *pOwner, CfxControl *pParent);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_GPS(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_AcceptEdit

const GUID GUID_CONTROL_NAVBUTTON_ACCEPTEDIT = {0x9fe929ec, 0x6e4b, 0x4997, {0x8c, 0x9e, 0xaa, 0xb5, 0x88, 0x65, 0x45, 0x7a}};

class CctControl_NavButton_AcceptEdit: public CctControl_NavButton
{
protected:
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_AcceptEdit(CfxPersistent *pOwner, CfxControl *pParent);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_AcceptEdit(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_CancelEdit

const GUID GUID_CONTROL_NAVBUTTON_CANCELEDIT = {0xb29a2264, 0xa10e, 0x4e1d, {0xbc, 0x83, 0x17, 0x8c, 0xe4, 0xbc, 0xd, 0x68}};

class CctControl_NavButton_CancelEdit: public CctControl_NavButton
{
protected:
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_CancelEdit(CfxPersistent *pOwner, CfxControl *pParent);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_CancelEdit(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Jump

const GUID GUID_CONTROL_NAVBUTTON_JUMP = {0xda65dda3, 0xf04f, 0x48a6, {0x96, 0xdf, 0x95, 0x44, 0xe3, 0xf, 0xcd, 0xdc}};

class CctControl_NavButton_Jump: public CctControl_NavButton
{
protected:
    XGUID _baseScreenId, _jumpScreenId;
    VOID InternalDefineProperties(CfxFiler &F);
    VOID InternalDefineResources(CfxFilerResource &F);
    VOID InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent);
public:
    CctControl_NavButton_Jump(CfxPersistent *pOwner, CfxControl *pParent);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID OnPenClick(INT X, INT Y);
};
extern CfxControl *Create_Control_NavButton_Jump(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_Navigate

const GUID GUID_CONTROL_NAVIGATE = {0xdd2f292a, 0x4c6b, 0x44d7, {0x92, 0xc5, 0x9c, 0x39, 0x22, 0xed, 0x03, 0x50}};
class CctControl_Navigate: public CfxControl
{
protected:
	BOOL _showOptions, _showGPS, _showSave1, _showSave2, _showBack, _showNext, _showHome, _showSkip;
    CctControl_NavButton_AcceptEdit *_acceptEditButton;
    CctControl_NavButton_CancelEdit *_cancelEditButton;
    CctControl_NavButton_Options *_optionsButton;
    CctControl_NavButton_GPS *_gpsButton;
    CctControl_NavButton_Save *_save1Button;
    CctControl_NavButton_Save *_save2Button;
    CctControl_NavButton_Back *_backButton;
    CctControl_NavButton_Next *_nextButton;
    CctControl_NavButton_Skip *_skipButton;
    CctControl_NavButton_Home *_homeButton;
    
    UINT _buttonWidth, _buttonBorderWidth, _buttonBorderLineWidth;
    ControlBorderStyle _buttonBorderStyle;

    XFONT _timerFont, _skipFont;
    XGUID _homeScreenId, _nextScreenId, _save1ScreenId, _save2ScreenId, _skipScreenId;
    BOOL _takeGPS;
    INT _gpsSkipTimeout;
    VOID UpdateButtons();
public:
    CctControl_Navigate(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_Navigate();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    XGUID *GetSave1Target()    { return &_save1ScreenId; }
    BOOL GetSave1Visible()     { return _showSave1;      }
    XGUID *GetSave2Target()    { return &_save2ScreenId; }
    BOOL GetSave2Visible()     { return _showSave2;      }
};

extern CfxControl *Create_Control_Navigate(CfxPersistent *pOwner, CfxControl *pParent);
