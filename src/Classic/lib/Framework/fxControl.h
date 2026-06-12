#pragma once
#include "pch.h"
#include "fxClasses.h"
#include "fxLists.h"
#include "fxCanvas.h"

const int ACTION_WIDTH  = 24;
const int ACTION_HEIGHT = 24;

enum class ControlDock : int8_t {None=0, Top, Bottom, Left, Right, Fill, Action};
enum class ControlBorderStyle : int8_t {None=0, Single, Round1, Round2};

class CfxControl;

//
// CfxResource
//
class CfxResource: public CfxPersistent
{
protected:
    FXRESOURCEHEADER *_header = nullptr;
    FXFILEMAP _fileMap;
    UINT64 PadOffset(UINT64 Value) { return (Value + 3) & ~3; }
public:
    CfxResource(CfxPersistent *pOwner, const char *pFileName);
    ~CfxResource();

    bool Verify();

    FXRESOURCEHEADER *GetHeader(CfxControl *pControl) { return _header; }

    FXPROFILE *GetProfile(CfxControl *pControl, int Index);
    void ReleaseProfile(FXPROFILE *pProfile);
    FXFONTRESOURCE *GetFont(CfxControl *pControl, XFONT Font);
    void ReleaseFont(FXFONTRESOURCE *pFontData);
    FXSOUNDRESOURCE *GetSound(CfxControl *pControl, XSOUND Sound);
    void ReleaseSound(FXSOUNDRESOURCE *pSoundData);
    FXDIALOG *GetDialog(CfxControl *pControl, const GUID *pObject);
    void ReleaseDialog(FXDIALOG *pDialog);
    void *GetObject(CfxControl *pControl, XGUID Object, BYTE Attribute=0);
    void ReleaseObject(void *pObject);
    FXBITMAPRESOURCE *GetBitmap(CfxControl *pControl, XBITMAP Bitmap);
    void ReleaseBitmap(FXBITMAPRESOURCE *pBitmapData);
    FXBINARYRESOURCE *GetBinary(CfxControl *pControl, XBINARY Binary);
    void ReleaseBinary(FXBINARYRESOURCE *pBinaryData);
    bool GetNextScreen(CfxControl *pControl, XGUID ScreenId, XGUID *pNextScreenId);
    FXGOTO *GetGoto(CfxControl *pControl, int Index);
    void ReleaseGoto(FXGOTO *pGoto);
	FXMOVINGMAP *GetMovingMap(CfxControl *pControl, int Index);
	void ReleaseMovingMap(FXMOVINGMAP *pMovingMap);
};

//
// CfxControl
//
class CfxControl: public CfxPersistent
{
protected:
    int _version = 0;
    bool _acceptChildren = false, _composite = false, _canScale = false, _supportsDrag = false;
    bool _visible = false, _enabled = false, _transparent = false;
    CfxControl *_parent = nullptr;
    TList<CfxControl *> _controls;
    GUID _typeId;
    char *_lockProperties = nullptr, *_lockPropertiesDefault = nullptr;
    char *_lockPropertiesBasic = nullptr;
    char _borderStyleDefault[4] = {}, _borderWidthDefault[4] = {};
    COLOR _borderColor = 0, _color = 0;
    ControlDock _dock = ControlDock::None;
    int _borderWidth = 0, _borderLineWidth = 0;
    ControlBorderStyle _borderStyle = ControlBorderStyle::None;
    int _id = 0;
    int _left = 0, _top = 0, _width = 0, _height = 0;
    int _oneSpace = 0;
    XFONT _font = 0;
    const char *_fontDefault = nullptr;
    int _tag = 0;

    void InitControl(const GUID *pTypeId, const bool AcceptChildren = false);
    void InitLockProperties(const char *pDefault);
    void InitBorder(ControlBorderStyle BorderStyle, int BorderWidth);
    void InitFont(const char *pFont);
    void InitFont(FXFONT &F, const char *Value);
    void InitFont(int &F, const char *Value);

    void InsertControl(CfxControl *pControl);
    void RemoveControls(int Count);

