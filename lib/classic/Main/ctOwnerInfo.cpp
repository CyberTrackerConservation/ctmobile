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

#include "ctOwnerInfo.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "ctSession.h"

CfxControl *Create_Control_OwnerInfo(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_OwnerInfo(pOwner, pParent);
}

CctControl_OwnerInfo::CctControl_OwnerInfo(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Memo(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_OWNERINFO);
    InitLockProperties("Style;Font");
    _style = oikName;
}

CctControl_OwnerInfo::~CctControl_OwnerInfo()
{
}

VOID CctControl_OwnerInfo::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CctControl_OwnerInfo::DefineProperties(CfxFiler &F)
{
    CfxControl_Memo::DefineProperties(F);

    F.DefineValue("Style", dtInt8, &_style, F_ZERO);

    if (F.IsReader())
    {
        CTRESOURCEHEADER *header = NULL;
        if (IsLive())
        {
            header = GetSession(this)->GetResourceHeader();
        }

        switch (_style)
        {
        case oikName:
            SetCaption(header ? header->OwnerName : "Name");
            break;

        case oikOrganization:
            SetCaption(header ? header->OwnerOrganization : "Organization");
            break;
            
        case oikAddress1:
            SetCaption(header ? header->OwnerAddress1 : "Address 1");
            break;

        case oikAddress2:
            SetCaption(header ? header->OwnerAddress2 : "Address 2");
            break;

        case oikAddress3:
            SetCaption(header ? header->OwnerAddress3 : "Address 3");
            break;

        case oikPhone:
            SetCaption(header ? header->OwnerPhone : "Phone");
            break;

        case oikEmail:
            SetCaption(header ? header->OwnerEmail : "Email");
            break;

        case oikWebSite:
            SetCaption(header ? header->OwnerWebSite : "Web site");
            break;

        default:
            SetCaption("Invalid style");
        }
    }
}

VOID CctControl_OwnerInfo::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,    &_alignment,   items);
    F.DefineValue("Auto height",  dtBoolean,  &_autoHeight);
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    //F.DefineValue("Caption",      dtPText,    &_caption,     "MEMO;ROWHEIGHT(64)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight,   "MIN(0)");
    F.DefineValue("Right to left", dtBoolean, &_rightToLeft);
    F.DefineValue("Scroll width", dtInt32,    &_scrollBarWidth, "MIN(7);MAX(40)");
    F.DefineValue("Style",        dtInt8,     &_style,       "LIST(Name Organization Address1 Address2 Address3 Phone Email WebSite)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}
