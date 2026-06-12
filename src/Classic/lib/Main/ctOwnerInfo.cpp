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
    _style = OwnerInfoStyle::Name;
}

CctControl_OwnerInfo::~CctControl_OwnerInfo()
{
}

void CctControl_OwnerInfo::DefineResources(CfxFilerResource &F)
{
}

void CctControl_OwnerInfo::DefineProperties(CfxFiler &F)
{
    CfxControl_Memo::DefineProperties(F);

    F.DefineValue("Style", dtInt8, &_style, F_ZERO);

    if (F.IsReader())
    {
        CTRESOURCEHEADER *header = nullptr;
        if (IsLive())
        {
            header = GetSession(this)->GetResourceHeader();
        }

        switch (_style)
        {
        case OwnerInfoStyle::Name:
            SetCaption(header ? header->OwnerName : "Name");
            break;

        case OwnerInfoStyle::Organization:
            SetCaption(header ? header->OwnerOrganization : "Organization");
            break;

        case OwnerInfoStyle::Address1:
            SetCaption(header ? header->OwnerAddress1 : "Address 1");
            break;

        case OwnerInfoStyle::Address2:
            SetCaption(header ? header->OwnerAddress2 : "Address 2");
            break;

        case OwnerInfoStyle::Address3:
            SetCaption(header ? header->OwnerAddress3 : "Address 3");
            break;

        case OwnerInfoStyle::Phone:
            SetCaption(header ? header->OwnerPhone : "Phone");
            break;

        case OwnerInfoStyle::Email:
            SetCaption(header ? header->OwnerEmail : "Email");
            break;

        case OwnerInfoStyle::WebSite:
            SetCaption(header ? header->OwnerWebSite : "Web site");
            break;

        default:
            SetCaption("Invalid style");
        }
    }
}

void CctControl_OwnerInfo::DefinePropertiesUI(CfxFilerUI &F) 
{
}
