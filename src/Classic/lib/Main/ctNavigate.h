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
    bool _center = false, _stretch = false, _proportional = false;
    bool _transparentImage = false;
    XBITMAP _bitmap = 0;
    virtual void InternalDefineProperties(CfxFiler &F)                         {}
    virtual void InternalDefineResources(CfxFilerResource &F)                  {}
    virtual void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) {}
public:
    CctControl_NavButton(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NavButton();

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void DefineProperties(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};
extern CfxControl *Create_Control_NavButton(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Back

const GUID GUID_CONTROL_NAVBUTTON_BACK = {0xd4eec6a5, 0x3288, 0x4ae4, {0xa9, 0x41, 0x8f, 0xe3, 0x46, 0x8f, 0xf2, 0x46}};

class CctControl_NavButton_Back: public CctControl_NavButton
{
protected:
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_Back(CfxPersistent *pOwner, CfxControl *pParent);
    void OnPenClick(int X, int Y) override;
    void OnKeyUp(BYTE KeyCode, bool *pHandled) override;
};
extern CfxControl *Create_Control_NavButton_Back(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Next

const GUID GUID_CONTROL_NAVBUTTON_NEXT = {0x644e799f, 0x7737, 0x4cdd, {0xaa, 0x16, 0xa, 0x2e, 0x88, 0xbc, 0xf7, 0xec}};

class CctControl_NavButton_Next: public CctControl_NavButton
{
protected:
    XGUID _nextScreenId = 0;
    void InternalDefineProperties(CfxFiler &F) override;
    void InternalDefineResources(CfxFilerResource &F) override;
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_NavButton_Next(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NavButton_Next();
    void Setup(XGUID NextScreenId) { _nextScreenId = NextScreenId; }
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void OnPenClick(int X, int Y) override;
    void OnKeyUp(BYTE KeyCode, bool *pHandled) override;
};
extern CfxControl *Create_Control_NavButton_Next(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Home

const GUID GUID_CONTROL_NAVBUTTON_HOME = {0x7e9d4bd9, 0xd472, 0x49c4, { 0xbe, 0x6e, 0xe4, 0xd, 0x96, 0x82, 0x88, 0xe5}};

class CctControl_NavButton_Home: public CctControl_NavButton
{
protected:
    XGUID _homeScreenId = 0;
    void InternalDefineProperties(CfxFiler &F) override;
    void InternalDefineResources(CfxFilerResource &F) override;
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_Home(CfxPersistent *pOwner, CfxControl *pParent);
    void Setup(XGUID HomeScreenId) { _homeScreenId = HomeScreenId; }
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_Home(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Skip

const GUID GUID_CONTROL_NAVBUTTON_SKIP = {0x683dc482, 0xbbce, 0x455b, {0x90, 0x23, 0x15, 0x26, 0x33, 0xbc, 0xba, 0x60}};

class CctControl_NavButton_Skip: public CctControl_NavButton
{
protected:
    XGUID _skipScreenId = 0;
    bool _filterExecuted = false;
    char *_filterCaption = nullptr;
    void ExecuteFilter();
    void InternalDefineProperties(CfxFiler &F) override;
    void InternalDefineResources(CfxFilerResource &F) override;
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_Skip(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NavButton_Skip();
    void ClearFilterText();
    void Setup(XGUID SkipScreenId);
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_Skip(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Save

const GUID GUID_CONTROL_NAVBUTTON_SAVE = {0xee341cf4, 0x7a4c, 0x47ab, {0xb8, 0xf0, 0x6e, 0x8f, 0x9a, 0x49, 0xba, 0x4e}};

class CctControl_NavButton_Save: public CctControl_NavButton
{
protected:
    XGUID _saveScreenId = 0;
    bool _takeGPS = false;
    int _gpsSkipTimeout = 0;
    int _saveNumber = 0;
    void InternalDefineProperties(CfxFiler &F) override;
    void InternalDefineResources(CfxFilerResource &F) override;
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_Save(CfxPersistent *pOwner, CfxControl *pParent);

    void SetSaveNumber(int Value) { _saveNumber = Value; }
    void Setup(bool TakeGPS, int GpsSkipTimeout, XGUID SaveScreenId);

    void DefinePropertiesUI(CfxFilerUI &F) override;
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_Save(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Options

const GUID GUID_CONTROL_NAVBUTTON_OPTIONS = {0xe41c6e5b, 0x7a0d, 0x4805, {0x9a, 0x64, 0xa2, 0x8c, 0x6, 0x48, 0x87, 0x35}};

class CctControl_NavButton_Options: public CctControl_NavButton
{
protected:
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_Options(CfxPersistent *pOwner, CfxControl *pParent);
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_Options(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_GPS

const GUID GUID_CONTROL_NAVBUTTON_GPS = {0x46befad1, 0x2a97, 0x4cca, {0x82, 0xcc, 0xa4, 0x2c, 0xb5, 0xe9, 0x37, 0x8d}};

class CctControl_NavButton_GPS: public CctControl_NavButton
{
protected:
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_GPS(CfxPersistent *pOwner, CfxControl *pParent);
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_GPS(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_AcceptEdit

const GUID GUID_CONTROL_NAVBUTTON_ACCEPTEDIT = {0x9fe929ec, 0x6e4b, 0x4997, {0x8c, 0x9e, 0xaa, 0xb5, 0x88, 0x65, 0x45, 0x7a}};

class CctControl_NavButton_AcceptEdit: public CctControl_NavButton
{
protected:
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_AcceptEdit(CfxPersistent *pOwner, CfxControl *pParent);
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_AcceptEdit(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_CancelEdit

const GUID GUID_CONTROL_NAVBUTTON_CANCELEDIT = {0xb29a2264, 0xa10e, 0x4e1d, {0xbc, 0x83, 0x17, 0x8c, 0xe4, 0xbc, 0xd, 0x68}};

class CctControl_NavButton_CancelEdit: public CctControl_NavButton
{
protected:
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_CancelEdit(CfxPersistent *pOwner, CfxControl *pParent);
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_CancelEdit(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_NavButton_Jump

const GUID GUID_CONTROL_NAVBUTTON_JUMP = {0xda65dda3, 0xf04f, 0x48a6, {0x96, 0xdf, 0x95, 0x44, 0xe3, 0xf, 0xcd, 0xdc}};

class CctControl_NavButton_Jump: public CctControl_NavButton
{
protected:
    XGUID _baseScreenId = 0, _jumpScreenId = 0;
    void InternalDefineProperties(CfxFiler &F) override;
    void InternalDefineResources(CfxFilerResource &F) override;
    void InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent) override;
public:
    CctControl_NavButton_Jump(CfxPersistent *pOwner, CfxControl *pParent);
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void OnPenClick(int X, int Y) override;
};
extern CfxControl *Create_Control_NavButton_Jump(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
// CctControl_Navigate

const GUID GUID_CONTROL_NAVIGATE = {0xdd2f292a, 0x4c6b, 0x44d7, {0x92, 0xc5, 0x9c, 0x39, 0x22, 0xed, 0x03, 0x50}};
class CctControl_Navigate: public CfxControl
{
protected:
	bool _showOptions = false, _showGPS = false, _showSave1 = false, _showSave2 = false, _showBack = false, _showNext = false, _showHome = false, _showSkip = false;
    CctControl_NavButton_AcceptEdit *_acceptEditButton = nullptr;
    CctControl_NavButton_CancelEdit *_cancelEditButton = nullptr;
    CctControl_NavButton_Options *_optionsButton = nullptr;
    CctControl_NavButton_GPS *_gpsButton = nullptr;
    CctControl_NavButton_Save *_save1Button = nullptr;
    CctControl_NavButton_Save *_save2Button = nullptr;
    CctControl_NavButton_Back *_backButton = nullptr;
    CctControl_NavButton_Next *_nextButton = nullptr;
    CctControl_NavButton_Skip *_skipButton = nullptr;
    CctControl_NavButton_Home *_homeButton = nullptr;

    int _buttonWidth = 0, _buttonBorderWidth = 0, _buttonBorderLineWidth = 0;
    ControlBorderStyle _buttonBorderStyle = {};

    XFONT _timerFont = 0, _skipFont = 0;
    XGUID _homeScreenId = 0, _nextScreenId = 0, _save1ScreenId = 0, _save2ScreenId = 0, _skipScreenId = 0;
    bool _takeGPS = false;
    int _gpsSkipTimeout = 0;
    void UpdateButtons();
public:
    CctControl_Navigate(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_Navigate();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    XGUID *GetSave1Target()    { return &_save1ScreenId; }
    bool GetSave1Visible()     { return _showSave1;      }
    XGUID *GetSave2Target()    { return &_save2ScreenId; }
    bool GetSave2Visible()     { return _showSave2;      }
};

extern CfxControl *Create_Control_Navigate(CfxPersistent *pOwner, CfxControl *pParent);
