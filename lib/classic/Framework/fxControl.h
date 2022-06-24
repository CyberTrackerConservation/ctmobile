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
#include "fxClasses.h"
#include "fxLists.h"
#include "fxCanvas.h"

const UINT ACTION_WIDTH  = 24;
const UINT ACTION_HEIGHT = 24;

enum ControlDock {dkNone=0, dkTop, dkBottom, dkLeft, dkRight, dkFill, dkAction};
enum ControlBorderStyle {bsNone=0, bsSingle, bsRound1, bsRound2};

class CfxControl;

//
// CfxResource
//
class CfxResource: public CfxPersistent
{
protected:
    FXRESOURCEHEADER *_header;
public:
    CfxResource(CfxPersistent *pOwner): CfxPersistent(pOwner) { _header = NULL; }
    virtual BOOL Verify()=0;
   
    virtual FXRESOURCEHEADER *GetHeader(CfxControl *pControl) { return _header; }

    virtual FXPROFILE *GetProfile(CfxControl *pControl, UINT Index)=0;
    virtual VOID ReleaseProfile(FXPROFILE *pProfile)=0;
    virtual FXFONTRESOURCE *GetFont(CfxControl *pControl, XFONT Font)=0;
    virtual VOID ReleaseFont(FXFONTRESOURCE *pFontData)=0;
    virtual FXSOUNDRESOURCE *GetSound(CfxControl *pControl, XSOUND Sound)=0;
    virtual VOID ReleaseSound(FXSOUNDRESOURCE *pSoundData)=0;
    virtual FXDIALOG *GetDialog(CfxControl *pControl, const GUID *pObject)=0;
    virtual VOID ReleaseDialog(FXDIALOG *pDialog)=0;
    virtual VOID *GetObject(CfxControl *pControl, XGUID Object, BYTE Attribute=0)=0;
    virtual VOID ReleaseObject(VOID *pObject)=0;
    virtual FXBITMAPRESOURCE *GetBitmap(CfxControl *pControl, XBITMAP Bitmap)=0;
    virtual VOID ReleaseBitmap(FXBITMAPRESOURCE *pBitmapData)=0;
    virtual FXBINARYRESOURCE *GetBinary(CfxControl *pControl, XBINARY Binary)=0;
    virtual VOID ReleaseBinary(FXBINARYRESOURCE *pBinaryData)=0;
    virtual BOOL GetNextScreen(CfxControl *pControl, XGUID ScreenId, XGUID *pNextScreenId)=0;
    virtual FXGOTO *GetGoto(CfxControl *pControl, UINT Index)=0;
    virtual VOID ReleaseGoto(FXGOTO *pGoto)=0;
	virtual FXMOVINGMAP *GetMovingMap(CfxControl *pControl, UINT Index)=0;
	virtual VOID ReleaseMovingMap(FXMOVINGMAP *pMovingMap)=0;
};

//
// CfxControl
//
class CfxControl: public CfxPersistent
{
protected:
    UINT _version;
    BOOL _acceptChildren, _composite, _canScale;
    BOOL _visible, _enabled, _transparent;
    CfxControl *_parent;
    TList<CfxControl *> _controls;
    GUID _typeId;
    CHAR *_lockProperties, *_lockPropertiesDefault;
    CHAR *_lockPropertiesBasic; 
    CHAR _borderStyleDefault[4], _borderWidthDefault[4];
    COLOR _borderColor, _color;
    ControlDock _dock;
    UINT _borderWidth, _borderLineWidth;
    ControlBorderStyle _borderStyle;
    UINT _id;
    INT _left, _top, _width, _height;
    INT _oneSpace;
    XFONT _font;
    const CHAR *_fontDefault;
    UINT _tag;

    VOID InitControl(const GUID *pTypeId, const BOOL AcceptChildren = FALSE);
    VOID InitLockProperties(const CHAR *pDefault);
    VOID InitBorder(ControlBorderStyle BorderStyle, UINT BorderWidth);
    VOID InitFont(const CHAR *pFont);
    VOID InitFont(FXFONT &F, const CHAR *Value);
    VOID InitFont(UINT &F, const CHAR *Value);

    VOID InsertControl(CfxControl *pControl);
    VOID RemoveControls(INT Count);

