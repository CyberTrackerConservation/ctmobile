#pragma once
#include "pch.h"
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"

// {023FC6A7-B186-4a67-AB30-322C4DC1408A}
const GUID GUID_CONTROL_ELEMENTURLLIST = { 0x23fc6a7, 0xb186, 0x4a67, { 0xab, 0x30, 0x32, 0x2c, 0x4d, 0xc1, 0x40, 0x8a } };

class CctControl_ElementUrlList: public CfxControl_List
{
protected:
    bool _autoNext = false;
    XGUID _elementId = 0;
    int _selectedIndex = 0;
    bool _centerText = false, _rightToLeft = false;
    char *_separator = nullptr;

    char *_url = nullptr;
    char *_userName = nullptr;
    char *_password = nullptr;

    TList<char*> _list; // The list of items to draw
    char *_pListBuffer = nullptr; // Backing memory for each list item

    bool _required = false;
    char *_listItems = nullptr;   // Unparsed items

    bool _waitingForResponse = false;
    bool _waitingInvert = false;
    
    char *GetItem(int Index);
    void ParseList();

    void OnSessionEnter(CfxControl *pSender, void *pParams);
    void OnSessionEnterNext(CfxControl *pSender, void *pParams);
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    void OnUrlReceived(const char *pFileName, int ErrorCode) override;
    void OnTimer() override;

    void SetSelectedIndex(int Index);

public:
    CctControl_ElementUrlList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementUrlList();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_ElementUrlList(CfxPersistent *pOwner, CfxControl *pParent);

