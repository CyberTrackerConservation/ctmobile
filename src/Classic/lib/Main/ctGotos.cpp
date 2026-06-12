#include "fxApplication.h"
#include "fxUtils.h"
#include "fxGPS.h"
#include "fxMath.h"
#include "ctGotos.h"
#include "ctSession.h"
#include "ctActions.h"

//*************************************************************************************************
// CctControl_GotoList

CfxControl *Create_Control_GotoList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_GotoList(pOwner, pParent);
}

CctControl_GotoList::CctControl_GotoList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_GOTOLIST);
    InitLockProperties("");

    _textColor = _borderColor;

    _selectedIndex = -1;
    _rightToLeft = false;
    memset(&_cachePosition, 0, sizeof(_cachePosition));

    SetCaption("Goto list");
}

CctControl_GotoList::~CctControl_GotoList()
{
}

void CctControl_GotoList::DefineResources(CfxFilerResource &F)
{
}

void CctControl_GotoList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);

    if (F.IsReader())
    {
        UpdateScrollBar();
    }
}

void CctControl_GotoList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

void CctControl_GotoList::DefinePropertiesUI(CfxFilerUI &F) 
{
}

int CctControl_GotoList::GetItemCount()
{
    int Count = 0;

    if (IsLive())
    {
        CctSession *session = GetSession(this);
        Count = 1;  // None
        Count += session->GetCustomGotoCount(); // Custom
        Count += session->GetResourceHeader()->GotosCount;  // Static
    }

    return Count;
}

FXGOTO *CctControl_GotoList::GetItem(int Index, bool *pBoundary)
{
    *pBoundary = false;

    if (Index == 0)
    {
        memset(&_cacheItem, 0, sizeof(FXGOTO));
        strcpy(_cacheItem.Title, "---");
        return &_cacheItem;
    }

    CctSession *session = GetSession(this);
    int customCount = session->GetCustomGotoCount();

    Index--;
    if (Index < customCount)
    {
        *pBoundary = (Index == 0);
        return session->GetCustomGoto(Index);
    }

    Index -= customCount;
    CfxResource *resource = GetResource();
    FXGOTO *pGoto = resource->GetGoto(this, Index);
    if (pGoto)
    {
        _cacheItem = *pGoto;
        resource->ReleaseGoto(pGoto);
    }

    *pBoundary = (Index == 0);
    return &_cacheItem;
}

void CctControl_GotoList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    int itemHeight = pItemRect->Bottom - pItemRect->Top + 1;

    // Paint if selected
    bool selected = ((int)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the caption
    int flags = TEXT_ALIGN_VCENTER;
    FXRECT rect = *pItemRect;
    rect.Right -= 2 * _oneSpace;
    rect.Left += 2 * _oneSpace;

    if (Index == 0)
    {
        flags |= TEXT_ALIGN_HCENTER;
    }
    else if (_rightToLeft)
    {
        flags |= TEXT_ALIGN_RIGHT;
    }

    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);

    bool boundary = false;
    FXGOTO *pGoto = GetItem(Index, &boundary);
    if (pGoto)
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, pGoto->Title);

        if ((Index > 0) && TestGPS(&_cachePosition))
        {
            int tw = pCanvas->TextWidth(pFont, "00000000 km");
            flags = TEXT_ALIGN_VCENTER;
            if (_rightToLeft)
            {
                rect.Right = rect.Left + tw;
                flags |= TEXT_ALIGN_LEFT;
            }
            else
            {
                rect.Left = rect.Right - tw;
                flags |= TEXT_ALIGN_RIGHT;
            }

            double d = CalcDistanceKm(pGoto->Y, pGoto->X, _cachePosition.Latitude, _cachePosition.Longitude);
            
            char caption[16], captionF[16];
            if ((int)d == 0)
            {
                d *= 1000;
                sprintf(caption, "%ld m", (int)d);
            }
            else 
            {
                PrintF(captionF, d, sizeof(captionF)-1, d < 10 ? 3 : 1); 
                sprintf(caption, "%s km", captionF);
            }

            pCanvas->DrawTextRect(pFont, &rect, flags, caption);
        }
    }
    else
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, "Bad waypoint index");
    }

    if (boundary)
    {
        FXRECT rect = *pItemRect;
        rect.Bottom = rect.Top + _oneSpace;
        pCanvas->State.BrushColor = _borderColor;
        pCanvas->FillRect(&rect);
    }
}

void CctControl_GotoList::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive())
    {
        SetPaintTimer(2000);
        _cachePosition = GetEngine(this)->GetGPS()->Position;
        CfxControl_List::OnPaint(pCanvas, pRect);
    }
    else
    {
        CfxControl_Panel::OnPaint(pCanvas, pRect);
    }
}

void CctControl_GotoList::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        _selectedIndex = index;

        if (index == 0)
        {
            FXGOTO gotoNone = {MAGIC_GOTO};
            GetEngine(this)->SetCurrentGoto(&gotoNone);
        }
        else
        {
            bool boundary = false;
            FXGOTO *pGoto = GetItem(index, &boundary);
            if (pGoto)
            {
                GetEngine(this)->SetCurrentGoto(pGoto);
            }
        }

        Repaint();
    }
}

//*************************************************************************************************
// CctGoto_Painter

int CctGoto_Painter::GetItemCount()
{
    return 3;
}

void CctGoto_Painter::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
{
    char caption[32];

    int itemHeight = pRect->Bottom - pRect->Top + 1;

    // Left side rect
    FXRECT lRect = *pRect;
    lRect.Left += (2 * _parent->GetBorderLineWidth()) + itemHeight;
    lRect.Right = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;

    // Right side rect
    FXRECT rRect = *pRect;
    rRect.Left = lRect.Right + 1;

    switch (Index)
    {
    case 0:
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, "Title");
        pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, _goto.Title);
        break;
    case 1:
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, "Latitude");
        PrintF(caption, _goto.Y, sizeof(caption)-1, 6);
        pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, caption);
        break;
    case 2:
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, "Longitude");
        PrintF(caption, _goto.X, sizeof(caption)-1, 6);
        pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, caption);
        break;
    }
}
