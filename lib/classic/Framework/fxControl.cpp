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

#include "fxControl.h"
#include "fxUtils.h"
#include "fxApplication.h"

// Global control registry
CfxControlRegistry ControlRegistry;

//*************************************************************************************************
// CfxControl

CfxControl::CfxControl(CfxPersistent *pOwner, CfxControl *pParent): CfxPersistent(pOwner), _controls()
{
    InitControl(&GUID_0, TRUE);

    _version = GetVersion();
    _lockProperties = _lockPropertiesDefault = NULL;
    _lockPropertiesBasic = NULL;

    _composite = FALSE;
    _visible = _enabled = TRUE;
    _transparent = FALSE;

    _id = 0;
    _dock = dkNone;
    InitBorder(bsNone, 0);
    _borderLineWidth = 1;
    _color = 0xFFFFFF;
    _borderColor = 0;
    _parent = pParent;
    _canScale = TRUE;
    _oneSpace = 1;

    _top = _left = _width = _height = 0;

    InitFont(F_FONT_DEFAULT_M);

    if (pParent)
    {
        pParent->InsertControl(this);
    }
}

CfxControl::~CfxControl()
{
    MEM_FREE(_lockProperties);
    MEM_FREE(_lockPropertiesDefault);
    MEM_FREE(_lockPropertiesBasic);

    ClearControls();

    if (_parent)
    {
        _parent->_controls.Remove(this);
        _parent = NULL;
    }
}

#ifdef _WIN32_WCE 
void *CfxControl::operator new(size_t sz)
{
    return MEM_ALLOC(sz);
}

void CfxControl::operator delete(void *p)
{
    MEM_FREE(p);
}
#endif

VOID CfxControl::InitControl(const GUID *pTypeId, const BOOL AcceptChildren)
{
    InitGuid(&_typeId, pTypeId);
    _acceptChildren = AcceptChildren;
}

VOID CfxControl::InitLockProperties(const CHAR *pDefault)
{
#ifdef CLIENT_DLL
    if (pDefault)
    {
        _lockProperties = AllocString(pDefault);
        _lockPropertiesDefault = AllocString(pDefault);
    }
#endif
}

VOID CfxControl::InitBorder(ControlBorderStyle BorderStyle, UINT BorderWidth)
{
    _borderStyle = BorderStyle;
    _borderWidth = BorderWidth;
#ifdef CLIENT_DLL
    sprintf(_borderStyleDefault, "%d", (UINT)BorderStyle);
    sprintf(_borderWidthDefault, "%d", (UINT)BorderWidth);
#endif
}

VOID CfxControl::InitFont(const CHAR *pFont)
{
    InitFont(_font, pFont);
    _fontDefault = pFont;
}

VOID CfxControl::InitFont(FXFONT &F, const CHAR *Value)
{
    memset(F, 0, sizeof(FXFONT));
    memmove(F, Value, min(strlen(Value), sizeof(FXFONT)));
}

VOID CfxControl::InitFont(UINT &F, const CHAR *Value)
{
    F = 0;
}

BOOL CfxControl::IsLive()
{
#ifdef CLIENT_DLL
    if (GetApplication(this) != NULL)
    {
        return GetApplication(this)->GetLive();
    }
    else
    {
        return FALSE;
    }
#else
    return TRUE;     
#endif
}

UINT CfxControl::GetDefaultFontHeight()
{
    CfxCanvas *canvas = GetCanvas(this);
    if (canvas == NULL) 
    {
        return 0;
    }

    CfxResource *resource = GetResource();
    if (resource == NULL)
    {
        return 0;
    }

    FXFONTRESOURCE *font = resource->GetFont(this, *GetDefaultFont());
    if (font == NULL)
    {
        return 0;
    }

    UINT result = canvas->FontHeight(font);

    resource->ReleaseFont(font);
    
    return result;
}

VOID CfxControl::Repaint()
{
    if (IsLive() && GetEngine(this)->RequestPaint())
    {
        GetHost(this)->Paint();
    }
}

VOID CfxControl::ResizeHeight(UINT Value)
{ 
    if (_height == (INT)Value) return;

    _height = Value;
    
    CfxEngine *engine = GetEngine(this);
    if (engine)
    {
        engine->RequestRealign();
    }   
}

VOID CfxControl::SetPaintTimer(UINT Elapse)
{
    if (IsLive() && (Elapse > 0))
    {
        GetEngine(this)->SetPaintTimer(Elapse);
    }
}

VOID CfxControl::SetTimer(UINT Elapse)
{
    if (IsLive())
    {
        GetEngine(this)->SetTimer(this, Elapse);
    }
}

