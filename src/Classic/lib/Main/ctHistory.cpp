#include "ctHistory.h"
#include "fxApplication.h"
#include "fxUtils.h"

#define HISTORY_MIN_HEIGHT 22

ElementAttribute IconObjects[3] = {ElementAttribute::Icon32, ElementAttribute::Icon50, ElementAttribute::Icon100};

CfxControl *Create_Control_History(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_History(pOwner, pParent);
}

CctControl_History::CctControl_History(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _cacheSnapshot(), _cache()
{
    InitControl(&GUID_CONTROL_HISTORY);
    InitBorder(ControlBorderStyle::Single, 0);

    _dock = ControlDock::Top;
    _height = 32;

    _index = -1;
    _minHeight = HISTORY_MIN_HEIGHT;

    _transparentItems = false;

    _backButton = new CfxControl_Button(pOwner, this);
    _backButton->SetComposite(true);
    _backButton->SetBorderStyle(ControlBorderStyle::None);
    _backButton->SetBorderWidth(0);
    _backButton->SetOnClick(this, (NotifyMethod)&CctControl_History::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CctControl_History::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, this);
    _nextButton->SetComposite(true);
    _nextButton->SetBorderStyle(ControlBorderStyle::None);
    _nextButton->SetBorderWidth(0);
    _nextButton->SetOnClick(this, (NotifyMethod)&CctControl_History::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CctControl_History::OnNextPaint);
}

void CctControl_History::CheckCache()
{
   // If not live, clear the cache
    CctSession *session = GetSession(this);
    if (!session)
    {
        RebuildCache();
        return;
    }
   
    // If the counts mismatch, rebuild 
    int count = session->GetCurrentAttributes()->GetCount();
    if (count != _cacheSnapshot.GetCount())
    {
        RebuildCache();
        return;
    }

    for (int i=0; i<count; i++)
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

void CctControl_History::RebuildCache()
{
    _cacheSnapshot.Clear();
    _cache.Clear();
	_index = -1;

    CctSession *session = GetSession(this);
    if (!session) return;

    CfxResource *resource = GetResource();
    if (!resource) return;

    int count = session->GetCurrentAttributes()->GetCount();
    for (int i=0; i<count; i++)
    {
        XGUID elementId = session->GetCurrentAttributes()->Get(i).ElementId; 

        _cacheSnapshot.Add(elementId);

        for (int j=0; j<ARRAYSIZE(IconObjects); j++)
        {
            FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, elementId, (BYTE)IconObjects[j]);
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

void CctControl_History::FixButtons()
{
    CheckCache();

	if (_index == -1)
    {
        _index = GetItemCount() - 1;
    }

    _backButton->SetVisible(_index - (int)GetVisibleItemCount() >= 0);
    _nextButton->SetVisible(_index + 1 < (int)GetItemCount());
}

void CctControl_History::OnBackClick(CfxControl *pSender, void *pParams)
{
    if (_index > 0)
    {
        _index--;
    }       

    FixButtons();
    Repaint();
}

void CctControl_History::OnNextClick(CfxControl *pSender, void *pParams)
{
    if (_index < (int)GetItemCount()-1)
    {
        _index++;
    }

    FixButtons();
    Repaint();
}

void CctControl_History::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawBackArrow(pParams->Rect, _color, _borderColor, _backButton->GetDown());
}

void CctControl_History::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawNextArrow(pParams->Rect, _color, _borderColor, _nextButton->GetDown());
}

void CctControl_History::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, "22");
    F.DefineValue("TransparentItems", dtBoolean, &_transparentItems, F_FALSE);

    // Initial value for templates
    //_minHeight = max(HISTORY_MIN_HEIGHT, _minHeight);
}

void CctControl_History::DefineState(CfxFiler &F)
{
    F.DefineValue("Index", dtInt32, &_index);
}

void CctControl_History::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_History::DefineResources(CfxFilerResource &F)
{
}

void CctControl_History::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, max(_minHeight, Height));

    _backButton->SetBounds(_borderWidth, _borderWidth, 12 * _oneSpace, GetClientHeight());
    _nextButton->SetBounds(GetWidth() - _borderWidth - 12 * _oneSpace, _borderWidth, 12 * _oneSpace, GetClientHeight());

    FixButtons();
}

int CctControl_History::GetItemCount()
{
    CheckCache();
    return _cache.GetCount();
}

bool CctControl_History::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, ATTRIBUTE *pAttribute)
{
    CfxResource *resource = GetResource();
    
    int itemHeight = pRect->Bottom - pRect->Top + 1;

    for (int j=0; j<ARRAYSIZE(IconObjects); j++)
    {
        FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(this, pAttribute->ElementId, (BYTE)IconObjects[j]);
        if (bitmap)
        {
            FX_ASSERT(bitmap->Magic == MAGIC_BITMAP);
            pCanvas->StretchDraw(bitmap, pRect->Left + 1, pRect->Top + 1, itemHeight - 2, itemHeight - 2, false, _transparentItems);
            resource->ReleaseObject(bitmap);
            return true;
        }
    }

    return false;
}

int CctControl_History::GetVisibleItemCount()
{
    int clientWidth = GetClientWidth() - _backButton->GetWidth() - _nextButton->GetWidth() - 2;
    if (GetClientHeight() > 1)
    {
        return clientWidth / (GetClientHeight() - 1);
    }
    else
    {
        return 0;
    }
}

void CctControl_History::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    // Fill with _color
    pCanvas->State.BrushColor = _color;
    pCanvas->FillRect(pRect);

    FixButtons();
    if (_index < 0) return;
    
    // Calculations to speed up drawing
    int lineWidth = GetBorderLineWidth();
    int itemHeight = GetClientHeight();
    int count = (int)GetItemCount();
    int xstart = pRect->Left + _backButton->GetWidth();
    int ystart = pRect->Top;

    int index = max(_index + 1 - (int)GetVisibleItemCount(), 0);

    CctSession *session = GetSession(this);
    CctControl_Navigate *navigate = session ? (CctControl_Navigate *)(session->FindControlByType(&GUID_CONTROL_NAVIGATE)) : nullptr;

    for (int i=0; i<GetVisibleItemCount(); i++, index++)
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
            bool save1Visible = navigate->GetSave1Visible() && CompareXGuid(&attribute->ScreenId, navigate->GetSave1Target());
            bool save2Visible = !save1Visible && navigate->GetSave2Visible() && CompareXGuid(&attribute->ScreenId, navigate->GetSave2Target());
            
            if (save1Visible || save2Visible)
            {
                int w = (rect.Right - rect.Left + 1) / 2;
                FXPOINTLIST polygon;
                FXPOINT triangle[] = {{0, 0}, {(int)w, 0}, {0, (int)w}};
            
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
