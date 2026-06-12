#pragma once
#include "pch.h"
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_NUMBERLIST = {0x70267b40, 0x4e8f, 0x4224, {0x9a, 0xb4, 0xe4, 0x13, 0xf7, 0xe6, 0x96, 0x48}};

//
// CctControl_NumberList
//
class CctControl_NumberList: public CfxControl_List
{
protected:
    bool _autoNext = false;
    XGUID _elementId = 0;
    int _startValue = 0, _endValue = 0, _interval = 0;
    int _selectedIndex = 0;
    int _autoSelectIndex = 0;
    CctRetainState _retainState;
    char *_resultGlobalValue = nullptr;

    int GetColumnCount() override { return _columnCount; }

    void SetSelectedIndex(int Index);
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    void OnSessionEnterNext(CfxControl *pSender, void *pParams);
public:
    CctControl_NumberList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NumberList();

    void ResetState() override;

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;

    void OnPenClick(int X, int Y) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;
};

extern CfxControl *Create_Control_NumberList(CfxPersistent *pOwner, CfxControl *pParent);

