#pragma once

#include "pch.h"
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"

// {986B4CE5-8266-40ba-8436-65CFC380C7DE}
const GUID GUID_CONTROL_ELEMENTLOOKUPLIST = { 0x986b4ce5, 0x8266, 0x40ba, { 0x84, 0x36, 0x65, 0xcf, 0xc3, 0x80, 0xc7, 0xde } };

class CctControl_ElementLookupList: public CfxControl_List
{
protected:
    bool _autoNext = false;
    XGUID _elementId = 0;
    int _selectedIndex = 0;
    bool _centerText = false, _rightToLeft = false;

    char *_resultGlobalValue = nullptr;

    TList<char*> _list; // The list of items to draw
    char *_pListBuffer = nullptr; // Backing memory for each list item

    bool _required = false;
    char *_listItems = nullptr;   // Unparsed items
    char *_listName = nullptr;    // List name
    
    char *GetItem(int Index);
    void ParseLookupList();
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);

    void SetSelectedIndex(int Index);

public:
    CctControl_ElementLookupList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementLookupList();

    char *GetItems()  { return _listItems;    }
    void SetItems(char *pValue) { MEM_FREE(_listItems); _listItems = AllocString(pValue); }
    char *GetName()   { return _listName; }

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index) override;

    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_ElementLookupList(CfxPersistent *pOwner, CfxControl *pParent);

