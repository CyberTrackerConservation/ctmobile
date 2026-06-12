#include "fxApplication.h"
#include "fxUtils.h"
#include "ctElementImageGrid.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementImageGrid1(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementImageGrid1(pOwner, pParent);
}

CfxControl *Create_Control_ElementImageGrid2(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementImageGrid2(pOwner, pParent);
}

CctControl_ElementImageGrid::CctControl_ElementImageGrid(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitLockProperties("");
    InitBorder(ControlBorderStyle::Single, 1);

    _transparentImage = false;
    _bitmap = 0;

    _rows = 4;
    _columns = 4;
    _selectedX = -1;
    _selectedY = -1;

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementImageGrid::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementImageGrid::OnSessionNext);
}

CctControl_ElementImageGrid::~CctControl_ElementImageGrid()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
    freeX(_bitmap);
}

void CctControl_ElementImageGrid::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if ((_selectedX == -1) || (_selectedY == -1) || !IsValidItem(_selectedX, _selectedY))
    {
        *((bool *)pParams) = false;
    }
}

void CctControl_ElementImageGrid::OnSessionNext(CfxControl *pSender, void *pParams)
{
    DoSessionNext(pSender);
}

void CctControl_ElementImageGrid::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementImageGrid::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Columns", dtInt32, &_columns, "4");
    F.DefineValue("Rows", dtInt32, &_rows, "4");
    F.DefineValue("TransparentImage", dtBoolean, &_transparentImage, F_FALSE);
    F.DefineXBITMAP("Bitmap", &_bitmap);
}

void CctControl_ElementImageGrid::DefineState(CfxFiler &F)
{
    CfxControl::DefineState(F);
    F.DefineValue("SelectedX", dtInt32, &_selectedX);
    F.DefineValue("SelectedY", dtInt32, &_selectedY);
}

void CctControl_ElementImageGrid::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementImageGrid::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    FXRECT selectedRect;
    bool selected = GetItemRect(_selectedX, _selectedY, &selectedRect);
    OffsetRect(&selectedRect, pRect->Left, pRect->Top);

    CfxResource *resource = GetResource(); 
    FXBITMAPRESOURCE *bitmap = resource->GetBitmap(this, _bitmap);
    if (bitmap)
    {
        FXRECT rect;
        GetBitmapRect(bitmap, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1, true, true, false, &rect);
        pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, false, _transparentImage);

        if (selected)
        {
            FXRECT oldClipRect = pCanvas->State.ClipRect;
            pCanvas->State.ClipRect = selectedRect;
            pCanvas->State.BrushColor = 0;
            pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, true, _transparentImage);
            pCanvas->State.ClipRect = oldClipRect;
        }
        resource->ReleaseBitmap(bitmap);
    }
    else
    {
        if (selected)
        {
            pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
            pCanvas->FillRect(&selectedRect);
        }
    }

    int i, j;
    for (j=0; j<_rows; j++)
    for (i=0; i<_columns; i++)
    {
        if (IsValidItem(i, j)) continue;

        FXRECT rect;
        if (GetItemRect(i, j, &rect))
        {
            OffsetRect(&rect, pRect->Left, pRect->Top);
            pCanvas->MoveTo(rect.Left, rect.Top);
            pCanvas->LineTo(rect.Right, rect.Bottom);
            pCanvas->MoveTo(rect.Left, rect.Bottom);
            pCanvas->LineTo(rect.Right, rect.Top);
        }
    }

    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.LineWidth = _borderLineWidth;
    int clientW = GetClientWidth();
    for (i=1; i<_columns; i++)
    {
        int x = pRect->Left + (i * clientW) / _columns;
        pCanvas->MoveTo(x, pRect->Top);
        pCanvas->LineTo(x, pRect->Bottom);
    }

    int clientH = GetClientHeight();
    for (i=1; i<_rows; i++)
    {
        int y = pRect->Top + (i * clientH) / _rows;
        pCanvas->MoveTo(pRect->Left, y);
        pCanvas->LineTo(pRect->Right, y);
    }
}


bool CctControl_ElementImageGrid::GetItemRect(int X, int Y, FXRECT *pRect)
{
    int clientW = GetClientWidth();
    int clientH = GetClientHeight();

    if ((X != -1) && (Y != -1))
    {   
        pRect->Left = (X * clientW) / _columns;
        pRect->Top = (Y * clientH) / _rows;

        pRect->Right = ((X+1) * clientW) / _columns;
        pRect->Bottom = ((Y+1) * clientH) / _rows;

        return true;
    }
    else 
    {
        return false;
    }
}

