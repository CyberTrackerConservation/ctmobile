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
    InitBorder(bsSingle, 1);

    _transparentImage = FALSE;
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

VOID CctControl_ElementImageGrid::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if ((_selectedX == -1) || (_selectedY == -1) || !IsValidItem(_selectedX, _selectedY))
    {
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementImageGrid::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    DoSessionNext(pSender);
}

VOID CctControl_ElementImageGrid::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineBitmap(_bitmap);
#endif
}

VOID CctControl_ElementImageGrid::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Columns", dtInt32, &_columns, "4");
    F.DefineValue("Rows", dtInt32, &_rows, "4");
    F.DefineValue("TransparentImage", dtBoolean, &_transparentImage, F_FALSE);
    F.DefineXBITMAP("Bitmap", &_bitmap);
}

VOID CctControl_ElementImageGrid::DefineState(CfxFiler &F)
{
    CfxControl::DefineState(F);
    F.DefineValue("SelectedX", dtInt32, &_selectedX);
    F.DefineValue("SelectedY", dtInt32, &_selectedY);
}

VOID CctControl_ElementImageGrid::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columns,     "MIN(2);MAX(64)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Image",        dtPGraphic,  &_bitmap,      "BITDEPTH(16);HINTWIDTH(2048);HINTHEIGHT(2048)");
    F.DefineValue("Rows",         dtInt32,    &_rows,        "MIN(2);MAX(64)");
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent Image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_ElementImageGrid::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    FXRECT selectedRect;
    BOOL selected = GetItemRect(_selectedX, _selectedY, &selectedRect);
    OffsetRect(&selectedRect, pRect->Left, pRect->Top);

    CfxResource *resource = GetResource(); 
    FXBITMAPRESOURCE *bitmap = resource->GetBitmap(this, _bitmap);
    if (bitmap)
    {
        FXRECT rect;
        GetBitmapRect(bitmap, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1, TRUE, TRUE, FALSE, &rect);
        pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, FALSE, _transparentImage);

        if (selected)
        {
            FXRECT oldClipRect = pCanvas->State.ClipRect;
            pCanvas->State.ClipRect = selectedRect;
            pCanvas->State.BrushColor = 0;
            pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, TRUE, _transparentImage);
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

    UINT i, j;
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
    UINT clientW = GetClientWidth();
    for (i=1; i<_columns; i++)
    {
        UINT x = pRect->Left + (i * clientW) / _columns;
        pCanvas->MoveTo(x, pRect->Top);
        pCanvas->LineTo(x, pRect->Bottom);
    }

    UINT clientH = GetClientHeight();
    for (i=1; i<_rows; i++)
    {
        UINT y = pRect->Top + (i * clientH) / _rows;
        pCanvas->MoveTo(pRect->Left, y);
        pCanvas->LineTo(pRect->Right, y);
    }
}


BOOL CctControl_ElementImageGrid::GetItemRect(INT X, INT Y, FXRECT *pRect)
{
    UINT clientW = GetClientWidth();
    UINT clientH = GetClientHeight();

    if ((X != -1) && (Y != -1))
    {   
        pRect->Left = (X * clientW) / _columns;
        pRect->Top = (Y * clientH) / _rows;

        pRect->Right = ((X+1) * clientW) / _columns;
        pRect->Bottom = ((Y+1) * clientH) / _rows;

        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}

VOID CctControl_ElementImageGrid::OnPenDown(INT X, INT Y)
{
    INT nx = (X * _columns) / GetClientWidth();
    INT ny = (Y * _rows) / GetClientHeight();

    if (IsValidItem(nx, ny))
    {
        _selectedX = nx;
        _selectedY = ny;
    }
}

VOID CctControl_ElementImageGrid::OnPenUp(INT X, INT Y)
{
    Repaint();
}

//*************************************************************************************************

CctControl_ElementImageGrid1::CctControl_ElementImageGrid1(CfxPersistent *pOwner, CfxControl *pParent): CctControl_ElementImageGrid(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTIMAGEGRID1, FALSE);
    InitLockProperties("Columns;Elements;Image;Rows");
}

