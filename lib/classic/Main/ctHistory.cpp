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

#include "ctHistory.h"
#include "fxApplication.h"
#include "fxUtils.h"

#define HISTORY_MIN_HEIGHT 22

BYTE IconObjects[3] = {eaIcon32, eaIcon50, eaIcon100};

CfxControl *Create_Control_History(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_History(pOwner, pParent);
}

CctControl_History::CctControl_History(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _cacheSnapshot(), _cache()
{
    InitControl(&GUID_CONTROL_HISTORY);
    InitBorder(bsSingle, 0);

    _dock = dkTop;
    _height = 32;

    _index = -1;
    _minHeight = HISTORY_MIN_HEIGHT;

    _transparentItems = FALSE;

    _backButton = new CfxControl_Button(pOwner, this);
    _backButton->SetComposite(TRUE);
    _backButton->SetBorderStyle(bsNone);
    _backButton->SetBorderWidth(0);
    _backButton->SetOnClick(this, (NotifyMethod)&CctControl_History::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CctControl_History::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, this);
    _nextButton->SetComposite(TRUE);
    _nextButton->SetBorderStyle(bsNone);
    _nextButton->SetBorderWidth(0);
    _nextButton->SetOnClick(this, (NotifyMethod)&CctControl_History::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CctControl_History::OnNextPaint);
}

VOID CctControl_History::CheckCache()
{
   // If not live, clear the cache
    CctSession *session = GetSession(this);
    if (!session)
    {
        RebuildCache();
        return;
    }
   
    // If the counts mismatch, rebuild 
    UINT count = session->GetCurrentAttributes()->GetCount();
    if (count != _cacheSnapshot.GetCount())
    {
        RebuildCache();
        return;
    }

    for (UINT i=0; i<count; i++)
    {
        XGUID item1 = session->GetCurrentAttributes()->GetPtr(i)->ElementId;
        XGUID item2 = _cacheSnapshot.Get(i);
        if (!CompareXGuid(&item1, &item2))
        {
            RebuildCache();
            break;
        }
    }
}

VOID CctControl_History::RebuildCache()
{
    _cacheSnapshot.Clear();
    _cache.Clear();
	_index = -1;

    CctSession *session = GetSession(this);
    if (!session) return;

    CfxResource *resource = GetResource();
    if (!resource) return;

    UINT count = session->GetCurrentAttributes()->GetCount();
    for (UINT i=0; i<count; i++)
    {
        XGUID elementId = session->GetCurrentAttributes()->Get(i).ElementId; 

        _cacheSnapshot.Add(elementId);

        for (UINT j=0; j<ARRAYSIZE(IconObjects); j++)
        {
            FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, elementId, IconObjects[j]);
            if (bitmap)
            {
                HISTORYITEM item;
                item.ActualIndex = i;
                item.IconAttribute = IconObjects[j];
                _cache.Add(item);
                resource->ReleaseObject(bitmap);
                break;
            }
        }
    }
}

VOID CctControl_History::FixButtons()
{
    CheckCache();

	if (_index == -1)
    {
        _index = GetItemCount() - 1;
    }

    _backButton->SetVisible(_index - (INT)GetVisibleItemCount() >= 0);
    _nextButton->SetVisible(_index + 1 < (INT)GetItemCount());
}

VOID CctControl_History::OnBackClick(CfxControl *pSender, VOID *pParams)
{
    if (_index > 0)
    {
        _index--;
    }       

    FixButtons();
    Repaint();
}

VOID CctControl_History::OnNextClick(CfxControl *pSender, VOID *pParams)
{
    if (_index < (INT)GetItemCount()-1)
    {
        _index++;
    }

    FixButtons();
    Repaint();
}

VOID CctControl_History::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawBackArrow(pParams->Rect, _color, _borderColor, _backButton->GetDown());
}

VOID CctControl_History::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawNextArrow(pParams->Rect, _color, _borderColor, _nextButton->GetDown());
}

VOID CctControl_History::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, "22");
    F.DefineValue("TransparentItems", dtBoolean, &_transparentItems, F_FALSE);

    // Initial value for templates
    //_minHeight = max(HISTORY_MIN_HEIGHT, _minHeight);
}

VOID CctControl_History::DefineState(CfxFiler &F)
{
    F.DefineValue("Index", dtInt32, &_index);
}

VOID CctControl_History::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum height", dtInt32,   &_minHeight);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent items", dtBoolean, &_transparentItems);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

VOID CctControl_History::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);

    //
    // This code is just a formality, it's just to bypass the checks that the server does to ensure we
    // aren't asking for media haven't requested.
    //
    CctSession *session = GetSession(this);
    if (session)
    {
        for (UINT i=0; i<session->GetCurrentAttributes()->GetCount(); i++)
        {
            for (UINT j=0; j<ARRAYSIZE(IconObjects); j++)
            {
                F.DefineObject(session->GetCurrentAttributes()->GetPtr(i)->ElementId, IconObjects[j]);
            }
        }
    }
