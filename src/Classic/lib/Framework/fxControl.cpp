#include "fxControl.h"
#include "fxUtils.h"
#include "fxApplication.h"

using namespace CyberTracker;

// Global control registry
CfxControlRegistry ControlRegistry;

//*************************************************************************************************
// CfxControl

CfxControl::CfxControl(CfxPersistent *pOwner, CfxControl *pParent): CfxPersistent(pOwner), _controls()
{
    InitControl(&GUID_0, true);

    _version = GetVersion();
    _lockProperties = _lockPropertiesDefault = nullptr;
    _lockPropertiesBasic = nullptr;

    _composite = false;
    _visible = _enabled = true;
    _transparent = false;

    _id = 0;
    _dock = ControlDock::None;
    InitBorder(ControlBorderStyle::None, 0);
    _borderLineWidth = 1;
    _color = 0xFFFFFF;
    _borderColor = 0;
    _parent = pParent;
    _canScale = true;
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
        _parent = nullptr;
    }
}

void CfxControl::InitControl(const GUID *pTypeId, const bool AcceptChildren)
{
    InitGuid(&_typeId, pTypeId);
    _acceptChildren = AcceptChildren;
}

void CfxControl::InitLockProperties(const char *pDefault)
{
}

void CfxControl::InitBorder(ControlBorderStyle BorderStyle, int BorderWidth)
{
    _borderStyle = BorderStyle;
    _borderWidth = BorderWidth;
}

void CfxControl::InitFont(const char *pFont)
{
    InitFont(_font, pFont);
    _fontDefault = pFont;
}

void CfxControl::InitFont(FXFONT &F, const char *Value)
{
    memset(F, 0, sizeof(FXFONT));
    memmove(F, Value, min(strlen(Value), sizeof(FXFONT)));
}

void CfxControl::InitFont(int &F, const char *Value)
{
    F = 0;
}

bool CfxControl::IsLive()
{
    return true;
}

int CfxControl::GetDefaultFontHeight()
{
    CfxCanvas *canvas = GetCanvas(this);
    if (canvas == nullptr) 
    {
        return 0;
    }

    CfxResource *resource = GetResource();
    if (resource == nullptr)
    {
        return 0;
    }

    FXFONTRESOURCE *font = resource->GetFont(this, *GetDefaultFont());
    if (font == nullptr)
    {
        return 0;
    }

    int result = canvas->FontHeight(font);

    resource->ReleaseFont(font);
    
    return result;
}

void CfxControl::Repaint()
{
    if (IsLive() && GetEngine(this)->RequestPaint())
    {
        GetHost(this)->Paint();
    }
}

void CfxControl::ResizeHeight(int Value)
{ 
    if (_height == (int)Value) return;

    _height = Value;
    
    CfxEngine *engine = GetEngine(this);
    if (engine)
    {
        engine->RequestRealign();
    }   
}

void CfxControl::SetPaintTimer(int Elapse)
{
    if (IsLive() && (Elapse > 0))
    {
        GetEngine(this)->SetPaintTimer(Elapse);
    }
}

void CfxControl::SetTimer(int Elapse)
{
    if (IsLive())
    {
        GetEngine(this)->SetTimer(this, Elapse);
    }
}

void CfxControl::KillTimer()
{
    if (IsLive())
    {
        GetEngine(this)->KillTimer(this);
    }
}

void CfxControl::SetOwner(CfxPersistent *pOwner)
{
    CfxPersistent::SetOwner(pOwner);

    for (int i=0; i<_controls.GetCount(); i++)
    {
        _controls.Get(i)->SetOwner(pOwner);
    }
}

void CfxControl::SetParent(CfxControl *pParent)
{
    _parent->_controls.Remove(this);
    pParent->_controls.Add(this);
    _parent = pParent;
}

void CfxControl::ClearControls()
{
    if (IsLive())
    {
        GetEngine(this)->SetCaptureControl(nullptr);
    }

    while (_controls.GetCount())
    {
        delete _controls.Get(0);
    }
    _controls.Clear();
}

void CfxControl::RemoveControls(int Count)
{    
    for (int i=0; i<Count; i++)
    {
        delete _controls.Get(0);
    }
}

void CfxControl::InsertControl(CfxControl *pControl)
{
    _controls.Insert(0, pControl);
}

int CfxControl::GetVersion()
{
    return 0;
}