VOID CfxControl::KillTimer()
{
    if (IsLive())
    {
        GetEngine(this)->KillTimer(this);
    }
}

VOID CfxControl::SetOwner(CfxPersistent *pOwner)
{
    _owner = pOwner;

    for (UINT i=0; i<_controls.GetCount(); i++)
    {
        _controls.Get(i)->SetOwner(pOwner);
    }
}

VOID CfxControl::SetParent(CfxControl *pParent)
{
    _parent->_controls.Remove(this);
    pParent->_controls.Add(this);
    _parent = pParent;
}

VOID CfxControl::ClearControls()
{    
    if (IsLive())
    {
        GetEngine(this)->SetCaptureControl(NULL);
    }

    while (_controls.GetCount())
    {
        delete _controls.Get(0);
    }
    _controls.Clear();
}

VOID CfxControl::RemoveControls(INT Count)
{    
    for (INT i=0; i<Count; i++)
    {
        delete _controls.Get(0);
    }
}

VOID CfxControl::InsertControl(CfxControl *pControl)
{
    _controls.Insert(0, pControl);
}

UINT CfxControl::GetVersion()
{
    return 0;
}

VOID CfxControl::DefineControls(CfxFiler &F)
{
    if (F.DefineBegin("Controls"))
    {
        if (F.IsWriter())
        {
            for (INT i=_controls.GetCount()-1; i>=0; i--)
            {
                CfxControl *control = _controls.Get(i);
                if (!control->GetComposite())
                {
                    F.DefineBegin("Control");
                    F.DefineValue("Type", dtGuid, &control->_typeId);
                    control->DefineProperties(F);
                    F.DefineEnd();
                }
            }
            F.ListEnd();
        }
        else
        {
            FX_ASSERT(F.IsReader());

            INT oldControlIndex = (INT)_controls.GetCount() - 1;

            while (!F.ListEnd())
            {
                if (F.DefineBegin("Control"))
                {
                    GUID controlTypeId = {0};
                    F.DefineValue("Type", dtGuid, (VOID *)&controlTypeId);

                    CfxControl *control = NULL;

                    // Look for an existing control
                    if (oldControlIndex >= 0)
                    {
                        control = _controls.Get(oldControlIndex);
                        if (!CompareGuid(&control->_typeId, &controlTypeId))
                        {
                            // Mismatch ids, so remove controls between 0 and oldControlIndex
                            RemoveControls(oldControlIndex + 1);
                            oldControlIndex = 0;

                            // We have to create the control manually
                            control = NULL;
                        }
                    }

                    // Create it if we have to...
                    if (!control)
                    {
                        control = ControlRegistry.CreateRegisteredControl(&controlTypeId, _owner, this);
                    }

                    // Skip missing controls 
                    if (control) 
                    {
                        control->DefineProperties(F);
                    }

                    oldControlIndex--;

                    F.DefineEnd();
                }
                else break;
            }

            RemoveControls(oldControlIndex + 1);
        }

        F.DefineEnd();
    }
}

VOID CfxControl::DefineControlsState(CfxFiler &F)
{
    for (INT i=_controls.GetCount()-1; i>=0; i--)
    {
        _controls.Get(i)->DefineState(F);
    }
}

VOID CfxControl::GetBounds(FXRECT *pRect)
{
    pRect->Left = (INT)_left;
    pRect->Top = (INT)_top;
    pRect->Right = (INT)(_left + _width - 1);
    pRect->Bottom = (INT)(_top + _height -1);
}

VOID CfxControl::GetAbsoluteBounds(FXRECT *pRect)
{
    CfxControl *p = _parent;

    GetBounds(pRect);

    while (p)
    {
        OffsetRect(pRect, (INT)p->_left, (INT)p->_top);
        p = p->_parent;
    }
}

VOID CfxControl::GetClientBounds(FXRECT *pRect)
{
    pRect->Left = _left + _borderWidth;
    pRect->Top =  _top + _borderWidth;
    pRect->Right = _left + _width - _borderWidth - 1;
    pRect->Bottom = _top + _height - _borderWidth - 1;
}

BOOL CfxControl::GetIsVisible()
{
    BOOL visible = _visible;
    CfxControl *control = this;

    while (visible && control)
    {
        visible = visible && (control->_visible);
        control = control->_parent;
    }

    return visible;
}

BOOL CfxControl::GetEnabled()
{
    BOOL enabled = _enabled;
    CfxControl *control = this;

    while (enabled && control)
    {
        enabled = enabled && (control->_enabled);
        control = control->_parent;
    }

    return enabled;
}