#endif
}

VOID CctControl_History::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, max(_minHeight, Height));

    _backButton->SetBounds(_borderWidth, _borderWidth, 12 * _oneSpace, GetClientHeight());
    _nextButton->SetBounds(GetWidth() - _borderWidth - 12 * _oneSpace, _borderWidth, 12 * _oneSpace, GetClientHeight());

    FixButtons();
}

UINT CctControl_History::GetItemCount()
{
    CheckCache();
    return _cache.GetCount();
}

BOOL CctControl_History::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, ATTRIBUTE *pAttribute)
{
    CfxResource *resource = GetResource();
    
    UINT itemHeight = pRect->Bottom - pRect->Top + 1;

    for (UINT j=0; j<ARRAYSIZE(IconObjects); j++)
    {
        FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, pAttribute->ElementId, IconObjects[j]);
        if (bitmap)
        {
            FX_ASSERT(bitmap->Magic == MAGIC_BITMAP);
            pCanvas->StretchDraw(bitmap, pRect->Left + 1, pRect->Top + 1, itemHeight - 2, itemHeight - 2, FALSE, _transparentItems);
            resource->ReleaseObject(bitmap);
            return TRUE;
        }
    }

    return FALSE;
}

UINT CctControl_History::GetVisibleItemCount()
{
    UINT clientWidth = GetClientWidth() - _backButton->GetWidth() - _nextButton->GetWidth() - 2;
    if (GetClientHeight() > 1)
    {
        return clientWidth / (GetClientHeight() - 1);
    }
    else
    {
        return 0;
    }
}

VOID CctControl_History::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    // Fill with _color
    pCanvas->State.BrushColor = _color;
    pCanvas->FillRect(pRect);

    FixButtons();
    if (_index < 0) return;
    
    // Calculations to speed up drawing
    UINT lineWidth = GetBorderLineWidth();
    UINT itemHeight = GetClientHeight();
    INT count = (INT)GetItemCount();
    INT xstart = pRect->Left + _backButton->GetWidth();
    INT ystart = pRect->Top;

    INT index = max(_index + 1 - (INT)GetVisibleItemCount(), 0);

    CctSession *session = GetSession(this);
    CctControl_Navigate *navigate = session ? (CctControl_Navigate *)(session->FindControlByType(&GUID_CONTROL_NAVIGATE)) : NULL;

    for (UINT i=0; i<GetVisibleItemCount(); i++, index++)
    {
        if (index >= count) break;

        FXRECT rect;
        rect.Left   = xstart + lineWidth;
        rect.Top    = ystart + lineWidth;
        rect.Right  = xstart + itemHeight - lineWidth - 1;
        rect.Bottom = ystart + itemHeight - lineWidth - 1;

        ATTRIBUTE *attribute = session->GetCurrentAttributes()->GetPtr(_cache.GetPtr(index)->ActualIndex);
        pCanvas->State.LineWidth = 1;
        if (!PaintItem(pCanvas, &rect, attribute)) continue;

        if (navigate)
        {
            BOOL save1Visible = navigate->GetSave1Visible() && CompareXGuid(&attribute->ScreenId, navigate->GetSave1Target());
            BOOL save2Visible = !save1Visible && navigate->GetSave2Visible() && CompareXGuid(&attribute->ScreenId, navigate->GetSave2Target());
            
            if (save1Visible || save2Visible)
            {
                UINT w = (rect.Right - rect.Left + 1) / 2;
                FXPOINTLIST polygon;
                FXPOINT triangle[] = {{0, 0}, {w, 0}, {0, w}};
            
                pCanvas->State.LineColor = _borderColor;
                pCanvas->State.LineWidth = lineWidth;
                if (save1Visible)
                {
                    pCanvas->State.BrushColor = _borderColor;
                    FILLPOLYGON(pCanvas, triangle, xstart, ystart);
                }
                else if (save2Visible)
                {
                    pCanvas->State.BrushColor = _color;
                    FILLPOLYGON(pCanvas, triangle, xstart, ystart);
                    POLYGON(pCanvas, triangle, xstart, ystart);
                    w += lineWidth;
                }
                pCanvas->State.LineColor = _color;
                pCanvas->State.LineWidth = 1;
                pCanvas->MoveTo(xstart, ystart + w);
                pCanvas->LineTo(xstart + w, ystart);
            }
        }

        xstart += itemHeight - lineWidth;
        InflateRect(&rect, lineWidth, lineWidth);
        pCanvas->State.LineColor = _borderColor;
        pCanvas->State.LineWidth = lineWidth;
        pCanvas->FrameRect(&rect);
    }
}