void CfxControl::DefineControls(CfxFiler &F)
{
    if (F.DefineBegin("Controls"))
    {
        if (F.IsWriter())
        {
            for (int i=_controls.GetCount()-1; i>=0; i--)
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

            int oldControlIndex = (int)_controls.GetCount() - 1;

            while (!F.ListEnd())
            {
                if (F.DefineBegin("Control"))
                {
                    GUID controlTypeId = {0};
                    F.DefineValue("Type", dtGuid, (void *)&controlTypeId);

                    CfxControl *control = nullptr;

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
                            control = nullptr;
                        }
                    }

                    // Create it if we have to...
                    if (!control)
                    {
                        control = ControlRegistry.CreateRegisteredControl(&controlTypeId, GetOwner(), this);
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

void CfxControl::DefineControlsState(CfxFiler &F)
{
    for (int i=_controls.GetCount()-1; i>=0; i--)
    {
        _controls.Get(i)->DefineState(F);
    }
}

void CfxControl::GetBounds(FXRECT *pRect)
{
    pRect->Left = (int)_left;
    pRect->Top = (int)_top;
    pRect->Right = (int)(_left + _width - 1);
    pRect->Bottom = (int)(_top + _height -1);
}

void CfxControl::GetAbsoluteBounds(FXRECT *pRect)
{
    CfxControl *p = _parent;

    GetBounds(pRect);

    while (p)
    {
        OffsetRect(pRect, (int)p->_left, (int)p->_top);
        p = p->_parent;
    }
}

void CfxControl::GetClientBounds(FXRECT *pRect)
{
    pRect->Left = _left + _borderWidth;
    pRect->Top =  _top + _borderWidth;
    pRect->Right = _left + _width - _borderWidth - 1;
    pRect->Bottom = _top + _height - _borderWidth - 1;
}

bool CfxControl::GetIsVisible()
{
    bool visible = _visible;
    CfxControl *control = this;

    while (visible && control)
    {
        visible = visible && (control->_visible);
        control = control->_parent;
    }

    return visible;
}

bool CfxControl::GetEnabled()
{
    bool enabled = _enabled;
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
    if ((_borderStyle > ControlBorderStyle::Single) && IsLive() &&
        (GetHost(this)->GetProfile()->BitDepth == 1))
    {
        return ControlBorderStyle::Single;
    }
    else
    {       
        return _borderStyle;
    }
}    

void CfxControl::ResetState()
{
}

void CfxControl::DefineProperties(CfxFiler &F)
{
    if (F.IsWriter())
    {
        _version = GetVersion();
    }

    F.DefineValue("Version",         dtInt32,   &_version, F_ZERO);
    F.DefineValue("LockProperties",  dtPText,   &_lockProperties, _lockPropertiesDefault);
    F.DefineValue("LockPropertiesBasic", dtPText, &_lockPropertiesBasic);
    F.DefineValue("Id",              dtInt32,   &_id, F_ZERO);
    
    if (_dock != ControlDock::Action)
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

void CfxControl::DefineState(CfxFiler &F)
{
    if (_acceptChildren)
    {
        DefineControlsState(F);
    }
}

void CfxControl::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CfxControl::DefineResources(CfxFilerResource &F)
{
}


void CfxControl::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (_dock == ControlDock::Action)
    {
        pCanvas->DrawAction(pRect);
    }
}

void CfxControl::OnPenDown(int X, int Y)
{
}

void CfxControl::OnPenUp(int X, int Y)
{
}

void CfxControl::OnPenMove(int X, int Y)
{
}

void CfxControl::OnPenDrag(int StartX, int StartY, int X, int Y)
{
}

void CfxControl::OnPenClick(int X, int Y)
{
}

void CfxControl::OnKeyDown(BYTE KeyCode, bool *pHandled)
{
}

void CfxControl::OnKeyUp(BYTE KeyCode, bool *pHandled)
{
}

void CfxControl::OnCloseDialog(bool *pCanClose)
{
}

void CfxControl::OnDialogResult(GUID *pDialogId, void *pParams)
{
}

void CfxControl::SetBounds(int Left, int Top, int Width, int Height)
{
    //FX_ASSERT((Width >= 0) && (Height >= 0));
    _left   = Left;
    _top    = Top;
    _width  = Width;
    _height = Height;

    if (_dock == ControlDock::Action)
    {
        _width = ACTION_WIDTH;
        _height = ACTION_HEIGHT;
    }
}

void CfxControl::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    if ((_dock >= ControlDock::Top) && (_dock <= ControlDock::Right))
    {
        int nW = (_width * ScaleX) / 100;
        if ((nW == 0) && (_width > 0))
        {
            nW = 1;
        }
        _width = nW;

        int nH = (_height * ScaleY) / 100;
        if ((nH == 0) && (_height > 0))
        {
            nH = 1;
        }
        _height = nH;
    }
    
    int nBW = (_borderWidth * ScaleBorder) / 100;
    if ((nBW == 0) && (_borderWidth > 0))
    {
        nBW = 1;
    }
    _borderWidth = nBW;
    _borderLineWidth = max(1, (_borderLineWidth * ScaleBorder) / 100);
    _oneSpace = max(1, ScaleY / 100);
}

CfxControl *CfxControl::EnumControls(int Index)
{
    return (Index < _controls.GetCount()) ? _controls.Get(Index) : nullptr;
}

CfxControl *CfxControl::FindControlByType(const GUID *pTypeId)
{
    if (CompareGuid(&_typeId, pTypeId))
    {
        return this;
    }

    for (int i=0; i<_controls.GetCount(); i++)
    {
        CfxControl *result = _controls.Get(i)->FindControlByType(pTypeId);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

CfxControl *CfxControl::FindControl(const int Id)
{
    if (_id == Id)
    {
        return this;
    }

    for (int i=0; i<_controls.GetCount(); i++)
    {
        CfxControl *result = _controls.Get(i)->FindControl(Id);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

void CfxControl::SetBorder(ControlBorderStyle Value, int Width, int LineWidth, COLOR Color)
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
    _name = nullptr;
    InitLockProperties("Name");
    _lockPropertiesBasic = nullptr; // AllocString("Name");
    InitFont(F_FONT_DEFAULT_SB);
}

CfxScreen::~CfxScreen()
{
    MEM_FREE(_name);
    _name = nullptr;
}

void CfxScreen::SetName(char *pName)
{
    MEM_FREE(_name);
    _name = nullptr;

    if (pName)
    {
        _name = (char *) MEM_ALLOC(strlen(pName) + 1);
        if (_name)
        {
            strcpy(_name, pName);
        }
    }
}

void CfxScreen::DefineProperties(CfxFiler &F)
{
    F.DefineValue("Name",            dtPText, &_name);
    F.DefineValue("LockProperties",  dtPText, &_lockProperties, _lockPropertiesDefault);
    F.DefineValue("LockPropertiesBasic", dtPText, &_lockPropertiesBasic);//, "Name");
    F.DefineXFONT("Font", &_font, _fontDefault);

    DefineControls(F);
}

void CfxScreen::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CfxScreen::DefineResources(CfxFilerResource &F)
{
}

void CfxScreen::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
}


//*************************************************************************************************
// CfxControlRegistry

CfxControlRegistry::CfxControlRegistry(): _controls()
{
}

CfxControl *CfxControlRegistry::CreateRegisteredControl(const GUID *pControlTypeId, CfxPersistent *pOwner, CfxControl *pParent)
{
    for (int i=0; i<_controls.GetCount(); i++)
    {
        GUID *value = _controls.Get(i).TypeId;
        if (CompareGuid(pControlTypeId, value))
        {
            return _controls.Get(i).FnFactory(pOwner, pParent);
        }
    }

    //BUGBUG
    return nullptr;

    FX_ASSERT(false);
    return nullptr;
}

char *CfxControlRegistry::LookupControlName(const GUID* pControlTypeId)
{
    for (int i=0; i<_controls.GetCount(); i++)
    {
        GUID *value = _controls.Get(i).TypeId;
        if (CompareGuid(pControlTypeId, value))
        {
            return _controls.Get(i).Name;
        }
    }

    return nullptr;
}

void CfxControlRegistry::RegisterControl(const GUID *pControlTypeId, const char *pControlName, Fn_Control_Factory FnFactory, bool UserVisible, int BitmapResourceId, int CategoryId)
{
    RegisteredControl control = {0};

    control.TypeId = (GUID *)pControlTypeId;
    
    //strncpy(control.Name, pControlName, sizeof(control.Name)-1);
    //control.Name[sizeof(control.Name)-1] = 0;
    control.Name = (char *)pControlName;
    
    control.UserVisible = UserVisible;
    control.BitmapResourceId = BitmapResourceId;
    control.CategoryId = CategoryId;
    control.FnFactory = FnFactory;

    _controls.Add(control);
}

bool CfxControlRegistry::EnumRegisteredControls(int Index, GUID *pControlTypeId, char *pControlName, int *pBitmapResourceId, int *pCategoryId)
{
    if (Index >= _controls.GetCount()) 
    {
        return false;
    }

    int i = 0, j = 0;
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

                return true;
            }
            j++;
        }
        i++;
    }

    return false;
}

//*************************************************************************************************
// CfxResource

CfxResource::CfxResource(CfxPersistent *pOwner, const char *pFileName): CfxPersistent(pOwner), _fileMap()
{
    _header = nullptr;

    if (FxMapFile(pFileName, &_fileMap))
    {
        _header = (FXRESOURCEHEADER *)_fileMap.BasePointer;
    }
}

CfxResource::~CfxResource()
{
    FxUnmapFile(&_fileMap);
	_header = nullptr;
}

bool CfxResource::Verify()
{
    return (_header != nullptr) && (_header->Magic == MAGIC_RESOURCE);
}

FXPROFILE *CfxResource::GetProfile(CfxControl *pControl, int Index)
{
    if (Index < _header->ProfileCount)
    {
        return (FXPROFILE *)((UINT64)_fileMap.BasePointer + _header->FirstProfile + Index * (sizeof(FXPROFILE) + 4));
    }
    else
    {
        return nullptr;
    }
}

void CfxResource::ReleaseProfile(FXPROFILE *pProfile)
{
}

FXFONTRESOURCE *CfxResource::GetFont(CfxControl *pControl, XFONT Font)
{
    return (Font == 0) ? nullptr : (FXFONTRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Font);
}

void CfxResource::ReleaseFont(FXFONTRESOURCE *pFontData)
{
}

FXBITMAPRESOURCE *CfxResource::GetBitmap(CfxControl *pControl, XBITMAP Bitmap)
{
    return (Bitmap == 0) ? nullptr : (FXBITMAPRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Bitmap);
}

void CfxResource::ReleaseBitmap(FXBITMAPRESOURCE *pBitmapData)
{
}

FXSOUNDRESOURCE *CfxResource::GetSound(CfxControl *pControl, XSOUND Sound)
{
    return (Sound == 0) ? nullptr : (FXSOUNDRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Sound);
}

void CfxResource::ReleaseSound(FXSOUNDRESOURCE *pSoundData)
{
}

FXDIALOG *CfxResource::GetDialog(CfxControl *pControl, const GUID *pObject)
{
    UINT64 offset = (UINT64)_fileMap.BasePointer + _header->FirstDialog;

    int i = 0;
    while (i < _header->DialogCount)
    {
        FXDIALOG *data = (FXDIALOG *)(offset + sizeof(FXOBJECTHEADER));
        FX_ASSERT(data->Magic == MAGIC_SCREEN);

        if (CompareGuid(pObject, &data->Guid))
        {
            return data;
        }
        i++;

        int resourceSize = *(int *)(offset - sizeof(int));
        offset = PadOffset(offset + resourceSize + sizeof(int));
    }

    return nullptr;
}

void CfxResource::ReleaseDialog(FXDIALOG *pDialog)
{
}

void *CfxResource::GetObject(CfxControl *pControl, XGUID Object, BYTE Attribute)
{
    FX_ASSERT(Verify());

    if (Object == 0)
    {
        return nullptr;
    }

    // Tracks the resource file offset
    UINT64 offset = (UINT64)_fileMap.BasePointer + Object;

    while (1)
    {
        FXOBJECTHEADER *header = (FXOBJECTHEADER *)offset;

        // Done searching: resource not found
        if (header->Magic != MAGIC_OBJECT)
        {
            break;
        }

        // Check for a match
        if (header->Attribute == Attribute)
        {
            return (void *)(offset + sizeof(FXOBJECTHEADER));
        }

        if (header->Attribute > Attribute)
        {
            break;
        }

        // No match, so skip ahead
        offset = PadOffset(offset + sizeof(FXOBJECTHEADER) + header->Size);
    }

    // Failed
    return nullptr;
}

void CfxResource::ReleaseObject(void *pObject)
{
}

FXBINARYRESOURCE *CfxResource::GetBinary(CfxControl *pControl, XBINARY Binary)
{
    return (Binary == 0) ? nullptr : (FXBINARYRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Binary);
}

void CfxResource::ReleaseBinary(FXBINARYRESOURCE *pBinaryData)
{
}

FXGOTO *CfxResource::GetGoto(CfxControl *pControl, int Index)
{
    return (_header->GotosOffset == 0) ? nullptr : (FXGOTO *)((UINT64)_fileMap.BasePointer + (UINT64)_header->GotosOffset + Index * sizeof(FXGOTO));
}

void CfxResource::ReleaseGoto(FXGOTO *pGoto)
{
}

FXMOVINGMAP *CfxResource::GetMovingMap(CfxControl *pControl, int Index)
{
    return (_header->MovingMapsOffset == 0) ? nullptr : (FXMOVINGMAP *)((UINT64)_fileMap.BasePointer + (UINT64)_header->MovingMapsOffset + Index * sizeof(FXMOVINGMAP));
}

void CfxResource::ReleaseMovingMap(FXMOVINGMAP *pMovingMap)
{
}

bool CfxResource::GetNextScreen(CfxControl *pControl, XGUID ScreenId, XGUID *pNextScreenId)
{
	int i = 0;
    int *currId = (int *)((UINT64)_fileMap.BasePointer + _header->NextScreenList);

	*pNextScreenId = 0;

	while (i < _header->NextScreenCount)
	{
		if (*currId == ScreenId)
		{
			*pNextScreenId = *(currId + 1);

			return true;
		}

		i += 2;
		currId += 2;
	}

	return false;
}

