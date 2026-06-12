#pragma once
#include "pch.h"
#include "fxControl.h"
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"
#include "ctNavigate.h"

const GUID GUID_CONTROL_HISTORY = {0x5e1cb980, 0xd452, 0x4037, {0xb2, 0x9a, 0x48, 0xc0, 0x6a, 0xe5, 0xb2, 0x15}};

struct HISTORYITEM
{
    int ActualIndex;
    ElementAttribute IconAttribute;
};

//
// CctControl_History
//
class CctControl_History: public CfxControl
{
protected:
    CfxControl_Button *_backButton = nullptr, *_nextButton = nullptr;
    int _index = 0;
    bool _transparentItems = false;
    int _minHeight = 0;
    
    TList<XGUID> _cacheSnapshot;
    TList<HISTORYITEM> _cache;

    void CheckCache();
    void RebuildCache();

    void OnBackClick(CfxControl *pSender, void *pParams);
    void OnNextClick(CfxControl *pSender, void *pParams);
    void OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    int GetVisibleItemCount();
    void FixButtons();
public:
    CctControl_History(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    int GetItemCount();
    bool PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, ATTRIBUTE *pAttribute);
};

extern CfxControl *Create_Control_History(CfxPersistent *pOwner, CfxControl *pParent);
