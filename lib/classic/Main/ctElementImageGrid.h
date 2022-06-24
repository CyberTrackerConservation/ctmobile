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

#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"

//
// CctControl_ElementImageGrid
//
class CctControl_ElementImageGrid: public CfxControl
{
protected:
    UINT _rows, _columns;
    XBITMAP _bitmap;
    BOOL _transparentImage;
    UINT _selectedX, _selectedY;
    BOOL GetItemRect(INT X, INT Y, FXRECT *pRect);
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    virtual BOOL IsValidItem(INT X, INT Y)=0;
    virtual VOID DoSessionNext(CfxControl *pSender)=0;
public:
    CctControl_ElementImageGrid(CfxPersistent *pOwner, CfxControl *pParent);
    virtual ~CctControl_ElementImageGrid();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
};

//
// CctControl_ElementImageGrid1
//
const GUID GUID_CONTROL_ELEMENTIMAGEGRID1 = {0xd2bc4b92, 0xae3a, 0x4ace, {0x9c, 0xa4, 0xaf, 0xf1, 0x16, 0xbd, 0xb1, 0x66}};

enum ELEMENTIMAGEGRIDFORMAT {gfExcel, gfNumber};

class CctControl_ElementImageGrid1: public CctControl_ElementImageGrid
{
protected:
    XGUIDLIST _elements;
    BOOL IsValidItem(INT X, INT Y);
    VOID DoSessionNext(CfxControl *pSender);
public:
    CctControl_ElementImageGrid1(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};
extern CfxControl *Create_Control_ElementImageGrid1(CfxPersistent *pOwner, CfxControl *pParent);

//
// CctControl_ElementImageGrid2
//
const GUID GUID_CONTROL_ELEMENTIMAGEGRID2 = {0xfb310e77, 0xa4ae, 0x4840, {0x93, 0x5b, 0x1f, 0x7e, 0x57, 0x87, 0xc1, 0xc3}};

class CctControl_ElementImageGrid2: public CctControl_ElementImageGrid
{
protected:
    XGUID _elementId;
    ELEMENTIMAGEGRIDFORMAT _format;
    BOOL IsValidItem(INT X, INT Y);
    VOID DoSessionNext(CfxControl *pSender);
public:
    CctControl_ElementImageGrid2(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};
extern CfxControl *Create_Control_ElementImageGrid2(CfxPersistent *pOwner, CfxControl *pParent);