BOOL CctControl_ElementImageGrid1::IsValidItem(INT X, INT Y)
{
    if ((X == -1) || (Y == -1))
    {
        return FALSE;
    }
    else
    {
        return (Y * _columns + X < _elements.GetCount());
    }
}

VOID CctControl_ElementImageGrid1::DoSessionNext(CfxControl *pSender)
{
    CctAction_MergeAttribute action(this);
    action.SetupAdd(_id, _elements.Get(_selectedY * _columns + _selectedX));
    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_ElementImageGrid1::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CctControl_ElementImageGrid::DefineResources(F);
    for (UINT i=0; i<_elements.GetCount(); i++)
    {
        F.DefineObject(_elements.Get(i), eaName);
    }
#endif
}

VOID CctControl_ElementImageGrid1::DefineProperties(CfxFiler &F)
{
    CctControl_ElementImageGrid::DefineProperties(F);
    F.DefineXOBJECTLIST("Elements", &_elements);
}

VOID CctControl_ElementImageGrid1::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columns,     "MIN(2);MAX(64)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Elements",     dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Image",        dtPGraphic,  &_bitmap,      "BITDEPTH(16);HINTWIDTH(2048);HINTHEIGHT(2048)");
    F.DefineValue("Rows",         dtInt32,    &_rows,        "MIN(2);MAX(64)");
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent Image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

//*************************************************************************************************

CctControl_ElementImageGrid2::CctControl_ElementImageGrid2(CfxPersistent *pOwner, CfxControl *pParent): CctControl_ElementImageGrid(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTIMAGEGRID2, FALSE);
    InitLockProperties("Columns;Element;Format;Image;Rows");

    _format = gfExcel;
    InitXGuid(&_elementId);
}

BOOL CctControl_ElementImageGrid2::IsValidItem(INT X, INT Y)
{
    return TRUE;
}

VOID CctControl_ElementImageGrid2::DoSessionNext(CfxControl *pSender)
{
    // Add the Attribute for this Element
    if (IsNullXGuid(&_elementId)) return;

    UINT index = _selectedY * _columns + _selectedX;
    CctAction_MergeAttribute action(this);

    switch (_format) 
    {
    case gfExcel:
        {
            CHAR valueX[16] = "";
            CHAR valueY[8] = "";
            INT v = _selectedX / 26;
            
            if (v > 0)
            {
                CHAR letter[2] = {'A' + (BYTE)v, '\0'};
                strcat(valueX, letter);
            }

            {
                v = _selectedX - v * 26;
                CHAR letter[2] = {'A' + (BYTE)v, '\0'};
                strcat(valueX, letter);
            }
            
            itoa(_selectedY+1, valueY, 10);

            UINT len = strlen(valueX) + strlen(valueY) + 1;
            CHAR *pValue = (CHAR *)MEM_ALLOC(len);
            memset(pValue, 0, len);
            strcpy(pValue, valueX);
            strcat(pValue, valueY);
            action.SetupAdd(_id, _elementId, dtPText, &pValue);
            MEM_FREE(pValue);
        }
        break;

    case gfNumber:
        {
            UINT value = _selectedY * _rows + _selectedX + 1;
            action.SetupAdd(_id, _elementId, dtInt32, &value);
        }
        break;

    default:
        FX_ASSERT(FALSE);
    }

    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_ElementImageGrid2::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CctControl_ElementImageGrid::DefineResources(F);
    F.DefineObject(_elementId, eaName);
#endif
}

VOID CctControl_ElementImageGrid2::DefineProperties(CfxFiler &F)
{
    CctControl_ElementImageGrid::DefineProperties(F);
    F.DefineValue("Format", dtInt8, &_format, F_ZERO);
    F.DefineXOBJECT("Element", &_elementId);
}

VOID CctControl_ElementImageGrid2::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columns,     "MIN(2);MAX(64)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Element",      dtGuid,    &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Image",        dtPGraphic,  &_bitmap,      "BITDEPTH(16);HINTWIDTH(2048);HINTHEIGHT(2048)");

    CHAR items[256];
    sprintf(items, "LIST(Excel=%d Number=%d)", gfExcel, gfNumber);
    F.DefineValue("Format",       dtInt8,     &_format, items);

    F.DefineValue("Rows",         dtInt32,    &_rows,        "MIN(2);MAX(64)");
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent Image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