ControlBorderStyle CfxControl::GetBorderStyle()
{
    if ((_borderStyle > bsSingle) && IsLive() &&
        (GetHost(this)->GetProfile()->BitDepth == 1))
    {
        return bsSingle;
    }
    else
    {       
        return _borderStyle;
    }
}    

VOID CfxControl::ResetState()
{
}

VOID CfxControl::DefineProperties(CfxFiler &F)
{
    if (F.IsWriter())
    {
        _version = GetVersion();
    }

    F.DefineValue("Version",         dtInt32,   &_version, F_ZERO);
    F.DefineValue("LockProperties",  dtPText,   &_lockProperties, _lockPropertiesDefault);
    F.DefineValue("LockPropertiesBasic", dtPText, &_lockPropertiesBasic);
    F.DefineValue("Id",              dtInt32,   &_id, F_ZERO);
    
    if (_dock != dkAction)
    {
        F.DefineValue("BorderWidth",     dtInt32,   &_borderWidth, _borderWidthDefault);
        F.DefineValue("BorderLineWidth", dtInt32,   &_borderLineWidth, F_ONE);
        F.DefineValue("BorderStyle",     dtInt8,    &_borderStyle, _borderStyleDefault);
        F.DefineValue("BorderColor",     dtColor,   &_borderColor, F_COLOR_BLACK);
        F.DefineValue("Color",           dtColor,   &_color, F_COLOR_WHITE);
        F.DefineValue("Composite",       dtBoolean, &_composite, F_FALSE);
        F.DefineValue("Visible",         dtBoolean, &_visible, F_TRUE);
        F.DefineValue("Transparent",     dtBoolean, &_transparent, F_FALSE);
        F.DefineValue("Align",           dtByte,    &_dock);
        F.DefineValue("Left",            dtInt32,   &_left);
        F.DefineValue("Top",             dtInt32,   &_top);
        F.DefineValue("Width",           dtInt32,   &_width);
        F.DefineValue("Height",          dtInt32,   &_height);

        F.DefineXFONT("Font", &_font, _fontDefault);

        if (_acceptChildren)
        {
            DefineControls(F);
        }
    }
}

VOID CfxControl::DefineState(CfxFiler &F)
{
    if (_acceptChildren)
    {
        DefineControlsState(F);
    }
}

VOID CfxControl::DefinePropertiesUI(CfxFilerUI &F)
{
}

VOID CfxControl::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxControl::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (_dock == dkAction)
    {
        pCanvas->DrawAction(pRect);
    }
}

VOID CfxControl::OnPenDown(INT X, INT Y)
{
}

VOID CfxControl::OnPenUp(INT X, INT Y)
{
}

VOID CfxControl::OnPenMove(INT X, INT Y)
{
}

VOID CfxControl::OnPenClick(INT X, INT Y)
{
}

VOID CfxControl::OnKeyDown(BYTE KeyCode, BOOL *pHandled)
{
}

VOID CfxControl::OnKeyUp(BYTE KeyCode, BOOL *pHandled)
{
}

VOID CfxControl::OnCloseDialog(BOOL *pCanClose)
{
}

VOID CfxControl::OnDialogResult(GUID *pDialogId, VOID *pParams)
{
}

VOID CfxControl::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    FX_ASSERT((Width >= 0) && (Height >= 0));
    _left   = Left;
    _top    = Top;
    _width  = Width;
    _height = Height;

    if (_dock == dkAction)
    {
        _width = ACTION_WIDTH;
        _height = ACTION_HEIGHT;
    }
}

VOID CfxControl::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    if ((_dock >= dkTop) && (_dock <= dkRight))
    {
        UINT nW = (_width * ScaleX) / 100;
        if ((nW == 0) && (_width > 0))
        {
            nW = 1;
        }
        _width = nW;

        UINT nH = (_height * ScaleY) / 100;
        if ((nH == 0) && (_height > 0))
        {
            nH = 1;
        }
        _height = nH;
    }
    
    UINT nBW = (_borderWidth * ScaleBorder) / 100;
    if ((nBW == 0) && (_borderWidth > 0))
    {
        nBW = 1;
    }
    _borderWidth = nBW;
    _borderLineWidth = max(1, (_borderLineWidth * ScaleBorder) / 100);
    _oneSpace = max(1, ScaleY / 100);
}

CfxControl *CfxControl::EnumControls(UINT Index)
{
    return (Index < _controls.GetCount()) ? _controls.Get(Index) : NULL;
}