    virtual UINT GetVersion();
    virtual VOID DefineControls(CfxFiler &F);
    virtual VOID DefineControlsState(CfxFiler &F);
public:
    CfxControl(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl();

	#ifdef _WIN32_WCE 
    void *operator new(size_t);
    void operator delete(void *);
    #endif
    
    VOID SetOwner(CfxPersistent *pOwner);
    VOID SetParent(CfxControl *pControl);

    VOID ClearControls();
    VOID Repaint();
    VOID SetPaintTimer(UINT Elapse);
    VOID SetTimer(UINT Elapse);
    VOID KillTimer();

    VOID GetBounds(FXRECT *pRect);
    VOID GetAbsoluteBounds(FXRECT *pRect);
    VOID GetClientBounds(FXRECT *pRect);
    UINT GetControlCount()                      { return _controls.GetCount(); }
    CfxControl *GetControl(UINT Index)          { return _controls.Get(Index); }
    BOOL IsLive();

    BOOL GetAcceptChildren()                      { return _acceptChildren;      }
    virtual CfxControl *GetChildParent()          { return this;                 }
    virtual BOOL GetIntegrated()                  { return _composite;           }

    BOOL GetComposite()                           { return _composite;           }
	VOID SetComposite(BOOL Value)                 { _composite = Value;          }
    BOOL GetVisible()                             { return _visible;             }
    VOID SetVisible(BOOL Value)                   { _visible = Value;            }
    BOOL GetIsVisible();
    BOOL GetEnabled();
    VOID SetEnabled(BOOL Value)                   { _enabled = Value;            }
    BOOL GetTransparent()                         { return _transparent;         }
    VOID SetTransparent(BOOL Value)               { _transparent = Value;        }
    UINT GetId()                                  { return _id;                  }
    VOID SetId(UINT Id)                           { _id = Id;                    }
    GUID *GetTypeId()                             { return &_typeId;             }
    CHAR **GetLockPropertiesPtr()                 { return &_lockProperties;     }
    CHAR **GetLockPropertiesBasicPtr()            { return &_lockPropertiesBasic; }
    ControlBorderStyle GetBorderStyle();
    VOID SetBorderStyle(ControlBorderStyle Value) { _borderStyle = Value;        }
    UINT GetBorderWidth()                         { return (UINT)_borderWidth;   }
    VOID SetBorderWidth(UINT Value)               { _borderWidth = Value;        }
    UINT GetBorderLineWidth()                     { return (UINT)_borderLineWidth; }
    VOID SetBorderLineWidth(UINT Value)           { _borderLineWidth = Value;   }
    COLOR GetColor()                              { return _color;              }
    VOID SetColor(COLOR Color)                    { _color = Color;             }
    COLOR GetBorderColor()                        { return _borderColor;        }
    VOID SetBorderColor(COLOR Color)              { _borderColor = Color;       }
    INT GetTop()                                  { return (INT)_top;           }
    INT GetLeft()                                 { return (INT)_left;          }
    UINT GetWidth()                               { return (UINT)_width;        }
    UINT GetHeight()                              { return (UINT)_height;       }
    VOID SetHeight(UINT Value)                    { _height = Value;            }
    VOID ResizeHeight(UINT Value);
    BOOL GetCanScale()                            { return _canScale;           }
    VOID SetCanScale(BOOL Value)                  { _canScale = Value;          }
    UINT GetClientWidth()                         { return (UINT)(_width - _borderWidth * 2);  }
    UINT GetClientHeight()                        { return (UINT)(_height - _borderWidth * 2); }
    ControlDock GetDock()                         { return _dock;               }
    VOID SetDock(ControlDock Value)               { _dock = Value;              }
    CfxControl *GetParent()                       { return _parent;             }
    VOID SetBorder(ControlBorderStyle Value, UINT Width, UINT LineWidth, COLOR Color);
    UINT GetTag()                                 { return _tag;                }
    VOID SetTag(UINT Value)                       { _tag = Value;               }
    
    VOID AssignFont(XFONT *pValue)                { memcpy(&_font, pValue, sizeof(XFONT)); }
    VOID SetFont(const CHAR *pValue)              { InitFont(pValue);                      }

    virtual XFONT *GetDefaultFont()               { return &_font;              }
    UINT GetDefaultFontHeight();

    virtual CfxResource *GetResource()            { return _parent ? _parent->GetResource() : NULL; }

    virtual VOID ResetState();

    virtual VOID DefineProperties(CfxFiler &F);
    virtual VOID DefineState(CfxFiler &F);
    virtual VOID DefinePropertiesUI(CfxFilerUI &F);
    virtual VOID DefineResources(CfxFilerResource &F);

    virtual VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    virtual VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    virtual CfxControl *EnumControls(UINT Index);
    virtual CfxControl *FindControlByType(const GUID *pTypeId);
    virtual CfxControl *FindControl(const UINT Id);

    // Events
    virtual VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    virtual VOID OnPenDown(INT X, INT Y);
    virtual VOID OnPenUp(INT X, INT Y);
    virtual VOID OnPenMove(INT X, INT Y);
    virtual VOID OnPenClick(INT X, INT Y);
    virtual VOID OnKeyDown(BYTE KeyCode, BOOL *pHandled);
    virtual VOID OnKeyUp(BYTE KeyCode, BOOL *pHandled);
    virtual VOID OnCloseDialog(BOOL *pHandled);
    virtual VOID OnDialogResult(GUID *pDialogId, VOID *pParams);
};

//
// CfxScreen
//
class CfxScreen: public CfxControl
{
    CHAR *_name;
public:
    CfxScreen(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxScreen();

    CHAR *GetName()                   { return _name;          }
    VOID SetName(CHAR *pName); 
    CHAR **GetNamePtr()               { return &_name;         }
    XFONT *GetFont()                  { return &_font;         }

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
};

extern CfxControl* Create_Control_Screen(CfxPersistent *pOwner, CfxControl *pParent);

//
// CfxDialog
//
class CfxDialog: public CfxControl
{
public:
    CfxDialog(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent) {} 
    virtual VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)=0;
    virtual VOID PostInit(CfxControl *pSender) {}
};

// 
// CfxControlRegistry
//
typedef CfxControl* (*Fn_Control_Factory)(CfxPersistent *pOwner, CfxControl *pParent);

struct RegisteredControl
{
    GUID *TypeId;
    CHAR *Name;
    BOOL UserVisible;
    UINT BitmapResourceId;
    UINT CategoryId;
    Fn_Control_Factory FnFactory;
};

class CfxControlRegistry 
{
protected:
    TList<RegisteredControl> _controls;
public:
    CfxControlRegistry();
    CfxControl *CreateRegisteredControl(const GUID *pControlTypeId, CfxPersistent *pOwner, CfxControl *pParent);
    VOID RegisterControl(const GUID *pControlTypeId, const CHAR *pControlName, Fn_Control_Factory FnFactory, BOOL UserVisible=TRUE, UINT BitmapResourceId=0, UINT CategoryId=0);
    BOOL EnumRegisteredControls(UINT Index, GUID *pControlTypeId, CHAR *pControlName=NULL, UINT *pBitmapResourceId=NULL, UINT *pCategoryId=NULL);
};

// Global registry
extern CfxControlRegistry ControlRegistry;

