#pragma once
#include "pch.h"
#include "fxList.h"
#include "ctSession.h"

//*************************************************************************************************
//
// CctControl_Gotos
//

const GUID GUID_CONTROL_GOTOLIST = {0x7b7db615, 0xbb0d, 0x49e7, {0xa0, 0xf2, 0x47, 0xce, 0xf8, 0x9, 0x74, 0x19}};

class CctControl_GotoList: public CfxControl_List
{
protected:
    int _selectedIndex = 0;
    bool _centerText = false, _rightToLeft = false;
    FXGPS_POSITION _cachePosition;
    FXGOTO _cacheItem;
    
    FXGOTO *GetItem(int Index, bool *pBoundary);
public:
    CctControl_GotoList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_GotoList();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_GotoList(CfxPersistent *pOwner, CfxControl *pParent);

class CctGoto_Painter: public CfxListPainter
{
protected:
    FXGOTO _goto;
public:
    CctGoto_Painter(CfxPersistent *pOwner, CfxControl *pParent): CfxListPainter(pOwner, pParent), _goto() { }
    ~CctGoto_Painter() { }

    void Connect(FXGOTO *pGoto) { _goto = *pGoto; }

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;
};