void CctControl_ElementImageGrid::OnPenDown(int X, int Y)
{
    int nx = (X * _columns) / GetClientWidth();
    int ny = (Y * _rows) / GetClientHeight();

    if (IsValidItem(nx, ny))
    {
        _selectedX = nx;
        _selectedY = ny;
    }
}

void CctControl_ElementImageGrid::OnPenUp(int X, int Y)
{
    Repaint();
}

//*************************************************************************************************

CctControl_ElementImageGrid1::CctControl_ElementImageGrid1(CfxPersistent *pOwner, CfxControl *pParent): CctControl_ElementImageGrid(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTIMAGEGRID1, false);
    InitLockProperties("Columns;Elements;Image;Rows");
}

bool CctControl_ElementImageGrid1::IsValidItem(int X, int Y)
{
    if ((X == -1) || (Y == -1))
    {
        return false;
    }
    else
    {
        return (Y * _columns + X < _elements.GetCount());
    }
}

void CctControl_ElementImageGrid1::DoSessionNext(CfxControl *pSender)
{
    CctAction_MergeAttribute action(this);
    action.SetupAdd(_id, _elements.Get(_selectedY * _columns + _selectedX));
    ((CctSession *)pSender)->ExecuteAction(&action);
}

void CctControl_ElementImageGrid1::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementImageGrid1::DefineProperties(CfxFiler &F)
{
    CctControl_ElementImageGrid::DefineProperties(F);
    F.DefineXOBJECTLIST("Elements", &_elements);
}

void CctControl_ElementImageGrid1::DefinePropertiesUI(CfxFilerUI &F) 
{
}

//*************************************************************************************************

CctControl_ElementImageGrid2::CctControl_ElementImageGrid2(CfxPersistent *pOwner, CfxControl *pParent): CctControl_ElementImageGrid(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTIMAGEGRID2, false);
    InitLockProperties("Columns;Element;Format;Image;Rows");

    _format = ELEMENTIMAGEGRIDFORMAT::Excel;
    InitXGuid(&_elementId);
}

bool CctControl_ElementImageGrid2::IsValidItem(int X, int Y)
{
    return true;
}

void CctControl_ElementImageGrid2::DoSessionNext(CfxControl *pSender)
{
    // Add the Attribute for this Element
    if (IsNullXGuid(&_elementId)) return;

    int index = _selectedY * _columns + _selectedX;
    CctAction_MergeAttribute action(this);

    switch (_format) 
    {
    case ELEMENTIMAGEGRIDFORMAT::Excel:
        {
            char valueX[16] = "";
            char valueY[8] = "";
            int v = _selectedX / 26;
            
            if (v > 0)
            {
                char letter[2] = {(char)('A' + (BYTE)v), '\0'};
                strcat(valueX, letter);
            }

            {
                v = _selectedX - v * 26;
                char letter[2] = {(char)('A' + (BYTE)v), '\0'};
                strcat(valueX, letter);
            }
            
            itoa(_selectedY+1, valueY, 10);

            int len = strlen(valueX) + strlen(valueY) + 1;
            char *pValue = (char *)MEM_ALLOC(len);
            memset(pValue, 0, len);
            strcpy(pValue, valueX);
            strcat(pValue, valueY);
            action.SetupAdd(_id, _elementId, dtPText, &pValue);
            MEM_FREE(pValue);
        }
        break;

    case ELEMENTIMAGEGRIDFORMAT::Number:
        {
            int value = _selectedY * _rows + _selectedX + 1;
            action.SetupAdd(_id, _elementId, dtInt32, &value);
        }
        break;

    default:
        FX_ASSERT(false);
    }

    ((CctSession *)pSender)->ExecuteAction(&action);
}

void CctControl_ElementImageGrid2::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementImageGrid2::DefineProperties(CfxFiler &F)
{
    CctControl_ElementImageGrid::DefineProperties(F);
    F.DefineValue("Format", dtInt8, &_format, F_ZERO);
    F.DefineXOBJECT("Element", &_elementId);
}

void CctControl_ElementImageGrid2::DefinePropertiesUI(CfxFilerUI &F) 
{
}

