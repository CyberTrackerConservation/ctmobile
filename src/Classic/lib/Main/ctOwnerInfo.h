#pragma once
#include "pch.h"
#include "fxMemo.h"

const GUID GUID_CONTROL_OWNERINFO = {0x4023eaa, 0x6f34, 0x48b1, {0xa8, 0x60, 0x43, 0xd2, 0xec, 0xc, 0x89, 0x3c}};

enum class OwnerInfoStyle : int8_t {Name=0, Organization, Address1, Address2, Address3, Phone, Email, WebSite};

//
// CctControl_OwnerInfo
//
class CctControl_OwnerInfo: public CfxControl_Memo
{
protected:
    OwnerInfoStyle _style = OwnerInfoStyle::Name;
public:
    CctControl_OwnerInfo(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_OwnerInfo();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_OwnerInfo(CfxPersistent *pOwner, CfxControl *pParent);