    virtual int GetVersion();
    virtual void DefineControls(CfxFiler &F);
    virtual void DefineControlsState(CfxFiler &F);
public:
    CfxControl(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl();

    void SetOwner(CfxPersistent *pOwner);
    void SetParent(CfxControl *pControl);

    void ClearControls();
    void Repaint();
    void SetPaintTimer(int Elapse);
    void SetTimer(int Elapse);
    void KillTimer();

    void GetBounds(FXRECT *pRect);
    void GetAbsoluteBounds(FXRECT *pRect);
    void GetClientBounds(FXRECT *pRect);
    int GetControlCount()                      { return _controls.GetCount(); }
    CfxControl *GetControl(int Index)          { return _controls.Get(Index); }
    bool IsLive();

    bool GetSupportsDrag()                        { return _supportsDrag;        }
    bool GetAcceptChildren()                      { return _acceptChildren;      }
    virtual CfxControl *GetChildParent()          { return this;                 }
    virtual bool GetIntegrated()                  { return _composite;           }

    bool GetComposite()                           { return _composite;           }
	void SetComposite(bool Value)                 { _composite = Value;          }
    bool GetVisible()                             { return _visible;             }
    void SetVisible(bool Value)                   { _visible = Value;            }
    bool GetIsVisible();
    bool GetEnabled();
    void SetEnabled(bool Value)                   { _enabled = Value;            }
    bool GetTransparent()                         { return _transparent;         }
    void SetTransparent(bool Value)               { _transparent = Value;        }
    int GetId()                                  { return _id;                  }
    void SetId(int Id)                           { _id = Id;                    }
    GUID *GetTypeId()                             { return &_typeId;             }
    char **GetLockPropertiesPtr()                 { return &_lockProperties;     }
    char **GetLockPropertiesBasicPtr()            { return &_lockPropertiesBasic; }
    ControlBorderStyle GetBorderStyle();
    void SetBorderStyle(ControlBorderStyle Value) { _borderStyle = Value;        }
    int GetBorderWidth()                         { return (int)_borderWidth;   }
    void SetBorderWidth(int Value)               { _borderWidth = Value;        }
    int GetBorderLineWidth()                     { return (int)_borderLineWidth; }
    void SetBorderLineWidth(int Value)           { _borderLineWidth = Value;   }
    COLOR GetColor()                              { return _color;              }
    void SetColor(COLOR Color)                    { _color = Color;             }
    COLOR GetBorderColor()                        { return _borderColor;        }
    void SetBorderColor(COLOR Color)              { _borderColor = Color;       }
    int GetTop()                                  { return (int)_top;           }
    int GetLeft()                                 { return (int)_left;          }
    int GetWidth()                               { return (int)_width;        }
    int GetHeight()                              { return (int)_height;       }
    void SetHeight(int Value)                    { _height = Value;            }
    void ResizeHeight(int Value);
    bool GetCanScale()                            { return _canScale;           }
    void SetCanScale(bool Value)                  { _canScale = Value;          }
    int GetClientWidth()                         { return (int)(_width - _borderWidth * 2);  }
    int GetClientHeight()                        { return (int)(_height - _borderWidth * 2); }
    ControlDock GetDock()                         { return _dock;               }
    void SetDock(ControlDock Value)               { _dock = Value;              }
    CfxControl *GetParent()                       { return _parent;             }
    void SetBorder(ControlBorderStyle Value, int Width, int LineWidth, COLOR Color);
    int GetTag()                                 { return _tag;                }
    void SetTag(int Value)                       { _tag = Value;               }
    
    void AssignFont(XFONT *pValue)                { memcpy(&_font, pValue, sizeof(XFONT)); }
    void SetFont(const char *pValue)              { InitFont(pValue);                      }

    virtual XFONT *GetDefaultFont()               { return &_font;              }
    int GetDefaultFontHeight();

    virtual CfxResource *GetResource()            { return _parent ? _parent->GetResource() : nullptr; }

    virtual void ResetState();

    virtual void DefineProperties(CfxFiler &F);
    virtual void DefineState(CfxFiler &F);
    virtual void DefinePropertiesUI(CfxFilerUI &F);
    virtual void DefineResources(CfxFilerResource &F);
    virtual void SetBounds(int Left, int Top, int Width, int Height);
    virtual void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize);
    virtual CfxControl *EnumControls(int Index);
    virtual CfxControl *FindControlByType(const GUID *pTypeId);
    virtual CfxControl *FindControl(const int Id);

    // Events
    virtual void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    virtual void OnPenDown(int X, int Y);
    virtual void OnPenUp(int X, int Y);
    virtual void OnPenMove(int X, int Y);
    virtual void OnPenDrag(int StartX, int StartY, int X, int Y);
    virtual void OnPenClick(int X, int Y);
    virtual void OnKeyDown(BYTE KeyCode, bool *pHandled);
    virtual void OnKeyUp(BYTE KeyCode, bool *pHandled);
    virtual void OnCloseDialog(bool *pHandled);
    virtual void OnDialogResult(GUID *pDialogId, void *pParams);
};

//
// CfxScreen
//
class CfxScreen: public CfxControl
{
    char *_name = nullptr;
public:
    CfxScreen(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxScreen();

    char *GetName()                   { return _name;          }
    void SetName(char *pName); 
    char **GetNamePtr()               { return &_name;         }
    XFONT *GetFont()                  { return &_font;         }

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
};

extern CfxControl* Create_Control_Screen(CfxPersistent *pOwner, CfxControl *pParent);

//
// CfxDialog
//
class CfxDialog: public CfxControl
{
public:
    CfxDialog(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent) {} 
    virtual void Init(CfxControl *pSender, void *pParam, int ParamSize)=0;
    virtual void PostInit(CfxControl *pSender) {}
};

// 
// CfxControlRegistry
//
typedef CfxControl* (*Fn_Control_Factory)(CfxPersistent *pOwner, CfxControl *pParent);

struct RegisteredControl
{
    GUID *TypeId;
    char *Name;
    bool UserVisible;
    int BitmapResourceId;
    int CategoryId;
    Fn_Control_Factory FnFactory;
};

class CfxControlRegistry 
{
protected:
    TList<RegisteredControl> _controls;
public:
    CfxControlRegistry();
    CfxControl *CreateRegisteredControl(const GUID *pControlTypeId, CfxPersistent *pOwner, CfxControl *pParent);
    char *LookupControlName(const GUID* pControlTypeId);
    void RegisterControl(const GUID *pControlTypeId, const char *pControlName, Fn_Control_Factory FnFactory, bool UserVisible=true, int BitmapResourceId=0, int CategoryId=0);
    bool EnumRegisteredControls(int Index, GUID *pControlTypeId, char *pControlName=nullptr, int *pBitmapResourceId=nullptr, int *pCategoryId=nullptr);
};

// Global registry
extern CfxControlRegistry ControlRegistry;