CfxControl *CfxControl::FindControlByType(const GUID *pTypeId)
{
    if (CompareGuid(&_typeId, pTypeId))
    {
        return this;
    }

    for (UINT i=0; i<_controls.GetCount(); i++)
    {
        CfxControl *result = _controls.Get(i)->FindControlByType(pTypeId);
        if (result)
        {
            return result;
        }
    }

    return NULL;
}

CfxControl *CfxControl::FindControl(const UINT Id)
{
    if (_id == Id)
    {
        return this;
    }

    for (UINT i=0; i<_controls.GetCount(); i++)
    {
        CfxControl *result = _controls.Get(i)->FindControl(Id);
        if (result)
        {
            return result;
        }
    }

    return NULL;
}

VOID CfxControl::SetBorder(ControlBorderStyle Value, UINT Width, UINT LineWidth, COLOR Color)
{
    InitBorder(Value, Width);
    _borderLineWidth = LineWidth;
    _borderColor = Color;
}

//*************************************************************************************************
// CfxScreen

CfxControl* Create_Control_Screen(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxScreen(pOwner, pParent);
}

CfxScreen::CfxScreen(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    _id = 1;
    _name = NULL;
    InitLockProperties("Name");
    _lockPropertiesBasic = NULL; // AllocString("Name");
    InitFont(F_FONT_DEFAULT_SB);
}

CfxScreen::~CfxScreen()
{
    MEM_FREE(_name);
    _name = NULL;
}

VOID CfxScreen::SetName(CHAR *pName)
{
    MEM_FREE(_name);
    _name = NULL;

    if (pName)
    {
        _name = (CHAR *) MEM_ALLOC(strlen(pName) + 1);
        if (_name)
        {
            strcpy(_name, pName);
        }
    }
}

VOID CfxScreen::DefineProperties(CfxFiler &F)
{
    F.DefineValue("Name",            dtPText, &_name);
    F.DefineValue("LockProperties",  dtPText, &_lockProperties, _lockPropertiesDefault);
    F.DefineValue("LockPropertiesBasic", dtPText, &_lockPropertiesBasic);//, "Name");
    F.DefineXFONT("Font", &_font, _fontDefault);

    DefineControls(F);
}

VOID CfxScreen::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Name", dtPText, &_name);
#endif
}

VOID CfxScreen::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxScreen::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
}

//*************************************************************************************************
// CfxControlRegistry

CfxControlRegistry::CfxControlRegistry(): _controls()
{
}

CfxControl *CfxControlRegistry::CreateRegisteredControl(const GUID *pControlTypeId, CfxPersistent *pOwner, CfxControl *pParent)
{
    for (UINT i=0; i<_controls.GetCount(); i++)
    {
        GUID *value = _controls.Get(i).TypeId;
        if (CompareGuid(pControlTypeId, value))
        {
            return _controls.Get(i).FnFactory(pOwner, pParent);
        }
    }

    //BUGBUG
    return NULL;

    FX_ASSERT(FALSE);
    return NULL;
}

VOID CfxControlRegistry::RegisterControl(const GUID *pControlTypeId, const CHAR *pControlName, Fn_Control_Factory FnFactory, BOOL UserVisible, UINT BitmapResourceId, UINT CategoryId)
{
    RegisteredControl control = {0};

    control.TypeId = (GUID *)pControlTypeId;
    
    //strncpy(control.Name, pControlName, sizeof(control.Name)-1);
    //control.Name[sizeof(control.Name)-1] = 0;
    control.Name = (CHAR *)pControlName;
    
    control.UserVisible = UserVisible;
    control.BitmapResourceId = BitmapResourceId;
    control.CategoryId = CategoryId;
    control.FnFactory = FnFactory;

    _controls.Add(control);
}

BOOL CfxControlRegistry::EnumRegisteredControls(UINT Index, GUID *pControlTypeId, CHAR *pControlName, UINT *pBitmapResourceId, UINT *pCategoryId)
{
    if (Index >= _controls.GetCount()) 
    {
        return FALSE;
    }

    UINT i = 0, j = 0;
    while (i < _controls.GetCount()) 
    {
        RegisteredControl control = _controls.Get(i);
        if (control.UserVisible)
        {
            if (Index == j)
            {
                memmove(pControlTypeId, control.TypeId, sizeof(GUID));
                
                if (pControlName)
                {
                    strncpy(pControlName, control.Name, strlen(control.Name));
                    pControlName[strlen(control.Name)] = 0;
                }
                
                if (pBitmapResourceId)
                {
                    *pBitmapResourceId = control.BitmapResourceId;
                }

                if (pCategoryId)
                {
                    *pCategoryId = control.CategoryId;
                }

                return TRUE;
            }
            j++;
        }
        i++;
    }

    return FALSE;
}

