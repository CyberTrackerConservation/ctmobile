#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"

const GUID GUID_CONTROL_ELEMENTCAMERA = {0x2f601f3, 0x28bd, 0x4d1f, {0xa9, 0xdd, 0xca, 0xa7, 0x1a, 0xbc, 0x25, 0xfc}};

//
// CctControl_ElementCamera
//
class CctControl_ElementCamera: public CfxControl_Panel
{
protected:
    GUID _sightingUniqueId;
    XGUID _elementId = 0;
    bool _supported = false;
    bool _required = false;
    bool _waitForTaken = false, _pending = false;
    FXIMAGE_QUALITY _imageQuality = FXIMAGE_QUALITY::Default;
    FXBITMAPRESOURCE *_cacheBitmap = nullptr;

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_ElementCamera(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementCamera();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void OnPictureTaken(char *pFileName, bool Success) override;

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
};

extern CfxControl *Create_Control_ElementCamera(CfxPersistent *pOwner, CfxControl *pParent);
